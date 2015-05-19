#pragma once

#include "SoyTypes.h"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyEvent.h"
#include "SoyScope.h"
#include "SoyString.h"



namespace std
{
	class DebugStream;
	
	//	cross platform debug output stream
	//	std::Debug << XXX << std::endl;
	extern DebugStream	Debug;
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
	class DebugStreamBuf : public streambuf
	{
	public:
		DebugStreamBuf() :
			mEnableStdOut	( true )
		{
		};
		~DebugStreamBuf()	{	flush();	}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);		// disallow copy construction
		void operator= (DebugStreamBuf const &);	// disallow copy assignment

		Soy::HeapString&	GetBuffer();
		
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

};



//	gr: move this to... string?
namespace Soy
{
	std::string	FormatSizeBytes(uint64 bytes);
	
}


class ofScopeTimerWarning
{
public:
	ofScopeTimerWarning(const char* Name,uint64 WarningTimeMs,bool AutoStart=true,std::ostream& Output=std::Debug) :
		mName				( Name ),
		mWarningTimeMs		( WarningTimeMs ),
		mStopped			( true ),
		mReportedOnLastStop	( false ),
		mAccumulatedTime	( 0 ),
		mOutputStream		( Output )
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
			mOutputStream << mName << " took " << mAccumulatedTime << "ms to execute" << std::endl;
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
	std::string			mName;
	bool				mStopped;
	bool				mReportedOnLastStop;
	uint64				mAccumulatedTime;
	std::ostream&		mOutputStream;
};



