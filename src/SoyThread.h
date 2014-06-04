#pragma once

#include "ofxSoylent.h"




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



//  different types on different platforms. No need for it to be an int
class SoyThreadId
{
public:
    typedef std::thread::id TYPE;
	static const TYPE		Invalid;

public:
    SoyThreadId() :
        mId ( Invalid )
    {
    }
    SoyThreadId(const TYPE& id) :
        mId ( id )
    {
    }

    inline bool     operator==(const TYPE& id) const        {   return mId == id;   }
    inline bool     operator==(const SoyThreadId& id) const {   return mId == id.mId;   }
    inline bool     operator!=(const TYPE& id) const        {   return mId != id;   }
    inline bool     operator!=(const SoyThreadId& id) const {   return mId != id.mId;   }
    inline bool     operator<(const TYPE& id) const			{   return mId < id;   }
    inline bool     operator<(const SoyThreadId& id) const	{   return mId < id.mId;   }
    inline bool     operator>(const TYPE& id) const			{   return mId > id;   }
    inline bool     operator>(const SoyThreadId& id) const	{   return mId > id.mId;   }
    
public:
    TYPE            mId;
};


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
	SoyThreadId     getThreadId() const					{	return mThread.get_id();	}
	void			sleep(int ms)						{	std::this_thread::sleep_for( std::chrono::milliseconds(ms) );	}

#if defined(TARGET_WINDOWS) 
	DWORD			GetNativeThreadId()					{	return ::GetThreadId( static_cast<HANDLE>( mThread.native_handle() ) );	}
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
	SoyThread(const char* threadName)
	{
		if ( threadName )
			setThreadName( threadName );
	}

	static SoyThreadId	GetCurrentThreadId()
	{
        return SoyThreadId( std::this_thread::get_id() );
	}

	void	startThread(bool blocking, bool verbose)
	{
		ofThread::startThread( blocking, verbose );
#if defined(NO_OPENFRAMEWORKS)
		setThreadName( mThreadName );
#else
		setThreadName( getPocoThread().getName() );
#endif
	}


	void	setThreadName(const std::string& name)
	{
#if defined(NO_OPENFRAMEWORKS)
		//auto ThreadId = getThreadId();
		mThreadName = name;
		int ThreadId = GetNativeThreadId();
#else
		auto ThreadId = getPocoThread().tid();
		getPocoThread().setName( name );
#endif
		
		//	set the OS thread name
		//	http://msdn.microsoft.com/en-gb/library/xcb2z8hs.aspx
	#if defined(TARGET_WINDOWS)
		const DWORD MS_VC_EXCEPTION=0x406D1388;
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
		info.szName = name.c_str();
		info.dwThreadID = ThreadId;
		info.dwFlags = 0;

		//	this will fail if the OS thread hasn't started yet
		if ( info.dwThreadID != 0 )
		{
			__try
			{
				RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	#endif
	}
};

