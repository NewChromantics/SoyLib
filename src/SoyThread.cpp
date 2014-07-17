#include "SoyThread.h"
#include <SoyDebug.h>

const SoyThreadId::TYPE SoyThreadId::Invalid = SoyThreadId::TYPE();


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
