#pragma once



//--------------------------------------------
//	identifier
//--------------------------------------------
class SoyRef
{
public:
	SoyRef() :
		mRef	( 0 )
	{
	}
	explicit SoyRef(uint32 Ref) :
		mRef	( Ref )
	{
	}

	bool		IsValid() const							{	return (*this) != SoyRef();	}
	inline bool	operator==(const SoyRef& That) const	{	return mRef == That.mRef;	}
	inline bool	operator!=(const SoyRef& That) const	{	return mRef != That.mRef;	}

public:
	uint32		mRef;
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyRef& Value)
{
	str << "R." << Value.mRef;
	return str;
}

