#include "SoyDebug.h"
#include "SoyThread.h"

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
#if defined(TARGET_WINDOWS)
		OutputDebugStringA( mBuffer.c_str() );
#elif defined(TARGET_OSX)
		std::cout << Buffer.c_str();
		//NSLog(@"%s", message);
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


