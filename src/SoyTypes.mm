#include "SoyTypes.h"


Soy::TVersion Platform::GetOsVersion()
{
	auto OsVersion = [[NSProcessInfo processInfo] operatingSystemVersion];
	
	return Soy::TVersion( OsVersion.majorVersion, OsVersion.minorVersion, OsVersion.patchVersion );
}
