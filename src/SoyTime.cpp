#include "SoyTime.h"
#include <regex>
#include <sstream>
#include "SoyString.h"

#if defined(TARGET_OSX)||defined(TARGET_IOS)
#include <mach/mach_time.h>
#endif

//	gr: solve this properly and move to somewhere in SoyTypes
#if defined(TARGET_ANDROID)
namespace std
{
	long long	stoll(const std::string& IntegerStr)
	{
		Soy_AssertTodo();
	}
}
#endif

#if defined(TARGET_LINUX)
#include <unistd.h>
#include <sys/time.h>
#endif

bool SoyTime::FromString(const std::string& String)
{
	std::regex Pattern("T?([0-9]+)$" );
	std::smatch Match;
	if ( !std::regex_match( String, Match, Pattern ) )
		return false;
	
	//	extract long long
	auto IntegerStr = Match[1].str();
	auto Time = std::stoll( IntegerStr,0 ,0 );
	if ( Time < 0 )
		return false;
	mTime = Time;
	return true;
}


std::string SoyTime::ToString() const
{
	throw std::runtime_error("Dont use SoyTime::ToString");
}

SoyTime SoyTime::UpTime()
{
#if defined(TARGET_UWP) || defined(TARGET_WINDOWS)
	//	timeGetTime is ms since windows started, but not on UWP
	//auto MilliSecs = timeGetTime();
	auto MilliSecs = GetTickCount();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)
	//	https://stackoverflow.com/a/4753909/355753
	const int64_t OneMillion = 1000 * 1000;
	static mach_timebase_info_data_t s_timebase_info = {0,0};
	if ( s_timebase_info.numer == 0 )
		mach_timebase_info(&s_timebase_info);
	
	//	mach_absolute_time() returns billionth of seconds,
	//	so divide by one million to get milliseconds
	auto MilliSecs = (mach_absolute_time() * s_timebase_info.numer) / (OneMillion * s_timebase_info.denom);
	/*
	 var uptime = timespec()
	 if clock_gettime(CLOCK_MONOTONIC_RAW, &uptime) == 0 {
	 return Double(uptime.tv_sec) + Double(uptime.tv_nsec) / 1000000000.0
	 }
	 */
#elif defined(TARGET_ANDROID)||defined(TARGET_PS4)||defined(TARGET_LUMIN)||defined(TARGET_LINUX)
	struct timespec ts;
	unsigned theTick = 0U;
	clock_gettime(CLOCK_REALTIME, &ts);
	theTick = ts.tv_nsec / 1000000;
	theTick += ts.tv_sec * 1000;
	auto MilliSecs = theTick;
#else
	#error GetSystemTime undefined on target
#endif
	
	//	we +1 so we never have zero for a "real" time
	if ( MilliSecs == 0 )
		MilliSecs = 1;
	
	return std::chrono::milliseconds( MilliSecs );
}


SoyTime SoyTime::Now()
{
	//	ms since jan 1 1970
	//	https://stackoverflow.com/a/59359900/355753
#if defined(TARGET_WINDOWS) || defined(TARGET_UWP)
	auto DurationSinceEpoch = std::chrono::system_clock::now().time_since_epoch();
	auto DurationSinceEpochMs = std::chrono::duration_cast<std::chrono::milliseconds>(DurationSinceEpoch);
	auto MilliSecs = DurationSinceEpochMs.count();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_ANDROID)||defined(TARGET_PS4)||defined(TARGET_LUMIN)||defined(TARGET_LINUX)
	struct timeval now;
	auto Result = gettimeofday( &now, nullptr );
	if ( Result != 0 )
		Platform::ThrowLastError("gettimeofday");
	auto MilliSecs = (unsigned long long) now.tv_usec/1000 +
		(unsigned long long) now.tv_sec*1000;
#else 
#error GetSystemTime undefined on target
#endif
	
	//	we +1 so we never have zero for a "real" time
	if ( MilliSecs == 0 )
		MilliSecs = 1;

	return std::chrono::milliseconds( MilliSecs );
}

