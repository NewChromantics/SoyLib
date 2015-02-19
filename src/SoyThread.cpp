#include "SoyThread.h"
#include "SoyDebug.h"



#if defined(NO_OPENFRAMEWORKS)
ofThread::ofThread() :
	mIsRunning		( false )
{
}
#endif


#if defined(NO_OPENFRAMEWORKS)
ofThread::~ofThread()
{
	waitForThread();

	destroy();
}
#endif


#if defined(NO_OPENFRAMEWORKS)
bool ofThread::create(unsigned int stackSize)
{
    return true;
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofThread::destroy()
{
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofThread::waitForThread(bool Stop)
{
	if ( isThreadRunning() )
	{
		if ( Stop )
			stopThread();
	}
	
	//	if thread is active, then wait for it to finish and join it
	if ( mThread.joinable() )
		mThread.join();

	mThread = std::thread();
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofThread::stopThread()
{
	//	thread's loop will stop on next loop
	mIsRunning = false;
}
#endif


#if defined(NO_OPENFRAMEWORKS)
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
#endif


#if defined(NO_OPENFRAMEWORKS)
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
#endif

#if defined(PLATFORM_OSX)
#include <pthread.h>
#endif

void SoyThread::SetThreadName(std::string name,std::thread::native_handle_type ThreadId)
{
#if defined(TARGET_OSX)

	//	has to be called whilst in this thread as OSX doesn't take a thread parameter
	if ( std::this_thread::get_id() != ThreadId )
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
#if !defined(NO_OPENFRAMEWORKS)
	auto ThreadId = getPocoThread().tid();
	getPocoThread().setName( name );
#endif
	
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


void SoyWorkerThread::Start()
{
	//	already have a thread
	if ( !Soy::Assert( !HasThread(), "Thread already created" ) )
		return;
	
	//	start thread
	mThread = std::thread( [this]{	this->SoyWorker::Start();	} );
}

void SoyWorkerThread::WaitToFinish()
{
	Stop();
	
	//	if thread is active, then wait for it to finish and join it
	if ( mThread.joinable() )
		mThread.join();
	mThread = std::thread();
}

