#include "SoyThread.h"



#if defined(NO_OPENFRAMEWORKS)
ofThread::ofThread() :
#if defined(STD_THREAD)
#elif defined(TARGET_WINDOWS)
    mHandle			( nullptr ),
    mThreadId		( 0 ),
#endif
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
#if defined(STD_THREAD)
    return true;
#else
	if ( mHandle )
		return true;

	mHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, stackSize, 
		threadFunc, this, CREATE_SUSPENDED, &mThreadId ));
	
	if ( !mHandle )
		return false;
	
	return true;
#endif
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofThread::destroy()
{
#if defined(STD_THREAD)
#else
	if ( mHandle )
	{
		CloseHandle(mHandle);
		mHandle = nullptr;
        mThreadId = 0;
	}
#endif
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
	
#if defined(STD_THREAD)
    mThread.join();
    mThread = std::thread();
#else
	//	for for handle to disapear
	if ( mHandle )
	{
		WaitForSingleObject( mHandle, INFINITE);
		//	destroy here?
	}
#endif
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
#if defined(STD_THREAD)
    mThread = std::thread( threadFunc, this );
#else
	if ( ResumeThread( mHandle ) == -1 )
	{
		//	failed
		mIsRunning = false;
		return false;
	}
#endif
	return true;
}
#endif


#if defined(NO_OPENFRAMEWORKS)
unsigned int ofThread::threadFunc(void *args)
{
	ofThread* pThread = reinterpret_cast<ofThread*>(args);
	
	if ( pThread )
		pThread->threadedFunction();

#if !defined(STD_THREAD) && defined(TARGET_WINDOWS)
	_endthreadex(0);
#endif
	return 0;
}
#endif
