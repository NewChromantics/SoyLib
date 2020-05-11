#include "SoyDebug.h"
#import <Foundation/Foundation.h>




#if defined(TARGET_OSX)
void Platform::DebugPrint(const char* String)
{
//	NSString* MacString = Soy::StringToNSString( String );

	NSLog( @"%s", String );
}
#endif


#if defined(TARGET_IOS)
void Platform::DebugPrint(const char* String)
{
	//	NSString* MacString = Soy::StringToNSString( String );
	
	NSLog( @"%s", String );
}
#endif
