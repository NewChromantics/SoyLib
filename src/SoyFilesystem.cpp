#include "SoyFilesystem.h"
#include "SoyDebug.h"
#include "HeapArray.hpp"

#if defined(TARGET_WINDOWS)
#include <filesystem>
#endif

#if defined(TARGET_LINUX)
#include <filesystem>
#include <unistd.h>	//	gethostname
#include <sys/stat.h>
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

#if defined(TARGET_LINUX)
	std::string	ExeFilename;
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
static void OnFileChanged(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
	auto& FileWatch = *reinterpret_cast<Platform::TFileMonitor*>( clientCallBackInfo );
	char **paths = reinterpret_cast<char **>(eventPaths);
	
	for ( int e=0;	e<numEvents;	e++ )
	{
		std::string Filename( paths[e] );
		//const FSEventStreamEventFlags& EventFlags( eventFlags[e] );
		//const FSEventStreamEventId EventIds( eventIds[e] );
		
		if ( FileWatch.mOnChanged )
			FileWatch.mOnChanged();
	}
}
#endif


Platform::TFileMonitor::TFileMonitor(const std::string& Filename)

#if defined(TARGET_OSX)
:
	mStream	( StreamRelease )
#endif
{
	//	debug callback
	auto DebugOnChanged = [=]()
	{
		std::Debug << Filename << " changed" << std::endl;
	};
	mOnChanged = DebugOnChanged;
	
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


Platform::TFileMonitor::~TFileMonitor()
{
}


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


#if defined(TARGET_WINDOWS) && !defined(HOLOLENS_SUPPORT)
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



#if defined(TARGET_WINDOWS) && defined(HOLOLENS_SUPPORT)
bool EnumDirectory(const std::string& Directory,std::function<bool(WIN32_FIND_DATA&)> OnItemFound)
{
	return false;
}
#endif

#if defined(TARGET_WINDOWS)
bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	//	because this can matter
	const char* DirectoryDelim = "\\";

	auto OnFindItem = [&](WIN32_FIND_DATAA& FindData)
	{
		try
		{
			auto UrlType = GetPathType( FindData );
			auto Filename = Directory + DirectoryDelim + GetFilename( FindData );
			
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
	SearchDirectory << Directory << DirectoryDelim << "*";

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
	
#if defined(TARGET_OSX)
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



#if defined(TARGET_WINDOWS)
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

#if defined(TARGET_WINDOWS)
void Platform::ShowFileExplorer(const std::string& Path)
{
#if defined(HOLOLENS_SUPPORT)
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

	try
	{
		auto Result = CoInitialize(nullptr);
		Platform::IsOkay(Result, "CoInitialize for SHOpenFolderAndSelectItems");
		Result = SHOpenFolderAndSelectItems(PathList, 0, 0, 0);
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
	return ExeFilename;
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

#if defined(TARGET_WINDOWS)
bool Platform::FileExists(const std::string& Path)
{
	auto FullPath = GetFullPathFromFilename(Path);
	return ::PathFileExistsA(FullPath.c_str());
}
#endif

#if defined(TARGET_LINUX)
namespace std
{
	namespace filesystem
	{
		bool is_directory(const std::string& Path);
	}
}
#endif

#if defined(TARGET_WINDOWS)||defined(TARGET_LINUX)
bool Platform::DirectoryExists(const std::string& Path)
{
	auto FullPath = GetFullPathFromFilename(Path);
	return std::filesystem::is_directory(FullPath);
	//return ::PathIsDirectoryA(FullPath.c_str());
	//auto Attribs = ::GetFileAttributesA(FullPath.c_str());
}
#endif

#if defined(TARGET_LINUX)
bool std::filesystem::is_directory(const std::string& Path)
{
	struct stat Stat;
	if (stat(Path.c_str(), &Stat) != 0)
		return false;	//	error
	
	bool isdir = S_ISDIR(Stat.st_mode);
	return isdir;
}
#endif

bool Platform::IsFullPath(const std::string& Path)
{
#if defined(TARGET_WINDOWS)
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

#if defined(TARGET_LINUX)
#include <linux/limits.h>
#endif


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

#elif defined(TARGET_WINDOWS)&&!defined(HOLOLENS_SUPPORT)

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


#if defined(TARGET_PS4)||defined(TARGET_LINUX)
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
	::Platform::CreateDirectory(Filename);
	
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

#if defined(TARGET_WINDOWS)||defined(TARGET_LINUX)
std::string Platform::GetAppResourcesDirectory()
{
	return Platform::GetDllPath();
}
#endif


#if defined(TARGET_LINUX)
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
