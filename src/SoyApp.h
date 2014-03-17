#pragma once

#include "ofxSoylent.h"
//#include "..\..\..\addons\ofxiPhone\src\ofxiPhone.h"
//ofxiPhoneApp

class ofxXmlSettings;

namespace Soy
{
	template<typename TYPE>
	inline bool		ReadXmlData(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag=true);
	
	template<typename READTYPE,typename TYPE>
	inline bool		ReadXmlDataAs(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag=true);
	
	template<typename TYPE>
	inline void		WriteXmlData(ofxXmlSettings& xml,const char* Name,const TYPE& Value,bool Tag=true);

	template<typename TYPE>
	inline bool		ReadXmlParameter(ofxXmlSettings& xml,ofParameter<TYPE>& Value,bool Tag=true);
	
	template<typename TYPE>
	inline void		WriteXmlParameter(ofxXmlSettings& xml,const ofParameter<TYPE>& Value,bool Tag=true);

	template<typename TYPE>
	bool			LoadXml(const std::string& Filename,TYPE& Object,const std::string& RootTag);

	template<typename TYPE>
	bool			SaveXml(const std::string& Filename,const TYPE& Object,const std::string& RootTag);
	
	template<typename TYPE>
	bool			GetXml(ofxXmlSettings& xml,const TYPE& Object,const std::string& RootTag);

	template<typename TYPE>
	bool			GetXmlString(std::string& xml,const TYPE& Object,const std::string& RootTag);

	template<typename TYPE>
	inline bool		ReadXmlDataAsParameter(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag=true);
	
	template<typename TYPE>
	inline void		WriteXmlDataAsParameter(ofxXmlSettings& xml,const char* Name,const TYPE& Value,bool Tag=true);
	

	//	refactoring; new specialisations using ofParameter internally
	template<> bool	ReadXmlData<vec3f>(ofxXmlSettings& xml,const char* Name,vec3f& Value,bool Tag);
	template<> void	WriteXmlData<vec3f>(ofxXmlSettings& xml,const char* Name,const vec3f& Value,bool Tag);
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
	{
		auto* Element = xml.getStoredHandle().Element();
		assert( Element );
		if ( !Element )
			return;
		Element->SetAttribute( Name, Buffer.c_str() );
	}
}

template<typename TYPE>
inline void Soy::WriteXmlParameter(ofxXmlSettings& xml,const ofParameter<TYPE>& Value,bool Tag)
{
	assert( !Value.getName().empty() );
	std::string ValueString = Value.toString();

	if ( Tag )
		xml.addValue( Value.getName(), ValueString );
	else
	{
		auto* Element = xml.getStoredHandle().Element();
		assert( Element );
		if ( !Element )
			return;
		Element->SetAttribute( Value.getName(), ValueString );
	}
}


template<uint32 SIZE>
inline const TString& operator>>(const TString& Source,BufferString<SIZE>& Destination)
{
	Destination << Source;
	return Source;
}

//	if not tag, then data is stored as an attribute
template<typename READTYPE,typename TYPE>
inline bool Soy::ReadXmlDataAs(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag)
{
	READTYPE ValueAs = static_cast<READTYPE>( Value );
	bool Result = ReadXmlData( xml, Name, ValueAs, Tag );
	Value = static_cast<READTYPE>( ValueAs );
	return Result;
}


//	if not tag, then data is stored as an attribute
template<typename TYPE>
inline bool Soy::ReadXmlData(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag)
{
	string data;
	if ( Tag )
		data = xml.getValue( Name, data );
	else
	{
		auto* Element = xml.getStoredHandle().Element();
		auto* Attrib = Element ? Element->Attribute( Name ) : nullptr;
		if ( !Attrib )
			return false;
		data = Attrib;
	}

	if ( data.empty() )
		return false;

	//	convert
	TString Buffer = data.c_str();
	Buffer >> Value;
	return true;
}


//	if not tag, then data is stored as an attribute
template<typename TYPE>
inline bool Soy::ReadXmlParameter(ofxXmlSettings& xml,ofParameter<TYPE>& Value,bool Tag)
{
	assert( !Value.getName().empty() );
	string data;
	if ( Tag )
		data = xml.getValue( Value.getName(), data );
	else
	{
		auto* Element = xml.getStoredHandle().Element();
		auto* Attrib = Element ? Element->Attribute( Value.getName() ) : nullptr;
		if ( !Attrib )
			return false;
		data = *Attrib;
	}

	if ( data.empty() )
		return false;

	//	convert
	Value.fromString( data );
	return true;
}


template<typename OBJECT>
class TXmlArrayMetaMutable
{
public:
	explicit TXmlArrayMetaMutable(const char* TagName,Array<OBJECT>& Objects) :
		mTagName	( TagName ),
		mObjects	( Objects )
	{
	}

	//	todo: find a way to do this without having a const and non-const TXmlArrayMeta
	Array<OBJECT>&			GetObjectsMutable() const	{	return const_cast<Array<OBJECT>&>( mObjects );	}

public:
	const char*				mTagName;

private:
	Array<OBJECT>&			mObjects;
};


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
inline ofxXmlSettings& operator>>(ofxXmlSettings& xml,TXmlArrayMetaMutable<OBJECT>& Array)
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


bool ofFileToString(TString& ContentString,const char* Filename);
bool ofStringToFile(const char* Filename,const TString& ContentString);


template<typename TYPE>
inline bool Soy::LoadXml(const std::string& Filename,TYPE& Object,const std::string& RootTag)
{
	ofxXmlSettings xml;
	if ( !xml.loadFile( Filename ) )
		return false;

	//	check root tag
	if ( !xml.pushTag( RootTag ) )
		return false;

	//	import xml
	xml >> Object;
	xml.popTag();
	return true;
}

template<typename TYPE>
inline bool Soy::SaveXml(const std::string& Filename,const TYPE& Object,const std::string& RootTag)
{
	ofxXmlSettings xml;
	if ( !GetXml( xml, Object, RootTag ) )
		return false;

	//	save 
	if ( !xml.saveFile( Filename ) )
		return false;

	return true;
}

template<typename TYPE>
inline bool Soy::GetXml(ofxXmlSettings& xml,const TYPE& Object,const std::string& RootTag)
{
	//	clear xml here?
	if ( !xml.pushTag( RootTag, xml.addTag( RootTag ) ) )
		return false;

	//	export xml data
	xml << Object;
	xml.popTag();
	return true;
}

template<typename TYPE>
inline bool Soy::GetXmlString(std::string& xmlString,const TYPE& Object,const std::string& RootTag)
{
	ofxXmlSettings xml;
	if ( !GetXml( xml, Object, RootTag ) )
		return false;

	xmlString.clear();
	xml.copyXmlToString( xmlString );
	return true;
}

template<typename TYPE>
inline bool Soy::ReadXmlDataAsParameter(ofxXmlSettings& xml,const char* Name,TYPE& Value,bool Tag)
{
	ofParameter<TYPE> Param( Name, Value );
	if ( !ReadXmlParameter( xml, Param, Tag ) )
		return false;
	Value = Param.get();
	return true;
}
	
template<typename TYPE>
inline void Soy::WriteXmlDataAsParameter(ofxXmlSettings& xml,const char* Name,const TYPE& Value,bool Tag)
{
	ofParameter<TYPE> Param( Name, Value );
	return WriteXmlParameter( xml, Param, Tag );
}




class TBitReader
{
public:
	TBitReader(const ArrayBridge<char>& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	
	bool						Read(int& Data,int BitCount);
	bool						Read(uint64& Data,int BitCount);
	bool						Read(uint8& Data,int BitCount);
	int							Read(int BitCount)					{	int Data;	return Read( Data, BitCount) ? Data : -1;	}
	unsigned int				BitPosition() const					{	return mBitPos;	}

	template<int BYTES,typename STORAGE>
	bool						ReadBytes(STORAGE& Data,int BitCount);

private:
	const ArrayBridge<char>&	mData;
	unsigned int				mBitPos;	//	current bit-to-read/write-pos (the tail)
};

class TBitWriter
{
public:
	TBitWriter(ArrayBridge<char>& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	
	unsigned int				BitPosition() const					{	return mBitPos;	}
	
	void						Write(uint8 Data,int BitCount);
	void						Write(uint16 Data,int BitCount);
	void						Write(uint32 Data,int BitCount);
	void						Write(uint64 Data,int BitCount);
	void						WriteBit(int Bit);

	template<int BYTES,typename STORAGE>
	void						WriteBytes(STORAGE Data,int BitCount);

private:
	ArrayBridge<char>&	mData;
	unsigned int		mBitPos;	//	current bit-to-read/write-pos (the tail)
};