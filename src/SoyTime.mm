#include "SoyTime.h"

#if defined(__OBJC__)
SoyTime Soy::Platform::GetTime(CMTime Time)
{
	if ( CMTIME_IS_INVALID( Time ) )
		return SoyTime();
	
	//	missing CMTimeGetSeconds ? link to the CoreMedia framework :)
	Float64 TimeSec = CMTimeGetSeconds(Time);
	UInt64 TimeMs = 1000.f*TimeSec;
	return SoyTime( std::chrono::milliseconds(TimeMs) );
}
#endif


#if defined(__OBJC__)
CMTime Soy::Platform::GetTime(SoyTime Time)
{
	return CMTimeMake( Time.mTime, 1000 );
}
#endif

#if defined(__OBJC__)
SoyTime Soy::Platform::GetTime(CFTimeInterval Time)
{
	auto TimeMs = size_cast<uint64>(Time*1000.0);
	return SoyTime( std::chrono::milliseconds(TimeMs) );
}
#endif
