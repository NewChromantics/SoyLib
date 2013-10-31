#include "SoyThread.h"


#if defined(NO_OPENFRAMEWORKS)
ofThread::ofThread() :
	mHandle			( nullptr ),
	mThreadId		( 0 ),
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
	if ( mHandle )
		return true;

	mHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, stackSize, 
		threadFunc, this, CREATE_SUSPENDED, &mThreadId ));
	
	if ( !mHandle )
		return false;
	
	return true;
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofThread::destroy()
{
	if ( mHandle )
	{
		CloseHandle(mHandle);
		mHandle = nullptr;
	}
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
	
	//	for for handle to disapear
	if ( mHandle )
	{
		WaitForSingleObject( mHandle, INFINITE);
		//	destroy here?
	}
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

	if ( !mHandle )
		return false;

	//	already running 
	if ( isThreadRunning() )
		return true;

	//	mark as running
	mIsRunning = true;

	//	start thread
	if ( ResumeThread( mHandle ) == -1 )
	{
		//	failed
		mIsRunning = false;
		return false;
	}

	return true;
}
#endif


#if defined(NO_OPENFRAMEWORKS)
unsigned int __stdcall ofThread::threadFunc(void *args)
{
	ofThread* pThread = reinterpret_cast<ofThread*>(args);
	
	if ( pThread )
		pThread->threadedFunction();

	_endthreadex(0);
	return 0;
} 
#endif
