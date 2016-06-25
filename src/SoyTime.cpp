#include "SoyTime.h"
#include <regex>
#include <sstream>


bool SoyTime::FromString(const std::string& String)
{
	std::regex Pattern("T?([0-9]+)$" );
	std::smatch Match;
	if ( !std::regex_match( String, Match, Pattern ) )
		return false;
	
	//	extract long long
	auto IntegerStr = Match[1].str();
	auto Time = std::stoll( IntegerStr );
	if ( Time < 0 )
		return false;
	mTime = Time;
	return true;
}


std::string SoyTime::ToString() const
{
	std::stringstream ss;
	ss << (*this);
	return ss.str();
}

SoyTime SoyTime::Now()
{
#if defined(TARGET_WINDOWS)
	auto MilliSecs = timeGetTime();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_ANDROID)||defined(TARGET_PS4)
	struct timeval now;
	gettimeofday( &now, NULL );
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

