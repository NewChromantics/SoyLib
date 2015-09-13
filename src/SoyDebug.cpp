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


namespace Soy
{
	std::string		DebugContext = "Pop";	//	where applicable on platforms a context/tag for debug prints
};


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
void Soy::Platform::DebugPrint(const std::string& Message)
{
	__android_log_print( ANDROID_LOG_INFO, Soy::DebugContext.c_str(), "pop: %s", Message.c_str() );
}
#endif


#if defined(TARGET_WINDOWS)
void Soy::Platform::DebugPrint(const std::string& Message)
{
	OutputDebugStringA( Message.c_str() );
}
#endif

#if defined(USE_HEAP_STRING)
//	singleton so the heap is created AFTER the heap register
prmem::Heap& GetDebugStreamHeap()
{
	static prmem::Heap DebugStreamHeap(true, true,"Debug stream heap");
	return DebugStreamHeap;
}
#endif


std::DebugBufferString& std::DebugStreamBuf::GetBuffer()
{
	if ( !ThreadBuffer )
	{
#if defined(USE_HEAP_STRING)
		//auto& Heap = SoyThread::GetHeap( SoyThread::GetCurrentThreadNativeHandle() );
		auto& Heap = GetDebugStreamHeap();
		ThreadBuffer = Heap.Alloc<Soy::HeapString>( Heap.GetStlAllocator() );
		
		auto* NonTlsThreadBufferPtr = ThreadBuffer;
		auto Dealloc = [NonTlsThreadBufferPtr](const std::thread::native_handle_type&)
		{
			GetDebugStreamHeap().Free( NonTlsThreadBufferPtr );
		};
		SoyThread::GetOnThreadCleanupEvent()->AddListener( Dealloc );
#else
		ThreadBuffer = new std::string();
#endif
	}
	return *ThreadBuffer;
}



void std::DebugStreamBuf::flush()
{
	auto& Buffer = GetBuffer();
	
	if ( Buffer.length() > 0 )
	{
		//	gr: change these to be OS/main defined callbacks in OnFlush
		bool PlatformDebugPrint = true;
		bool PlatformStdout = mEnableStdOut;
	
#if defined(TARGET_WINDOWS)

		//	if there's a debugger attached output to that, otherwise to-screen
		PlatformStdout &= !Soy::Platform::IsDebuggerAttached();
		PlatformDebugPrint = Soy::Platform::IsDebuggerAttached();

#elif defined(TARGET_OSX)

		static bool UseNsLog = false;
		PlatformDebugPrint = UseNsLog;

#endif
		if ( PlatformStdout )
		{
			std::lock_guard<std::mutex> lock(CoutLock);
			std::cout << Buffer.c_str();
			std::cout << std::flush;
		}
		
		if ( PlatformDebugPrint )
		{
			std::string BufferStr(Buffer.c_str());
			Soy::Platform::DebugPrint( BufferStr );
		}
		
		if ( mOnFlush.HasListeners() )
		{
			std::string BufferStr(Buffer.c_str());
			mOnFlush.OnTriggered(BufferStr);
		}
		
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
	return IsDebuggerPresent()==TRUE;
#endif
	
#if defined(TARGET_OSX)
	return XCodeDebuggerAttached();
#endif
	
	return false;
}


bool Soy::Platform::DebugBreak()
{
#if defined(TARGET_OSX)
	static bool DoBreak = true;
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

#if defined(TARGET_WINDOWS)
	static bool DoBreak = true;
	if (DoBreak)
	{
		::DebugBreak();
	}
	return true;
#endif

	//	not supported
	return false;
}

