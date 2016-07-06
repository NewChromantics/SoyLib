#include "SoyDebug.h"
#import <Foundation/Foundation.h>




#if defined(TARGET_OSX)
void Platform::DebugPrint(const std::string& String)
{
//	NSString* MacString = Soy::StringToNSString( String );

	NSLog( @"%s", String.c_str() );
}
#endif


#if defined(TARGET_IOS)
void Soy::Platform::DebugPrint(const std::string& String)
{
	//	NSString* MacString = Soy::StringToNSString( String );
	
	NSLog( @"%s", String.c_str() );
}
#endif
