#pragma once

#include "SoyEvent.h"
#include "SoyTime.h"
#include <CoreServices/CoreServices.h>

namespace Soy
{
	SoyTime		GetFileTimestamp(const std::string& Filename);
	
	class TFileWatch;
};


namespace Soy
{
	class TFileWatch;
};

class Soy::TFileWatch
{
public:
	TFileWatch(const std::string& Filename);
	~TFileWatch();
	
	SoyEvent<const std::string>	mOnChanged;
	
#if defined(TARGET_OSX)
	CFPtr<CFStringRef>		mPathString;
	CFPtr<FSEventStreamRef>	mStream;
#endif
};

