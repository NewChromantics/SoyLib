#pragma once

#include "SoyTypes.h"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyScope.h"
#include "SoyString.h"


#include <iostream>

namespace std
{
	inline std::ostream& Debug = cerr;	//	inline variable is c++17
}


namespace Platform
{
	bool	DebugBreak();
	bool	IsDebuggerAttached();
	void	DebugPrint(const char* String);
	
#if defined(TARGET_LUMIN) || defined(TARGET_ANDROID)
	extern const char*	LogIdentifer;	//	define this in your app
#endif
}



//	gr: move this to... string?
namespace Soy
{
	std::string	FormatSizeBytes(uint64 bytes);
	class TScopeTimer;
	class TScopeTimerPrint;
	class TScopeTimerStream;
	class TScopeTimerFunc;
}



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
		return std::chrono::milliseconds( Delta );
	}
	bool				Report(bool Force=false)
	{
		if ( mAccumulatedTime >= mWarningTimeMs || Force )
		{
			if ( mReportFunc )
				mReportFunc( std::chrono::milliseconds(mAccumulatedTime) );
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

