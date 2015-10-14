#pragma once

#include <string>

//	bonjour/multicast DNS interface.
namespace Platform
{
	class TMulticaster;
}

class SoyMulticaster
{
public:
	SoyMulticaster(const std::string& Protocol,const std::string& Domain="local.");
	
public:
	std::string		mProtocol;
	std::string		mDomain;
	std::shared_ptr<Platform::TMulticaster>	mInternal;
};

