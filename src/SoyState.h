#pragma once


template<class TPARENT>
class TState
{
public:
	TState(TPARENT& Parent) :	
		mParent	( Parent )
	{
	}
	virtual ~TState()			{}

	virtual void	Update(float TimeStep)		{};	//	update
	virtual void	Render(float TimeStep)		{};	//	render
	virtual bool	Exit()						{	return true;	}	//	keep exiting until true (finished)

protected:
	TPARENT&		GetParent()					{	return mParent;	}

private:
	TPARENT&		mParent;
};


template<class TPARENT>
class TStateManager
{
public:
	TStateManager() :
		mState	( NULL )
	{
	}
	virtual ~TStateManager()	{}

	void						UpdateStates(float TimeStep);
	void						RenderStates(float TimeStep);

	template<class TSTATE>
	void						ChangeState()					{	ChangeState( *new TSTATE( GetStateParent() ) );	}
	template<class TSTATE,typename ARG1>
	void						ChangeState(const ARG1& Arg1)	{	ChangeState( *new TSTATE( GetStateParent(), Arg1 ) );	}

protected:
	virtual TPARENT&			GetStateParent()=0;

private:
	void						ChangeState(TState<TPARENT>& NewState);

public:
	ofEvent<TPARENT&>			mOnStateChanged;	
	TState<TPARENT>*			mState;
	Array<TState<TPARENT>*>		mExitingState;		//	currently destructing states
};






#include "SoyState.hpp"
