#pragma once


#include "String.hpp"


class SoyTime
{
public:
	explicit SoyTime(bool InitToNow=false) : 
		mTime	( InitToNow ? Now().GetTime() : 0 )
	{
	}
	explicit SoyTime(uint64 Time) :
		mTime	( Time )
	{
	}
	SoyTime(const SoyTime& Time) :
		mTime	( Time.GetTime() )
	{
	}
	template <typename S,class ARRAYTYPE>
	explicit SoyTime(const Soy::String2<S,ARRAYTYPE>& String) :
		mTime	( 0 )
	{
		FromString( std::string( String ) );
	}

	bool			FromString(std::string String);
	std::string		ToString() const;

	uint64			GetTime() const							{	return mTime;	}
	bool			IsValid() const							{	return mTime!=0;	}
	static SoyTime	Now()									{	return SoyTime( ofGetElapsedTimeMillis()+1 );	}	//	we +1 so we never have zero for a "real" time

	inline bool		operator==(const SoyTime& Time) const	{	return mTime == Time.mTime;	}
	inline bool		operator!=(const SoyTime& Time) const	{	return mTime != Time.mTime;	}
	inline bool		operator<(const SoyTime& Time) const	{	return mTime < Time.mTime;	}
	inline bool		operator<=(const SoyTime& Time) const	{	return mTime <= Time.mTime;	}
	inline bool		operator>(const SoyTime& Time) const	{	return mTime > Time.mTime;	}
	inline bool		operator>=(const SoyTime& Time) const	{	return mTime >= Time.mTime;	}
	inline SoyTime&	operator+=(const uint64& Step) 			{	mTime += Step;	return *this;	}
	inline SoyTime&	operator+=(const SoyTime& Step)			{	mTime += Step.GetTime();	return *this;	}
	inline SoyTime&	operator-=(const uint64& Step) 			{	mTime -= Step;	return *this;	}
	inline SoyTime&	operator-=(const SoyTime& Step)			{	mTime -= Step.GetTime();	return *this;	}

private:
	uint64	mTime;
};
DECLARE_TYPE_NAME( SoyTime );



template<class STRING>
inline STRING& operator<<(STRING& str,const SoyTime& Time)
{
	BufferString<100> Buffer;
	Buffer.PrintText("T%09Iu", Time.GetTime() );
	str << Buffer;
	return str;
}


template<class STRING>
inline const STRING& operator>>(const STRING& str,SoyTime& Time)
{
	if ( str.GetLength() != 10 )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	if ( str[0] != 'T' )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	BufferString<100> Buffer = static_cast<const char*>( &str[1] );
	int TimeValue = 0;
	if ( !Buffer.GetInteger( TimeValue ) )
	{
		assert(false);
		Time = SoyTime();
		return str;
	}

	Time = SoyTime( static_cast<uint64>( TimeValue ) );
	return str;
}

