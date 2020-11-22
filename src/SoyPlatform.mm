#include "SoyPlatform.h"
#include "SoyString.h"


#if defined(TARGET_IOS)
#include <UIKit/UIKit.h>
#endif

std::string Platform::GetComputerName()
{
#if defined(TARGET_OSX)
	//	https://stackoverflow.com/questions/4063129/get-my-macs-computer-name
	//	this is blocking, so... might be good to promise() this on startup? and cache it? block when called...
	//	localizedName: Jonathan's MacBook
	//	name: "Jonathans-Macbook", or "jonathans-macbook.local"
	auto* Name = [[NSHost currentHost] localizedName];
	return Soy::NSStringToString(Name);
#elif defined(TARGET_IOS)
	//	needs UIKit
	auto* Name = [[UIDevice currentDevice] name];
	return Soy::NSStringToString(Name);
#endif
}

