#include "SoyDebug.h"
#import <Foundation/Foundation.h>

//	gr: move this to SoyString.mm
namespace Soy
{
	NSString*	StringToNSString(const std::string& String);
	std::string	NSStringToString(NSString* String);
};

std::string Soy::NSStringToString(NSString* String)
{
	return std::string([String UTF8String]);
}

NSString* Soy::StringToNSString(const std::string& String)
{
	NSString* MacString = [NSString stringWithCString:String.c_str() encoding:[NSString defaultCStringEncoding]];
	return MacString;
}



#if defined(TARGET_OSX)
void Soy::Platform::DebugPrint(const std::string& String)
{
//	NSString* MacString = Soy::StringToNSString( String );

	NSLog( @"%s", String.c_str() );
}
#endif
