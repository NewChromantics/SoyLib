#include "SoyTime.h"
#include <regex>


bool SoyTime::FromString(std::string String)
{
	std::regex Pattern("T?([0-9]+)$" );
	std::smatch Match;
	if ( !std::regex_match( String, Match, Pattern ) )
		return false;
	
	//	extract int
	auto& IntegerStr = Match[1].str();
	int Time = std::stoi( IntegerStr );
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

