#include "SoyFilesystem.h"
#include "SoyDebug.h"
#include "heaparray.hpp"

#if defined(TARGET_OSX)
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <CoreServices/CoreServices.h>
#endif


namespace Platform
{
#if defined(TARGET_OSX) || defined(TARGET_IOS)
	void	EnumNsDirectory(const std::string& Directory,std::function<void(const std::string&)> OnFileFound,bool Recursive);
#endif
}


SoyTime Soy::GetFileTimestamp(const std::string& Filename)
{
	/*
#if defined(TARGET_OSX)
	struct tm Time;
	memset( &Time, 0, sizeof(Time) );

	struct stat attrib;			// create a file attribute structure
	auto Result = stat( Filename.c_str(), &attrib);		// get the attributes of afile.txt
	if ( Result != 0 )
		return SoyTime();
		
	// create a time structure
	if ( !gmtime_r( &attrib.st_mtime, &Time ) )
		return SoyTime();
	
	//return Time.
#endif
	 */
	throw Soy::AssertException( std::string(__func__)+" not implemented");
	return SoyTime();
}

#if defined(TARGET_OSX)
auto StreamRelease = [](FSEventStreamRef& Stream)
{
	FSEventStreamFlushSync( Stream );
	FSEventStreamUnscheduleFromRunLoop( Stream, CFRunLoopGetMain(), kCFRunLoopDefaultMode );
	FSEventStreamStop( Stream );
	FSEventStreamInvalidate( Stream );
	FSEventStreamRelease( Stream );
	Stream = nullptr;
};
#endif


#if defined(TARGET_OSX)
void OnFileChanged(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
	auto& FileWatch = *reinterpret_cast<Soy::TFileWatch*>( clientCallBackInfo );
	char **paths = reinterpret_cast<char **>(eventPaths);
	
	for ( int e=0;	e<numEvents;	e++ )
	{
		std::string Filename( paths[e] );
		//const FSEventStreamEventFlags& EventFlags( eventFlags[e] );
		//const FSEventStreamEventId EventIds( eventIds[e] );
		
		FileWatch.mOnChanged.OnTriggered( Filename );
	}
}
#endif


Soy::TFileWatch::TFileWatch(const std::string& Filename)

#if defined(TARGET_OSX)
:
	mStream	( StreamRelease )
#endif
{
	//	debug callback
	auto DebugOnChanged = [](const std::string& Filename)
	{
		std::Debug << Filename << " changed" << std::endl;
	};
	mOnChanged.AddListener( DebugOnChanged );
	
#if defined(TARGET_OSX)
	
	//std::string Filename("/Volumes/Code/PopTrack/");
	mPathString.Retain( CFStringCreateWithCString( nullptr, Filename.c_str(), kCFStringEncodingUTF8 ) );
	CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mPathString.mObject, 1, NULL);
	
	CFAbsoluteTime latency = 0.2; /* Latency in seconds */
	FSEventStreamContext Context = {0, this, NULL, NULL, NULL};

	
	/* Create the stream, passing in a callback */
	mStream.mObject = FSEventStreamCreate(NULL,
										  &OnFileChanged,
										  &Context,
										  pathsToWatch,
										  kFSEventStreamEventIdSinceNow,
										  latency,
										  kFSEventStreamCreateFlagFileEvents
										  );
	
	//	add to new runloop
	FSEventStreamScheduleWithRunLoop( mStream.mObject, CFRunLoopGetMain(), kCFRunLoopDefaultMode );
	FSEventStreamStart( mStream.mObject );
#endif
}


Soy::TFileWatch::~TFileWatch()
{
}


#if defined(TARGET_WINDOWS)
std::string GetFilename(WIN32_FIND_DATA& FindData)
{
	std::string Filename = FindData.cFileName;
	return Filename;
}
#endif


#if defined(TARGET_WINDOWS)
SoyPathType::Type GetPathType(WIN32_FIND_DATA& FindData)
{
	auto Directory = bool_cast(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	if ( Directory )
	{
		//	skip magic dirs
		auto Filename = GetFilename( FindData );
		if ( Filename == "." )
			return SoyPathType::Unknown;
		if ( Filename == ".." )
			return SoyPathType::Unknown;

		return SoyPathType::Directory;
	}

	return SoyPathType::File;
}
#endif


#if defined(TARGET_WINDOWS)
bool EnumDirectory(const std::string& Directory,std::function<bool(WIN32_FIND_DATA&)> OnItemFound)
{
	WIN32_FIND_DATA FindData;
	ZeroMemory( &FindData, sizeof(FindData) );
	auto Handle = FindFirstFile( Directory.c_str(), &FindData );

	//	invalid starting point... is okay?
	if ( Handle == INVALID_HANDLE_VALUE )
		return true;

	bool Result = true;

	try
	{
		//	notify on first file
		Result = OnItemFound( FindData );

		while ( Result )
		{
			if ( !FindNextFile( Handle, &FindData ) )
			{
				auto ErrorValue = Soy::Platform::GetLastError();
				if ( ErrorValue != ERROR_NO_MORE_FILES )
				{
					std::stringstream Error;
					Error << "FindNextFile error: " << Soy::Platform::GetErrorString( ErrorValue );
					throw Soy::AssertException( Error.str() );
				}
				break;
			}
			
			Result = OnItemFound( FindData );
		}
	}
	catch (std::exception& e)
	{
		FindClose( Handle );
		throw;
	}

	FindClose( Handle );
	return Result;
}
#endif



#if defined(TARGET_WINDOWS)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(std::string&,SoyPathType::Type)> OnPathFound)
{
	auto OnFindItem = [&](WIN32_FIND_DATA& FindData)
	{
		try
		{
			auto UrlType = GetPathType( FindData );
			auto Filename = GetFilename( FindData );
			
			//	if this returns false, bail
			if ( !OnPathFound( Filename, UrlType ) )
				return false;
		}
		catch(std::exception& e)
		{
			std::Debug << "Error extracting path meta; " << e.what() << std::endl;
		}
		return true;
	};

	std::stringstream SearchDirectory;
	SearchDirectory << Directory << "/*";

	return ::EnumDirectory( SearchDirectory.str(), OnFindItem );
}
#endif


#if defined(TARGET_ANDROID)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(std::string&,SoyPathType::Type)> OnPathFound)
{
	//	todo, bail early
	return false;
}
#endif


void Platform::EnumFiles(const std::string& Directory,std::function<void(const std::string&)> OnFileFound)
{
	if ( Directory.empty() )
		return;

	bool Recursive = false;
	
	Array<std::string> SearchDirectories;
	SearchDirectories.PushBack( Directory );
	
	//	don't get stuck!
	static int MatchLimit = 1000;
	int MatchCount = 0;
	
	while ( !SearchDirectories.IsEmpty() )
	{
		auto Dir = SearchDirectories.PopAt(0);
		
		auto AddFile = [&](std::string& Path,SoyPathType::Type PathType)
		{
			MatchCount++;

			if ( PathType == SoyPathType::Directory )
			{
				if ( !Recursive )
					return true;
				
				SearchDirectories.PushBack( Path );
			}
			else if ( PathType == SoyPathType::File )
			{
				OnFileFound( Path );
			}
			
			if ( MatchCount > MatchLimit )
			{
				std::Debug << "Hit match limit (" << MatchCount << ") bailing in case we've got stuck in a loop" << std::endl;
				return false;
			}
			
			return true;
		};
		
		if ( !Platform::EnumDirectory( Dir, AddFile ) )
			break;
	}
}


void Platform::GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions)
{
#if defined(TARGET_OSX)
	Extensions.PushBack(".ds_store");
#endif
}



void Platform::CreateDirectory(const std::string& Path)
{
	//	does path have any folder deliniators?
	auto LastForwardSlash = Path.find_last_of('/');
	auto LastBackSlash = Path.find_last_of('\\');
	
	//	gr: assumes standard npos is <0. but turns out its unsigned, so >0!
	//static_assert( std::string::npos < 0, "Expecting string npos to be -1");
	auto Last = LastBackSlash;
	if ( Last == std::string::npos )
		Last = LastForwardSlash;
	if ( Last == std::string::npos )
		return;
	
	//	real path string
	auto Directory = Path.substr(0, Last);
	if ( Directory.empty() )
		return;
	
#if defined(TARGET_OSX)
	mode_t Permissions = S_IRWXU|S_IRWXG|S_IRWXO;
	//	mode_t Permissions = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	if ( mkdir( Directory.c_str(), Permissions ) != 0 )
#elif defined(TARGET_WINDOWS)
		SECURITY_ATTRIBUTES* Permissions = nullptr;
	if ( !CreateDirectory( Directory.c_str(), Permissions ) )
#else
		if ( false )
#endif
		{
			auto LastError = Soy::Platform::GetLastError();
#if defined(TARGET_WINDOWS)
			if ( LastError != ERROR_ALREADY_EXISTS )
#else
				if ( LastError != EEXIST )
#endif
				{
					std::stringstream Error;
					Error << "Failed to create directory " << Directory << ": " << Soy::Platform::GetErrorString(LastError);
					throw Soy::AssertException( Error.str() );
				}
		}
}

