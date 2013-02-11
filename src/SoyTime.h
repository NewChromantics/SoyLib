#pragma once



class SoyTime
{
public:
	explicit SoyTime() : 
		mTime	( 0 )
	{
	}
	explicit SoyTime(uint32 Time) :
		mTime	( Time )
	{
	}
	SoyTime(const SoyTime& Time) :
		mTime	( Time.GetTime() )
	{
	}

	uint32			GetTime() const							{	return mTime;	}

	inline bool		operator==(const SoyTime& Time) const	{	return mTime == Time.mTime;	}
	inline bool		operator!=(const SoyTime& Time) const	{	return mTime != Time.mTime;	}
	inline bool		operator<(const SoyTime& Time) const	{	return mTime < Time.mTime;	}
	inline bool		operator<=(const SoyTime& Time) const	{	return mTime <= Time.mTime;	}
	inline bool		operator>(const SoyTime& Time) const	{	return mTime > Time.mTime;	}
	inline bool		operator>=(const SoyTime& Time) const	{	return mTime >= Time.mTime;	}

private:
	uint32	mTime;
};



template<class STRING>
inline STRING& operator<<(STRING& str,const SoyTime& timecode)
{
	BufferString<100> Buffer;
	Buffer.PrintText("T%09Iu", timecode.GetTime() );
	str << Buffer;
	return str;
}




