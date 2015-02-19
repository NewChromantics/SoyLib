#pragma once

#include "ofxSoylent.h"
#include "SoyEvent.h"
#include <condition_variable>


#if defined(NO_OPENFRAMEWORKS)

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
#endif

#include <thread>
#include <mutex>


#if defined(TARGET_WINDOWS)
    #include <Shlwapi.h>
    #pragma comment(lib,"Shlwapi.lib")
#endif

#if defined(NO_OPENFRAMEWORKS)

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

#endif // NO_OPENFRAMEWORKS




#if defined(NO_OPENFRAMEWORKS)


class ofThread
{
public:
	ofThread();
	virtual ~ofThread();

	bool			startThread(bool blocking, bool verbose);
	void			stopThread();
	bool			isThreadRunning()					{	return mIsRunning;	}
	void			waitForThread(bool stop = true);
	std::thread::id		GetThreadId() const					{	return mThread.get_id();	}
	std::string		GetThreadName() const				{	return mThreadName;	}
	static void		Sleep(int ms=0)
	{
		//	slow down cpu usage on OSX until I find out how to do this properly
#if defined(TARGET_OSX)
		ms += 10;
#endif
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
	bool			create(unsigned int stackSize=0);
	void			destroy();
	virtual void	threadedFunction() = 0;

	static unsigned int STDCALL	threadFunc(void *args);

protected:
	std::string		mThreadName;

private:
	volatile bool	mIsRunning;

    std::thread		mThread;
};
#endif // NO_OPENFRAMEWORKS








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




class SoyThread : public ofThread
{
public:
	SoyThread(std::string threadName)	
	{
		if ( !threadName.empty() )
		{
			mThreadName	= threadName;
			SetThreadName( threadName );
		}
	}

	static std::thread::native_handle_type	GetCurrentThreadNativeHandle()
	{
#if defined(TARGET_WINDOWS)
		return ::GetCurrentThread();
#endif
	}
	static std::thread::id	GetCurrentThreadId()
	{
        return std::this_thread::get_id();
	}

	void	startThread(bool blocking=true, bool verbose=true)
	{
		ofThread::startThread( blocking, verbose );
#if defined(NO_OPENFRAMEWORKS)
		SetThreadName( mThreadName );
#else
		SetThreadName( getPocoThread().getName() );
#endif
	}

	void		SetThreadName(std::string Name)	{	SetThreadName(Name, GetCurrentThreadNativeHandle()); }
	static void	SetThreadName(std::string Name,std::thread::native_handle_type ThreadHandle);
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
	void				WakeOnEvent(SoyEvent<TYPE>& Event)
	{
		auto HandlerFunc = [this](TYPE& Param)
		{
			this->Wake();
		};
		Event.AddListener( HandlerFunc );
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

class SoyWorkerThread : public SoyWorker
{
public:
	SoyWorkerThread(std::string ThreadName,SoyWorkerWaitMode::Type WaitMode) :
		SoyWorker	( WaitMode ),
		mThreadName	( ThreadName )
	{
		SoyWorker::mOnStart.AddListener( *this, &SoyWorkerThread::OnStart );
	}
	
	virtual void		Start() override;
	void				WaitToFinish();
	std::thread::id		GetThreadId() const		{	return mThread.get_id();	}
	std::thread::native_handle_type	GetThreadNativeHandle() 	{	return mThread.native_handle();	}
	bool				IsCurrentThread() const	{	return GetThreadId() == std::this_thread::get_id();	}

protected:
	bool				HasThread() const		{	return mThread.get_id() != std::thread::id();	}
	void				OnStart(bool&)			{	SoyThread::SetThreadName( mThreadName, GetThreadNativeHandle() );	}
	
private:
	std::string			mThreadName;
	std::thread			mThread;
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
	bool			IsEmpty() const			{	return mJobs.IsEmpty();	}
	TYPE			Pop();
	bool			Push(const TYPE& Job);
	
public:
	Array<TYPE>		mJobs;
	ofMutex			mJobLock;
	SoyEvent<int>	mOnQueueAdded;
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
	int JobCount;
	{
		ofMutex::ScopedLock Lock( mJobLock );
		mJobs.PushBack( Job );
		JobCount = mJobs.GetSize();
	}
	mOnQueueAdded.OnTriggered(JobCount);
	return true;
}

