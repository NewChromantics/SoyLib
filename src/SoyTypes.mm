#include "SoyTypes.h"
#include "SoyDebug.h"

Soy::TVersion Platform::GetOsVersion()
{
	auto OsVersion = [[NSProcessInfo processInfo] operatingSystemVersion];
	
	return Soy::TVersion( OsVersion.majorVersion, OsVersion.minorVersion, OsVersion.patchVersion );
}

void Platform::ExecuteTryCatchObjc(std::function<void()> Functor)
{
	@try
	{
		Functor();
	}
	@catch(NSException* e)
	{
		throw Soy::AssertException(e);
		//Soy::NSErrorToString( e ) << ">";
	}
}
