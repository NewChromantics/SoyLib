#pragma once


#include "string.hpp"


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
		//	format T012345678
		//	check length
		if ( String.GetLength() == 9+1 )
		{
			if ( String[0] == 'T' )
			{
				//	get int from the numbers
				BufferString<10> IntString( &String[1] );
				int32 Value;
				if ( IntString.GetInteger( Value ) )
					mTime = Value;
			}
		}
	}


	uint64			GetTime() const							{	return mTime;	}
	bool			IsValid() const							{	return mTime!=0;	}
	static SoyTime	Now()									{	return SoyTime( ofGetElapsedTimeMillis() );	}

	inline bool		operator==(const SoyTime& Time) const	{	return mTime == Time.mTime;	}
	inline bool		operator!=(const SoyTime& Time) const	{	return mTime != Time.mTime;	}
	inline bool		operator<(const SoyTime& Time) const	{	return mTime < Time.mTime;	}
	inline bool		operator<=(const SoyTime& Time) const	{	return mTime <= Time.mTime;	}
	inline bool		operator>(const SoyTime& Time) const	{	return mTime > Time.mTime;	}
	inline bool		operator>=(const SoyTime& Time) const	{	return mTime >= Time.mTime;	}

private:
	uint64	mTime;
};



template<class STRING>
inline STRING& operator<<(STRING& str,const SoyTime& timecode)
{
	BufferString<100> Buffer;
	Buffer.PrintText("T%09Iu", timecode.GetTime() );
	str << Buffer;
	return str;
}




