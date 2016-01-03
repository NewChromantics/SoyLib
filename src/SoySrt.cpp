#include "SoySrt.h"
#include "SoyString.h"
#include "SoyStream.h"
#include <regex>

namespace Srt
{
	class TFrame;
	
	SoyTime		DecodeTime(const std::string& Hours,const std::string& Minutes,const std::string& Seconds,const std::string& Milliseconds);
	void		EncodeTime(std::stringstream& Out,const SoyTime& Time);
	void		DecodeTimeRange(const std::string& TimeString,SoyTime& Start,SoyTime& End);
	void		EncodeTimeRange(std::stringstream& Out,const SoyTime& Start,const SoyTime& End);
	
	const char*	LineFeed = "\r\n";
	const char*	DoubleLineFeed = "\r\n\r\n";
}


template<typename TYPE>
void GetInt(TYPE& Value,const std::string& String)
{
	if ( Soy::StringToType( Value, String ) )
		return;

	std::stringstream Error;
	Error << "Failed to convert " << String << " to int(" << Soy::GetTypeName<TYPE>() << ")";
	throw Soy::AssertException( Error.str() );
}


SoyTime Srt::DecodeTime(const std::string& Hours,const std::string& Minutes,const std::string& Seconds,const std::string& Milliseconds)
{
	size_t hr,min,sec,ms;
	GetInt( hr, Hours );
	GetInt( min, Minutes );
	GetInt( sec, Seconds );
	GetInt( ms, Milliseconds );
	
	uint64 Time = 0;
	Time += ms;
	Time += sec * 1000;
	Time += min * 1000 * 60;
	Time += hr * 1000 * 60 * 60;
	return SoyTime( Time );
}

void Srt::EncodeTime(std::stringstream& Out,const SoyTime& Time)
{
	auto TotalMs = Time.mTime;
	auto Ms = TotalMs % 1000;
	auto TotalSec = (TotalMs-Ms) / 1000;
	auto Sec = TotalSec % 60;
	auto TotalMin = (TotalSec-Sec) / 60;
	auto Min = TotalMin % 60;
	auto TotalHour = (TotalMin-Min) / 60;
	auto Hour = TotalHour;
	
	if ( Hour < 10 )	Out << '0';
	Out << Hour << ':';
	
	if ( Min < 10 )	Out << '0';
	Out << Min << ':';
	
	if ( Sec < 10 )	Out << '0';
	Out << Sec << ',';

	if ( Ms < 100 )	Out << '0';
	if ( Ms < 10 )	Out << '0';
	Out << Ms;
}

void Srt::DecodeTimeRange(const std::string& TimeString,SoyTime& Start,SoyTime& End)
{
	static const char* PatternStr = "^([0-9]+):([0-9]+):([0-9]+),([0-9]+) --> ([0-9]+):([0-9]+):([0-9]+),([0-9]+)$";
	std::regex Pattern(PatternStr);
	std::smatch Match;
	if ( !std::regex_match( TimeString, Match, Pattern ) )
	{
		std::stringstream Error;
		Error << "Failed to parse time range; \"" << TimeString << "\"";
		throw Soy::AssertException( Error.str() );
	}

	Start = DecodeTime( Match[1].str(), Match[2].str(), Match[3].str(), Match[4].str() );
	End = DecodeTime( Match[5].str(), Match[6].str(), Match[7].str(), Match[8].str() );
}

void Srt::EncodeTimeRange(std::stringstream& Out,const SoyTime& Start,const SoyTime& End)
{
	EncodeTime( Out, Start );
	Out << " --> ";
	EncodeTime( Out, End );
}



Srt::TFrame::TFrame() :
	mIndex	( 0 )
{
}
	
TProtocolState::Type Srt::TFrame::Decode(TStreamBuffer& Buffer)
{
	//	look for double terminator
	std::string SrtData;
	if ( !Buffer.Pop( DoubleLineFeed, SrtData, false ) )
		return TProtocolState::Waiting;

	//	split lines
	Array<std::string> Lines;
	Soy::StringSplitByString( GetArrayBridge(Lines), SrtData, LineFeed, false );

	try
	{
		Soy::Assert( Lines.GetSize() > 0, "Missing index line" );
		GetInt( mIndex, Lines[0] );
		Soy::Assert( Lines.GetSize() > 1, "Time range line" );
		DecodeTimeRange( Lines[1], mStart, mEnd );
	
		//	all the rest are lines
		for ( int i=2;	i<Lines.GetSize();	i++ )
		{
			if ( i > 2 )
				mString += '\n';
			mString += Lines[i];
		}
	}
	catch(std::exception& e)
	{
		std::Debug << "Failed to decode SRT frame; " << e.what() << std::endl;
		return TProtocolState::Ignore;
	}
	
	return TProtocolState::Finished;
}

void Srt::TFrame::Encode(TStreamBuffer& Buffer)
{
	std::stringstream Output;
	
	Output << mIndex << LineFeed;
	EncodeTimeRange( Output, mStart, mEnd );
	Output << LineFeed;
	
	//	split the line so we don't get empty lines (double line feed=terminator)
	bool HasOutputTrailingLinefeed = false;
	auto OutputLine = [&Output,&HasOutputTrailingLinefeed](const std::string& Chunk,const char& Splitter)
	{
		Output << Chunk << LineFeed;
		HasOutputTrailingLinefeed = true;
		return true;
	};
	Soy::StringSplitByMatches( OutputLine, mString, LineFeed, false );

	if ( !HasOutputTrailingLinefeed )
		Output << LineFeed;
	
	//	final empty line
	Output << LineFeed;
	
	Buffer.Push( Output.str() );
}


