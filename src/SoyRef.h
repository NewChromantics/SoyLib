#pragma once

namespace Private
{
	static const int SoyRef_MaxStringLength = sizeof(uint64)/sizeof(char);
}
typedef BufferString<Private::SoyRef_MaxStringLength+1> SoyRefString;

//--------------------------------------------
//	a string<->int identifier. This is NOT packet (not like tootle's TRef)
//--------------------------------------------
class SoyRef
{
public:
	static const int MaxStringLength = Private::SoyRef_MaxStringLength;

public:
	SoyRef() :
		mRef	( 0 )
	{
	}
	explicit SoyRef(const char* Name) :
		mRef	( FromString( Name ) )
	{
	}

	bool			IsValid() const							{	return (*this) != SoyRef();	}
	SoyRefString	ToString() const;
	void			Increment();
	SoyRef&			operator++()							{	Increment();	return *this;	}
	inline bool		operator==(const SoyRef& That) const	{	return mRef == That.mRef;	}
	inline bool		operator!=(const SoyRef& That) const	{	return mRef != That.mRef;	}

private:
	static uint64	FromString(const SoyRefString& String);

public:
	uint64			mRef;
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyRef& Value)
{
	str << Value.ToString();
	return str;
}

