#pragma once

#include "SoyTypes.h"
#include "SoyEvent.h"
#include <condition_variable>
#include "MemHeap.hpp"
#include "HeapArray.hpp"



namespace Soy
{
	class TSemaphore;
	class Mutex_Profiled;
}

//	gr: big flaw with this class... if you create, then wait. It'll never complete. Need an "assigned" flag
class Soy::TSemaphore
{
public:
	TSemaphore() :
		mCompleted	( false )
	{
	}
	
	bool		IsCompleted() const						{	return mCompleted;	}
	void		Wait(const char* TimerName=nullptr);
	void		OnCompleted();
	
	//	if whatever we're waiting for had an error, we re-throw the message in Wait() as a Soy::AssertException
	void		OnFailed(const char* ThrownError)
	{
		mThrownError = ThrownError ? ThrownError : "Failed with unspecified error";
		OnCompleted();
	}
	
private:
	std::mutex				mLock;
	std::condition_variable	mConditional;
	volatile bool			mCompleted;
	std::string				mThrownError;
};



//	dumb, but simple "jobs to execute" system. Used to force operations on main thread or opengl thread
//	merge this with pop job and soy worker systems!
namespace PopWorker
{
	class TContext;
	class TJobQueue;
	class TJob;
	class TJob_Function;	//	simple job for lambda's
};



class PopWorker::TJob
{
public:
	TJob() :
		mSemaphore			( nullptr ),
		mCatchExceptions	( true )
	{
	}

	virtual void			Run()=0;		//	throws on error, otherwise assuming success
	
	Soy::TSemaphore*		mSemaphore;
	bool					mCatchExceptions;
};



class PopWorker::TJob_Function : public TJob
{
public:
	TJob_Function(std::function<void()> Function) :
	mFunction	( Function )
	{
	}
	
	virtual void		Run() override
	{
		return mFunction();
	}
	
	std::function<void()>	mFunction;
};


class PopWorker::TContext
{
public:
	virtual bool	Lock()=0;
	virtual void	Unlock()=0;
};


class PopWorker::TJobQueue
{
public:
	virtual bool	IsLocked(std::thread::id Thread)		{	return false;	}	//	is this THREAD exclusively locked
	bool			IsLockedToAnyThread()					{	return IsLocked( std::thread::id() );	}

	void			Flush(TContext& Context);
	size_t			GetJobCount() const						{	return mJobs.size();	}
	bool			HasJobs() const							{	return !mJobs.empty();	}
	void			PushJob(std::function<void()> Lambda);
	void			PushJob(std::function<void()> Lambda,Soy::TSemaphore& Semaphore);
	void			PushJob(std::shared_ptr<TJob>& Job)									{	PushJobImpl( Job, nullptr );	}
	void			PushJob(std::shared_ptr<TJob>& Job,Soy::TSemaphore& Semaphore)		{	PushJobImpl( Job, &Semaphore );	}
	
protected:
	virtual void	PushJobImpl(std::shared_ptr<TJob>& Job,Soy::TSemaphore* Semaphore);
	
private:
	void			RunJob(std::shared_ptr<TJob>& Job);

public:
	SoyEvent<std::shared_ptr<TJob>>		mOnJobPushed;
	
private:
	std::vector<std::shared_ptr<TJob>>	mJobs;			//	gr: change this to a nice soy ringbuffer
	std::recursive_mutex				mJobLock;
};



//	common, maybe rename
template<typename TYPE>
class TDefferedDeleteJob : public PopWorker::TJob
{
public:
	TDefferedDeleteJob(std::shared_ptr<TYPE>& Pointer) :
	mPointer	( Pointer )
	{
	}
	
	virtual void		Run() override
	{
		//	clear pointer to release in opengl thread!
		//	gr; quite common for this to result in a crash... so... re-queue?
		auto RefCount = mPointer.use_count();
		int Retrys = 3;
		while ( RefCount > 1 )
		{
			static auto WaitMs = 10;
			std::Debug << "Warning: TDefferedDeleteJob<" << Soy::GetTypeName<TYPE>() << "> has refcount=" << RefCount << "; ";
			
			if ( Retrys > 0 )
			{
				std::Debug << "waiting " << WaitMs << "ms (retry x" << Retrys << ")";
				std::this_thread::sleep_for( std::chrono::milliseconds(WaitMs) );
				Retrys--;
			}
			else
			{
				//	gr: check just in case we get spruious output
				std::Debug << "releasing opengl thread pointer, won't dealloc on opengl thread!";
			}

			//	try again counter
			RefCount = mPointer.use_count();
			std::Debug << " (now refcount=" << RefCount << ")" << std::endl;
		}
		mPointer.reset();
	}
	
	std::shared_ptr<TYPE>	mPointer;
};








//	c++11 threads are better!
//	gr: if mscv > 2012?

template <class M>
class ScopedLock
	/// A class that simplifies thread synchronization
	/// with a mutex.
	/// The constructor accepts a Mutex (and optionally
	/// a timeout value in milliseconds) and locks it.
	/// The destructor unlocks the mutex.
{
public:
	explicit ScopedLock(M& mutex): _mutex(mutex)
	{
		_mutex.lock();
	}
	
	~ScopedLock()
	{
		_mutex.unlock();
	}

private:
	M& _mutex;

	ScopedLock();
	ScopedLock(const ScopedLock&);
	ScopedLock& operator = (const ScopedLock&);
};


template <class M>
class ScopedLockTimed
	/// A class that simplifies thread synchronization
	/// with a mutex.
	/// The constructor accepts a Mutex (and optionally
	/// a timeout value in milliseconds) and locks it.
	/// The destructor unlocks the mutex.
{
public:
	explicit ScopedLockTimed(M& mutex,unsigned int TimeoutMs) :
		mMutex		( mutex ),
		mIsLocked	( false )
	{
		mIsLocked = mMutex.tryLock( TimeoutMs );
	}
	
	~ScopedLockTimed()
	{
		if ( mIsLocked )
			mMutex.unlock();
	}

	bool	IsLocked() const	{	return mIsLocked;	}

private:
	M&		mMutex;
	bool	mIsLocked;

	ScopedLockTimed();
	ScopedLockTimed(const ScopedLockTimed&);
	ScopedLockTimed& operator = (const ScopedLockTimed&);
};


#include <thread>
#include <mutex>


#if defined(TARGET_WINDOWS)
    #include <Shlwapi.h>
    #pragma comment(lib,"Shlwapi.lib")
#endif



class ofMutex
{
public:
	typedef ScopedLock<ofMutex> ScopedLock;

public:
	void lock()				{	mMutex.lock(); }
	void unlock()			{	mMutex.unlock();	}
	bool tryLock()			{	return mMutex.try_lock();	}

private:
    std::recursive_mutex    mMutex;
};


#if !defined(TARGET_ANDROID)
class ofMutexTimed
{
public:
	typedef ScopedLockTimed<ofMutexTimed> ScopedLockTimed;
	typedef ScopedLock<ofMutexTimed> ScopedLock;

public:
	void lock()					{	mMutex.lock(); }
	void unlock()				{	mMutex.unlock();	}
	bool tryLock()				{	return mMutex.try_lock();	}
	bool tryLock(int TimeoutMs)	{	return mMutex.try_lock_for( std::chrono::milliseconds(TimeoutMs) );	}

private:
    std::recursive_timed_mutex	mMutex;
};
#endif








template<class OBJECT>
class ofMutexT : public OBJECT, public ofMutex
{
public:
	ofMutexT()
	{
	}
	template<typename PARAM>
	explicit ofMutexT(PARAM& Param) :
		OBJECT	( Param )
	{
	}
	template<typename PARAM>
	explicit ofMutexT(const PARAM& Param) :
		OBJECT	( Param )
	{
	}

	OBJECT&			Get()				{	return *this;	}
	const OBJECT&	Get() const			{	return *this;	}
	ofMutex&		GetMutex()			{	return *this;	}
	ofMutex&		GetMutex() const	{	return const_cast<ofMutex&>( static_cast<const ofMutex&>(*this) );	}
};


template<class OBJECT>
class ofMutexM : public ofMutex
{
public:
	ofMutexM()
	{
	}
	template<typename PARAM>
	explicit ofMutexM(const PARAM& Param) :
		mMember	( Param )
	{
	}

	OBJECT&			Get()				{	return mMember;	}
	const OBJECT&	Get() const			{	return mMember;	}
	ofMutex&		GetMutex()			{	return *this;	}
	ofMutex&		GetMutex() const	{	return const_cast<ofMutex&>( static_cast<const ofMutex&>(*this) );	}

public:
	OBJECT			mMember;
};



//	thread wrapper. basically a giant while loop, if real efficiency is required, use the SoyWorkerThread which waits/sleeps/wakes on conditionals
class SoyThread
{
public:
	//	global thread events for platform hooks
	static SoyEvent<SoyThread>	OnThreadFinish;	//	call whilst in thread
	static SoyEvent<SoyThread>	OnThreadStart;	//	call whilst in thread

public:
	SoyThread(const std::string& ThreadName);
	virtual ~SoyThread();
	
	void					Start();
	void					Stop(bool WaitToFinish);			//	signal thread to stop
	void					WaitToFinish()				{	Stop(true);	}
	const std::string&		GetThreadName() const		{	return mThreadName;	}

	//bool					IsCurrentThread() const	{	return GetThreadId() == std::this_thread::get_id();	}
	std::thread::native_handle_type	GetThreadNativeHandle() 	{	return mThread.native_handle();	}
	static std::thread::native_handle_type	GetCurrentThreadNativeHandle();
	static std::thread::id					GetCurrentThreadId()	{	return std::this_thread::get_id();	}
	static std::string						GetThreadName(std::thread::native_handle_type ThreadHandle);
	static std::string						GetCurrentThreadName();
	
	static prmem::Heap&		GetHeap(std::thread::native_handle_type Thread);
	void					CleanupHeap();
	
protected:
	virtual void			Thread()=0;				//	wrapped in an IsRunning() loop
	bool					HasThread() const		{	return mThread.get_id() != std::thread::id();	}

private:
	static void				SetThreadName(const std::string& Name,std::thread::native_handle_type ThreadHandle);
	void					SetThreadName(const std::string& Name)		{	SetThreadName( Name, mThread.native_handle() );	}

	
	/*
	bool				startThread(bool blocking, bool verbose);
	void				stopThread();
	bool				isThreadRunning()					{	return mIsRunning;	}
	void				waitForThread(bool stop = true);
	std::thread::id		GetThreadId() const					{	return mThread.get_id();	}
	std::string			GetThreadName() const				{	return mThreadName;	}
	static void			Sleep(size_t ms=0)
	{
		std::this_thread::sleep_for( std::chrono::milliseconds(ms) );
	}
	
#if defined(TARGET_WINDOWS)
	DWORD			GetNativeThreadId()					{	return ::GetThreadId( GetThreadHandle() );	}
	HANDLE			GetThreadHandle()					{	return static_cast<HANDLE>( mThread.native_handle() );	}
#elif defined(TARGET_OSX)
	//    typedef pthread_t native_handle_type;
	std::thread::native_handle_type		GetNativeThreadId()					{	return mThread.native_handle();	}
#endif
	
protected:
	bool				create(unsigned int stackSize=0);
	void				destroy();
*/
private:
	std::string			mThreadName;
	volatile bool		mIsRunning;
	std::thread			mThread;
	SoyListenerId		mNameThreadListener;
	SoyListenerId		mHeapThreadListener;
};


//	gr: maybe change this to a policy?
namespace SoyWorkerWaitMode
{
	enum Type
	{
		NoWait,	//	no waiting, assume Iteration() blocks
		Wake,	//	manually woken
		Sleep,	//	don't wait for a wake, but do a sleep
	};
}

class SoyWorker
{
public:
	SoyWorker(SoyWorkerWaitMode::Type WaitMode) :
		mWaitMode	( WaitMode )
	{
	}
	virtual ~SoyWorker()	{}
	
	virtual std::string	WorkerName() const	{	return "SoyWorker";	}
	virtual void		Start();
	virtual void		Stop();
	bool				IsWorking() const	{	return mWorking;	}
	
	virtual bool		Iteration()=0;	//	return false to stop thread

	virtual void		Wake();
	template<typename TYPE>
	SoyListenerId		WakeOnEvent(SoyEvent<TYPE>& Event)
	{
		//	gr: maybe we can have some global that goes "this is no longer valid"
		auto HandlerFunc = [this](TYPE& Param)
		{
			this->Wake();
		};

		//	gr: can't really add a listener and then unsubscribe here... what if the event dies...
		return Event.AddListener( HandlerFunc );
	}
	void				SetWakeMode(SoyWorkerWaitMode::Type WakeMode)	{	mWaitMode = WakeMode;	Wake();	}
	SoyWorkerWaitMode::Type	GetWakeMode() const	{	return mWaitMode;	}

protected:
	virtual bool		CanSleep()							{	return true;	}	//	break out of conditional with this
	virtual std::chrono::milliseconds	GetSleepDuration()	{	return std::chrono::milliseconds(1000/60);	}

private:
	void				Loop();
	
public:
	SoyEvent<bool>		mOnPreIteration;
	SoyEvent<bool>		mOnStart;
	SoyEvent<bool>		mOnFinish;
	
private:
	std::condition_variable	mWaitConditional;
	std::mutex				mWaitMutex;
	bool					mWorking;
	SoyWorkerWaitMode::Type	mWaitMode;
};

class SoyWorkerDummy : public SoyWorker
{
public:
	SoyWorkerDummy() :
		SoyWorker	( SoyWorkerWaitMode::Wake )
	{
	}
	virtual bool		Iteration()	{	return true;	};
};

class SoyWorkerThread : public SoyWorker, protected SoyThread
{
public:
	SoyWorkerThread(std::string ThreadName,SoyWorkerWaitMode::Type WaitMode) :
		SoyWorker	( WaitMode ),
		SoyThread	( ThreadName )
	{
	}
	~SoyWorkerThread()
	{
		//	hail mary
		if ( IsWorking() )
			WaitToFinish();
	}
	
	virtual void		Start() override		{	Start( true );	}
	void				Start(bool ThrowIfAlreadyStarted);
	virtual void		Stop() override			{	SoyWorker::Stop();	SoyThread::Stop(false);	}
	void				WaitToFinish()			{	SoyWorker::Stop();	SoyThread::Stop(true);	}

	SoyThread&			GetThread()				{	return *this;	}
	const SoyThread&	GetThread() const		{	return *this;	}
//	std::thread::id		GetThreadId() const		{	return SoyThread::get_id();	}
//	const std::string&	GetThreadName() const	{	return SoyThread::GetThreadName();	}
	std::thread::native_handle_type	GetThreadNativeHandle() 	{	return SoyThread::GetThreadNativeHandle();	}

protected:
//	bool				HasThread() const		{	return SoyThread::get_id() != std::thread::id();	}
	virtual void		Thread() override;
};



class SoyWorkerJobThread : public SoyWorkerThread, public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	SoyWorkerJobThread(const std::string& Name) :
		SoyWorkerThread	( Name, SoyWorkerWaitMode::Wake )
	{
		WakeOnEvent( PopWorker::TJobQueue::mOnJobPushed );
	}
	
	//	thread
	virtual bool		Iteration() override	{	PopWorker::TJobQueue::Flush(*this);	return true;	}
	virtual bool		CanSleep() override		{	return !PopWorker::TJobQueue::HasJobs();	}	//	break out of conditional with this
	
	//	context
	virtual bool		Lock() override			{	return true;	}
	virtual void		Unlock() override		{}
};

/*
class SoyFunctionId
{
private:
	static uint64			gIdCounter;
	static ofMutex			gIdCounterLock;

public:
	static SoyFunctionId	Alloc()
	{
		ofMutex::ScopedLock Lock( gIdCounterLock );
		return SoyFunctionId( ++gIdCounter );
	}

public:
	SoyFunctionId(uint64 Id=0) :
		mId	( Id )
	{
	}

	operator	bool() const		{	return IsValid();	}
	bool		IsValid() const		{	return mId!=0;	}
	bool		operator==(const SoyFunctionId& Id) const	{	return mId == Id.mId;	}
	bool		operator!=(const SoyFunctionId& Id) const	{	return mId != Id.mId;	}

private:
	int			mId;
};

class SoyFunctionThread : public SoyThread
{
public:
	typedef std::function<void(void)> FUNCTION;

public:
	SoyFunctionThread(std::string Name) :
		SoyThread	( Name )
	{
		startThread(true,true);
	}

	SoyFunctionId		QueueFunction(FUNCTION Function)
	{
		if ( !mLock.tryLock() )
			return SoyFunctionId();
		mFunction = Function;
		SoyFunctionId FunctionId = SoyFunctionId::Alloc();
		mFunctionId = FunctionId;
		mLock.unlock();
		return FunctionId;
	}

	inline bool			operator==(SoyFunctionId FuncctionId) const
	{
		return (mFunctionId == FuncctionId);
	}

	virtual void	threadedFunction() override
	{
		while ( isThreadRunning() )
		{
			sleep(0);

			//	grab function and execute it
			if ( !mLock.tryLock() )
				continue;
			
			if ( mFunction )
			{
				mFunction();
				mFunction = nullptr;
				mFunctionId = SoyFunctionId();
			}
			mLock.unlock();
		}
	}

private:
	ofMutex			mLock;
	FUNCTION		mFunction;
	SoyFunctionId	mFunctionId;
};

class SoyScheduler
{
public:
	typedef std::function<void(void)> FUNCTION;

public:
	SoyScheduler(int ThreadCount)
	{
		for ( int i=0;	i<ThreadCount;	i++ )
		{
			std::stringstream ThreadName;
			ThreadName << "SoyFunctionThread[" << i << "]";
			mThreadPool.push_back().reset( new SoyFunctionThread(ThreadName.str()) );
		}
	}
	~SoyScheduler()
	{
		//	stop them all
		for ( auto it=mThreadPool.begin();	it!=mThreadPool.end();	it++ )
			(*it)->stopThread();

		//	now wait 
		for ( auto it=mThreadPool.begin();	it!=mThreadPool.end(); )
			(*it)->waitForThread();

		//	delete
		mThreadPool.clear();
	}

	SoyFunctionId		Run(FUNCTION Function)
	{
		//	keep looping through pool until there's a free one
		int i=0;
		while ( true )
		{
			auto& Thread = *mThreadPool[i];
			auto Id = Thread.QueueFunction( Function );
			if ( Id )
				return Id;

			i = (i+1) % mThreadPool.size();
		}
		assert(false);
		return SoyFunctionId();
	}

	void				Wait(SoyFunctionId FunctionId)
	{
		//	find thread with id, then wait for it
		for ( auto it=mThreadPool.begin();	it!=mThreadPool.end(); )
		{
			auto& Thread = *it;
			//	if this thread IS running this function id, wait for it
			while ( Thread == FunctionId )
			{
				//SwitchToThread();
				sleep(0);
			}
		}
	}

protected:
	std::vector<std::shared_ptr<SoyFunctionThread>>	mThreadPool;
};
*/




template<class TYPE>
class TLockQueue
{
public:
	TLockQueue(prmem::Heap& Heap=prcore::Heap) :
		mJobs	( Heap )
	{
	}
	bool			IsEmpty() const			{	return mJobs.IsEmpty();	}
	TYPE			Pop();
	bool			Push(const TYPE& Job);
	
public:
	Array<TYPE>			mJobs;
	ofMutex				mJobLock;
	SoyEvent<size_t>	mOnQueueAdded;
};

template<class TYPE>
inline TYPE TLockQueue<TYPE>::Pop()
{
	ofMutex::ScopedLock Lock( mJobLock );
	if ( mJobs.IsEmpty() )
		return TYPE();
	return mJobs.PopAt(0);
}

template<class TYPE>
inline bool TLockQueue<TYPE>::Push(const TYPE& Job)
{
	//assert( Job.IsValid() );
	size_t JobCount;
	{
		ofMutex::ScopedLock Lock( mJobLock );
		mJobs.PushBack( Job );
		JobCount = mJobs.GetSize();
	}
	mOnQueueAdded.OnTriggered(JobCount);
	return true;
}


class Soy::Mutex_Profiled
{
public:
	bool			try_lock()
	{
		return mMutex.try_lock();
	}

	void			lock(const std::string& LockName/*="<Unnamed>"*/,ssize_t WarningMs=-1)
	{
		static size_t DefaultMs = 5;
		if ( WarningMs < 0 )
			WarningMs = DefaultMs;
		
		//	measure how long we wait to acquire lock
		SoyTime LockTime(true);

		//	immediate uncontested lock
		if ( mMutex.try_lock() )
			return;
		
		mMutex.lock();
		
		//	got lock
		//	gr: this is optimised away
		SoyTime AcquireTime(true);
		auto AcquireMs = AcquireTime.GetTime() - LockTime.GetTime();
		//if ( AcquireMs >= WarningMs )
		{
			std::Debug << "Contested lock " << LockName << " took " << AcquireMs << "ms to acquire" << std::endl;
		}
	}
	
	void			unlock()
	{
		mMutex.unlock();
	}
	
	
	std::mutex		mMutex;
};

