#include "SoyRuntimeLibrary.h"
#include <dlfcn.h>
#include "heaparray.hpp"
#include "SoyTypes.h"
#include "SoyDebug.h"


void DebugEnvVar(const char* Key)
{
	const char* Value = getenv(Key);
	if ( !Value )
		Value = "<null>";
	std::Debug << Key << "=" << Value << std::endl;
}

std::string Soy::GetCurrentWorkingDir()
{
	Array<char> Buffer;
	Buffer.SetSize(300);
	
	while ( !getcwd( Buffer.GetArray(), Buffer.GetSize() ) )
	{
		//	check in case buffer isn't big enough
		auto LastError = Soy::Platform::GetLastError();
		if ( LastError == ERANGE )
		{
			Buffer.PushBlock(100);
			continue;
		}
		
		//	some other error
		std::stringstream Error;
		Error << "Failed to get current working directory: " << Soy::Platform::GetErrorString(LastError);
		throw Soy::AssertException( Error.str() );
	}
	
	return std::string( Buffer.GetArray() );
}



Soy::TRuntimeLibrary::TRuntimeLibrary(std::string Filename,std::function<bool(void)> LoadTest)
#if defined(TARGET_OSX)
:
	mHandle		( nullptr )
#endif
{
	if ( LoadTest && LoadTest() )
	{
		std::Debug << "Warning, load test succeeded before loading library..." << std::endl;
		return;
	}
	
#if defined(TARGET_OSX)
	//	link all symbols immediately
	int Mode = RTLD_LAZY | RTLD_GLOBAL;
	mHandle = dlopen( Filename.c_str(), Mode );
	
	if ( !mHandle )
	{
		DebugEnvVar("PATH");
		DebugEnvVar("@loader_path/");
		DebugEnvVar("@rpath/");
		DebugEnvVar("@executable_path/");
		DebugEnvVar("DYLD_LIBRARY_PATH");
		DebugEnvVar("DYLD_FRAMEWORK_PATH");
		std::Debug << "cwd: " << GetCurrentWorkingDir() << std::endl;
		
		const char* ErrorStr = dlerror();
		if ( !ErrorStr )
			ErrorStr = "<null error>";
		
		std::stringstream Error;
		Error << "dlopen failed for " << Filename << ": " << ErrorStr;
		throw Soy::AssertException( Error.str() );
	}
	
	//	run test
	if ( LoadTest && !LoadTest() )
	{
		Close();
		
		std::stringstream Error;
		Error << "Loaded library " << Filename << " but load-test failed.";
		
		throw Soy::AssertException( Error.str() );
	}
	
#else
	throw Soy::AssertException("Soy::TRuntimeLibrary not implemented");
#endif
}

void Soy::TRuntimeLibrary::Close()
{
#if defined(TARGET_OSX)
	if ( mHandle )
	{
		dlclose( mHandle );
		mHandle = nullptr;
	}
#endif
}

void* Soy::TRuntimeLibrary::GetSymbol(const char* Name)
{
#if defined(TARGET_OSX)
	//	throw?
	if ( !mHandle )
		return nullptr;
	return dlsym( mHandle, Name );
#else
	return nullptr;
#endif
}



