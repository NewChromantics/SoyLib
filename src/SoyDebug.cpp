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


#if defined(TARGET_WINDOWS)
namespace Platform
{
	//	gr: make a class for generic file handles as we use it for serial ports too.
	bool	ParentConsoleTried = false;
	HANDLE	ParentConsoleFileHandle = INVALID_HANDLE_VALUE;
	void	WriteToParentConsole(const std::string& String);
}
#endif


std::DebugStreamThreadSafeWrapper	std::Debug;

//	gr: although cout is threadsafe, it doesnt synchornise the output
std::mutex			CoutLock;



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

//	static per-thread
#if defined(TARGET_ANDROID)
//	gr: on android, having a __thread makes my DLL incompatible with unity's build/load (or androids?)
std::DebugBufferString* ThreadBuffer = nullptr;
#else
__thread std::DebugBufferString* ThreadBuffer = nullptr;	//	thread_local not supported on OSX
#endif



#if defined(TARGET_ANDROID)
void Platform::DebugPrint(const std::string& Message)
{
	__android_log_print( ANDROID_LOG_INFO, Platform::LogIdentifer, "pop: %s", Message.c_str() );
}
#endif


#if defined(TARGET_WINDOWS)
void Platform::DebugPrint(const std::string& Message)
{
	OutputDebugStringA( Message.c_str() );
}
#endif

#if defined(TARGET_PS4)
void Platform::DebugPrint(const std::string& Message)
{
	printf( "PopPs4: %s\n", Message.c_str() );
}
#endif

#if defined(TARGET_LUMIN)
void Platform::DebugPrint(const std::string& Message)
{
	ML_LOG_TAG( Info, Platform::LogIdentifer, "%s", Message.c_str() );
}
#endif

//	singleton so the heap is created AFTER the heap register
prmem::Heap& Soy::GetDebugStreamHeap()
{
#if defined(USE_HEAP_STRING)
	static prmem::Heap DebugStreamHeap(true, true,"Debug stream heap");
	return DebugStreamHeap;
#else
	throw Soy::AssertException("Not using debug stream heap");
#endif
}


std::DebugBufferString& std::DebugStreamBuf::GetBuffer()
{
	if ( !ThreadBuffer )
	{
#if defined(USE_HEAP_STRING)
		//auto& Heap = SoyThread::GetHeap( SoyThread::GetCurrentThreadNativeHandle() );
		auto& Heap = Soy::GetDebugStreamHeap();
		ThreadBuffer = Heap.Alloc<Soy::HeapString>( Heap.GetStlAllocator() );
		
		/*	gr: how is this supposed to work? dealloc when any thread closes??
				now streambuf is threadsafe... maybe this doesn't matter any more?
		auto* NonTlsThreadBufferPtr = ThreadBuffer;
		auto Dealloc = [NonTlsThreadBufferPtr](std::thread&)
		{
			GetDebugStreamHeap().Free( NonTlsThreadBufferPtr );
		};
		
		SoyThread::OnThreadFinish.AddListener( Dealloc );
		 */
#else
		ThreadBuffer = new std::string();
#endif
	}
	return *ThreadBuffer;
}


#if defined(TARGET_WINDOWS)
void Platform::WriteToParentConsole(const std::string& String)
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

	auto* Buffer = String.c_str();
	auto BufferSize = String.length();
	DWORD Written = 0;
	if (!WriteFile(ParentConsoleFileHandle, Buffer, BufferSize, &Written, nullptr))
	{
		//	failed to write, should report this!
	}
}
#endif


void std::DebugStreamBuf::flush()
{
	auto& Buffer = GetBuffer();
	
	if ( Buffer.length() > 0 )
	{
		//	gr: change these to be OS/main defined callbacks in OnFlush
		bool PlatformDebugPrint = true;
		bool PlatformStdout = mEnableStdOut;
		auto& PlatformOutputBuffer = std::cout;
	
		std::string BufferStr(Buffer.c_str());

#if defined(TARGET_WINDOWS)

		//	if there's a debugger attached output to that, otherwise to-screen
		//PlatformStdout &= !Platform::IsDebuggerAttached();
		PlatformStdout = true;
		PlatformDebugPrint = Platform::IsDebuggerAttached();

#elif defined(TARGET_OSX)

		static bool UseNsLog = false;
		PlatformDebugPrint = UseNsLog;

#endif
		if ( PlatformStdout )
		{
			std::lock_guard<std::mutex> lock(CoutLock);
			PlatformOutputBuffer << BufferStr;
			PlatformOutputBuffer << std::flush;
		}
		
		if ( PlatformDebugPrint )
		{
			Platform::DebugPrint( BufferStr );
		}
		
		if ( mOnFlush )
		{
			mOnFlush(BufferStr);
		}
		
		//	on windows, try and write to parent console window
#if defined(TARGET_WINDOWS)
		Platform::WriteToParentConsole(BufferStr);
#endif

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
#if defined(TARGET_OSX)
	static bool DoBreak = false;
	//	gr: supposedly this works, if you enable it in the scheme, but I don't know where it's declared
	//Debugger();
	if (DoBreak)
	{
		__asm__("int $3");
	}
	//	raise an interrupt
	//	raise(SIGINT);
	//raise(SIGUSR1);
	return true;
#endif

#if defined(TARGET_WINDOWS) && !defined(HOLOLENS_SUPPORT)
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

