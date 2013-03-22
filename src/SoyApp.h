#pragma once

#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"
//#include "..\..\..\addons\ofxiPhone\src\ofxiPhone.h"
//ofxiPhoneApp



inline BufferString<MAX_PATH> SoyGetFileExt(const char* Filename)
{
	string ExtStr = ofFilePath::getFileExt( Filename );
	BufferString<MAX_PATH> Ext = ExtStr.c_str();
	Ext.ToLower();
	return Ext;
}

namespace Soy
{
	inline	BufferString<20>	FormatSizeBytes(uint64 bytes);
}

inline BufferString<20> Soy::FormatSizeBytes(uint64 bytes)
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

//------------------------------------
//	lock which still applies when single threaded
//------------------------------------
class SoyLock
{
public:
	SoyLock() :
		mIsLocked	( false )
	{
	}

	bool		IsLocked() const	{	return mIsLocked;	}
	ofMutex&	GetMutex()			{	return mLock;	}

	void	Lock()		
	{
		assert( !IsLocked() );
		mLock.lock();
		mIsLocked = true;
	}
	
	bool	TryLock()		
	{
		if ( IsLocked() )
			return false;
		if ( !mLock.tryLock() )
			return false;
		assert( !IsLocked() );
		mIsLocked = true;
		return true;
	}
	
	void	Unlock()		
	{
		assert( IsLocked() );
		mLock.unlock();
		mIsLocked = false;
	}
	
private:
	ofMutex	mLock;
	bool	mIsLocked;
};



template<class OBJECT>
class ofMutexT : public OBJECT, public ofMutex
{
public:
	ofMutexT()
	{
	}
	template<typename PARAM>
	explicit ofMutexT(const PARAM& Param) :
		OBJECT	( Param )
	{
	}

	OBJECT&			Get()		{	return *this;	}
	const OBJECT&	Get() const	{	return *this;	}
};


//	flow-
//	mouse down - start gesture
//	app pops new gesture
//	mouse move - accumulate to gesture (ie, make path)
//	app pops gesture movement (gesture re-starts from end pos but no longer new)
//	mouse move - update gesture
//	mouse up - end of gesture
//	app pops gesture
//	app pops end of gesture


//	list of supported touch/mouse buttons
namespace SoyButton
{
	enum Type
	{
		Left = 0,
		Middle = 1,
		Right = 2,
		Four = 3,	//	touch finger #4
		Five = 4,	//	touch finger #5	
	};

	inline Type		FromMouseButton(int ButtonIndex)		{	return static_cast<Type>(ButtonIndex);	}
	inline int		ToMouseButton(Type Button)				{	return static_cast<int>(Button);	}
	inline void		GetArray(ArrayBridge<SoyButton::Type>& Array)
	{
		Array.PushBack( Left );
		Array.PushBack( Middle );
		Array.PushBack( Right );
		Array.PushBack( Four );
		Array.PushBack( Five );
	};
};



//	a gesture is a drag
class SoyGesture
{
public:
	SoyGesture(SoyButton::Type Button) :
		mButton		( Button ),
		mValid		( true ),
		mFirstDown	( false )
	{
	}
	SoyGesture() :
		mValid		( false ),
		mFirstDown	( false )
	{
	}

	SoyButton::Type			GetButton() const	{	return mButton;	}
	bool					IsValid() const		{	return mValid;	}
	bool					IsDragging() const	{	return mPath.GetSize() > 1;	}
	bool					IsFirstDown() const	{	return mFirstDown;	}
	bool					IsIdle() const		{	return mPath.GetSize() == 1;	}
	bool					IsDown() const		{	return !IsUp();	}
	bool					IsUp() const		{	return mPath.IsEmpty();	}

	inline bool				operator==(const SoyButton::Type& Button) const	{	return GetButton() == Button;	}

public:
	Array<vec2f>			mPath;		//	path since last pop, if one point then button is down, but not moving. If none then the mouse is released
	bool					mFirstDown;

private:
	SoyButton::Type			mButton;
	bool					mValid;
};


class SoyInput
{
public:
	SoyInput()
	{
	}

	//	input - turn into gestures
	void				OnMouseDown(const vec2f& Pos2,SoyButton::Type Button);
	void				OnMouseMove(const vec2f& Pos2,SoyButton::Type Button);
	void				OnMouseUp(const vec2f& Pos2,SoyButton::Type Button);

	//	output
	void				PopLock();
	bool				PopTryLock();
	SoyGesture			PopGesture();
	void				PopUnlock();
	void				CullGestures();		//	for when another UI is active and using inputs... obviously it would be better to manually distribute input to the other UI
	bool				PeekButtonDown(SoyButton::Type Button);	//	check if a button is currently down in the gesture queue. Used to get around lost-mouse-up issues

protected:
	void				PushGesture(const SoyGesture& NewGesture);
	SoyGesture*			GetLastGesture(SoyButton::Type Button);		//	get last gesture for this button

protected:
	SoyLock				mPopLock;
	Array<SoyGesture>	mGestures;			//	active gestures (ready to be popped)
	Array<SoyGesture>	mPendingGestures;	//	gestures accumulated whilst pop-locking
};


//--------------------------------------------
//	cross platform handling app
//--------------------------------------------
class SoyApp : public ofBaseApp
{
public:
	SoyApp();

	virtual void mouseDragged( int x, int y, int button );
	virtual void mousePressed( int x, int y, int button );
	virtual void mouseReleased(int x, int y, int button );
	virtual void mouseReleased();
	virtual void mouseMoved( int x, int y );

public:
	SoyInput	mInput;		//	cross platform gesture interface
};

