#include "SoyDebug.h"
#include "SoyThread.h"
#include "SoyString.h"
#include <iostream>

#if defined(TARGET_OSX)
#include <unistd.h>
#include <sys/sysctl.h>
#include <signal.h>
#endif


std::DebugStream	std::Debug;

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
__thread Soy::HeapString* ThreadBuffer = nullptr;	//	thread_local not supported on OSX


//	singleton so the heap is created AFTER the heap register
prmem::Heap& GetDebugStreamHeap()
{
	static prmem::Heap DebugStreamHeap(true, true,"Debug stream heap");
	return DebugStreamHeap;
}


Soy::HeapString& std::DebugStreamBuf::GetBuffer()
{
	if ( !ThreadBuffer )
	{
		//auto& Heap = SoyThread::GetHeap( SoyThread::GetCurrentThreadNativeHandle() );
		auto& Heap = GetDebugStreamHeap();
		ThreadBuffer = Heap.Alloc<Soy::HeapString>( Heap.GetStlAllocator() );
		
		auto* NonTlsThreadBufferPtr = ThreadBuffer;
		auto Dealloc = [NonTlsThreadBufferPtr](const std::thread::native_handle_type&)
		{
			GetDebugStreamHeap().Free( NonTlsThreadBufferPtr );
		};
		SoyThread::GetOnThreadCleanupEvent()->AddListener( Dealloc );
	}
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
			std::string BufferStr(Buffer.c_str());
			Soy::Platform::DebugPrint( BufferStr );
		}
		
		if ( mEnableStdOut )
		{
			std::lock_guard<std::mutex> lock(CoutLock);
			std::cout << Buffer.c_str();
			std::cout << std::flush;
		}
		//NSLog(@"%s", message);
#endif
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

