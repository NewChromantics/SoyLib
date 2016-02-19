#include "SoyUnity.h"




//	ScriptingStringPtr
//extern "C" const char* PlayerSettings_Get_Custom_PropIPhoneBundleIdentifier();
	

std::string Platform::GetBundleIdentifier()
{
	//	in editor this is the bundle of unity...
	//NSString* Version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleIdentifier"];
	NSString* Version = [[NSBundle mainBundle] bundleIdentifier];
	return Soy::NSStringToString( Version );
}

