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

	uint32	mTime;
};



template<class STRING>
inline STRING& operator<<(STRING& str,const SoyTime& timecode)
{
	BufferString<100> Buffer;
	Buffer.PrintText("T%09Iu", timecode.mTime );
	str << Buffer;
	return str;
}




