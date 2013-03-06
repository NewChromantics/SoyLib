#include "ofxSoylent.h"


	


template<class TPARENT>
void Soy::TStateManager<TPARENT>::UpdateStates(float TimeStep)
{
	//	exit old states
	for ( int i=mExitingState.GetSize()-1;	i>=0;	i-- )
	{
		auto* pState = mExitingState[i];
		if ( !pState->Exit() )
			continue;

		delete pState;
		mExitingState.RemoveBlock( i, 1 );
	}

	//	update current state
	if ( mState )
	{
		mState->Update( TimeStep );
	}

}

template<class TPARENT>
void Soy::TStateManager<TPARENT>::RenderStates(float TimeStep)
{
	//	render old states
	for ( int i=mExitingState.GetSize()-1;	i>=0;	i-- )
	{
		auto* pState = mExitingState[i];
		TRenderSceneScope Scene(__FUNCTION__);
		pState->Render( TimeStep );
	}

	//	update
	if ( mState )
	{
		TRenderSceneScope Scene(__FUNCTION__);
		mState->Render( TimeStep );
	}
}


template<class TPARENT>
void Soy::TStateManager<TPARENT>::ChangeState(TState<TPARENT>& NewState)
{
	//	exit the current state
	if ( mState )
	{
		mExitingState.PushBackUnique( mState );
		mState = NULL;
	}

	mState = &NewState;
	ofNotifyEvent( mOnStateChanged, GetStateParent() );
}
