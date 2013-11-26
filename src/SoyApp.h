#pragma once

#include "ofxSoylent.h"
//#include "..\..\..\addons\ofxiPhone\src\ofxiPhone.h"
//ofxiPhoneApp

class ofxXmlSettings;

namespace Soy
{
	template<typename TYPE>
	inline bool		ReadXmlData(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag=true);
	
	template<typename TYPE>
	inline void		WriteXmlData(ofxXmlSettings& xml,const char* Name,const TYPE& Value,bool Tag=true);
};

inline BufferString<MAX_PATH> SoyGetFileExt(const char* Filename)
{
	std::string ExtStr = ofFilePath::getFileExt( Filename );
	BufferString<MAX_PATH> Ext = ExtStr.c_str();
	Ext.ToLower();
	return Ext;
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
	
	vec2f					GetScreenPos() const		{	return IsUp() ? vec2f() : mPath.GetBack();	}
	vec2f					GetPrevScreenPos() const	{	return (mPath.GetSize() > 1) ? mPath[mPath.GetSize()-2] : GetScreenPos();	}

public:
	Array<vec2f>			mPath;		//	path since last pop, if one point then button is down, but not moving. If none then the mouse is released
	bool					mFirstDown;

private:
	SoyButton::Type			mButton;
	bool					mValid;
};
DECLARE_TYPE_NAME( SoyGesture );

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
/*
class ofxXmlSettings_Protected : public ofxXmlSettings
{
public:
	const char*	GetPushedTag()	{	ofxXmlSettings::storedHandle.
	*/
//	if not tag, then data is stored as an attribute
template<typename TYPE>
inline void Soy::WriteXmlData(ofxXmlSettings& xml,const char* Name,const TYPE& Value,bool Tag)
{
	TString Buffer;
	Buffer << Value;

	if ( Tag )
		xml.addValue( Name, static_cast<const char*>( Buffer ) );
	else
		xml.addAttribute( ":", Name, static_cast<const char*>( Buffer ), -1 );

}


template<uint32 SIZE>
inline const TString& operator>>(const TString& Source,BufferString<SIZE>& Destination)
{
	Destination << Source;
	return Source;
}

//	if not tag, then data is stored as an attribute
template<typename TYPE>
inline bool Soy::ReadXmlData(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag)
{
	string data;
	if ( Tag )
		data = xml.getValue( Name, data );
	else
		data = xml.getAttribute( ":", Name, data );

	if ( data.empty() )
		return false;

	//	convert
	TString Buffer = data.c_str();
	Buffer >> Value;
	return true;
}



template<typename OBJECT>
class TXmlArrayMeta
{
public:
	TXmlArrayMeta(const char* TagName,const Array<OBJECT>& Objects) :
		mTagName	( TagName ),
		mObjects	( Objects )
	{
	}

	//	todo: find a way to do this without having a const and non-const TXmlArrayMeta
	const Array<OBJECT>&	GetObjectsConst() const		{	return mObjects;	}
	Array<OBJECT>&			GetObjectsMutable() const	{	return const_cast<Array<OBJECT>&>( mObjects );	}

public:
	const char*				mTagName;

private:
	const Array<OBJECT>&	mObjects;
};

template<typename OBJECT>
inline ofxXmlSettings& operator<<(ofxXmlSettings& xml,const TXmlArrayMeta<OBJECT>& Array)
{
	auto& Objects = Array.GetObjectsConst();
	for ( int i=0;	i<Objects.GetSize();	i++ )
	{
		xml.addTag( Array.mTagName );
		if ( !xml.pushTag( Array.mTagName, i ) )
			continue;
		auto& Object = Objects[i];
		xml << Object;
		xml.popTag();
	}
	return xml;
}

template<typename OBJECT>
inline ofxXmlSettings& operator>>(ofxXmlSettings& xml,const TXmlArrayMeta<OBJECT>& Array)
{
	auto& Objects = Array.GetObjectsMutable();
	int TagCount = xml.getNumTags( Array.mTagName );
	for ( int i=0;	i<TagCount;	i++ )
	{
		if ( !xml.pushTag( Array.mTagName, i ) )
			continue;
		auto& NewObject = Objects.PushBack();
		xml >> NewObject;
		xml.popTag();
	}
	return xml;
}


template<class ARRAYTYPE>
inline bool ofFilenameStripPath(Soy::String2<char,ARRAYTYPE>& Filename)
{
	//	strip path
	int LastSlash = ofMax( Filename.GetLastChar('/'), Filename.GetLastChar('\\') );
	if ( LastSlash < 0 )
		return false;

	Filename.RemoveAt( 0, LastSlash+1 );
	return true;
}