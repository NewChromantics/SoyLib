#include "SoyApp.h"
#if defined(OPENFRAMEWORKS)
#include "ofxXmlSettings.h"
#endif


bool Soy::Windows::TConsoleApp::gIsRunning = false;


BOOL WINAPI Soy::Windows::TConsoleApp::ConsoleHandler(DWORD dwType)
{
	switch(dwType) 
	{
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			gIsRunning = false;

			//Returning would make the process exit!
			//We just make the handler sleep until the main thread exits,
			//or until the maximum execution time for this handler is reached.
			Sleep(10000);
		return TRUE;

	default:
		return FALSE;
	}
}


int Soy::Windows::TConsoleApp::RunLoop()
{
	assert( !gIsRunning );
	gIsRunning = true;
	SetConsoleCtrlHandler( ConsoleHandler, true );
	if ( !mApp.Init() )
		return 1;
	while ( gIsRunning )
	{
		if ( !mApp.Update() )
			break;
	}
	mApp.Exit();
	return 0;
}


#if defined(OPENFRAMEWORKS)

SoyApp::SoyApp()
{
	//	best timing setup
	ofSetVerticalSync( false );
	ofSetFrameRate( 60 );
	
	//	various stuff we should setup
	ofSeedRandom();
}

void SoyApp::mouseDragged( int x, int y, int button )
{
	mInput.OnMouseMove( vec2f( x,y ), SoyButton::FromMouseButton( button ) );
}


void SoyApp::mousePressed( int x, int y, int button )
{
	mInput.OnMouseDown( vec2f( x,y ), SoyButton::FromMouseButton( button ) );
}


void SoyApp::mouseReleased(int x, int y, int button )
{
	mInput.OnMouseUp( vec2f( x,y ), SoyButton::FromMouseButton( button ) );
}

	
void SoyApp::mouseReleased()
{
	//	check for lost-focus-mouse-ups
	BufferArray<SoyButton::Type,10> Buttons;
	SoyButton::GetArray( GetArrayBridge(Buttons) );

	for ( int i=0;	i<Buttons.GetSize();	i++ )
	{
		SoyButton::Type Button = Buttons[i];
		int ButtonIndex = SoyButton::ToMouseButton( Button );
		if ( ofGetMousePressed( ButtonIndex ) )
			continue;
		
		//	button is not currently down
		if ( !mInput.PeekButtonDown( Button ) )
			continue;
		
		//	push a mouse-up
		//	gr: we could grab pos from last gesture, but pos is unused in an up anyway
		mouseReleased( -1, -1, ButtonIndex );
	}
}

void SoyApp::mouseMoved( int x, int y )
{
	//	check for lost-focus-mouse-ups
	mouseReleased();
}




void SoyInput::OnMouseDown(const vec2f& Pos2,SoyButton::Type Button)
{
	//	if we have a previous gesture for this button which is NOT release, 
	//	then we've gone wrong somewhere. Lets cleanup...
	SoyGesture* pLastGesture = GetLastGesture( Button );
	if ( pLastGesture )
	{
		//	button is considered down, so lets fake a release to cleanup...
		if ( !pLastGesture->IsUp() )
		{
			//	gr: should the last gesture store the last-known place so we release where the mouse last was, instead of here...
			//	gr: openframeworks doesn't register a mouse up when we lose focus (eg. breakpoint). SIGH.
			//assert( pLastGesture->IsUp() );
			OnMouseUp( Pos2, Button );
		}
		pLastGesture = NULL;
	}

	//	create new gesture
	SoyGesture NewGesture( Button );
	NewGesture.mPath.PushBack( Pos2 );
	NewGesture.mFirstDown = true;
	PushGesture( NewGesture );
}


void SoyInput::OnMouseMove(const vec2f& Pos2,SoyButton::Type Button)
{
	//	update path on existing gesture...
	SoyGesture* pLastGesture = GetLastGesture( Button );

	//	if the last gesture was a release, then pretend there isn't a previous one
	if ( pLastGesture && pLastGesture->IsUp() )
		pLastGesture = NULL;

	//	new gesture... bit unexpected... but lets correct it
	if ( !pLastGesture )
	{
		//	double-click to maximise causes this assert...
		//assert( pLastGesture );
		OnMouseDown( Pos2, Button );
		return;
	}

	//	merge with last gesture
	pLastGesture->mPath.PushBack( Pos2 );
}


void SoyInput::OnMouseUp(const vec2f& Pos2,SoyButton::Type Button)
{
	SoyGesture* pLastGesture = GetLastGesture( Button );

	//	check this release isn't (wrongly) redundant
	if ( pLastGesture && pLastGesture->IsUp() )
	{
		assert( false );
		return;
	}

	//	create a release gesture
	SoyGesture Gesture( Button );
	assert( Gesture.IsUp() );
	PushGesture( Gesture );
}


//	todo: if the host requires the state of other buttons at the time of this pop, 
//		we should enumerate the queue up to the next UP's per-button and before 
//		this button is released. That SHOULD tell us the down-state of all other buttons at the time of pop
SoyGesture SoyInput::PopGesture()
{
	//ofMutex::ScopedLock Lock( mPopLock );

	if ( mGestures.IsEmpty() )
		return SoyGesture();

	//	get next gesture in the queue
	SoyGesture Result = mGestures[0];
	mGestures.RemoveBlock(0,1);

	//	simple pop if it's the end of a gesture
	if ( Result.IsUp() )
	{
		return Result;
	}

	//	now we modify the queue to keep this gesture active...
	//	first look for this button being released and therefore the end of this gesture
	SoyGesture* pNextGesture = mGestures.Find( Result.GetButton() );
	
	//	no next gesture, therefore we need to keep this gesture, but make it an idle one.
	//	place at the BACK so other buttons can get through
	if ( !pNextGesture )
	{
		SoyGesture ContinuedIdleGesture( Result.GetButton() );
		ContinuedIdleGesture.mPath.PushBack( Result.mPath.GetBack() );
		assert( !ContinuedIdleGesture.IsFirstDown() );
		assert( ContinuedIdleGesture.IsIdle() );
		PushGesture( ContinuedIdleGesture );
		return Result;
	}

	//	if this gesture is followed by a release, then we just let this one get popped
	if ( pNextGesture && pNextGesture->IsUp() )
		return Result;

	//	if the next gesture is dragging then really it should have already been merged...
	//	if it's idle, then er... seems it might be a bit redundant?
	assert(false);
	return Result;
}

SoyGesture* SoyInput::GetLastGesture(SoyButton::Type Button)
{
	for ( int i=mGestures.GetSize()-1;	i>=0;	i-- )
	{
		auto& Gesture = mGestures[i];
		if ( Gesture.GetButton() == Button )
			return &Gesture;
	}

	//	button is unused
	return NULL;
}


void SoyInput::PopLock()
{
	mPopLock.Lock();
}

bool SoyInput::PopTryLock()
{
	return mPopLock.TryLock();
}

void SoyInput::PopUnlock()
{
	mPopLock.Unlock();

	//	move pending gestures onto the gesture list
	mGestures.PushBackArray( mPendingGestures );
	mPendingGestures.Clear();
}

void SoyInput::PushGesture(const SoyGesture& Gesture)
{
	//	if locked, put it in the pending list
	if ( PopTryLock() )
	{
		SoyGesture& NewGesture = mGestures.PushBack( Gesture );
		PopUnlock();	//	can invalidate NewGesture
		return;
	}
	else
	{
		mPendingGestures.PushBack( Gesture );
	}
}

void SoyInput::CullGestures()
{
	if ( !PopTryLock() )
		return;

	while ( true )
	{
		SoyGesture Gesture = PopGesture();
		if ( !Gesture.IsValid() )
			break;
	}

	PopUnlock();
}


bool SoyInput::PeekButtonDown(SoyButton::Type Button)
{
	PopLock();

	//	button is not currently registered as being in a state
	SoyGesture* pLastGesture = GetLastGesture( Button );
	if ( !pLastGesture )
	{
		PopUnlock();
		return false;
	}
	
	//	last gesture is a release
	if ( pLastGesture->IsUp() )
	{
		PopUnlock();
		return false;
	}

	//	last gesture must be down
	assert( pLastGesture->IsDown() );
	PopUnlock();
	return true;
}



bool ofFileToString(TString& ContentString,const char* Filename)
{
	//	fopen is safe, but supress warning anyway
#if defined(TARGET_WINDOWS)
	FILE* File = nullptr;
	fopen_s( &File, Filename, "r" );
#else
	FILE* File = fopen( Filename, "r" );
#endif
	if ( !File )
		return false;

	//	read
	Array<char> Contents;
	while ( true )
	{
		BufferArray<char,1024> Buffer;
		Buffer.SetSize( Buffer.MaxSize() );
#if defined(TARGET_WINDOWS)
		auto CharsRead = fread_s( Buffer.GetArray(), Buffer.GetDataSize(), Buffer.GetElementSize(), Buffer.GetSize(), File );
#else
		auto CharsRead = std::fread( Buffer.GetArray(), Buffer.GetElementSize(), Buffer.GetSize(), File );
#endif
		Buffer.SetSize( CharsRead );

		Contents.PushBackArray( Buffer );

		//	EOF
		if ( Buffer.GetSize() != Buffer.MaxSize() )
			break;
	}
	fclose( File );

	//	add a terminator as there probably won't be one from the content reading
	Contents.PushBack('\0');
	ContentString.Reserve( Contents.GetSize() + 1 );
	ContentString = Contents.GetArray();

	return true;
}


bool ofStringToFile(const char* Filename,const TString& ContentString)
{
	//	fopen is safe, but supress warning anyway
#if defined(TARGET_WINDOWS)
	FILE* File = nullptr;
	fopen_s( &File, Filename, "w" );
#else
	FILE* File = fopen( Filename, "w" );
#endif
	if ( !File )
		return false;

	bool Success = true;
	int Written = 0;
	while ( Success && Written < ContentString.GetLength() )
	{
		int WriteElements = ContentString.GetLength() - Written;
#if defined(TARGET_WINDOWS)
		auto CharsWritten = fwrite( &ContentString[Written], ContentString.GetArray().GetElementSize(), WriteElements, File );
#else
#error todo
		//auto CharsWritten = std::fread( Buffer.GetArray(), Buffer.GetElementSize(), Buffer.GetSize(), File );
#endif
		if ( CharsWritten != WriteElements )
			Success = false;

		Written += CharsWritten;
	}
	fclose( File );

	return true;
}





template<> bool Soy::ReadXmlData<vec3f>(ofxXmlSettings& xml,const char* Name,vec3f& Value,bool Tag)
{
	return ReadXmlDataAsParameter( xml, Name, Value, Tag );
}

template<> void Soy::WriteXmlData<vec3f>(ofxXmlSettings& xml,const char* Name,const vec3f& Value,bool Tag)
{
	return WriteXmlDataAsParameter( xml, Name, Value, Tag );
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
		int Shift = (Bytes.GetSize()-1-i) * 8;
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
		int ByteIndex = Bytes.GetSize()-1 - i;
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

#endif
