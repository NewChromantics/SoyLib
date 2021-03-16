#include "SoyPlatform.h"
#include "SoyDebug.h"


#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
#include <unistd.h>	//	gethostname
#endif


#if defined(TARGET_LINUX)||defined(TARGET_ANDROID)
std::string Platform::GetComputerName()
{
	char Buffer[1024];
	auto Result = gethostname(Buffer, std::size(Buffer));
	if ( Result != 0 )
		Platform::ThrowLastError("gethostname");
	std::string Name(Buffer);
	return Name;
}
#endif

#if defined(TARGET_WINDOWS)
std::string Platform::GetComputerName()
{
	char Buffer[MAX_PATH];
	DWORD Length = std::size(Buffer);
	if ( !GetComputerNameA(Buffer,&Length) )
		Platform::ThrowLastError("GetComputerNameA");
	std::string Name(Buffer);
	return Name;
}
#endif



std::string Platform::GetEnvVar(const char* Key)
{
#if defined(TARGET_OSX)||defined(TARGET_LINUX) || defined(TARGET_ANDROID)
	const char* Value = getenv(Key);
	if ( !Value )
	{
		std::stringstream Error;
		Error << "Missing env var " << Key;
		throw Soy::AssertException( Error.str() );
	}
	return Value;
#elif defined(TARGET_WINDOWS)
	char* Buffer = nullptr;
	size_t BufferSize = 0;
	auto Result = _dupenv_s( &Buffer, &BufferSize, Key );

	if ( Result != S_OK || !Buffer )
	{
		std::stringstream Error;
		if ( Result == S_OK )
		{
			Error << "Null buffer returned for env var " << Key;
		}
		else
		{
			Error << "EnvVar " << Key << " error: " << ::Platform::GetErrorString(Result);
		}
		if ( Buffer )
		{
			free( Buffer );
			Buffer = nullptr;
		}
		throw Soy::AssertException( Error.str() );
	}

	std::string Value = Buffer;
	free( Buffer );
	Buffer = nullptr;
	return Value;
#else
	throw Soy::AssertException("GetEnvVar not implemented on this platform");
#endif
}


void Platform::SetEnvVar(const char* Key,const char* Value)
{
#if defined(TARGET_LINUX)
	int Overwrite = 1;
	auto Result = setenv( Key, Value, Overwrite );
	if ( Result != 0 )
		Platform::ThrowLastError("setenv()");
#else
	throw Soy::AssertException("SetEnvVar not implemented on this platform");
#endif
}


