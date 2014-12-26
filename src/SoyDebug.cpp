#include "SoyDebug.h"
#include "SoyThread.h"

//	gr: osx only?
#include <unistd.h>
#include <sys/sysctl.h>

std::DebugStream	std::Debug;


BufferString<20> Soy::FormatSizeBytes(uint64 bytes)
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
	BufferString<20> out;
	if ( ShowInteger )
		out << static_cast<int>( size );
	else
		out << size;
	out << sizesuffix;
	return out;
}


std::string& std::DebugStreamBuf::GetBuffer()
{
	auto Thread = SoyThread::GetCurrentThreadId();
	return mBuffers[Thread];
}



void std::DebugStreamBuf::flush()
{
	auto& Buffer = GetBuffer();
	if ( Buffer.length() > 0 )
	{
		//	gr: change these to be OS/main defined callbacks in OnFlush
		
#if defined(TARGET_WINDOWS)
		//	if there's a debugger attached output to that, otherwise to-screen
		if ( Soy::Platform::IsDebuggerAttached() )
			OutputDebugStringA( Buffer.c_str() );
		else
			std::cout << Buffer.c_str();
#elif defined(TARGET_OSX)
		//	todo: use NSLog!
		std::cout << Buffer.c_str();
		//NSLog(@"%s", message);
#endif
		mOnFlush.OnTriggered( Buffer );
		Buffer.erase();	// erase message buffer
	}
}

int std::DebugStreamBuf::overflow(int c)
{
	auto& Buffer = GetBuffer();
	Buffer += c;
	if (c == '\n') 
		flush();

	//	gr: what is -1? std::eof?
    return c == -1 ? -1 : ' ';
}



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

bool Soy::Platform::IsDebuggerAttached()
{
#if defined(TARGET_WINDOWS)
	return IsDebuggerPresent();
#endif
	
#if defined(TARGET_OSX)
	return XCodeDebuggerAttached();
#endif
	
	return false;
}

