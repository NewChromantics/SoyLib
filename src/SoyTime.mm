#include "SoyTime.h"

#if defined(__OBJC__)
SoyTime Soy::Platform::GetTime(CMTime Time)
{
	if ( CMTIME_IS_INVALID( Time ) )
		return SoyTime();
	
	Float64 TimeSec = CMTimeGetSeconds(Time);
	UInt64 TimeMs = 1000.f*TimeSec;
	return SoyTime( TimeMs );
}
#endif


#if defined(__OBJC__)
CMTime Soy::Platform::GetTime(SoyTime Time)
{
	return CMTimeMake( Time.mTime, 1000 );
}
#endif

