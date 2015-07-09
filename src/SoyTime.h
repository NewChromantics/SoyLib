#pragma once


#include <iomanip>
#include "SoyTypes.h"

#if defined(TARGET_OSX)||defined(TARGET_IOS)
#include <sys/time.h>
#endif


inline unsigned long long	ofGetSystemTime()
{
#if defined(TARGET_WINDOWS)
	return timeGetTime();
#elif defined(TARGET_OSX)||defined(TARGET_IOS)||defined(TARGET_ANDROID)
	struct timeval now;
	gettimeofday( &now, NULL );
	return
	(unsigned long long) now.tv_usec/1000 +
	(unsigned long long) now.tv_sec*1000;
#else 
#error GetSystemTime undefined on target
#endif
}
inline unsigned long long	ofGetElapsedTimeMillis()	{	return ofGetSystemTime();	}	//	gr: offrameworks does -StartTime
inline float				ofGetElapsedTimef()			{	return static_cast<float>(ofGetElapsedTimeMillis()) / 1000.f;	}


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
	explicit SoyTime(uint64 Time) :
		mTime	( Time )
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

