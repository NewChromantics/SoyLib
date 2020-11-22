#pragma once


#include "SoyTime.h"
#include "scope_ptr.h"

class SoyThreadLambda;

#if defined(TARGET_OSX)
#include <CoreServices/CoreServices.h>
#endif

#if (defined(TARGET_OSX)||defined(TARGET_IOS)) && defined(__OBJC__)
@protocol NSURL;
#endif

//	bloody windows macros
#if defined(TARGET_WINDOWS)
#undef GetComputerName
#endif


namespace SoyPathType
{
	enum Type
	{
		Unknown,
		File,
		Directory,
		Special,	//	on OSX, this includes devices like /dev/tty.xxx or /dev/console. Maybe we can specialise this as Pipe or Stream
	};
};


template<typename TYPE>
class ArrayBridge;



namespace Platform
{
	extern const char	DirectorySeperator;
	class TFileMonitor;
	
	void		EnumFiles(std::string Directory,std::function<void(const std::string&)> OnFileFound);	//	end with ** to recurse
	bool		EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound);
	void		GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions);
	void		CreateDirectory(const std::string& Path);	//	will strip filenames
	
	bool		IsFullPath(const std::string& Path);		//	 is this a fully qualified path, not relative etc
	std::string	GetFullPathFromFilename(const std::string& Filename);	//	if this is a directory, it should end with a slash
	std::string	GetDirectoryFromFilename(const std::string& Filename,bool IncludeTrailingSlash=true);

	//	implement platform specific "path interface" type?
#if (defined(TARGET_OSX)||defined(TARGET_IOS)) && defined(__OBJC__)
	NSURL*	GetUrl(const std::string& Filename);
#endif
	
	std::string			GetExePath();
	std::string			GetExeFilename();
	inline std::string	GetDllPath() { return GetExePath(); }
	void				GetExeArguments(ArrayBridge<std::string>&& Arguments);
	std::string			GetCurrentWorkingDirectory();

	//	gr: this is the resources dir inside .app on osx
	//	on windows it's just exe path
	std::string	GetAppResourcesDirectory();
	//	these are for ios, but should use OS-specified ones too
	std::string	GetDocumentsDirectory();
	std::string	GetTempDirectory();
	std::string	GetCacheDirectory();	//	ios; same as temp, but auto-cleared less frequently

	void		ShowFileExplorer(const std::string& Path);
	void		ShellExecute(const std::string& Path);
	void		ShellOpenUrl(const std::string& Url);
	
	bool		FileExists(const std::string& Path);
	bool		DirectoryExists(const std::string& Path);

	//	maybe not file system? generic platform stuff...
	std::string	GetComputerName();
	void		SetEnvVar(const char* Key,const char* Value);
	std::string	GetEnvVar(const char* Key);

#if defined(TARGET_LINUX)
	extern std::string	ExeFilename;
#endif
}

namespace Soy
{
	SoyTime		GetFileTimestamp(const std::string& Filename);
};




class Platform::TFileMonitor
{
public:
	TFileMonitor(const std::string& Filename);
	~TFileMonitor();
	
	std::function<void(const std::string& Filename)>	mOnChanged;	//	this has a filename param in case we're monitoring a directory

#if defined(TARGET_OSX)
	void						OnFileChanged(std::string& FilePath);
	CFPtr<CFStringRef>			mWatchPathString;
	std::string					mWatchPath;
	scope_ptr<FSEventStreamRef>	mStream;
#elif defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
	void								StartFileWatch(const std::string& Filename);
	void								WatchFileIteration(const std::string& Filename);
	void								StartDirectoryWatch(const std::string& Directory);
	void								WatchDirectoryIteration(const std::string& Directory);
	std::shared_ptr<SoyThreadLambda>	mWatchThread;
	HANDLE								mWatchHandle = nullptr;
#endif
};


namespace Soy
{
	//	gr: move file things to their own files!
	void		FileToArray(ArrayBridge<char>& Data,std::string Filename);
	inline void	FileToArray(ArrayBridge<char>&& Data,std::string Filename)		{	FileToArray( Data, Filename );	}
	void		FileToArray(ArrayBridge<uint8_t>& Data,std::string Filename);
	inline void	FileToArray(ArrayBridge<uint8_t>&& Data,std::string Filename)		{	FileToArray( Data, Filename );	}
	void		ArrayToFile(const ArrayBridge<char>&& Data,const std::string& Filename,bool Append=false);
	void		ArrayToFile(const ArrayBridge<uint8_t>&& Data,const std::string& Filename,bool Append=false);
	inline void	LoadBinaryFile(ArrayBridge<char>& Data,std::string Filename)	{	FileToArray( Data, Filename );	}
	void		ReadStream(ArrayBridge<uint8_t>& Data, std::istream& Stream);
	void		ReadStream(ArrayBridge<char>& Data, std::istream& Stream);
	void		ReadStream(ArrayBridge<char>&& Data, std::istream& Stream);
	bool		ReadStreamChunk( ArrayBridge<char>& Data, std::istream& Stream );
	inline bool	ReadStreamChunk( ArrayBridge<char>&& Data, std::istream& Stream )	{	return ReadStreamChunk( Data, Stream );		}
	void		StringToFile(std::string Filename,std::string String,bool Append=false);
	void		FileToString(std::string Filename,std::ostream& String);
	void		FileToString(std::string Filename,std::string& String);
	void		FileToStringLines(std::string Filename,ArrayBridge<std::string>& StringLines);
	inline void	FileToStringLines(std::string Filename,ArrayBridge<std::string>&& StringLines)	{	return FileToStringLines( Filename, StringLines );	}
}

