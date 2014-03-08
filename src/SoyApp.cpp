#include "SoyApp.h"
#include "ofxXmlSettings.h"     


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

