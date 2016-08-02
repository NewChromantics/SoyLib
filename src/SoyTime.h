#pragma once


#include <iomanip>
#include "SoyTypes.h"
#include "SoyMath.h"	//	just for the range specialisation... maybe move it to reduce includes
#if defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_PS4)
#include <sys/time.h>
#endif
#include <chrono>


class SoyTime;

namespace Soy
{
	namespace Platform
	{
#if defined(__OBJC__)
		SoyTime				GetTime(CMTime Time);
		CMTime				GetTime(SoyTime Time);
		SoyTime				GetTime(CFTimeInterval Time);
#endif
	}
}



//	gr: repalce uses of this with SoyTime
namespace Poco
{
	class Timestamp
	{
	public:
		Timestamp(int Value=0)
		{
		}
		inline bool		operator==(const int v) const			{	return false;	}
		inline bool		operator==(const Timestamp& t) const	{	return false;	}
		inline bool		operator!=(const Timestamp& t) const	{	return false;	}
	};
	class File
	{
	public:
		File(const char* Filename)	{}
		File(const std::string& Filename)	{}
		bool		exists() const	{	return false;	}
		Timestamp	getLastModified() const	{	return Timestamp();	}
	};
};



class SoyTime
{
public:
	explicit SoyTime(bool InitToNow=false) : 
		mTime	( InitToNow ? Now().GetTime() : 0 )
	{
	}
	__deprecated_prefix explicit SoyTime(uint64 Time) __deprecated :
		mTime	( Time )
	{
	}
	SoyTime(const std::chrono::milliseconds& ms) :
		mTime	( ms.count() )
	{
	}
	SoyTime(const SoyTime& Time) :
		mTime	( Time.GetTime() )
	{
	}
	explicit SoyTime(const std::string& String) :
		mTime	( 0 )
	{
		FromString( String );
	}

	bool			FromString(const std::string& String);
	std::string		ToString() const;

	uint64			GetTime() const							{	return mTime;	}
	bool			IsValid() const							{	return mTime!=0;	}
	static SoyTime	Now();
	ssize_t			GetDiff(const SoyTime& that) const
	{
		ssize_t a = size_cast<ssize_t>( this->GetTime() );
		ssize_t b = size_cast<ssize_t>( that.GetTime() );
		return a - b;
	}
	uint64			GetNanoSeconds() const					{	return mTime * 1000000;	}
	void			SetNanoSeconds(uint64 NanoSecs)			{	mTime = NanoSecs / 1000000;	}
	void			SetMicroSeconds(uint64 MicroSecs)		{	mTime = MicroSecs / 1000;	}
	uint64			GetMicroSeconds() const					{	return mTime * 1000;	}
	std::chrono::milliseconds	GetMilliSeconds() const		{	return std::chrono::milliseconds(mTime);	}
	float			GetSecondsf() const						{	return mTime / 1000.f;	}

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
	inline SoyTime	operator+(const SoyTime& B) const		{	return SoyTime( std::chrono::milliseconds(mTime + B.mTime) );	}
	inline SoyTime	operator-(const SoyTime& B) const		{	return SoyTime( std::chrono::milliseconds(mTime - B.mTime) );	}

public:	//	gr: temporarily public during android/ios merge
	uint64	mTime;
};
DECLARE_TYPE_NAME( SoyTime );


inline std::ostream& operator<< (std::ostream &out,const SoyTime &in)
{
	out << 'T' << std::setfill('0') << std::setw(9) << in.GetTime();
	return out;
}


inline std::istream& operator>> (std::istream &in,SoyTime &out)
{
	std::string TimeStr;
	in >> TimeStr;
	
	if ( in.fail() )
		out = SoyTime();
	else
		out.FromString( TimeStr );

	return in;
}


template<>
inline float Soy::Range(const SoyTime& Value,const SoyTime& Start,const SoyTime& End)
{
	auto Value_Start = size_cast<ssize_t>(Value.GetTime()) - size_cast<ssize_t>(Start.GetTime());
	auto End_Start = size_cast<ssize_t>(End.GetTime()) - size_cast<ssize_t>(Start.GetTime());
	
	return static_cast<float>(Value_Start) / static_cast<float>(End_Start);
}
