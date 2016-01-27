#pragma once

#include "SoyEvent.h"
#include "SoyTime.h"
#include <scope_ptr.h>

#if defined(TARGET_OSX)
#include <CoreServices/CoreServices.h>
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
#if defined(__OBJC__)
	void	EnumNsDirectory(const std::string& Directory,std::function<void(const std::string&)> OnFileFound,bool Recursive);
#endif
	
	void	EnumFiles(const std::string& Directory,std::function<void(const std::string&)> OnFileFound);
	bool	EnumDirectory(const std::string& Directory,std::function<bool(std::string&,SoyPathType::Type)> OnPathFound);
	void	GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions);
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

