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

	//	gr: this is just to make it pretty and remove the protocol really...
	Soy::StringTrimLeft( AbsolutePath, "file://localhost", true );

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




bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(std::string&,SoyPathType::Type)> OnPathFound)
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

