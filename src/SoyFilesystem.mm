#include "SoyFilesystem.h"
#include "SoyAvf.h"
#include <CoreFoundation/CoreFoundation.h>




bool GetUrlKeyBool(NSURL* Path,NSString* Key)
{
	NSNumber* KeyValue = nil;
	NSError* Error = nil;
	[Path getResourceValue:&KeyValue forKey:Key error:&Error];
	
	if ( !KeyValue )
	{
		std::stringstream Error;
		Error << "Key not found " << Soy::NSStringToString(Key);
		throw Soy::AssertException( Error.str() );
	}

	auto Value = [KeyValue boolValue];
	return Value;
}

std::string UrlGetFilename(NSURL* Path)
{
	auto* AbsolutePathNs = [Path absoluteString];
	auto AbsolutePath = Soy::NSStringToString( AbsolutePathNs );
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
	try
	{
		if ( GetUrlKeyBool( Path, NSURLIsDirectoryKey ) )
			return SoyPathType::Directory;
	}
	catch(...)
	{
	}
	
	
	try
	{
		if ( GetUrlKeyBool( Path, NSURLIsRegularFileKey ) )
			return SoyPathType::File;
	}
	catch(...)
	{
	}

	return SoyPathType::Unknown;
}




bool EnumDirectory(const std::string& Directory,std::function<bool(std::string&,SoyPathType::Type)> OnPathFound)
{
	auto directoryURL = Avf::GetUrl( Directory );
 
	NSArray* Keys = [NSArray arrayWithObjects:NSURLIsDirectoryKey, NSURLIsPackageKey, NSURLLocalizedNameKey, nil];
	
	auto ErrorHandler = ^(NSURL *url, NSError *error) {
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


void Platform::EnumNsDirectory(const std::string& Directory,std::function<void(const std::string&)> OnFileFound,bool Recursive)
{
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
				Soy::StringTrimLeft( Path, "file://", true );
				OnFileFound( Path );
			}
			
			if ( MatchCount > MatchLimit )
			{
				std::Debug << "Hit match limit (" << MatchCount << ") bailing in case we've got stuck in a loop" << std::endl;
				return false;
			}
			
			return true;
		};
		
		if ( !EnumDirectory( Dir, AddFile ) )
			break;
	}
}

