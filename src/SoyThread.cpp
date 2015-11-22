#include "SoyThread.h"
#include "SoyDebug.h"
#if defined(PLATFORM_OSX)
#include <pthread.h>
#endif

namespace Soy
{
	namespace Private
	{
		std::map<std::thread::native_handle_type,std::shared_ptr<prmem::Heap>>	ThreadHeaps;
	}
}

SoyEvent<SoyThread> SoyThread::OnThreadFinish;
SoyEvent<SoyThread> SoyThread::OnThreadStart;


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



void PopWorker::TJobQueue::Flush(TContext& Context)
{
	bool FlushError = true;
	
	ofScopeTimerWarning LockTimer("Waiting for job lock",5,false);
	while ( true )
	{
		LockTimer.Start(true);
		//	pop task
		mJobLock.lock();
		std::shared_ptr<TJob> Job;
		auto NextJob = mJobs.begin();
		if ( NextJob != mJobs.end() )
		{
			Job = *NextJob;
			mJobs.erase( NextJob );
		}
		//bool MoreJobs = !mJobs.empty();
		mJobLock.unlock();
		LockTimer.Stop(false);
		if ( LockTimer.Report() )
			std::Debug << "Job queue has " << mJobs.size() << std::endl;
		
		if ( !Job )
			break;
		
		//	lock the context
		if ( !Context.Lock() )
		{
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
	
	mOnJobPushed.OnTriggered( Job );
}


SoyThread::SoyThread(const std::string& ThreadName) :
	mThreadName	( ThreadName ),
	mIsRunning	( false )
{
	//	POSIX needs to name threads IN the thread. so do that for everyone by default
	auto NameThread = [this](SoyThread& Thread)
	{
	//	Soy::Assert( std::this_thread::id == Thread.get_id(), "Shouldn't call this outside of thread" );
		if ( !mThreadName.empty() )
			SetThreadName( mThreadName );
	};
	mNameThreadListener = OnThreadStart.AddListener( NameThread );
	
	auto CleanupHeapWrapper = [this](SoyThread&)
	{
		CleanupHeap();
	};
	mHeapThreadListener = OnThreadFinish.AddListener( CleanupHeapWrapper );
}

SoyThread::~SoyThread()
{
	OnThreadStart.RemoveListener( mNameThreadListener );
	OnThreadFinish.RemoveListener( mHeapThreadListener );
}


void SoyThread::Stop(bool WaitToFinish)
{
	//	thread's loop will stop on next loop
	mIsRunning = false;
	
	//	wait for thread to exit
	if ( WaitToFinish )
	{
		//	if thread is active, then wait for it to finish and join it
		if ( mThread.joinable() )
			mThread.join();

		mThread = std::thread();
	}
}

void SoyThread::Start()
{
	//	already running
	if ( mThread.get_id() != std::thread::id() )
		return;
	
	//	maybe stopping...
	Soy::Assert( !mIsRunning, "Thread doesn't exist, but is marked as running");
	
	auto ThreadFuncWrapper = [this](void*)
	{
		this->mIsRunning = true;

		SoyThread::OnThreadStart.OnTriggered(*this);
		
		//	gr: even if we're not catching exceptions, java NEEDS us to cleanup the threads or the OS will abort us
		try
		{
			while ( this->mIsRunning )
			{
				this->Thread();
			}
			SoyThread::OnThreadFinish.OnTriggered(*this);
		}
		catch(...)
		{
			SoyThread::OnThreadFinish.OnTriggered(*this);
			throw;
		}
		
		return 0;
	};
	
	auto CatchException_ThreadFuncWrapper = [=](void* x)
	{
		try
		{
			return ThreadFuncWrapper(x);
		}
		catch(std::exception& e)
		{
			std::Debug << "Caught SoyThread " << GetThreadName() << " exception: " << e.what() << std::endl;
		}
		catch(...)
		{
			std::Debug << "Caught SoyThread " << GetThreadName() << " unknown exception." << std::endl;
		}
		
		return 0;
	};

	//	gr: catch all. Maybe only enable this for "release" builds?
	//		android is throwing an exception, but callstack doesn't contain exception handler/throw indicator... so seeing if we can trap it
	//	crash with exception is still useful in release though...
	#if defined(TARGET_ANDROID)
	bool CatchExceptions = true;
	#else
	bool CatchExceptions = !Soy::Platform::IsDebuggerAttached();
	#endif
	
	//	start thread
	if ( CatchExceptions )
	{
		mThread = std::thread( CatchException_ThreadFuncWrapper, nullptr );
	}
	else
	{
		mThread = std::thread( ThreadFuncWrapper, nullptr );
	}
}

std::thread::native_handle_type SoyThread::GetCurrentThreadNativeHandle()
{
#if defined(TARGET_WINDOWS)
	return ::GetCurrentThread();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_ANDROID)
	return ::pthread_self();
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
		OldThreadName = Soy::Platform::GetLastErrorString();
	}
	
	return OldThreadName;
#elif defined(TARGET_ANDROID)
	//	pthread_getname_np is oddly missing from the ndk. not in the kernel, but people are referencing it in stack overflow?!
	//	gr: styled to the DEBUG log thread names in adb logcat
	std::thread::native_handle_type CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
	std::stringstream OldThreadName;
	OldThreadName << "Thread-" << CurrentThread;
	return OldThreadName.str();
#else
	return "<NoThreadNameOnThisPlatform>";
#endif
}



void SoyThread::SetThreadName(const std::string& _Name,std::thread::native_handle_type ThreadId)
{
	//	max name length found from kernel source;
	//	https://android.googlesource.com/platform/bionic/+/40eabe2/libc/bionic/pthread_setname_np.cpp
#if defined(TARGET_ANDROID)
	// "This value is not exported by kernel headers."
#define MAX_TASK_COMM_LEN 16
	std::string Name = _Name.substr( 0, MAX_TASK_COMM_LEN-1 );
#else
	auto& Name = _Name;
#endif
	
	
	auto OldThreadName = GetCurrentThreadName();
	
	
#if defined(TARGET_POSIX)
	
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
		std::string Error = (Result==ERANGE) ? "Name too long" : Soy::Platform::GetErrorString(Result);
		std::Debug << "Failed to change thread name from " << OldThreadName << " to " << Name << ": " << Error << std::endl;
	}

#endif
	
#if defined(TARGET_WINDOWS)
	
	//	wrap the try() function in a lambda to avoid the unwinding
	auto SetNameFunc = [](const char* ThreadName,HANDLE ThreadHandle)
	{
		DWORD ThreadId = ::GetThreadId(ThreadHandle);
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
	bool Dummy = true;
	mOnStart.OnTriggered(Dummy);
	
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
				mWaitConditional.wait_for( Lock, GetSleepDuration() );
				break;
			default:
				break;
		}
		
		if ( !IsWorking() )
			break;

		mOnPreIteration.OnTriggered(Dummy);
		
		if ( !Iteration() )
			break;
	}
	
	mOnFinish.OnTriggered(Dummy);
}


void SoyWorkerThread::Start(bool ThrowIfAlreadyStarted)
{
	//	already have a thread
	if ( !ThrowIfAlreadyStarted && HasThread() )
		return;
	if ( !Soy::Assert( !HasThread(), "Thread already created" ) )
		return;
	
	SoyThread::Start();
}


void SoyWorkerThread::Thread()
{
	SoyWorker::Start();
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
			Soy::Assert( pHeap!=nullptr, "Getting heap for thread that has been destroyed" );
			return prcore::Heap;
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

