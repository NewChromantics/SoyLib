#include "SoyThread.h"
#include "SoyDebug.h"



std::mutex SoyThread::mCleanupEventLock;
std::map<std::thread::native_handle_type,std::shared_ptr<SoyEvent<const std::thread::native_handle_type>>> SoyThread::mCleanupEvents;
std::map<std::thread::native_handle_type,std::shared_ptr<prmem::Heap>> SoyThread::mThreadHeaps;


void Soy::TSemaphore::OnCompleted()
{
	//	gr: don't think I should need this, but it's helping?
	//	gr: if not, change wait()'s to wait_for so it auto awakes and re-checks predicate
	mLock.lock();
	mCompleted = true;
	mLock.unlock();
	
	mConditional.notify_all();
}

void Soy::TSemaphore::Wait(const char* TimerName)
{
	if ( mCompleted )
		return;
	
	std::unique_lock<std::mutex> Lock( mLock );
	
	auto IsCompleted = [this]
	{
		return mCompleted;
	};
	
	if ( TimerName )
	{
		ofScopeTimerWarning Timer( TimerName, 0 );
	
		mConditional.wait( Lock, IsCompleted );
	}
	else
	{
		mConditional.wait( Lock, IsCompleted );
	}
}



void PopWorker::TJobQueue::Flush(TContext& Context)
{
	auto AutoUnlockContext = [&Context]
	{
		Context.Unlock();
	};
	
	bool FlushError = true;
	
	ofScopeTimerWarning LockTimer("Waiting for job lock",4,false);
	while ( true )
	{
		LockTimer.Start();
		//	pop task
		mLock.lock();
		std::shared_ptr<TJob> Job;
		auto NextJob = mJobs.begin();
		if ( NextJob != mJobs.end() )
		{
			Job = *NextJob;
			mJobs.erase( NextJob );
		}
		//bool MoreJobs = !mJobs.empty();
		mLock.unlock();
		LockTimer.Stop();
		
		if ( !Job )
			break;
		
		//	lock the context
		if ( !Context.Lock() )
		{
			mLock.lock();
			mJobs.insert( mJobs.begin(), Job );
			mLock.unlock();
			break;
		}
		
		//	flush errors from before iteration
		if ( FlushError )
		{
			//Opengl::IsOkay("JobQueue flush flush",false);
			FlushError = false;
		}
		
		auto ContextLock = SoyScope( nullptr, AutoUnlockContext );
		
		//	execute task, if it returns false, we don't run any more this time and re-insert
		if ( !Job->Run( std::Debug ) )
		{
			mLock.lock();
			mJobs.insert( mJobs.begin(), Job );
			mLock.unlock();
			break;
		}
		
		//	mark job as finished
		if ( Job->mSemaphore )
			Job->mSemaphore->OnCompleted();
	}
}


void PopWorker::TJobQueue::PushJob(std::function<bool ()> Function)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJobImpl( Job, nullptr );
}

void PopWorker::TJobQueue::PushJob(std::function<bool ()> Function,Soy::TSemaphore& Semaphore)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJobImpl( Job, &Semaphore );
}

void PopWorker::TJobQueue::PushJobImpl(std::shared_ptr<TJob>& Job,Soy::TSemaphore* Semaphore)
{
	Soy::Assert( Job!=nullptr, "Job expected" );
	
	Job->mSemaphore = Semaphore;
	
	mLock.lock();
	mJobs.push_back( Job );
	mLock.unlock();
	
	mOnJobPushed.OnTriggered( Job );
}




ofThread::ofThread() :
	mIsRunning		( false )
{
}


ofThread::~ofThread()
{
	waitForThread();

	destroy();
}

bool ofThread::create(unsigned int stackSize)
{
    return true;
}

void ofThread::destroy()
{
}

void ofThread::waitForThread(bool Stop)
{
	if ( isThreadRunning() )
	{
		if ( Stop )
			stopThread();
	}
	
	//	if thread is active, then wait for it to finish and join it
	if ( mThread.joinable() )
	{
		auto ThreadHandle = mThread.native_handle();
		
		mThread.join();

		//	gr: before or after join?
		SoyThread::OnThreadCleanup( ThreadHandle );
	}

	mThread = std::thread();
}

void ofThread::stopThread()
{
	//	thread's loop will stop on next loop
	mIsRunning = false;
}

bool ofThread::startThread(bool blocking, bool verbose)
{
	if ( !create() )
		return false;

	//	already running 
	if ( isThreadRunning() )
		return true;

	//	mark as running
	mIsRunning = true;

	//	start thread
    mThread = std::thread( threadFunc, this );
	return true;
}

unsigned int ofThread::threadFunc(void *args)
{
	ofThread* pThread = reinterpret_cast<ofThread*>(args);
	
	if ( pThread )
	{
		pThread->threadedFunction();
		pThread->mIsRunning = false;
		std::Debug << "Thread " << pThread->GetThreadName() << " finished" << std::endl;
	}
	return 0;
}


#if defined(PLATFORM_OSX)
#include <pthread.h>
#endif

void SoyThread::SetThreadName(std::string name,std::thread::native_handle_type ThreadId)
{
#if defined(TARGET_OSX)

	auto CurrentThread = SoyThread::GetCurrentThreadNativeHandle();
	
	//	has to be called whilst in this thread as OSX doesn't take a thread parameter
	if ( CurrentThread != ThreadId )
	{
		std::Debug << "Trying to set thread name " << name << ", out-of-thread" << std::endl;
		return;
	}
	int Result = pthread_setname_np( name.c_str() );
	if ( Result != 0 )
	{
		std::Debug << "Failed to set thread name to " << name << ": " << Result << std::endl;
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
	
	SetNameFunc( name.c_str(), ThreadId );

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
		
		static bool CatchExceptions = !Soy::Platform::IsDebuggerAttached();

		if ( CatchExceptions )
		{
			try
			{
				if ( !Iteration() )
					break;
			} catch ( std::exception& e )
			{
				std::Debug << "caught exception in SoyWorker" << e.what() << std::endl;
				break;
			} catch ( ... )
			{
				std::Debug << "caught unknown-type-of exception in SoyWorker";
			}
		}
		else
		{
			if ( !Iteration() )
				break;
		}
	}
	
	mOnFinish.OnTriggered(Dummy);
}


void SoyWorkerThread::Start()
{
	//	already have a thread
	if ( !Soy::Assert( !HasThread(), "Thread already created" ) )
		return;
	
	auto StartFunc = [this]
	{
		//	enable thread cancellation
#if defined(TARGET_OSX)
		auto EnabledCancel = 0 == pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,nullptr);
		if ( !EnabledCancel )
			std::Debug << "unable to enable thread cancelling" << std::endl;
		auto EnabledAsyncCancel = 0 == pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,nullptr);
		if ( !EnabledAsyncCancel )
			std::Debug << "unable to enable async thread cancelling" << std::endl;
#endif
		//	name thread!
		SoyThread::SetThreadName( mThreadName, SoyThread::GetCurrentThreadNativeHandle() );
		this->SoyWorker::Start();
	};
	
	//	start thread
	mThread = std::thread( StartFunc );
}

void SoyWorkerThread::WaitToFinish()
{
	Stop();
	
	//	if thread is active, then wait for it to finish and join it
	if ( mThread.joinable() )
	{
		auto Thread = mThread.native_handle();
		mThread.join();
		
		//	gr: before or after join?
		SoyThread::OnThreadCleanup( Thread );
	}
	mThread = std::thread();
}


std::shared_ptr<SoyEvent<const std::thread::native_handle_type>> SoyThread::GetOnThreadCleanupEvent()
{
	auto Thread = SoyThread::GetCurrentThreadNativeHandle();
	//	gr: look out for bad thread id's!
//	if ( !Soy::Assert( ThreadId != std::thread::id(), "Adding thread cleanup, but cannot resolve thread id" ) )
//		return nullptr;

	//	create/get map entry for cleanup events
	std::lock_guard<std::mutex> Lock( mCleanupEventLock );
	auto& Event = mCleanupEvents[Thread];
	if ( !Event )
		Event.reset( new SoyEvent<const std::thread::native_handle_type>() );

	return Event;
}

bool SoyThread::OnThreadCleanup(std::thread::native_handle_type Thread)
{
	//	get cleanup event
	mCleanupEventLock.lock();
	auto pEvent = mCleanupEvents.find(Thread);
	mCleanupEventLock.unlock();

	//	no callbacks
	if ( pEvent == mCleanupEvents.end() )
		return true;
	
	auto& Event = pEvent->second;
	if ( Event )
	{
		Event->OnTriggered( Thread );
		Event.reset();
	}
	
	mCleanupEventLock.lock();
	mCleanupEvents.erase(pEvent);
	mCleanupEventLock.unlock();
	
	//	cleanup all memory in thread's heap
	auto HeapIt = mThreadHeaps.find(Thread);
	if ( HeapIt != mThreadHeaps.end() )
	{
		//	deallocate the heap and all it's allocations
		HeapIt->second.reset();
		//	keep map small, but maybe have thread instability
	//	mThreadHeaps.erase( HeapIt );
	}
	
	return true;
}


prmem::Heap& SoyThread::GetHeap(std::thread::native_handle_type Thread)
{
	//	grab heap
	auto HeapIt = mThreadHeaps.find(Thread);
	
	if ( HeapIt != mThreadHeaps.end() )
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
	mThreadHeaps[Thread] = NewHeap;
	return *NewHeap;
}

