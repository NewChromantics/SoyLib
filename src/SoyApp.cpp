#include "SoyApp.h"



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
	auto& NewGesture = PushGesture( Button );
	NewGesture.mPath.PushBack( Pos2 );
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
		assert( pLastGesture );
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
	auto& Gesture = PushGesture( Button );
	assert( Gesture.IsUp() );
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
		SoyGesture& ContinuedIdleGesture = PushGesture( Result.GetButton() );
		ContinuedIdleGesture.mPath.PushBack( Result.mPath.GetBack() );
		assert( ContinuedIdleGesture.IsIdle() );
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

SoyGesture& SoyInput::PushGesture(const SoyGesture& Gesture)
{
	//	if locked, put it in the pending list
	if ( PopTryLock() )
	{
		SoyGesture& NewGesture = mGestures.PushBack( Gesture );
		PopUnlock();
		return NewGesture;
	}
	else
	{
		return mPendingGestures.PushBack( Gesture );
	}
}

