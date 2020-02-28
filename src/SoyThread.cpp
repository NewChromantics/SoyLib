#include "SoyThread.h"
#include "SoyDebug.h"
#include <future>


#if defined(TARGET_PS4)
#include <pthread.h>
#include <pthread_np.h>
#define MAX_THREAD_NAME	32	//	from https://ps4.scedev.net/resources/documents/SDK/3.500/Kernel-Reference/0101.html
#endif

#if defined(TARGET_ANDROID)
//	max name length found from kernel source;
//	https://android.googlesource.com/platform/bionic/+/40eabe2/libc/bionic/pthread_setname_np.cpp
// "This value is not exported by kernel headers."
#define MAX_TASK_COMM_LEN 16
#define MAX_THREAD_NAME	MAX_TASK_COMM_LEN
#endif

namespace Java
{
	void	FlushThreadLocals();
}

namespace Soy
{
	namespace Private
	{
		std::map<std::thread::native_handle_type,std::shared_ptr<prmem::Heap>>	ThreadHeaps;
	}
}


void Soy::TSemaphore::OnCompleted()
{
	mCompleted = true;
	
	mConditional.notify_all();
}


void Soy::TSemaphore::Wait(const char* TimerName)
{
	if ( mCompleted )
	{
		if ( !mThrownError.empty() )
			throw Soy::AssertException( mThrownError );
		return;
	}
	
	std::unique_lock<std::mutex> Lock( mLock );
	
	//	gr: sometimes this gets stuck... so timeout and re-read IsCompleted...
	//		seems to only happen with multiple opengl contexts... but I don't see how that's relaveant
	static int TimeoutMs = 666;

	auto IsCompleted = [this]
	{
		return mCompleted;
	};
	
	if ( TimerName )
	{
		ofScopeTimerWarning Timer( TimerName, 0 );

		while ( !IsCompleted() )
			mConditional.wait_for( Lock, std::chrono::milliseconds(TimeoutMs), IsCompleted );
	}
	else
	{
		while ( !IsCompleted() )
			mConditional.wait_for( Lock, std::chrono::milliseconds(TimeoutMs), IsCompleted );
	}
	
	if ( !mCompleted )
		throw Soy::AssertException("Broke out of semaphore");

	if ( !mThrownError.empty() )
		throw Soy::AssertException( mThrownError );
}

void Soy::TSemaphore::WaitAndReset(const char* TimerName)
{
	//	wait, then reset the status so it can be re-waited and re-resolved
	Wait(TimerName);
	Reset();
}

void Soy::TSemaphore::Reset()
{
	//	gr: should this be locked?
	//		with this going with Wait() could we get a race 
	//		condition? maybe the reset needs to move explicity inside the wait()'s lock
	std::unique_lock<std::mutex> Lock(mLock);
	mThrownError = std::string();
	mCompleted = false;
}

PopWorker::TJobQueue::~TJobQueue()
{
	//	to try and reduce crashes, hold the lock here as we destruct
	if ( mJobLock.try_lock() )
	{
		mJobLock.unlock();
	}
	else
	{
		std::Debug << "Warning: job queue is locked, still processing? Locking to avoid crash" << std::endl;
		mJobLock.lock();
		mJobLock.unlock();
	}
}

std::shared_ptr<PopWorker::TJob> PopWorker::TJobQueue::PopNextJob(TContext& Context,size_t& SmallestDelay)
{
	std::lock_guard<std::recursive_mutex> Lock(mJobLock);

	//	find next ready job
	for ( auto i=0;	i<mJobs.size();	i++ )
	{
		auto pJob = mJobs[i];

		//	unexpected null job
		if ( !pJob )
		{
			std::Debug << "Found null job in queue" << std::endl;
			auto it = mJobs.begin()+i;
			mJobs.erase( it );
			return nullptr;
		}
		
		//	not ready, skip
		auto DelayMs = pJob->GetRunDelay();
		if ( DelayMs > 0 )
		{
			if ( SmallestDelay == 0 )
				SmallestDelay = DelayMs;
			SmallestDelay = std::min( SmallestDelay, DelayMs );
			continue;
		}
		
		//	send this back
		auto it = mJobs.begin()+i;
		mJobs.erase( it );
		return pJob;
	}
	
	//	nothing in queue ready to go
	return nullptr;
}

void PopWorker::TJobQueue::Flush(TContext& Context,std::function<void(std::chrono::milliseconds)> Sleep)
{
	bool FlushError = true;
	
	//	get the smallest delay > 0ms
	size_t SmallestDelayMs = 0;
	
	//ofScopeTimerWarning LockTimer("Waiting for job lock",5,false);
	while ( true )
	{
		//LockTimer.Start(true);
		auto Job = PopNextJob(Context,SmallestDelayMs);
		/*LockTimer.Stop(false);
		if ( LockTimer.Report() )
		{
			std::Debug << "Job queue has " << mJobs.size() << std::endl;
		}
		*/
		if ( !Job )
			break;
		
		//	lock the context, if it fails, put the job back in the queue
		try
		{
			Context.Lock();
		}
		catch(std::exception& e)
		{
			std::Debug << "Failed to lock context, putting job back in the queue; " << e.what() << std::endl;
			//	gr: maybe have an unpop
			mJobLock.lock();
			mJobs.insert( mJobs.begin(), Job );
			mJobLock.unlock();
			break;
		}
		
		//	flush errors from before iteration
		if ( FlushError )
		{
			//Opengl::IsOkay("JobQueue flush flush",false);
			FlushError = false;
		}
		
		RunJob( Job );
		Context.Unlock();
	}
	
	if ( SmallestDelayMs > 0 )
	{
		//	half the time, as we assume there will be a delay until we get around to it again
		//	we could assume the loop takes say, 4ms and just remove 4ms from it
		SmallestDelayMs /= 2;
		Sleep( std::chrono::milliseconds(SmallestDelayMs) );
	}
}


void PopWorker::TJobQueue::RunJob(std::shared_ptr<TJob>& Job)
{
	Soy::Assert( Job!=nullptr, "Job expected" );

	if ( Job->mCatchExceptions )
	{
		try
		{
			Job->Run();
			
			//	mark job as finished
			if ( Job->mSemaphore )
				Job->mSemaphore->OnCompleted();
		}
		catch (std::exception& e)
		{
			if ( Job->mSemaphore )
				Job->mSemaphore->OnFailed( e.what() );
			else
				std::Debug << "Exception executing job " << e.what() << " (no semaphore)" << std::endl;
		}
	}
	else
	{
		Job->Run();
		
		//	mark job as finished
		if ( Job->mSemaphore )
			Job->mSemaphore->OnCompleted();
	}
}

void PopWorker::TJobQueue::QueueDeleteAll()
{
	std::lock_guard<std::recursive_mutex> Lock(mJobLock);
	mJobs.clear();
}

void PopWorker::TJobQueue::PushJob(std::function<void()> Function)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJobImpl( Job, nullptr );
}

void PopWorker::TJobQueue::PushJob(std::function<void()> Function,Soy::TSemaphore& Semaphore)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJobImpl( Job, &Semaphore );
}

void PopWorker::TJobQueue::PushJobImpl(std::shared_ptr<TJob>& Job,Soy::TSemaphore* Semaphore)
{
	Soy::Assert( Job!=nullptr, "Job expected" );
	
	Job->mSemaphore = Semaphore;
	
	//	try and avoid inception deadlocks
	bool WillWait = (Semaphore!=nullptr);	//	assumption
	auto ThisThreadId = std::this_thread::get_id();
	if ( IsLocked(ThisThreadId) && WillWait )
	{
		//	execute immediately, assume any throws will be caught by the queue flush...
		std::Debug << "Warning: caught job inception, executing immediately to avoid deadlock" << std::endl;
	
		RunJob( Job );
		return;
	}
	
	mJobLock.lock();
	mJobs.push_back( Job );
	mJobLock.unlock();
	
	if ( mOnJobPushed )
		mOnJobPushed( Job );
}


SoyThread::SoyThread(const std::string& ThreadName) :
	mThreadName	( ThreadName )
{
}

SoyThread::~SoyThread()
{
	try
	{
		Stop(true);
	}
	catch (std::exception& e)
	{
		std::Debug << __PRETTY_FUNCTION__ << "; Exception stopping thread; " << e.what() << std::endl;
	}
}


void SoyThread::Stop(bool WaitToFinish)
{
	//	thread's loop will stop on next loop
	if (mThreadState == Running)
		mThreadState = Stopping;

	//	gr: here, we may have already WaitToFinish'd and thrown if the thread failed
	//		this just triggers it again... is that intended?
	if (WaitToFinish)
		this->WaitToFinish();
}

#if defined(TARGET_WINDOWS)
namespace Platform
{
	namespace ThreadState
	{
		enum TYPE
		{
			_WAIT_ABANDONED = WAIT_ABANDONED,
			_WAIT_OBJECT_0 = WAIT_OBJECT_0,
			_WAIT_TIMEOUT = WAIT_TIMEOUT,
			_WAIT_FAILED = WAIT_FAILED
		};
	}
	std::string	GetThreadStateString(ThreadState::TYPE State);
}
#endif


#if defined(TARGET_WINDOWS)
std::string Platform::GetThreadStateString(ThreadState::TYPE State)
{
	switch (State)
	{
	case ThreadState::_WAIT_ABANDONED:	return "_WAIT_ABANDONED";
	case ThreadState::_WAIT_OBJECT_0:	return "_WAIT_OBJECT_0";
	case ThreadState::_WAIT_TIMEOUT:	return "_WAIT_TIMEOUT";
	case ThreadState::_WAIT_FAILED:	return "_WAIT_FAILED";
	default:
		return "ThreadState::UNKNOWN";
	}
}
#endif

void SoyThread::WaitToFinish()
{
	//	never started
	if (mThreadState == NotStarted)
		return;

	//	wait for thread to exit
	//	gr: Joinable() doesn't block until its finished, so
	//		now we wait on the semaphore
	//		We throw if the thread threw, so we can use WaitToFinish
	//		as a way to check what happened what happened in the thread
	//		we should throw here if the thread threw

	auto DestroyThread = [&]()
	{
		//	now join now that it's finished
		if (mThread.joinable())
			mThread.join();

		//	delete
		mThread = std::thread();
	};

	//	get OS thread state
#if defined(TARGET_WINDOWS)
	{
		//	0 = success, and means it's exited, but still running, I think
		auto ThreadState = WaitForSingleObject(mThread.native_handle(), 0);
		std::Debug << this->mThreadName << " Thread State is " << Platform::GetThreadStateString( static_cast<Platform::ThreadState::TYPE>(ThreadState)) << "(" << ThreadState << ")" << std::endl;
	}
#endif
	

	try
	{
		std::stringstream TimerName;
		TimerName << "Waiting for thread " << mThreadName << " to finish";
		mFinishedSemaphore.Wait(TimerName.str().c_str());
		DestroyThread();
	}
	catch(std::exception& e)
	{
		DestroyThread();
		throw;
	}		
}

void SoyThread::Start()
{
	//	already running
	if ( mThread.get_id() != std::thread::id() )
		return;
		
	auto ThreadFuncWrapper = [](void* pThis)
	{
		auto* This = reinterpret_cast<SoyThread*>(pThis);
		This->mThreadState = Running;

		//	POSIX needs to name threads IN the thread. so do that for everyone by default
		This->OnThreadStart();
		
		//	gr: even if we're not catching exceptions, java NEEDS us to cleanup the threads or the OS will abort us
		try
		{
			while ( This->IsThreadRunning() )
			{
				if (!This->ThreadIteration())
					break;

				if ( This == nullptr )
					throw Soy::AssertException("How did this get null");
			}
			
			This->OnThreadFinish(std::string());
		}
		catch(std::exception& e)
		{
			//	gr: this shouldn't throw, but just set the error (with OnThreadFinish)
			This->OnThreadFinish(e.what());
		}
		catch(...)
		{
			This->OnThreadFinish("Unknown exception");
		}
		
		std::Debug << "Thread has exited" << std::endl;
		return 0;
	};

	mThread = std::thread( ThreadFuncWrapper, this );
}

std::thread::native_handle_type SoyThread::GetCurrentThreadNativeHandle()
{
#if defined(TARGET_WINDOWS)
	return ::GetCurrentThread();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_ANDROID)||defined(TARGET_LUMIN)
	return ::pthread_self();
#elif defined(TARGET_PS4)
	ScePthread Handle = scePthreadSelf();
	return Handle;
#else
#error SoyThread::GetCurrentThreadNativeHandle Platform not handled
#endif
}


std::string SoyThread::GetCurrentThreadName()
{
#if defined(TARGET_OSX)
	
	std::thread::native_handle_type CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
	
	//	grab current thread name
	char OldNameBuffer[200] = "<initialised>";
	std::string OldThreadName;
	auto Result = pthread_getname_np( CurrentThread, OldNameBuffer, sizeof(OldNameBuffer) );
	OldNameBuffer[sizeof(OldNameBuffer)-1] = '\0';
	if ( Result == 0 )
	{
		OldThreadName = OldNameBuffer;
		if ( OldThreadName.empty() )
			OldThreadName = "<no name>";
	}
	else
	{
		OldThreadName = Platform::GetLastErrorString();
	}
	
	return OldThreadName;
#elif defined(TARGET_ANDROID)
	//	pthread_getname_np is oddly missing from the ndk. not in the kernel, but people are referencing it in stack overflow?!
	//	gr: styled to the DEBUG log thread names in adb logcat
	std::thread::native_handle_type CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
	std::stringstream OldThreadName;
	OldThreadName << "Thread-" << CurrentThread;
	return OldThreadName.str();

#elif defined(TARGET_PS4)

	//	argh unsafe
	{
		char Buffer[MAX_THREAD_NAME] = {'\0'};
		auto Handle = GetCurrentThreadNativeHandle();
		auto Result = scePthreadGetname( Handle, Buffer );
		if ( Result == SCE_OK )
		{
			if ( Buffer[0] == '\0' )
				return "<NullThreadName>";
			return Buffer;
		}
		return "<ErrorGettingThreadName>";
	}


#elif defined(TARGET_WINDOWS)
	/*
	//	https://stackoverflow.com/a/41902967/355753
	//	if the thread name was set with SetThreadDescription, we can retrieve it
	try
	{
		auto CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
		PWSTR NameBuffer = nullptr;
		auto Result = GetThreadDescription(CurrentThread, &NameBuffer);
		Platform::IsOkay(Result, "GetThreadDescription");
		LocalFree(NameBuffer);
		std::wstring NameW(NameBuffer);
		auto Name = Soy::WStringToString(NameW);
		return Name;
	}
	catch (std::exception& e)
	{
		std::stringstream Name;
		Name << "Unnamed thread [" << e.what() << "]";
		return Name.str();
	}
	*/
	return "<NoThreadName>";
#else
	return "<NoThreadNameOnThisPlatform>";
#endif
}

void SoyThread::OnThreadStart()
{
	//	Soy::Assert( std::this_thread::id == Thread.get_id(), "Shouldn't call this outside of thread" );
	if ( !mThreadName.empty() )
		SetThreadName( mThreadName );
}

void SoyThread::OnThreadFinish(const std::string& Exception)
{
	CleanupHeap();

	//	if there was an error, flag semaphore with error
	if (!Exception.empty())
		mFinishedSemaphore.OnFailed(Exception.c_str());
	else
		mFinishedSemaphore.OnCompleted();
}

void SoyThread::SetThreadName(const std::string& _Name,std::thread::native_handle_type ThreadId)
{
#if defined(MAX_THREAD_NAME)
	std::string Name = _Name.substr( 0, MAX_THREAD_NAME-1 );
#else
	auto& Name = _Name;
#endif
	
	
	auto OldThreadName = GetCurrentThreadName();
		

#if defined(TARGET_PS4)

	//auto Result = pthread_rename_np( ThreadId, Name.c_str() );
	auto Handle = GetCurrentThreadNativeHandle();
	auto Result = scePthreadRename( Handle, Name.c_str() );

	if ( Result == SCE_OK )
	{
		std::Debug << "Renamed thread from " << OldThreadName << " to " << Name << ": " << std::endl;
	}
	else
	{
		std::string Error = (Result==ERANGE) ? "Name too long" : Platform::GetErrorString(Result);
		std::Debug << "Failed to change thread name from " << OldThreadName << " to " << Name << ": " << Error << std::endl;
	}

#elif defined(TARGET_POSIX)
	
#if defined(TARGET_OSX)||defined(TARGET_IOS)
	//	has to be called whilst in this thread as OSX doesn't take a thread parameter
	std::thread::native_handle_type CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
	if ( CurrentThread != ThreadId )
	{
		std::Debug << "Trying to change thread name from " << OldThreadName << " to " << Name << ", out-of-thread" << std::endl;
		return;
	}
	auto Result = pthread_setname_np( Name.c_str() );
#else
	auto Result = pthread_setname_np( ThreadId, Name.c_str() );
#endif
	
	if ( Result == 0 )
	{
		std::Debug << "Renamed thread from " << OldThreadName << " to " << Name << ": " << std::endl;
	}
	else
	{
		std::string Error = (Result==ERANGE) ? "Name too long" : Platform::GetErrorString(Result);
		std::Debug << "Failed to change thread name from " << OldThreadName << " to " << Name << ": " << Error << std::endl;
	}

#elif defined(TARGET_WINDOWS)
	
	//	try and use the new SetThreadDescription
	static bool UseSetThreadDescription = false;
	if (UseSetThreadDescription)
	{
		//	gr: this si failing, but ThreadHandle seems to be 0 when going up the callstack there IS a 64bit handle value on the std::thread...
		auto ThreadHandle = GetCurrentThread();
		auto NameW = Soy::StringToWString(Name);
		const WCHAR* NameBuffer = NameW.c_str();
		auto Result = SetThreadDescription(ThreadId, NameBuffer);
		try
		{
			std::stringstream Error;
			Error << "SetThreadDescription(" << Name << ")";
			Platform::IsOkay(Result, Error.str());
			return;
		}
		catch (std::exception& e)
		{
			std::Debug << "Error in SetThreadDescription() " << e.what() << std::endl;
		}
	}

	//	wrap the try() function in a lambda to avoid the unwinding
	auto SetNameFunc = [](const char* ThreadName,HANDLE ThreadHandle)
	{
		//	https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2019
		//	ThreadId -1 == calling thread
		//	gr: this isn't working with GetThreadId, but -1 is...
		//DWORD ThreadId = ::GetThreadId(ThreadHandle);
		DWORD ThreadId = -1;
		//	set the OS thread name
		//	http://msdn.microsoft.com/en-gb/library/xcb2z8hs.aspx
		const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} THREADNAME_INFO;
#pragma pack(pop)

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = ThreadName;
		info.dwThreadID = ThreadId;
		info.dwFlags = 0;

		//	this will fail if the OS thread hasn't started yet
		if (info.dwThreadID != 0)
		{
			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	};
	
	SetNameFunc( Name.c_str(), ThreadId );

#else

	std::Debug << "Platform does not allow thread rename from " << OldThreadName << " to " << Name << std::endl;

#endif
}



std::string SoyThread::GetThreadName(std::thread::native_handle_type ThreadId)
{
#if defined(TARGET_OSX)
	
	char Buffer[255] = {0};
	auto Result = pthread_getname_np( ThreadId, Buffer, sizeof(Buffer) );

	if ( Result != 0 )
	{
		std::Debug << "Failed to get thread name [" << Result << "]" << std::endl;
		return "Thread??";
	}
	
	std::stringstream Name;
	Name << Buffer;
	
	if ( Name.str().empty() )
	{
		if ( pthread_main_np() != 0 )
			Name << "Main thread";
		else
			Name << "?" << reinterpret_cast<std::uintptr_t>(ThreadId);
	}

	return Name.str();
#endif
	
	return "Thread??";
}

SoyWorker::~SoyWorker()
{
	std::unique_lock<std::mutex> Lock( mWaitMutex );
}

void SoyWorker::Start()
{
	mWorking = true;
	Loop();
}

void SoyWorker::Stop()
{
	mWorking = false;
	Wake();
}

void SoyWorker::Wake()
{
	if ( mWaitMode != SoyWorkerWaitMode::Wake )
	{
		//std::Debug << WorkerName() << "::Wake ignored as WaitMode is not Wake" << std::endl;
		return;
	}
	mWaitConditional.notify_all();
}

void SoyWorker::Loop()
{
	std::unique_lock<std::mutex> Lock( mWaitMutex );
	
	//	first call
	if ( mOnStart )
		mOnStart();

	auto SleepDuration = GetSleepDuration();

	while ( IsWorking() )
	{
		//	do wait
		switch ( mWaitMode )
		{
			case SoyWorkerWaitMode::Wake:
				if ( CanSleep() )
					mWaitConditional.wait( Lock );
				break;
			case SoyWorkerWaitMode::Sleep:
				mWaitConditional.wait_for( Lock, SleepDuration );
				break;
			default:
				break;
		}
		
		if ( !IsWorking() )
			break;

		if ( mOnPreIteration )
			mOnPreIteration();
		
#if defined(TARGET_ANDROID)
		Java::FlushThreadLocals();
#endif
		
		auto RuntimeSleep = [&](std::chrono::milliseconds Ms)
		{
			mWaitConditional.wait_for( Lock, Ms );
		};
		
		if ( !Iteration( RuntimeSleep ) )
			break;
	}
	
	if ( mOnFinish )
		mOnFinish();
}


void SoyWorkerThread::Start(bool ThrowIfAlreadyStarted)
{
	//	already have a thread
	if ( !ThrowIfAlreadyStarted && HasThread() )
	{
		Wake();
		return;
	}
	if ( !Soy::Assert( !HasThread(), "Thread already created" ) )
		return;
	
	SoyThread::Start();
}


bool SoyWorkerThread::ThreadIteration()
{
	//	this blocks and does its own iterating
	SoyWorker::Start();
	
	return false;
}

void SoyThread::CleanupHeap()
{
	//	cleanup all memory in thread's heap
	auto HeapIt = Soy::Private::ThreadHeaps.find( mThread.native_handle() );
	if ( HeapIt != Soy::Private::ThreadHeaps.end() )
	{
		//	deallocate the heap and all it's allocations
		HeapIt->second.reset();
		//	keep map small, but maybe have thread instability
	//	mThreadHeaps.erase( HeapIt );
	}
}


prmem::Heap& SoyThread::GetHeap(std::thread::native_handle_type Thread)
{
	auto& Heaps = Soy::Private::ThreadHeaps;
	
	//	grab heap
	auto HeapIt = Heaps.find(Thread);
	
	if ( HeapIt != Heaps.end() )
	{
		auto pHeap = HeapIt->second;

		//	if key exists, but null, the thread has been cleaned up. issue a warning and return the default heap
		if ( !pHeap )
		{
			throw Soy::AssertException("Getting heap for thread that has been destroyed" );
		}
		
		return *pHeap;
	}

	//	alloc new heap
	std::stringstream HeapName;
	HeapName << "Thread Heap " << GetThreadName(Thread);
	std::shared_ptr<prmem::Heap> NewHeap( new prmem::Heap( true, true, HeapName.str().c_str() ) );
	Heaps[Thread] = NewHeap;
	return *NewHeap;
}


void Platform::ExecuteDelayed(std::chrono::milliseconds Delay,std::function<void()> Lambda)
{
	auto Thread = [=]()
	{
		std::this_thread::sleep_for( Delay );
		Lambda();
	};
	
	// Use async to launch a function (lambda) in parallel
	std::async( std::launch::async, Thread );
}


void Platform::ExecuteDelayed(std::chrono::high_resolution_clock::time_point FutureTime,std::function<void()> Lambda)
{
	auto Thread = [=]()
	{
		std::this_thread::sleep_until( FutureTime );
		Lambda();
	};
	
	// Use async to launch a function (lambda) in parallel
	std::async( std::launch::async, Thread );
}



bool SoyWorkerJobThread::Iteration()
{
	throw Soy::AssertException("Should be calling iteration with sleep");
}

bool SoyWorkerJobThread::Iteration(std::function<void(std::chrono::milliseconds)> Sleep)
{
	PopWorker::TJobQueue::Flush( *this, Sleep );
	return true;
}
