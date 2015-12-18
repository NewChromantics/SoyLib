#pragma once

#include <string>
#include "SoyEvent.h"
#include "Array.hpp"


//	bonjour/multicast DNS interface.
namespace Platform
{
	class TMulticaster;
}

class TServiceMeta
{
public:
	TServiceMeta() :
		mPort				( 0 ),
		mIncludesPeerToPeer	( false )
	{
	}
	
public:
	std::string		mName;
	std::string		mType;
	std::string		mDomain;
	std::string		mHostName;
	int				mPort;
	bool			mIncludesPeerToPeer;
};

class SoyMulticaster
{
public:
	SoyMulticaster(const std::string& Protocol,const std::string& Domain="local.");
	
	void			EnumServices(ArrayBridge<TServiceMeta>&& Metas);
	
public:
	SoyEvent<const TServiceMeta>	mOnFoundService;
	SoyEvent<const std::string>		mOnError;
	
private:
	std::string		mProtocol;
	std::string		mDomain;
	std::shared_ptr<Platform::TMulticaster>	mInternal;
};

