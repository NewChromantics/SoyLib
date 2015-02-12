#include "SoyDebug.h"
#include "SoyThread.h"
#include "SoyString.h"

//	gr: osx only?
#include <unistd.h>
#include <sys/sysctl.h>


namespace Soy
{
	//	defualt on, but allow build options to change default (or call Soy::EnableAssertThrow)
#if !defined(SOY_ENABLE_ASSERT_THROW)
#define SOY_ENABLE_ASSERT_THROW true
#endif
	bool	gEnableThrowInAssert = SOY_ENABLE_ASSERT_THROW;
}


std::DebugStream	std::Debug;

//	gr: although cout is threadsafe, it doesnt synchornise the output
std::mutex			CoutLock;



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


//	static per-thread
__thread std::string* ThreadBuffer = nullptr;	//	thread_local not supported on OSX

std::string& std::DebugStreamBuf::GetBuffer()
{
	if ( !ThreadBuffer )
		ThreadBuffer = new std::string();
	return *ThreadBuffer;
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
		{
			OutputDebugStringA( Buffer.c_str() );
		}
		else if ( mEnableStdOut )
		{
			std::lock_guard<std::mutex> lock(CoutLock);
			std::cout << Buffer.c_str();
			std::cout << std::flush;
		}
#elif defined(TARGET_OSX)
		static bool UseNsLog = false;
		if ( UseNsLog )
		{
			Soy::Platform::DebugPrint( Buffer );
		}
		
		if ( mEnableStdOut )
		{
			std::lock_guard<std::mutex> lock(CoutLock);
			std::cout << Buffer.c_str();
			std::cout << std::flush;
		}
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


void Soy::EnableThrowInAssert(bool Enable)
{
	gEnableThrowInAssert = Enable;
}

bool Soy::Assert(bool Condition, std::ostream& ErrorMessage ) throw( AssertException )
{
	return Assert( Condition, [&ErrorMessage]{	return Soy::StreamToString(ErrorMessage);	} );
}


#include <signal.h>
bool Soy::Platform::DebugBreak()
{
#if defined(TARGET_OSX)
	//	gr: supposedly this works, if you enable it in the scheme, but I don't know where it's declared
	//Debugger();
	
	//	raise an interrupt
	//	raise(SIGINT);
	raise(SIGUSR1);
	return true;
#endif
	
	//	not supported
	return false;
}

bool Soy::Assert(bool Condition,std::function<std::string()> ErrorMessageFunc) throw(AssertException)
{
	if ( Condition )
		return true;
	
	std::string ErrorMessage = ErrorMessageFunc();
	
	std::Debug << "Assert: " << ErrorMessage << std::endl;
	
	//	if the debugger is attached, try and break the debugger without an exception so we can continue
	if ( Platform::IsDebuggerAttached() )
		if ( Soy::Platform::DebugBreak() )
			return false;
	
	//	sometimes we disable throwing an exception to stop hosting being taken down
	if ( !gEnableThrowInAssert )
		return false;
	
	//	throw exception
	throw Soy::AssertException( ErrorMessage );
	return false;
}

