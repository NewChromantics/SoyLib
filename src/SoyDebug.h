#pragma once

#include "SoyTypes.h"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyEvent.h"
#include "SoyScope.h"
#include "SoyString.h"


#if !defined(TARGET_ANDROID)
#define USE_HEAP_STRING
#endif

namespace std
{
	class DebugStreamThreadSafeWrapper;
	
	//	cross platform debug output stream
	//	std::Debug << XXX << std::endl;
	extern DebugStreamThreadSafeWrapper	Debug;
}


namespace Soy
{
	namespace Platform
	{
		bool	DebugBreak();
		bool	IsDebuggerAttached();
		void	DebugPrint(const std::string& String);
	}
	
};




namespace std
{
#if defined USE_HEAP_STRING
	typedef Soy::HeapString DebugBufferString;
#else
	typedef std::string DebugBufferString;
#endif

	class DebugStreamBuf : public streambuf
	{
	public:
		DebugStreamBuf() :
			mEnableStdOut	( true )
		{
		};
		~DebugStreamBuf()	
		{
			//	gr: currently WINDOWS on cleanup (OSX can't reach here atm but assume it suffers too)
			//		the debug heap gets destroyed (as its created later?) before the string that is flushed does
			//		so on flsh here, pointer still exists, but heap it was allocated from has gone.
			//		solution: subscribe to heap destruction?
			//		temp solution: no cleanup!
			//flush();	
		}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);		// disallow copy construction
		void operator= (DebugStreamBuf const &);	// disallow copy assignment

		DebugBufferString&	GetBuffer();
		
	public:
		bool			mEnableStdOut;
		SoyEvent<const std::string>	mOnFlush;		//	catch debug output
	};

	class DebugStream : public std::ostream
	{
	public:
		explicit DebugStream() : 
			std::basic_ostream<char,std::char_traits<char> > (&mBuffer) 
		{
		}

		SoyEvent<const std::string>&		GetOnFlushEvent()	{	return mBuffer.mOnFlush;	}
		
		//	toggle std output for this std debug stream
		void			EnableStdOut(bool Enable)	{	mBuffer.mEnableStdOut = Enable;	}
		
	private:
		DebugStreamBuf	mBuffer;
	};

	
	
	
	
	class DebugStreamThreadSafeWrapper
	{
	public:
		explicit DebugStreamThreadSafeWrapper()
		{
		}
		
		DebugStream&	LockStream()
		{
			mStreamLock.lock();
			return mStream;
		}
		void			UnlockStream(DebugStream& Stream)
		{
			Soy::Assert( &Stream == &mStream, "Wrong stream" );
			mStreamLock.unlock();
		}
		
		template<typename TYPE>
		DebugStreamThreadSafeWrapper&	operator<<(const TYPE& x)
		{
			std::lock_guard<std::recursive_mutex> Lock(mStreamLock);
			mStream << x;
			return *this;
		}

		// function that takes a custom stream, and returns it
		typedef DebugStreamThreadSafeWrapper& (*MyStreamManipulator)(DebugStreamThreadSafeWrapper&);
		
		// take in a function with the custom signature
		DebugStreamThreadSafeWrapper& operator<<(MyStreamManipulator manip)
		{
			std::lock_guard<std::recursive_mutex> Lock(mStreamLock);
			manip(*this);
			return *this;
		}
		/*
		static DebugStream& endl(DebugStream& Stream)
		{
			std::lock_guard<std::recursive_mutex> Lock(mStreamLock);
			Stream.mStream << std::endl;
			return Stream;
		}
		*/
		// this is the type of std::cout
		typedef std::basic_ostream<char, std::char_traits<char> > StreamType;
		
		// this is the function signature of std::endl
		typedef StreamType& (*Fstd_endl)(StreamType&);
		
		// define an operator<< to take in std::endl
		DebugStreamThreadSafeWrapper& operator<<(Fstd_endl manip)
		{
			// call the function, but we cannot return it's value
			manip(mStream);
			
			return *this;
		}
		
		SoyEvent<const std::string>&		GetOnFlushEvent()	{	return mStream.GetOnFlushEvent();	}
		
		//	toggle std output for this std debug stream
		void			EnableStdOut(bool Enable)	{	mStream.EnableStdOut(Enable);	}
		
		void			PushStreamSettings()
		{
			std::shared_ptr<Soy::TPushPopStreamSettings> Push( new Soy::TPushPopStreamSettings(mStream) );
			mPushPopSettings.push_back( Push );
		}
		void			PopStreamSettings()
		{
			mPushPopSettings.pop_back();
		}
		
	private:
		std::recursive_mutex	mStreamLock;
		DebugStream				mStream;
		
		//	gr: find a better stack system than allocating!
		std::vector<std::shared_ptr<Soy::TPushPopStreamSettings>>	mPushPopSettings;
	};
};



//	gr: move this to... string?
namespace Soy
{
	std::string	FormatSizeBytes(uint64 bytes);
	class TScopeTimer;
	class TScopeTimerPrint;
	class TScopeTimerStream;
	class TScopeTimerFunc;
}
//	legacy typename
typedef Soy::TScopeTimerPrint ofScopeTimerWarning;



class Soy::TScopeTimer
{
public:
	TScopeTimer(const char* Name,uint64 WarningTimeMs,std::function<void(SoyTime)> ReportFunc,bool AutoStart) :
		mWarningTimeMs		( WarningTimeMs ),
		mStopped			( true ),
		mReportedOnLastStop	( false ),
		mAccumulatedTime	( 0 ),
		mReportFunc			( ReportFunc )
	{
		//	visual studio 2013 won't let me user initialiser
		mName[0] = '\0';
		
		Soy::StringToBuffer( Name, mName );
		if ( AutoStart )
			Start( true );
	}
	~TScopeTimer()
	{
		if ( mStopped && !mReportedOnLastStop )
			Report();
		else
			Stop();
	}
	
	void				SetName(const std::string& Name)
	{
		Soy::StringToBuffer( Name, mName );
	}
	
	//	returns accumulated time since last stop
	SoyTime				Stop(bool DoReport=true)
	{
		if ( mStopped )
		{
			mReportedOnLastStop = false;
			return SoyTime();
		}

		SoyTime Now(true);
		uint64 Delta = Now.GetTime() - mStartTime.GetTime();
		mAccumulatedTime += Delta;

		mReportedOnLastStop = DoReport;
		bool DidReport = false;
		if ( DoReport )
			DidReport = Report();
		
		mStopped = true;
		return SoyTime( Delta );
	}
	bool				Report(bool Force=false)
	{
		if ( mAccumulatedTime >= mWarningTimeMs || Force )
		{
			if ( mReportFunc )
				mReportFunc( SoyTime(mAccumulatedTime) );
			//else
			//	std::Debug << mName << " took " << mAccumulatedTime << "ms to execute" << std::endl;
			return true;
		}
		return false;
	}

	void				Start(bool Reset=false)
	{
		assert( mStopped );

		if ( Reset )
			mAccumulatedTime = 0;
			
		SoyTime Now(true);
		mStartTime = Now;
		mStopped = false;
	}
	
protected:
	std::function<void(SoyTime)>	mReportFunc;
	SoyTime				mStartTime;
	uint64				mWarningTimeMs;
	char				mName[300];
	bool				mStopped;
	bool				mReportedOnLastStop;
	uint64				mAccumulatedTime;
};


class Soy::TScopeTimerPrint : public TScopeTimer
{
public:
	TScopeTimerPrint(const char* Name,uint64 WarningTimeMs,bool AutoStart=true) :
		TScopeTimer		( Name, WarningTimeMs, std::bind( &TScopeTimerPrint::ReportStr, this, std::placeholders::_1 ), AutoStart )
	{
	}

protected:
	void		ReportStr(SoyTime Time)
	{
		std::Debug << mName << " took " << Time.mTime << "ms/" << mWarningTimeMs << "ms to execute" << std::endl;
	}

protected:
};


class Soy::TScopeTimerStream : public TScopeTimer
{
public:
	TScopeTimerStream(const char* Name,uint64 WarningTimeMs,bool AutoStart,std::ostream& Output) :
		TScopeTimer		( Name, WarningTimeMs, std::bind( &TScopeTimerStream::ReportStr, this, std::placeholders::_1 ), AutoStart ),
		mOutputStream	( Output )
	{
	}
	
protected:
	void		ReportStr(SoyTime Time)
	{
		mOutputStream << mName << " took " << Time.mTime << "ms to execute" << std::endl;
	}
	
protected:
	std::ostream&		mOutputStream;
};

