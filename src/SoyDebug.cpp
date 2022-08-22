#include "SoyDebug.h"
#include "SoyThread.h"
#include "SoyString.h"
#include <iostream>

#if defined(TARGET_OSX)
#include <unistd.h>
#include <sys/sysctl.h>
#include <signal.h>
#endif

#if defined(TARGET_ANDROID)
#include <android/log.h>
#endif

#if defined(TARGET_LUMIN)
#include <ml_logging.h>
#endif


#if defined(TARGET_WINDOWS)&& !defined(TARGET_UWP)
namespace Platform
{
	//	gr: make a class for generic file handles as we use it for serial ports too.
	bool	ParentConsoleTried = false;
	HANDLE	ParentConsoleFileHandle = INVALID_HANDLE_VALUE;
	void	WriteToParentConsole(const char* String);
}
#endif



namespace Debug
{
	//	gr: although cout is threadsafe, it doesnt synchronise the output
	std::mutex	CoutCerrLock;

	//	use a lock to make cout/cerr prints synchronised
	bool	EnablePrint_CouterrSync = true;	

	//	gr: by default now, we debug to stderr. 
	//	as we want to use stdout sometimes for binary output
	bool	EnablePrint_Cout = false;
	bool	EnablePrint_Cerr = true;
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
	bool	EnablePrint_Platform = false;
#else
	bool	EnablePrint_Platform = true;
#endif
	bool	EnablePrint_Console = true;
	
	std::function<void(const char*)>	OnPrint;
}

std::string Soy::FormatSizeBytes(uint64 bytes)
{
	float size = static_cast<float>( bytes );
	const char* sizesuffix = "bytes";

	//	show bytes as an integer. 1.000 bytes makes no sense.
	bool ShowInteger = true;
	
	if ( size > 1024.f )
	{
		size /= 1024.f;
		sizesuffix = "kb";
		ShowInteger = false;
	}
	if ( size > 1024.f )
	{
		size /= 1024.f;
		sizesuffix = "mb";
		ShowInteger = false;
	}
	if ( size > 1024.f )
	{
		size /= 1024.f;
		sizesuffix = "gb";
		ShowInteger = false;
	}
	
	//$size = round($size,2);
	std::stringstream out;
	if ( ShowInteger )
		out << static_cast<int>( size );
	else
		out << size;
	out << sizesuffix;
	return out.str();
}


#if defined(TARGET_LINUX)
void Platform::DebugPrint(const char* Message)
{
	//	gr: superfolous? just write to cerr/cout
	//		find out if there's a gdb pipe or something
	printf("PopEngine: %s", Message);
}
#endif

#if defined(TARGET_ANDROID)
void Platform::DebugPrint(const char* String)
{
	__android_log_print( ANDROID_LOG_INFO, Platform::LogIdentifer, "pop: %s", String );
}
#endif


#if defined(TARGET_WINDOWS)
void Platform::DebugPrint(const char* String)
{
	OutputDebugStringA( String );
	WriteToParentConsole( String );
}
#endif

#if defined(TARGET_PS4)
void Platform::DebugPrint(const char* String)
{
	printf( "PopPs4: %s\n", String );
}
#endif

#if defined(TARGET_LUMIN)
void Platform::DebugPrint(const char* String)
{
	ML_LOG_TAG( Info, Platform::LogIdentifer, "%s", String );
}
#endif



#if defined(TARGET_WINDOWS) && !defined(TARGET_UWP)
void Platform::WriteToParentConsole(const char* String)
{
	//	if we don't have a handle, try and create one
	if (ParentConsoleFileHandle == INVALID_HANDLE_VALUE)
	{
		//	already tried and failed
		if (ParentConsoleTried)
			return;

		ParentConsoleTried = true;
		try
		{
			//	need to try and attach to the parent console (ie, cmd.exe)
			//	this fails if launched from explorer, visual studio etc
			//	without this, CONOUT$, CONIN$ etc won't open
			if (!AttachConsole(ATTACH_PARENT_PROCESS))
				Platform::ThrowLastError("AttachConsole(ATTACH_PARENT_PROCESS)");

			ParentConsoleFileHandle = ::CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
			if (ParentConsoleFileHandle == INVALID_HANDLE_VALUE)
				Platform::ThrowLastError("Couldn't open console handle CONOUT$");
		}
		catch (std::exception& e)
		{
			//	tried and failed
			std::cerr << e.what() << std::endl;
			return;
		}
	}

	auto BufferSize = strlen(String);
	DWORD Written = 0;
	if (!WriteFile(ParentConsoleFileHandle, String, BufferSize, &Written, nullptr))
	{
		//	failed to write, should report this!
	}
}
#endif


#if defined(TARGET_OSX)
//	http://stackoverflow.com/questions/4744826/detecting-if-ios-app-is-run-in-debugger
bool XCodeDebuggerAttached()
{
	int                 junk;
	int                 mib[4];
	struct kinfo_proc   info;
	size_t              size;
	
	// Initialize the flags so that, if sysctl fails for some bizarre
	// reason, we get a predictable result.
	
	info.kp_proc.p_flag = 0;
	
	// Initialize mib, which tells sysctl the info we want, in this case
	// we're looking for information about a specific process ID.
	
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();
	
	// Call sysctl.
	
	size = sizeof(info);
	junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
	assert(junk == 0);
	
	// We're being debugged if the P_TRACED flag is set.
	
	return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}
#endif


bool Platform::IsDebuggerAttached()
{
#if defined(TARGET_WINDOWS)
	return IsDebuggerPresent()==TRUE;
#endif
	
#if defined(TARGET_OSX)
	return XCodeDebuggerAttached();
#endif
	
	return false;
}


bool Platform::DebugBreak()
{
	//	gr: as of xcode 12.2, the asm errors and wont compile
	/*
#if defined(TARGET_OSX)
	static bool DoBreak = false;
	//	gr: supposedly this works, if you enable it in the scheme, but I don't know where it's declared
	//Debugger();
	if (DoBreak)
	{
		//		"invalid operand"
		__asm__("int $3");
	}
	//	raise an interrupt
	//	raise(SIGINT);
	//raise(SIGUSR1);
	return true;
#endif
*/

#if defined(TARGET_WINDOWS) && !defined(TARGET_UWP)
	static bool DoBreak = false;
	if (DoBreak)
	{
		::DebugBreak();
	}
	return true;
#endif

	//	not supported
	return false;
}

