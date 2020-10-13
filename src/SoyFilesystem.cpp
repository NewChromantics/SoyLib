#include "SoyFilesystem.h"
#include "SoyDebug.h"
#include "HeapArray.hpp"

#if defined(TARGET_WINDOWS)
#include <filesystem>
#include "SoyThread.h"
#include "magic_enum/include/magic_enum.hpp"
#endif

#if defined(TARGET_LINUX)
#include <filesystem>
#include <fcntl.h>
#endif

#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
#include <unistd.h>	//	gethostname
#include <sys/stat.h>
#include <limits.h>	//	PATH_MAX
//	gr: jetson seems to be missing PATH_MAX in limits.h...
#include <linux/limits.h>	//	PATH_MAX
#endif

#if defined(TARGET_OSX)
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <CoreServices/CoreServices.h>
#endif


//	posix directory reading
#if defined(TARGET_ANDROID)
#include <dirent.h>
#endif

#if defined(TARGET_WINDOWS)
#include <Shlobj.h>
#include <Shlwapi.h>
#include <shellapi.h>
#undef ShellExecute
#endif

#include <sstream>
#include <fstream>
#include <algorithm>

namespace Platform
{
#if defined(TARGET_OSX) || defined(TARGET_IOS)
	void	EnumNsDirectory(const std::string& Directory,std::function<void(const std::string&)> OnFileFound,bool Recursive);
#endif
}

#if defined(TARGET_WINDOWS)
const char Platform::DirectorySeperator = '\\';
#else
const char Platform::DirectorySeperator = '/';
#endif



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
static void OnFileChangedEvent(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
	auto& FileWatch = *reinterpret_cast<Platform::TFileMonitor*>( clientCallBackInfo );
	char **paths = reinterpret_cast<char **>(eventPaths);

	//	windows has the same thing; stream of events with path+change
	//	merge them together!
	for ( int e=0;	e<numEvents;	e++ )
	{
		std::string Filename( paths[e] );
		//const FSEventStreamEventFlags& EventFlags( eventFlags[e] );
		//const FSEventStreamEventId EventIds( eventIds[e] );
		FileWatch.OnFileChanged(Filename);
	}
}
#endif


Platform::TFileMonitor::TFileMonitor(const std::string& Filename)

#if defined(TARGET_OSX)
:
	mStream				( StreamRelease ),
	mWatchPath			( Filename )
#endif
{
	//	debug callback
	auto DebugOnChanged = [](const std::string& Filename)
	{
		std::Debug << Filename << " changed" << std::endl;
	};
	mOnChanged = DebugOnChanged;
	
#if defined(TARGET_OSX)
	
	//std::string Filename("/Volumes/Code/PopTrack/");
	mWatchPathString.Retain( CFStringCreateWithCString( nullptr, Filename.c_str(), kCFStringEncodingUTF8 ) );
	mWatchPath +="/";
	CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mWatchPathString.mObject, 1, NULL);
	
	CFAbsoluteTime latency = 0.2; /* Latency in seconds */
	FSEventStreamContext Context = {0, this, NULL, NULL, NULL};

	
	/* Create the stream, passing in a callback */
	mStream.mObject = FSEventStreamCreate(NULL,
										  &OnFileChangedEvent,
										  &Context,
										  pathsToWatch,
										  kFSEventStreamEventIdSinceNow,
										  latency,
										  kFSEventStreamCreateFlagFileEvents
										  );
	
	//	add to new runloop
	FSEventStreamScheduleWithRunLoop( mStream.mObject, CFRunLoopGetMain(), kCFRunLoopDefaultMode );
	FSEventStreamStart( mStream.mObject );

#elif defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)

	if (Platform::DirectoryExists(Filename))
		StartDirectoryWatch(Filename);
	else
		StartFileWatch(Filename);
#else
	Soy_AssertTodo();
#endif
}


Platform::TFileMonitor::~TFileMonitor()
{
#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
	mWatchThread.reset();
#endif
}

#if defined(TARGET_OSX)
void Platform::TFileMonitor::OnFileChanged(std::string& FilePath)
{
	//	make path relative to what we're monitoring
	if ( !Soy::StringTrimLeft(FilePath, mWatchPath, true ) )
	{
		std::Debug << "Warning: " << __PRETTY_FUNCTION__ << " path (" << FilePath << ") not prefixed with root directory(" << mWatchPath << ")" << std::endl;
	}
	mOnChanged( FilePath );
}
#endif

#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
void Platform::TFileMonitor::StartFileWatch(const std::string& Filename)
{
	if (!Platform::FileExists(Filename))
		throw Soy::AssertException("File doesn't exist");

	//	this API will notify of a directory change, but we don't know what changed
	auto WatchSubTree = false;
	//auto WatchSubTree = true;
	//auto NotifyFlags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
	auto NotifyFlags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

	// Watch the directory for file creation and deletion. 
	mWatchHandle = FindFirstChangeNotification(Filename.c_str(), WatchSubTree, NotifyFlags);

	if (mWatchHandle == INVALID_HANDLE_VALUE || mWatchHandle == nullptr)
		Platform::ThrowLastError("FindFirstChangeNotification");

	auto Loop = [=]()
	{
		this->WatchFileIteration(Filename);
		return true;
	};
	mWatchThread.reset(new SoyThreadLambda(__PRETTY_FUNCTION__,Loop));
}
#endif

#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
void Platform::TFileMonitor::WatchFileIteration(const std::string& Filename)
{
	// Wait for notification.
	std::Debug << "Waiting for notification for " << Filename << std::endl;
	HANDLE Handles[] = { mWatchHandle };
	auto WaitStatus = WaitForMultipleObjects( std::size(Handles), Handles, FALSE, INFINITE);

	switch (WaitStatus)
	{
	case WAIT_OBJECT_0:
	{
		this->mOnChanged(Filename);
		auto Status = FindNextChangeNotification(mWatchHandle);
		if (!Status)
			Platform::ThrowLastError("FindNextChangeNotification");
	}
	break;

	case WAIT_TIMEOUT:
		// A timeout occurred, this would happen if some value other 
		// than INFINITE is used in the Wait call and no changes occur.
		// In a single-threaded environment you might not want an
		// INFINITE wait.
		std::Debug << "No changes in the timeout period for " << Filename << std::endl;
		break;

	default:
		Platform::ThrowLastError("Unhandled WaitStatus.");
		ExitProcess(GetLastError());
		break;
	}
}
#endif


#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
void Platform::TFileMonitor::StartDirectoryWatch(const std::string& Directory)
{
	if (!Platform::DirectoryExists(Directory))
		throw Soy::AssertException("Directory doesn't exist");
	
	mWatchHandle = CreateFile(
		Directory.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL );

	auto Loop = [=]()
	{
		this->WatchDirectoryIteration(Directory);
		return true;
	};
	mWatchThread.reset(new SoyThreadLambda(__PRETTY_FUNCTION__, Loop));
}
#endif

#if defined(TARGET_WINDOWS)
namespace Platform
{
	namespace FileAction
	{
		enum Type
		{
			Added = FILE_ACTION_ADDED,
			Removed = FILE_ACTION_REMOVED,
			Modified = FILE_ACTION_MODIFIED,
			RenamedOldName = FILE_ACTION_RENAMED_OLD_NAME,
			RenamedNewName = FILE_ACTION_RENAMED_NEW_NAME
		};
	}
}
#endif

#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
void Platform::TFileMonitor::WatchDirectoryIteration(const std::string& Directory)
{
	uint8_t FileNotifyInfoBuffer[1024*2];
	DWORD BufferBytesUsed = 0;
		
	auto WatchSubTree = true;
	auto NotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
	auto Result = ReadDirectoryChangesW(mWatchHandle, FileNotifyInfoBuffer, sizeof(FileNotifyInfoBuffer), WatchSubTree, NotifyFilter, &BufferBytesUsed, nullptr, nullptr);
	if (Result == 0 )
		Platform::ThrowLastError("ReadDirectoryChangesW");

	if (BufferBytesUsed==0)
		Platform::ThrowLastError("ReadDirectoryChangesW buffer too small");

	//	walk over the results
	auto BytesRead = 0;
	while (BytesRead < BufferBytesUsed && BytesRead < sizeof(FileNotifyInfoBuffer))
	{
		auto& FileInfo = *reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&FileNotifyInfoBuffer[BytesRead]);

		auto FileNameLength = FileInfo.FileNameLength / sizeof(wchar_t);
		std::wstring FilenameW(FileInfo.FileName, FileNameLength);
		auto Filename = Soy::WStringToString(FilenameW);
		auto Change = *magic_enum::enum_cast<Platform::FileAction::Type>(FileInfo.Action);
		std::Debug << "File " << magic_enum::enum_name(Change) << " " << Filename << std::endl;
		
		
		auto Path = Directory + Platform::DirectorySeperator + Filename;
		if (DirectoryExists(Path))
		{
			//	ignore "directory has been modified"
			std::Debug << "Ignoring directory changed notifiucation" << std::endl;
		}
		else
		{
			//	gr: todo: handle visual studio's temp file changes (gather in this loop and remove redundant changes)
			//		created/modified abcd.tmp
			//		added thefilename.xyz.tmp
			//		removed thefilename.xyz.tmp
			//		renameOldName thefilename
			//		renameNewName thefilename.tmp
			//		renameoldname abcd.tmp
			//		rename New name thefilename
			if (Change != Platform::FileAction::RenamedOldName)
			{
				this->mOnChanged(Filename);
			}
		}

		//	goto next
		BytesRead += FileInfo.NextEntryOffset;
		//	0 means this is the last entry
		if (FileInfo.NextEntryOffset == 0)
			break;
	}
}
#endif

#if defined(TARGET_WINDOWS)
std::string GetFilename(WIN32_FIND_DATAA& FindData)
{
	std::string Filename = FindData.cFileName;
	return Filename;
}
#endif


#if defined(TARGET_WINDOWS)
SoyPathType::Type GetPathType(WIN32_FIND_DATAA& FindData)
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


#if defined(TARGET_WINDOWS) && !defined(TARGET_UWP)
bool EnumDirectory(const std::string& Directory,std::function<bool(WIN32_FIND_DATAA&)> OnItemFound)
{
	WIN32_FIND_DATAA FindData;
	ZeroMemory( &FindData, sizeof(FindData) );
	auto Handle = FindFirstFileA( Directory.c_str(), &FindData );

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
			if ( !FindNextFileA( Handle, &FindData ) )
			{
				auto ErrorValue = Platform::GetLastError();
				if ( ErrorValue != ERROR_NO_MORE_FILES )
				{
					std::stringstream Error;
					Error << "FindNextFile error: " << Platform::GetErrorString( ErrorValue );
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



#if defined(TARGET_WINDOWS) && defined(TARGET_UWP)
bool EnumDirectory(const std::string& Directory,std::function<bool(WIN32_FIND_DATA&)> OnItemFound)
{
	return false;
}
#endif

#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	auto OnFindItem = [&](WIN32_FIND_DATAA& FindData)
	{
		try
		{
			auto UrlType = GetPathType( FindData );
			//	gr: return path relative to original Directory
			//auto Filename = Directory + Platform::DirectorySeperator + GetFilename( FindData );
			auto Filename = GetFilename( FindData );

			//	ignore . and .. (maybe change this type to a system/ignore type?)
			if (UrlType == SoyPathType::Unknown)
				return true;

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
	SearchDirectory << Directory << Platform::DirectorySeperator << "*";

	return ::EnumDirectory( SearchDirectory.str(), OnFindItem );
}
#endif


#if defined(TARGET_ANDROID)
SoyPathType::Type GetPathType(struct dirent& DirEntry,const std::string& Filename)
{
	switch ( DirEntry.d_type )
	{
		case DT_DIR:
		{
			//	skip magic dirs
			if ( Filename == "." )
				return SoyPathType::Unknown;
			if ( Filename == ".." )
				return SoyPathType::Unknown;
			return SoyPathType::Directory;
		}
			
		//	regular file
		case DT_REG:
			return SoyPathType::File;
			
		//	maybe follow symbolic links?
		case DT_LNK:
			//readlink()
			
		default:
			return SoyPathType::Unknown;
	}
}
#endif

#if defined(TARGET_ANDROID)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	auto Handle = opendir( Directory.c_str() );
	if ( !Handle )
		throw Soy::AssertException( Platform::GetLastErrorString() );

	
	while ( true )
	{
		struct dirent* Entry = readdir( Handle );
		if ( Entry == nullptr )
			break;

		std::string Filename( Entry->d_name );
		auto Type = GetPathType( *Entry, Filename );

		std::stringstream FullPath;
		FullPath << Directory << "/" << Filename;
		
		if ( !OnPathFound( FullPath.str(), Type ) )
			return false;
	}
	
	closedir( Handle );
	return true;
}
#endif


void Platform::EnumFiles(std::string Directory,std::function<void(const std::string&)> OnFileFound)
{
	if ( Directory.empty() )
		return;

	//	if the dir ends with ** then we search recursively
	bool Recursive = false;
	if ( Soy::StringTrimRight( Directory, "**", true ) )
		Recursive = true;
	
	Array<std::string> SearchDirectories;
	SearchDirectories.PushBack( Directory );
	
	//	don't get stuck!
	//	gr: much higher for recursive directories... could do with a more solid checker though
	static int MatchLimit = 50000;
	int MatchCount = 0;
	
	while ( !SearchDirectories.IsEmpty() )
	{
		auto Dir = SearchDirectories.PopAt(0);
		//std::Debug << "Searching dir " << Dir << " recursive=" << Recursive << std::endl;
		
		auto AddFile = [&](const std::string& Path,SoyPathType::Type PathType)
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
		
		try
		{
			if ( !Platform::EnumDirectory( Dir, AddFile ) )
				break;
		}
		catch(std::exception& e)
		{
			std::Debug << "Exception enumerating directory " << Dir << ": " << e.what() << std::endl;
		}
		catch(...)
		{
			std::Debug << "Unknown exception enumerating directory " << Dir << std::endl;
		}
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
	
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
	mode_t Permissions = S_IRWXU|S_IRWXG|S_IRWXO;
	//	mode_t Permissions = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	if ( mkdir( Directory.c_str(), Permissions ) != 0 )
#elif defined(TARGET_WINDOWS)
		SECURITY_ATTRIBUTES* Permissions = nullptr;
	if ( !CreateDirectoryA( Directory.c_str(), Permissions ) )
#else
		if ( false )
#endif
		{
			auto LastError = Platform::GetLastError();
#if defined(TARGET_WINDOWS)
			if ( LastError != ERROR_ALREADY_EXISTS )
#else
				if ( LastError != EEXIST )
#endif
				{
					std::stringstream Error;
					Error << "Failed to create directory " << Directory << ": " << Platform::GetErrorString(LastError);
					throw Soy::AssertException( Error.str() );
				}
		}
}

#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)||defined(TARGET_UWP)
void Platform::ShellExecute(const std::string& Path)
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
void Platform::ShellOpenUrl(const std::string& Path)
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
void Platform::ShellExecute(const std::string& Path)
{
	//	https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea
	//	returns HINSTANCE but it's an int
	//	If the function succeeds, it returns a value greater than 32. 
	//	If the function fails, it returns an error value that indicates the cause of the failure.
	auto Result = ShellExecuteA(nullptr, "open", Path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	auto ResultInt = static_cast<int>(reinterpret_cast<std::uintptr_t>(Result));

	if (ResultInt > 32)
		return;

	auto Context = std::string("ShellExecute(") + Path + ")";
	Platform::IsOkay(ResultInt, Context);
}
#endif

#if defined(TARGET_WINDOWS)
void Platform::ShellOpenUrl(const std::string& UrlString)
{
	ShellExecute(UrlString);
}
#endif

#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
void Platform::ShowFileExplorer(const std::string& Path)
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_WINDOWS)
void Platform::ShowFileExplorer(const std::string& Path)
{
#if defined(TARGET_UWP)
	//	unsupported
#else
	auto CleanPath = Platform::GetFullPathFromFilename(Path);
	auto PathList = ILCreateFromPathA(CleanPath.c_str() );
	if ( !PathList )
	{
		std::stringstream Error;
		Error << "ILCreateFromPath(" << Path << ") failed, may not exist";
		throw Soy::AssertException(Error.str());
	}

	//	this is required, but often already run (maybe with other params), often uneeded
	try
	{
		auto Result = CoInitialize(nullptr);
		Platform::IsOkay(Result, "CoInitialize for SHOpenFolderAndSelectItems");
	}
	catch (std::exception& e)
	{
		std::Debug << e.what() << std::endl;
	}

	try
	{
		auto Result = SHOpenFolderAndSelectItems(PathList, 0, 0, 0);
		Platform::IsOkay(Result, "SHOpenFolderAndSelectItems");
		ILFree(PathList);
	}
	catch (std::exception& e)
	{
		ILFree(PathList);
		throw;
	}
#endif
}
#endif

#if defined(TARGET_LINUX)
std::string Platform::GetExeFilename()
{
	//	https://stackoverflow.com/a/4025415/355753
	char ExePath[PATH_MAX];
	// readlink does not null terminate!
	memset(ExePath,0,sizeof(ExePath));
	auto Length = readlink("/proc/self/exe", ExePath, PATH_MAX);
	if ( Length == -1 )
		Platform::ThrowLastError("readlink(/proc/self/exe)");
	std::string ExePathString( ExePath, Length );
	return ExePathString;
}
#endif


#if defined(TARGET_ANDROID)
std::string Platform::GetExeFilename()
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
void Platform::GetExeArguments(ArrayBridge<std::string>&& Arguments)
{
	//	https://stackoverflow.com/questions/1585989/how-to-parse-proc-pid-cmdline
	char CmdLine[PATH_MAX];

	memset(CmdLine,0,sizeof(CmdLine));

	int fd = open("/proc/self/cmdline", O_RDONLY);
	if (fd == -1)
		Platform::ThrowLastError("open(/proc/self/cmdline)");

	int Length = read(fd, CmdLine, PATH_MAX);
	if (read == -1)
		Platform::ThrowLastError("read(/proc/self/cmdline)");

	char *end = CmdLine + Length;
	for (char *p = CmdLine; p < end; /**/)
	{ 
		auto ArgString = std::string(p);
		Arguments.PushBack(ArgString);
		std::Debug << p << std::endl;
		while (*p++); // skip until start of next 0-terminated section
	}
	close(fd);

}
#endif

#if defined(TARGET_WINDOWS)
std::string Platform::GetExeFilename()
{
	static std::string ExeFilenameCache;

	if (ExeFilenameCache.length())
		return ExeFilenameCache;

	char Buffer[MAX_PATH] = { 0 };
	auto Length = GetModuleFileNameA( nullptr, Buffer, std::size(Buffer) );
	Length = std::min<size_t>(Length, sizeof(Buffer) - 1);
	Buffer[Length] = '\0';
	ExeFilenameCache = Buffer;
	return ExeFilenameCache;
}
#endif


std::string	Platform::GetExePath()
{
	auto ExeFilename = GetExeFilename();
	return GetDirectoryFromFilename(ExeFilename);
}


#if defined(TARGET_ANDROID)
bool Platform::FileExists(const std::string& Path)
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_ANDROID)
bool Platform::DirectoryExists(const std::string& Path)
{
	Soy_AssertTodo();
}
#endif

#if defined(TARGET_WINDOWS)||defined(TARGET_LINUX)
bool Platform::FileExists(const std::string& Path)
{
	//	if this errors, the file must not exist
	try
	{
		auto FullPath = GetFullPathFromFilename(Path);
		return std::filesystem::is_regular_file(FullPath);
	}
	catch(std::exception& e)
	{
		std::Debug << __PRETTY_FUNCTION__ << e.what() << std::endl;
		return false;
	}
	//auto FullPath = GetFullPathFromFilename(Path);
	//return ::PathFileExistsA(FullPath.c_str());
}
#endif

#if defined(TARGET_WINDOWS)||defined(TARGET_LINUX)
bool Platform::DirectoryExists(const std::string& Path)
{
	//	if this errors, the directory must not exist
	try
	{
		auto FullPath = GetFullPathFromFilename(Path);
		return std::filesystem::is_directory(FullPath);
	}
	catch(std::exception& e)
	{
		std::Debug << __PRETTY_FUNCTION__ << e.what() << std::endl;
		return false;
	}
	//return ::PathIsDirectoryA(FullPath.c_str());
	//auto Attribs = ::GetFileAttributesA(FullPath.c_str());
}
#endif

bool Platform::IsFullPath(const std::string& Path)
{
#if defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)
	auto IsRelative = PathIsRelativeA(Path.c_str());
	return !IsRelative;
#else
	try
	{
		//	expecting this to throw if not a valid filename
		auto FullPath = GetFullPathFromFilename(Path);

		//	gr: OSX will correct by adding / to end of the dir
		//	lets allow if Path is matching the start, but maybe we can do something better
		bool CaseSensitive = true;
		if ( !Soy::StringBeginsWith( FullPath, Path, CaseSensitive ) )
			return false;
		return true;
	}
	catch ( std::exception& e )
	{
		//	assume an OS error
		//	gr: should make a specialised exception type here
		return false;
	}
#endif
}


#if !defined(TARGET_IOS)&&!defined(TARGET_OSX)
std::string	Platform::GetFullPathFromFilename(const std::string& Filename)
{
	//	get a std::string Path; and then followup code will fix directory suffix
#if defined(TARGET_LINUX)
	char AbsolutePath[PATH_MAX] = { 0 };
	auto* ResultPath = realpath(Filename.c_str(), AbsolutePath);
	if (!ResultPath)
		Platform::ThrowLastError(std::string("realpath(") + Filename);
	std::string Path(AbsolutePath);

#elif defined(TARGET_WINDOWS)&&!defined(TARGET_UWP)

	char PathBuffer[MAX_PATH];
	char* FilenameStart = nullptr;	//	pointer to inside buffer
	//	gr: this pre-pends the CWD, what should it do if the file doesnt exist?
	auto PathBufferLength = GetFullPathNameA( Filename.c_str(), sizeof(PathBuffer), PathBuffer, &FilenameStart );

	auto LastError = Platform::GetLastError();
	if ( PathBufferLength == 0 )
	{
		//	error
		std::stringstream Error;
		Error << "GetFullPathName(" << Filename << ")";
		Platform::IsOkay( LastError, Error.str() );
	}

	if ( PathBufferLength > sizeof(PathBuffer) )
	{
		std::stringstream Error;
		Error << "GetFullPathName(" << Filename << ") overflows buffer (" << PathBufferLength << "/" << sizeof(PathBuffer) << ")";
		Platform::IsOkay( LastError, Error.str() );
	}
	PathBufferLength = std::min<size_t>( PathBufferLength, sizeof(PathBuffer)-1 );
	
	std::string Path(PathBuffer, PathBufferLength);
#else
	std::string Path;
	throw Soy::AssertException("GetFullPathFromFilename not implemented");
#endif
	//	if directory, append a slash
	//	gr: windows doesn't need this, so make sure there's a check
	if (Path.length() != 0 && Path.back() != DirectorySeperator)
	{
		if (std::filesystem::is_directory(Path))
			Path += DirectorySeperator;
	}
	return Path;
}
#endif

std::string	Platform::GetDirectoryFromFilename(const std::string& Filename,bool IncludeTrailingSlash)
{
	//	todo: OSX directory resolving functions fail if there is a double trailing slash, make sure this function cleans that
	//	if the path is a directory, make sure it ends with a slash
	if ( Platform::DirectoryExists(Filename) )
	{
		auto Directory = Filename;
		Soy::StringTrimRight(Directory,'/');
		Soy::StringTrimRight(Directory,'\\');
		Soy::StringTrimRight(Directory,'/');
		//	remove all trailing slashes
		//	make sure it ends in one
		if ( IncludeTrailingSlash )
			Directory += DirectorySeperator;
		return Directory;
	}
	
	//	todo: make use of platform funcs
	//	gr: why does rfind give us unsigned :|
	//	chop everything up to last slash
	//	todo: correct slashes to platform's seperator
	ssize_t LastSlasha = Filename.rfind('/');
	ssize_t LastSlashb = Filename.rfind('\\');
	ssize_t LastSlash = std::max( LastSlasha, LastSlashb );
	if ( LastSlash == std::string::npos )
		return "";

	auto Directory = Filename.substr( 0, LastSlash );
	if ( IncludeTrailingSlash )
		Directory += DirectorySeperator;
	return Directory;
}


#if defined(TARGET_PS4)||defined(TARGET_LINUX)||defined(TARGET_UWP)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	return false;
}
#endif




bool Soy::ReadStreamChunk(ArrayBridge<char>& Data,std::istream& Stream)
{
	//	dunno how much to read
	if ( Data.IsEmpty() )
		throw Soy::AssertException("Soy::ReadStreamChunk no data length specified, resorting to 1byte" );

	if ( Data.IsEmpty() )
		return false;
	
	auto Peek = Stream.peek();
	if ( Peek == std::char_traits<char>::eof() )
		return false;
	
	Stream.read( Data.GetArray(), Data.GetDataSize() );
	
	if ( Stream.fail() && !Stream.eof() )
		throw Soy::AssertException("Reading stream failed (not eof)");
	
	auto BytesRead = Stream.gcount();
	Data.SetSize( BytesRead );
	
	return true;
}


void Soy::ReadStream(ArrayBridge<char>&& Data,std::istream& Stream)
{
	ReadStream(Data,Stream);
}


void Soy::ReadStream(ArrayBridge<char>& Data,std::istream& Stream)
{
	//	gr: todo: re-use function above, just need to send lambda or something for PushBackArary

	//	gr: bigger buffer for much faster reading
	const int MaxBufferSize = 2*1024*1024;
	Array<char> Buffer(MaxBufferSize);

	while ( !Stream.eof() )
	{
		//	read a chunk
		Buffer.SetSize( MaxBufferSize );
		auto BufferBridge = GetArrayBridge( Buffer );
		if ( !Soy::ReadStreamChunk( BufferBridge, Stream ) )
			break;
		
		Data.PushBackArray( Buffer );
	}
}


void Soy::ReadStream(ArrayBridge<uint8_t>& Data,std::istream& Stream)
{
	//	gr: todo: re-use function above, just need to send lambda or something for PushBackArary
	BufferArray<char,5*1024> Buffer;
	while ( !Stream.eof() )
	{
		//	read a chunk
		Buffer.SetSize( Buffer.MaxSize() );
		auto BufferBridge = GetArrayBridge( Buffer );
		if ( !Soy::ReadStreamChunk( BufferBridge, Stream ) )
			break;
		
		Data.PushBackArray( Buffer );
	}
}

void Soy::FileToArray(ArrayBridge<char>& Data,std::string Filename)
{
	Filename = ::Platform::GetFullPathFromFilename(Filename);

	//	gr: would be nice to have an array! MemFileArray maybe, if it can be cross paltform...
	std::ifstream Stream( Filename, std::ios::binary|std::ios::in );
	
	if ( !Stream.is_open() )
	{
		std::stringstream Error;
		Error << "Failed to open " << Filename << " (" << ::Platform::GetLastErrorString() << ")";
		throw Soy::AssertException(Error.str());
	}
	
	//	read block by block
	try
	{
		ReadStream( Data, Stream);
		Stream.close();
	}
	catch(...)
	{
		Stream.close();
		throw;
	}
}

void Soy::FileToArray(ArrayBridge<uint8_t>& Data,std::string Filename)
{
	//	gr: would be nice to have an array! MemFileArray maybe, if it can be cross paltform...
	std::ifstream Stream( Filename, std::ios::binary|std::ios::in );
	
	if ( !Stream.is_open() )
	{
		std::stringstream Error;
		Error << "Failed to open " << Filename << " (" << ::Platform::GetLastErrorString() << ")";
		throw Soy::AssertException(Error.str());
	}
	
	//	read block by block
	try
	{
		ReadStream( Data, Stream );
		Stream.close();
	}
	catch(...)
	{
		Stream.close();
		throw;
	}
}

//	gr: quickest to code, should do a toll-free arraybridge cast!
template<typename TYPE>
void ArrayToFile(const ArrayBridge<TYPE>& Data,const std::string& Filename,bool Binary,bool Append)
{
	auto Mode = Binary ? (std::ios::out | std::ios::binary) : std::ios::out;
	if ( Binary )
		Mode |= std::ios::binary;
	if ( Append )
		Mode |= std::ios::app;
	
	std::ofstream File( Filename, Mode );
	if ( !File.is_open() )
		throw Soy::AssertException( std::string("Failed to open ")+Filename );
	
	if ( !Data.IsEmpty() )
	{
		auto* DataPtr = reinterpret_cast<const char*>( Data.GetArray() );
		File.write( DataPtr, Data.GetDataSize() );
	}
	
	if ( File.fail() )
	{
		File.close();
		
		std::stringstream Error;
		Error << "Failed to write " << Soy::FormatSizeBytes( Data.GetDataSize() ) << " to " << Filename;
		throw Soy::AssertException( Error.str() );
	}
	
	File.close();
}

void Soy::ArrayToFile(const ArrayBridge<char>&& Data,const std::string& Filename,bool Append)
{
	ArrayToFile(Data,Filename,false,Append);
}

void Soy::ArrayToFile(const ArrayBridge<uint8_t>&& Data,const std::string& Filename,bool Append)
{
	ArrayToFile(Data,Filename,true,Append);
}


void Soy::StringToFile(std::string Filename,std::string String,bool Append)
{
	auto Flags = Append ? (std::ios::app|std::ios::out) :  std::ios::out;
	if ( Append )
		Flags |= std::ios::app;
	
	std::ofstream File( Filename, Flags );
	if ( !File.is_open() )
	{
		std::stringstream Error;
		Error << "Failed to open file " << Filename;
		throw Soy::AssertException( Error.str() );
	}
	
	File << String;
	bool Success = !File.fail();
	
	File.close();

	if ( !Success )
	{
		std::stringstream Error;
		Error << "Failed to write to file " << Filename;
		throw Soy::AssertException( Error.str() );
	}
}



void Soy::FileToString(std::string Filename,std::string& String)
{
	//	gr: err surely a better way
	Array<char> StringData;
	auto StringDataBridge = GetArrayBridge( StringData );
	LoadBinaryFile( StringDataBridge, Filename );
	
	String = Soy::ArrayToString( StringDataBridge );
	/*
	 std::ifstream File( Filename, std::ios::in );
	 if ( !File.is_open() )
		return false;
	 
	 #error this only pulls out first word
	 File >> String;
	 bool Success = !File.fail();
	 
	 File.close();
	 return Success;
	 */
}


void Soy::FileToString(std::string Filename,std::ostream& String)
{
	//	gr: err surely a better way
	Array<char> StringData;
	auto StringDataBridge = GetArrayBridge( StringData );
	LoadBinaryFile( StringDataBridge, Filename );
	
	Soy::ArrayToString( StringDataBridge, String );
}

void Soy::FileToStringLines(std::string Filename,ArrayBridge<std::string>& StringLines)
{
	//	get file as string then parse
	std::string FileContents;
	FileToString( Filename, FileContents );
	
	Soy::SplitStringLines( StringLines, FileContents );
}

#if defined(TARGET_WINDOWS)||defined(TARGET_LINUX)||defined(TARGET_ANDROID)
std::string Platform::GetAppResourcesDirectory()
{
	return Platform::GetDllPath();
}
#endif


#if !defined(TARGET_IOS)
std::string	Platform::GetDocumentsDirectory()
{
	Soy_AssertTodo();
}
#endif

std::string	Platform::GetTempDirectory()
{
	Soy_AssertTodo();
}
 
std::string	Platform::GetCacheDirectory()
{
	Soy_AssertTodo();
}



#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
std::string Platform::GetComputerName()
{
	char Buffer[1024];
	auto Result = gethostname(Buffer, std::size(Buffer));
	if ( Result != 0 )
		Platform::ThrowLastError("gethostname");
	std::string Name(Buffer);
	return Name;
}
#endif

#if defined(TARGET_WINDOWS)
std::string Platform::GetComputerName()
{
	char Buffer[MAX_PATH];
	DWORD Length = std::size(Buffer);
	if ( !GetComputerNameA(Buffer,&Length) )
		Platform::ThrowLastError("GetComputerNameA");
	std::string Name(Buffer);
	return Name;
}
#endif

#if defined(TARGET_UWP)
LPWSTR* CommandLineToArgvW(LPWSTR CommandLine,int* ArgCount)
{
	//	GetCommandLineW exists on uwp, but this helper function doesnt
	ArgCount = 0;
	return nullptr;
}
#endif

#if defined(TARGET_WINDOWS)
void Platform::GetExeArguments(ArrayBridge<std::string>&& Arguments)
{
	//	split the command line
	auto CommandLine = GetCommandLineW();
	
	int ArgCount = 0;
	auto* Args = CommandLineToArgvW(CommandLine, &ArgCount);

	//	skip exe at 0
	for (auto i = 1; i < ArgCount;	i++ )
	{
		auto ArgN = Args[i];
		auto ArgString = Soy::WStringToString(ArgN);
		Arguments.PushBack(ArgString);
	}
	LocalFree(Args);
}
#endif



void Platform::SetEnvVar(const char* Key,const char* Value)
{
#if defined(TARGET_LINUX)
	int Overwrite = 1;
	auto Result = setenv( Key, Value, Overwrite );
	if ( Result != 0 )
		Platform::ThrowLastError("setenv()");
#else
	throw Soy::AssertException("SetEnvVar not implemented on this platform");
#endif
}

std::string Platform::GetEnvVar(const char* Key)
{
#if defined(TARGET_OSX)||defined(TARGET_LINUX)
	const char* Value = getenv(Key);
	if ( !Value )
	{
		std::stringstream Error;
		Error << "Missing env var " << Key;
		throw Soy::AssertException( Error.str() );
	}
	return Value;
#elif defined(TARGET_WINDOWS)
	char* Buffer = nullptr;
	size_t BufferSize = 0;
	auto Result = _dupenv_s( &Buffer, &BufferSize, Key );

	if ( Result != S_OK || !Buffer )
	{
		std::stringstream Error;
		if ( Result == S_OK )
		{
			Error << "Null buffer returned for env var " << Key;
		}
		else
		{
			Error << "EnvVar " << Key << " error: " << ::Platform::GetErrorString(Result);
		}
		if ( Buffer )
		{
			free( Buffer );
			Buffer = nullptr;
		}
		throw Soy::AssertException( Error.str() );
	}

	std::string Value = Buffer;
	free( Buffer );
	Buffer = nullptr;
	return Value;
#else
	throw Soy::AssertException("GetEnvVar not implemented on this platform");
#endif
}


std::string Platform::GetCurrentWorkingDirectory()
{
	Array<char> Buffer;
	Buffer.SetSize(300);

#if defined(TARGET_WINDOWS)
	while ( !_getcwd( Buffer.GetArray(), Buffer.GetSize() ) )
#elif defined(TARGET_PS4)||defined(TARGET_IOS)||defined(TARGET_ANDROID)
	throw Soy::AssertException("Platform doesn't support current working dir");
	while(false)
#else
	while ( !getcwd( Buffer.GetArray(), Buffer.GetSize() ) )
#endif
	{
		//	check in case buffer isn't big enough
		auto LastError = ::Platform::GetLastError();
		if ( LastError == ERANGE )
		{
			Buffer.PushBlock(100);
			continue;
		}
		
		//	some other error
		std::stringstream Error;
		Error << "Failed to get current working directory: " << ::Platform::GetErrorString(LastError);
		throw Soy::AssertException( Error.str() );
	}
	
	return std::string( Buffer.GetArray() );
}

