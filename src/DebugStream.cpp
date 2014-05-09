#include "DebugStream.h"
#include "SoyNet.h"


	std::DebugStream	std::Debug;



void std::DebugStreamBuf::flush()
{
	if (mBuffer.length() > 0) 
	{
		OutputDebugStringA( mBuffer.c_str() );
		mBuffer.erase();	// erase message buffer
	}
}

int std::DebugStreamBuf::overflow(int c)
{
	mBuffer += c;
	if (c == '\n') 
		flush();

	//	gr: what is -1? std::eof?
    return c == -1 ? -1 : ' ';
}

