#include "SoyFilesystem.h"
#include "SoyAvf.h"
#include <CoreFoundation/CoreFoundation.h>
#include "SoyString.h"

#if defined(TARGET_OSX)
#include <AppKit/AppKit.h>
#include <AppKit/NSWorkspace.h>
#endif

#if defined(TARGET_IOS)
#include <UIKit/UIKit.h>
#endif


namespace Platform
{
	bool	EnumDirectoryUnsafe(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound);
}

bool GetUrlKeyBool(NSURL* Path,NSString* Key)
{
	NSNumber* KeyValue = nil;
	NSError* Error = nil;
	if ( ![Path getResourceValue:&KeyValue forKey:Key error:&Error] )
	{
		std::stringstream Error;
		Error << "Key not found " << Soy::NSStringToString(Key);
		throw Soy::AssertException( Error.str() );
	}

	//	gr; this can be nil...
	//if ( !KeyValue )

	auto Value = [KeyValue boolValue];
	return Value;
}


std::string GetUrlKeyString(NSURL* Path,NSString* Key)
{
	NSString* KeyValue = nil;
	NSError* Error = nil;
	if ( ![Path getResourceValue:&KeyValue forKey:Key error:&Error] )
	{
		std::stringstream Error;
		Error << "Key not found " << Soy::NSStringToString(Key);
		throw Soy::AssertException( Error.str() );
	}

	auto ValueString = Soy::NSStringToString(KeyValue);
	return ValueString;
}


std::string UrlGetFilename(NSURL* Path)
{
	//	gr: we have a problem on ios where enumerating /var/appxxx
	//		enumerates filenames that are /private/var/appxxxx/filename
	//	/private/var is a symlink to /var
	//		so the solution is to always resolve so the initial dir will be the true path
	//	https://stackoverflow.com/a/16026678/355753
	Path = [Path URLByResolvingSymlinksInPath];
	
	//	gr: absoluteString is urlencoded
	//auto* AbsolutePathStrNs = [Path absoluteString];
	auto* AbsolutePathNs = [Path path];
	
	auto AbsolutePath = Soy::NSStringToString( AbsolutePathNs );

	//	gr: this is just to make it pretty and remove the protocol really...
	//	gr: somewhere else, I explicitly remove the protocol, instead of a hardcoded string...
	Soy::StringTrimLeft( AbsolutePath, "file://localhost", true );
	Soy::StringTrimLeft( AbsolutePath, "file://", true );

	return AbsolutePath;
	/*
	NSString* LocalizedName = nil;
	NSError* Error = nil;
	[Path getResourceValue:&LocalizedName forKey:NSURLNameKey error:&Error];

	return Soy::NSStringToString( LocalizedName );
	 */
}

SoyPathType::Type GetPathType(NSURL* Path)
{
	auto GetUrlKeyBool_Safe = [&](NSURLResourceKey Key)
	{
		try
		{
			auto Value = GetUrlKeyBool( Path, Key );
			return Value;
		}
		catch(...)
		{
			return false;
		}
	};
	
	if ( GetUrlKeyBool_Safe(NSURLIsDirectoryKey) )
		return SoyPathType::Directory;
	
	if ( GetUrlKeyBool_Safe(NSURLIsRegularFileKey) )
		return SoyPathType::File;
	
	//	gr: can't find a key test for our serial ones...
	{
		auto ResourceType = GetUrlKeyString( Path, NSURLFileResourceTypeKey );
		if ( ResourceType == "NSURLFileResourceTypeCharacterSpecial" )
			return SoyPathType::Special;
	}

	try
	{
		auto ResourceType = GetUrlKeyString( Path, NSURLFileResourceTypeKey );
		std::Debug << "Unhandled file type " << ResourceType << " for " << Soy::NSStringToString(Path.absoluteString) << std::endl;
	}
	catch(std::exception& e)
	{
		//
	}
	
	return SoyPathType::Unknown;
}



bool Platform::EnumDirectoryUnsafe(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	auto directoryURL = Platform::GetUrl( Directory );
	
	//	dir doesn't exist
	if ( directoryURL == nullptr )
		return true;
 
	NSArray* Keys = [NSArray arrayWithObjects:NSURLIsDirectoryKey, NSURLIsPackageKey, NSURLLocalizedNameKey, nil];
	
	auto ErrorHandler = ^(NSURL *url, NSError *error)
	{
		// Handle the error.
		// Return YES if the enumeration should continue after the error.
		return YES;
	};
	
	NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager]
										 enumeratorAtURL:directoryURL
										 includingPropertiesForKeys:Keys
										 options:(NSDirectoryEnumerationSkipsPackageDescendants |
												  NSDirectoryEnumerationSkipsHiddenFiles)
										 errorHandler:ErrorHandler];
 
	for (NSURL* url in enumerator)
	{
		//	this stops the enumerator going into subdirectories from this point
		[enumerator skipDescendants];
		
		try
		{
			auto UrlType = GetPathType( url );
			auto Filename = UrlGetFilename( url );
			
			//	if this returns false, bail
			if ( !OnPathFound( Filename, UrlType ) )
				return false;
		}
		catch(std::exception& e)
		{
			std::Debug << "Error extracting path meta; " << e.what() << std::endl;
		}
	}
	
	return true;
}


bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	//	catch obj-c exceptions
	@try
	{
		return EnumDirectoryUnsafe( Directory, OnPathFound );
	}
	@catch (NSException* e)
	{
		std::stringstream Error;
		Error << "Platform::EnumDirectory NSException " << Soy::NSErrorToString( e );
		throw Soy::AssertException( Error.str() );
	}
	@catch (...)
	{
		std::stringstream Error;
		Error << "Platform::EnumDirectory Unknown exception";
		throw Soy::AssertException( Error.str() );
	}
}

NSURL* Platform::GetUrl(const std::string& Filename)
{
	NSString* UrlString = Soy::StringToNSString( Filename );
	NSError *err;
	
	//	try as file which we can test for immediate fail
	NSURL* Url = [[NSURL alloc]initFileURLWithPath:UrlString];
	//	resolve ../ ./ etc
	Url = [Url standardizedURL];
	
	if ([Url checkResourceIsReachableAndReturnError:&err] == NO)
	{
		//	FILE is not reachable.
		
		//	try as url
		Url = [[NSURL alloc]initWithString:UrlString];
		
		//	gr: throw this error IF we KNOW it's a file we're trying to reach and not an url.
		//		check for ANY scheme?
		std::stringstream Error;
		Error << "Failed to reach file from url: " << Filename << "; " << Soy::NSErrorToString(err);
		throw Soy::AssertException( Error.str() );
	}
	
	return Url;
}


bool Platform::DirectoryExists(const std::string& Filename)
{
	NSString* Path = Soy::StringToNSString( Filename );
	BOOL IsDir = NO;
	BOOL IsFile = [[NSFileManager defaultManager] fileExistsAtPath:Path isDirectory:&IsDir];
	
	return IsDir && IsFile;
}

bool Platform::FileExists(const std::string& Filename)
{
	NSString* Path = Soy::StringToNSString( Filename );
	BOOL IsDir = NO;
	BOOL IsFile = [[NSFileManager defaultManager] fileExistsAtPath:Path isDirectory:&IsDir];
	
	return !IsDir && IsFile;

	/*
	NSString* UrlString = Soy::StringToNSString( Filename );
	NSError *err;
	
	//	try as file which we can test for immediate fail
	NSURL* Url = [[NSURL alloc]initFileURLWithPath:UrlString];
	if ([Url checkResourceIsReachableAndReturnError:&err] == NO)
		return false;
	
	return true;
	*/
}

void Platform::ShowFileExplorer(const std::string& Path)
{
#if defined(TARGET_OSX)
	auto PathUrl = GetUrl( Path );
	//	gr: this should have already thrown
	if ( !PathUrl )
		throw Soy::AssertException("Failed to get url");
	
	NSArray* FileURLs = [NSArray arrayWithObjects:PathUrl,nil];
	[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:FileURLs];

#else
	//	maybe this should popup the "share file" thing in ios
	throw Soy::AssertException("Platform::ShowFileExplorer not implemented");
#endif
}



std::string Platform::GetFullPathFromFilename(const std::string& Filename)
{
	@autoreleasepool
	{
		NSString* UrlString = Soy::StringToNSString( Filename );
		NSError *err = nullptr;
	
		//	try as file which we can test for immediate fail
		NSURL* Url = [[NSURL alloc]initFileURLWithPath:UrlString];
		//	resolve ../ ./ etc
		//	gr: this turns /Hello/Cat//../World into /Hello/Cat/World, not as expected
		Url = [Url standardizedURL];

		if ( !Url.fileURL )
		{
			std::stringstream Error;
			Error << Filename << " is not a file url (file not found)";
			throw Soy::AssertException( Error.str() );
		}

		if ([Url checkResourceIsReachableAndReturnError:&err] == YES)
		{
			//	get the absolute path
			//	but this includes the scheme, we need to remove it
			auto Scheme = Soy::NSStringToString( Url.scheme );
			Scheme += "://";
			auto AbsolutePath = Soy::NSStringToString(Url.absoluteString);
			Soy::StringTrimLeft( AbsolutePath, Scheme, true );
			return AbsolutePath;
		}
	
		std::stringstream Error;
		Error << "Unreachable file url: " << Filename << "; " << Soy::NSErrorToString(err);
		throw Soy::AssertException( Error.str() );
	}
}


void Platform::ShellExecute(const std::string& Path)
{
#if defined(TARGET_OSX)
	//	gr: this isn't proper shell execute
	//		maybe this should wrap Platform::AllocProcess
	auto ExitCode = system(Path.c_str());
	if ( ExitCode == -1 )
		Platform::ThrowLastError(std::string("system(") + Path + std::string(")"));
#elif defined(TARGET_IOS)
	ShellOpenUrl(Path);
#else
#error Missing target
#endif
}

void Platform::ShellOpenUrl(const std::string& UrlString)
{
	NSString* UrlStringN = Soy::StringToNSString( UrlString );
	//	gr: realised an url with spaces was returning nil (no errors)
	//		should we error, or urlescape (we'd hope it would just open with args, 
	//		so maybe url escaping should be per platform for their needs)
	//	https://stackoverflow.com/a/7996507/355753
	UrlStringN = [UrlStringN stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
	//UrlStringN = [UrlStringN stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLPathAllowedCharacterSet];
	auto* Url = [[NSURL alloc]initWithString:UrlStringN];
	
	if ( !Url )
	{
		std::stringstream Error;
		Error << "Address [" << UrlString << "] failed to convert to NSUrl";
		throw Soy::AssertException(Error);
	}
#if defined(TARGET_OSX)
	//	https://gist.github.com/piaoapiao/4103404
	//	https://stackoverflow.com/questions/17497561/opening-web-url-with-nsbutton-mac-os
	[[NSWorkspace sharedWorkspace] openURL:Url];
#elif defined(TARGET_IOS)
	Soy_AssertTodo();
	//	https://developer.apple.com/documentation/uikit/uiapplicationdelegate/1623112-application?language=objc
	//auto *application = [UIApplication sharedApplication];
	//[application openURL:Url];
#else
	#error Missing target
#endif
}

std::string Platform::GetAppResourcesDirectory()
{
	//	https://stackoverflow.com/a/24420220
	CFURLRef url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	if ( !url )
		throw Soy::AssertException("Failed to get bundle resources url");

	try
	{
		char PathChars[PATH_MAX];
		auto* Path8 = reinterpret_cast<UInt8*>(PathChars);
		if ( !CFURLGetFileSystemRepresentation(url, true, Path8, sizeof(PathChars) ) )
			throw Soy::AssertException("Failed to get bundle resources directory name from url");
	
		CFRelease(url);
		std::string Path( PathChars );
		if ( !Soy::StringEndsWith( Path, "/", true ) )
			Path += "/";
		return Path;
	}
	catch(...)
	{
		CFRelease(url);
		throw;
	}
}

#if defined(TARGET_IOS)||defined(TARGET_OSX)
std::string	Platform::GetDocumentsDirectory()
{
	//	NSCachesDirectory
	//	NSItemReplacementDirectory
	auto DirName = NSDocumentDirectory;
	//	gr: why is there more than one?
	auto* FileMan = [NSFileManager defaultManager];
	auto* DirUrls = [FileMan URLsForDirectory:DirName inDomains:NSUserDomainMask];
	if ( DirUrls.count == 0 )
		throw Soy::AssertException("NSDocumentDirectory returned no directorys");
	if ( DirUrls.count > 1 )
		std::Debug << "NSDocumentDirectory returned " << DirUrls.count << " dirs" << std::endl;
	auto* DirUrl = DirUrls[0];
	auto Filename = UrlGetFilename(DirUrl);
	return Filename;
}
#endif




std::string Platform::GetExeFilename()
{
	NSArray<NSString*>* ArgumentsNs = [[NSProcessInfo processInfo] arguments];
	std::string Exe = Soy::NSStringToString(ArgumentsNs[0]);
	return Exe;
}


void NSArray_String_ForEach(NSArray<NSString*>* Array,std::function<void(std::string&)> Enum,int StartIndex=0)
{
	auto Size = [Array count];
	for ( auto i=StartIndex;	i<Size;	i++ )
	{
		auto Element = [Array objectAtIndex:i];
		//auto StringNs = [Element stringValue];
		auto String = Soy::NSStringToString(Element);
		Enum( String );
	}
}

void Platform::GetExeArguments(ArrayBridge<std::string>&& Arguments)
{
	auto PushArgument = [&](std::string& Argument)
	{
		Arguments.PushBack( Argument );
	};
	NSArray<NSString*>* ArgumentsNs = [[NSProcessInfo processInfo] arguments];
	//	skip 1st which is exe
	NSArray_String_ForEach( ArgumentsNs, PushArgument, 1 );
}
