#pragma once


//--------------------------------------------
//	a string<->int identifier. This is NOT packet (not like tootle's TRef)
//--------------------------------------------
class SoyRef
{
public:
	static const size_t MaxStringLength = sizeof(uint64) / sizeof(char);

public:
	SoyRef() :
		mRef	( 0 )
	{
	}
	explicit SoyRef(const char* Name) :
		mRef	( FromString( Name ) )
	{
	}
	explicit SoyRef(const uint64 Int64) :
		mRef	( Int64 )
	{
	}
	explicit SoyRef(const std::string& Name) :
		mRef	( FromString(Name) )
	{
	}

	bool			IsValid() const							{	return (*this) != SoyRef();	}
	std::string		ToString() const;
	void			Increment();
	void			Increment(int IncCount)					{	assert( IncCount >= 0 );	while ( IncCount-- > 0 )	Increment();	}
	uint64			GetInt64() const						{	return mRef;	}
	int				GetDebugInt32() const					{	return mRef32[1];	}	//	second half changes the most
	SoyRef&			operator++()							{	Increment();	return *this;	}	//	++prefix
	SoyRef			operator++(int)							{	SoyRef Copy( *this );	this->Increment();	return Copy;	}	//	postfix++
	inline bool		operator==(const SoyRef& That) const	{	return mRef == That.mRef;	}
	inline bool		operator!=(const SoyRef& That) const	{	return mRef != That.mRef;	}
	inline bool		operator<(const SoyRef& That) const		{	return mRef < That.mRef;	}
	inline bool		operator>(const SoyRef& That) const		{	return mRef > That.mRef;	}

private:
	static uint64	FromString(const std::string& String);

public:
	struct
	{
		union
		{
			uint64	mRef;
			uint32	mRef32[2];
			char	mRefChars[SoyRef::MaxStringLength];
		};
	};
};

inline std::string& operator<<(std::string& str,const SoyRef& Value)
{
	str += std::string( Value.ToString().c_str() );
	return str;
}

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyRef& Value)
{
	str << Value.ToString();
	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,SoyRef& Value)
{
	Value = SoyRef( str );
	return str;
}

