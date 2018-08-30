#pragma once

//	we want this generic declaration, but including
//	CoreFoundation is NOT desirable in a very common header (soyString, SoyTypes_Mac etc)
//	so only include as required. Definition is happy in SoyString.mm

#include <CoreFoundation/CoreFoundation.h>


namespace Soy
{
	std::string	GetString(CFStringRef String);
};

