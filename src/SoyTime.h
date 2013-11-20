#pragma once


#include "String.hpp"


class SoyTime
{
public:
	explicit SoyTime(bool InitToNow=false) : 
		mTime	( InitToNow ? Now().GetTime() : 0 )
	{
	}
	explicit SoyTime(uint64 Time) :
		mTime	( Time )
	{
	}
	SoyTime(const SoyTime& Time) :
		mTime	( Time.GetTime() )
	{
	}
	template <typename S,class ARRAYTYPE>
	explicit SoyTime(const Soy::String2<S,ARRAYTYPE>& String) :
		mTime	( 0 )
	{
		//	format T012345678
		//	check length
		if ( String.GetLength() == 9+1 )
		{
			if ( String[0] == 'T' )
			{
				//	get int from the numbers
				BufferString<10> IntString( &String[1] );
				int32 Value;
				if ( IntString.GetInteger( Value ) )
					mTime = Value;
			}
		}
	}


	uint64			GetTime() const							{	return mTime;	}
	bool			IsValid() const							{	return mTime!=0;	}
	static SoyTime	Now()									{	return SoyTime( ofGetElapsedTimeMillis()+1 );	}	//	we +1 so we never have zero for a "real" time

	inline bool		operator==(const SoyTime& Time) const	{	return mTime == Time.mTime;	}
	inline bool		operator!=(const SoyTime& Time) const	{	return mTime != Time.mTime;	}
	inline bool		operator<(const SoyTime& Time) const	{	return mTime < Time.mTime;	}
	inline bool		operator<=(const SoyTime& Time) const	{	return mTime <= Time.mTime;	}
	inline bool		operator>(const SoyTime& Time) const	{	return mTime > Time.mTime;	}
	inline bool		operator>=(const SoyTime& Time) const	{	return mTime >= Time.mTime;	}
	inline SoyTime&	operator+=(const uint64& Step) 			{	mTime += Step;	return *this;	}
	inline SoyTime&	operator+=(const SoyTime& Step)			{	mTime += Step.GetTime();	return *this;	}

private:
	uint64	mTime;
};
DECLARE_TYPE_NAME( SoyTime );



template<class STRING>
inline STRING& operator<<(STRING& str,const SoyTime& Time)
{
	BufferString<100> Buffer;
	Buffer.PrintText("T%09Iu", Time.GetTime() );
	str << Buffer;
	return str;
}


template<class STRING>
inline const STRING& operator>>(const STRING& str,SoyTime& Time)
{
	if ( str.GetLength() != 10 )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	if ( str[0] != 'T' )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	BufferString<100> Buffer = static_cast<const char*>( &str[1] );
	int TimeValue = 0;
	if ( !Buffer.GetInteger( TimeValue ) )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	Time = SoyTime( static_cast<uint64>( TimeValue ) );
	return str;
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

