#pragma once


#include "String.hpp"
#include <iomanip>


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


inline std::ostream& operator<< (std::ostream &out,const SoyTime &in)
{
	out << 'T' << std::setfill('0') << std::setw(9) << in.GetTime();
	return out;
}

