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

	bool		IsValid() const	{	return mRef != 0;	}

public:
	uint32		mRef;
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyRef& Value)
{
	str << "R." << Value.mRef;
	return str;
}

