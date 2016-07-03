#include "SoyTime.h"


SoyTime Soy::Platform::GetTime(CMTime Time)
{
	if ( CMTIME_IS_INVALID( Time ) )
		return SoyTime();
	
	Float64 TimeSec = CMTimeGetSeconds(Time);
	UInt64 TimeMs = 1000.f*TimeSec;
	return SoyTime( std::chrono::milliseconds(TimeMs) );
}


CMTime Soy::Platform::GetTime(SoyTime Time)
{
	return CMTimeMake( Time.mTime, 1000 );
}

SoyTime Soy::Platform::GetTime(CFTimeInterval Time)
{
	UInt64 TimeMs = Time * 1000.0;
	return SoyTime( std::chrono::milliseconds(TimeMs) );
}
