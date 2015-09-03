#include "SoyString.h"
#import <Foundation/Foundation.h>


std::string Soy::NSStringToString(NSString* String)
{
	return std::string([String UTF8String]);
}

NSString* Soy::StringToNSString(const std::string& String)
{
	NSString* MacString = [NSString stringWithCString:String.c_str() encoding:[NSString defaultCStringEncoding]];
	return MacString;
}


std::string Soy::NSErrorToString(NSError* Error)
{
	if ( !Error )
		return "Error(null)";
	
	//	in case description is missing
	try
	{
		auto* ErrorNs = [Error description];
		return NSStringToString( ErrorNs );
	}
	catch ( ... )
	{
		return "Error(exception getting description)";
	}
}

