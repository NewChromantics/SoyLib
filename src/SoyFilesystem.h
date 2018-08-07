#pragma once

#include "SoyEvent.h"
#include "SoyTime.h"
#include "scope_ptr.h"

#if defined(TARGET_OSX)
#include <CoreServices/CoreServices.h>
#endif

#if (defined(TARGET_OSX)||defined(TARGET_IOS)) && defined(__OBJC__)
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


template<typename TYPE>
class ArrayBridge;



namespace Platform
{
	void		EnumFiles(std::string Directory,std::function<void(const std::string&)> OnFileFound);	//	end with ** to recurse
	bool		EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound);
	void		GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions);
	void		CreateDirectory(const std::string& Path);	//	will strip filenames
	
	bool		IsFullPath(const std::string& Path);		//	 is this a fully qualified path, not relative etc
	std::string	GetFullPathFromFilename(const std::string& Filename);
	std::string	GetDirectoryFromFilename(const std::string& Filename,bool IncludeTrailingSlash=true);

	//	implement platform specific "path interface" type?
#if (defined(TARGET_OSX)||defined(TARGET_IOS)) && defined(__OBJC__)
	NSURL*	GetUrl(const std::string& Filename);
#endif
	
#if defined(TARGET_WINDOWS)
	void		SetDllPath(const std::string& Path);
	std::string	GetDllPath();
#endif
	extern std::string	ExePath;

	
	//	gr: this is the resources dir inside .app on osx
#if defined(TARGET_OSX)
	std::string	GetAppResourcesDirectory();
#endif

	bool	ShowFileExplorer(const std::string& Path);
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


namespace Soy
{
	//	gr: move file things to their own files!
	void		FileToArray(ArrayBridge<char>& Data,std::string Filename);
	inline void	FileToArray(ArrayBridge<char>&& Data,std::string Filename)		{	FileToArray( Data, Filename );	}
	void		ArrayToFile(const ArrayBridge<char>&& Data,const std::string& Filename);
	inline void	LoadBinaryFile(ArrayBridge<char>& Data,std::string Filename)	{	FileToArray( Data, Filename );	}
	bool		ReadStream(ArrayBridge<char>& Data, std::istream& Stream, std::ostream& Error);
	bool		ReadStream(ArrayBridge<char>&& Data, std::istream& Stream, std::ostream& Error);
	bool		ReadStreamChunk( ArrayBridge<char>& Data, std::istream& Stream );
	inline bool	ReadStreamChunk( ArrayBridge<char>&& Data, std::istream& Stream )	{	return ReadStreamChunk( Data, Stream );		}
	bool		StringToFile(std::string Filename,std::string String);
	bool		FileToString(std::string Filename,std::string& String);
	bool		FileToString(std::string Filename,std::string& String,std::ostream& Error);
	bool		FileToStringLines(std::string Filename,ArrayBridge<std::string>& StringLines,std::ostream& Error);
	inline bool	FileToStringLines(std::string Filename,ArrayBridge<std::string>&& StringLines,std::ostream& Error)	{	return FileToStringLines( Filename, StringLines, Error );	}
}

