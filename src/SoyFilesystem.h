#pragma once

#include "SoyEvent.h"
#include "SoyTime.h"
#include <scope_ptr.h>

#if defined(TARGET_OSX)
#include <CoreServices/CoreServices.h>
#endif

#if defined(TARGET_OSX) && defined(__OBJC__)
@protocol NSURL;
#endif


namespace SoyPathType
{
	enum Type
	{
		Unknown,
		File,
		Directory,
	};
};




namespace Platform
{
	void	EnumFiles(std::string Directory,std::function<void(const std::string&)> OnFileFound);	//	end with ** to recurse
	bool	EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound);
	void	GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions);
	void	CreateDirectory(const std::string& Path);	//	will strip filenames
	
	//	implement platform specific "path interface" type?
#if defined(TARGET_OSX) && defined(__OBJC__)
	NSURL*	GetUrl(const std::string& Filename);
#endif
}

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
	CFPtr<CFStringRef>			mPathString;
	scope_ptr<FSEventStreamRef>	mStream;
#endif
};

