#pragma once

#include "SoyTypes.h"
#include "string.hpp"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyEvent.h"
#include "SoyScope.h"


namespace std
{
	class DebugStreamBuf : public streambuf
	{
	public:
		DebugStreamBuf()  { };
		~DebugStreamBuf()	{	flush();	}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);		// disallow copy construction
		void operator= (DebugStreamBuf const &);	// disallow copy assignment

		std::string&	GetBuffer();
		
	public:
		SoyEvent<const std::string>	mOnFlush;		//	catch debug output
	};

	class DebugStream : public basic_ostream<char,std::char_traits<char> >
	{
	public:
		explicit DebugStream() : 
			std::basic_ostream<char,std::char_traits<char> > (&mBuffer) 
		{
		}

		SoyEvent<const std::string>&		GetOnFlushEvent()	{	return mBuffer.mOnFlush;	}
		
	private:
		DebugStreamBuf	mBuffer;
	};

};


namespace std
{
	extern DebugStream	Debug;
}

namespace Soy
{
	BufferString<20>	FormatSizeBytes(uint64 bytes);
	
	namespace Platform
	{
		bool		IsDebuggerAttached();
		void		DebugPrint(const std::string& String);
	}
}


//	gr: oops, OF ofLogNotice isn't a function, this works for both
inline void ofLogNoticeWrapper(const std::string& Message)
{
	ofLogNotice( Message.c_str() );
}


class ofScopeTimerWarning
{
public:
	ofScopeTimerWarning(const char* Name,uint64 WarningTimeMs,bool AutoStart=true,ofDebugPrintFunc DebugPrintFunc=ofLogNoticeWrapper) :
		mName				( Name ),
		mWarningTimeMs		( WarningTimeMs ),
		mStopped			( true ),
		mReportedOnLastStop	( false ),
		mAccumulatedTime	( 0 ),
		mDebugPrintFunc		( DebugPrintFunc )
	{
		if ( AutoStart )
			Start( true );
	}
	~ofScopeTimerWarning()
	{
		if ( mStopped && !mReportedOnLastStop )
			Report();
		else
			Stop();
	}
	//	returns if a report was output
	bool				Stop(bool DoReport=true)
	{
		if ( mStopped )
		{
			mReportedOnLastStop = false;
			return false;
		}

		SoyTime Now(true);
		uint64 Delta = Now.GetTime() - mStartTime.GetTime();
		mAccumulatedTime += Delta;

		mReportedOnLastStop = DoReport;
		bool DidReport = false;
		if ( DoReport )
			DidReport = Report();
		
		mStopped = true;
		return DidReport;
	}
	bool				Report(bool Force=false)
	{
		if ( mAccumulatedTime >= mWarningTimeMs || Force )
		{
			if ( mDebugPrintFunc )
			{
				BufferString<200> Debug;
				Debug << mName << " took " << mAccumulatedTime << "ms to execute";
				(*mDebugPrintFunc)( static_cast<const char*>( Debug ) );
			}
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

	SoyTime				mStartTime;
	uint64				mWarningTimeMs;
	BufferString<200>	mName;
	bool				mStopped;
	bool				mReportedOnLastStop;
	uint64				mAccumulatedTime;
	ofDebugPrintFunc	mDebugPrintFunc;
};

