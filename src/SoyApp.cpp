#include "SoyApp.h"


SoyEvent<bool> gOnConsoleStop;


#if defined(TARGET_WINDOWS)
BOOL WINAPI Soy::Platform::TConsoleApp::ConsoleHandler(DWORD dwType)
{
	bool Dummy;
	switch(dwType) 
	{
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			gOnConsoleStop.OnTriggered(Dummy);

			//Returning would make the process exit immediately!
			//We just make the handler sleep until the main thread exits,
			//or until the maximum execution time for this handler is reached.
			Sleep(10000);
		return TRUE;

	default:
		return FALSE;
	}
}
#endif

void Soy::Platform::TConsoleApp::Exit()
{
	mWorker.Stop();
}

void Soy::Platform::TConsoleApp::WaitForExit()
{
	//	setup handler
#if defined(TARGET_WINDOWS)
	SetConsoleCtrlHandler( ConsoleHandler, true );
#endif
	auto& Worker = mWorker;
	auto OnConsoleStop = [&Worker](bool&)
	{
		Worker.Stop();
	};
	gOnConsoleStop.AddListener( OnConsoleStop );
	
	//	runs until something tells it to exit
	mWorker.Start();
}


template<int BYTECOUNT,typename STORAGE>
bool TBitReader::ReadBytes(STORAGE& Data,int BitCount)
{
	//	gr: definitly correct
	Data = 0;
	
	BufferArray<uint8,BYTECOUNT> Bytes;
	int ComponentBitCount = BitCount;
	while ( ComponentBitCount > 0 )
	{
		if ( !Read( Bytes.PushBack(), ofMin(8,ComponentBitCount) ) )
			return false;
		ComponentBitCount -= 8;
	}
		
	Data = 0;
	for ( int i=0;	i<Bytes.GetSize();	i++ )
	{
		int Shift = (i * 8);
		Data |= static_cast<STORAGE>(Bytes[i]) << Shift;
	}

	STORAGE DataBackwardTest = 0;
	for ( int i=0;	i<Bytes.GetSize();	i++ )
	{
		auto Shift = (Bytes.GetSize()-1-i) * 8;
		DataBackwardTest |= static_cast<STORAGE>(Bytes[i]) << Shift;
	}

	//	turns out THIS is the right way
	Data = DataBackwardTest;

	return true;
}

bool TBitReader::Read(int& Data,int BitCount)
{
	//	break up data
	if ( BitCount <= 8 )
	{
		uint8 Data8;
		if ( !Read( Data8, BitCount ) )
			return false;
		Data = Data8;
		return true;
	}
	
	if ( BitCount <= 8 )
		return ReadBytes<1>( Data, BitCount );

	if ( BitCount <= 16 )
		return ReadBytes<2>( Data, BitCount );

	if ( BitCount <= 32 )
		return ReadBytes<4>( Data, BitCount );

	//	todo:
	assert(false);
	return false;
}


bool TBitReader::Read(uint64& Data,int BitCount)
{
	if ( BitCount <= 8 )
		return ReadBytes<1>( Data, BitCount );

	if ( BitCount <= 16 )
		return ReadBytes<2>( Data, BitCount );

	if ( BitCount <= 32 )
		return ReadBytes<4>( Data, BitCount );

	if ( BitCount <= 64 )
		return ReadBytes<8>( Data, BitCount );

	assert(false);
	return false;
}

bool TBitReader::Read(uint8& Data,int BitCount)
{
	if ( BitCount <= 0 )
		return false;
	assert( BitCount <= 8 );

	//	current byte
	int CurrentByte = mBitPos / 8;
	int CurrentBit = mBitPos % 8;

	//	out of range 
	if ( CurrentByte >= mData.GetSize() )
		return false;

	//	move along
	mBitPos += BitCount;

	//	get byte
	Data = mData[CurrentByte];

	//	pick out certain bits
	//	gr: reverse endianess to what I thought...
	//Data >>= CurrentBit;
	Data >>= 8-CurrentBit-BitCount;
	Data &= (1<<BitCount)-1;

	return true;
}

void TBitWriter::Write(uint8 Data,int BitCount)
{
	//	gr: definitly correct
	assert( BitCount <= 8 && BitCount >= 0 );
	for ( int i=BitCount-1;	i>=0;	i-- )
	{
		int BitIndex = i;
		int Bit = Data & (1<<BitIndex);
		WriteBit( Bit ? 1 : 0 );
	}
}

template<int BYTES,typename STORAGE>
void TBitWriter::WriteBytes(STORAGE Data,int BitCount)
{
	//	gr: may not be correct
	BufferArray<uint8,BYTES> Bytes;
	for ( int i=0;	i<Bytes.MaxSize();	i++ )
	{
		int Shift = i*8;
		Bytes.PushBack( (Data>>Shift) & 0xff );
	}
	int BytesInUse = (BitCount+7) / 8;
	assert( Bytes.GetSize() >= BytesInUse );
	Bytes.SetSize( BytesInUse );

	for ( int i=0;	i<Bytes.GetSize();	i++ )
	{
		int ComponentBitCount = BitCount - (i*8);
		if ( ComponentBitCount <= 0 )
			continue;
		//	write in reverse
		auto ByteIndex = Bytes.GetSize()-1 - i;
		Write( Bytes[ByteIndex], ofMin(ComponentBitCount,8) );
	}
}

void TBitWriter::Write(uint16 Data,int BitCount)
{
	WriteBytes<2>( Data, BitCount );
}

void TBitWriter::Write(uint32 Data,int BitCount)
{
	WriteBytes<4>( Data, BitCount );
}

void TBitWriter::Write(uint64 Data,int BitCount)
{
	WriteBytes<8>( Data, BitCount );
}


void TBitWriter::WriteBit(int Bit)
{
	//	current byte
	int CurrentByte = mBitPos / 8;
	int CurrentBit = mBitPos % 8;

	if ( CurrentByte >= mData.GetSize() )
		mData.PushBack(0);

	mBitPos++;

	int WriteBit = 7-CurrentBit;
	unsigned char AddBit = Bit ? (1<<WriteBit) : 0;
	
	//	bit already set??
	if ( mData[CurrentByte] & AddBit )
	{
		assert( false );
	}

	mData[CurrentByte] |= AddBit;
}
