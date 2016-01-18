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

	void		DecodeTimeRange_NoRegex(const std::string& OrigTimeString,SoyTime& Start,SoyTime& End);
	
	//	gr: something(osx textedit?) stripped my linefeeds, so we might as well support them anyway
	const char* LineFeed_rn = "\r\n";
	const char* LineFeed_n = "\n";
	const char* DoubleLineFeed_rn = "\r\n\r\n";
	const char* DoubleLineFeed_n = "\n\n";

	const char* OutputLineFeed = LineFeed_n;
	const char* OutputDoubleLineFeed = DoubleLineFeed_n;
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

const char* RegexErrorToString(std::regex_error& Error)
{
	switch ( Error.code() )
	{
		case std::regex_constants::error_collate:		return "error_collate";
		case std::regex_constants::error_ctype:			return "error_ctype";
		case std::regex_constants::error_escape:		return "error_escape";
		case std::regex_constants::error_backref:		return "error_backref";
		case std::regex_constants::error_brack:			return "error_brack/mis matched []";
		case std::regex_constants::error_paren:			return "error_paren/mis matched ()";
		case std::regex_constants::error_brace:			return "error_brace";
		case std::regex_constants::error_badbrace:		return "error_badbrace";
		case std::regex_constants::error_range:			return "error_range";
		case std::regex_constants::error_space:			return "error_space";
		case std::regex_constants::error_badrepeat:		return "error_badrepeat";
		case std::regex_constants::error_complexity:	return "error_complexity";
		case std::regex_constants::error_stack:			return "error_stack";
#if defined(TARGET_OSX)
		case std::regex_constants::__re_err_grammar:	return "__re_err_grammar";
		case std::regex_constants::__re_err_empty:		return "__re_err_empty";
		case std::regex_constants::__re_err_unknown:	return "__re_err_unknown";
#endif
		default:
			return "Unknown regex error code";
	};
}

void Srt::DecodeTimeRange_NoRegex(const std::string& OrigTimeString,SoyTime& Start,SoyTime& End)
{
	auto TimeString = OrigTimeString;
	
	auto Match1 = Soy::StringPopUntil( TimeString, ':' );
	auto Match2 = Soy::StringPopUntil( TimeString, ':' );
	auto Match3 = Soy::StringPopUntil( TimeString, ',' );
	
	//	eat these
	auto IsArrowChar = [](char Char)
	{
		return Char == ' ' || Char == '-' || Char == '>';
	};
	auto Match4 = Soy::StringPopUntil( TimeString, IsArrowChar );
	Soy::StringTrimLeft( TimeString, IsArrowChar );
	auto Match5 = Soy::StringPopUntil( TimeString, ':' );
	auto Match6 = Soy::StringPopUntil( TimeString, ':' );
	auto Match7 = Soy::StringPopUntil( TimeString, ',' );
	auto Match8 = TimeString;
	
	/*
	std::Debug << "SRT split (" << OrigTimeString << ") found ["
					<< Match1 << "|"
					<< Match2 << "|"
					<< Match3 << "|" << Match4 << "|" << Match5 << "|" << Match6 << "|" << Match7 << "|" << Match8 << "]" << std::endl;
	*/
	Start = DecodeTime( Match1, Match2, Match3, Match4 );
	End = DecodeTime( Match5, Match6, Match7, Match8 );
}

void Srt::DecodeTimeRange(const std::string& TimeString,SoyTime& Start,SoyTime& End)
{
	//	gr: android either won't support brackets, or regex_match fails
	DecodeTimeRange_NoRegex( TimeString, Start, End );
	return;

	//	http://stackoverflow.com/a/8060116/355753
	std::regex_constants::syntax_option_type Flags = std::regex_constants::basic;
	//static const char* PatternStr = "^([0-9]+):([0-9]+):([0-9]+),([0-9]+) --> ([0-9]+):([0-9]+):([0-9]+),([0-9]+)$";
	static const char* PatternStr = "([0-9]+)";
	std::Debug << "making pattern... #" << PatternStr << "#" << std::endl;
	
	std::smatch Match;
	try
	{
		std::regex Pattern(PatternStr, Flags);
		std::Debug << "std::regex_match(" << TimeString << ")..." << std::endl;
		if ( !std::regex_match( TimeString, Match, Pattern ) )
		{
			std::stringstream Error;
			Error << "Failed to parse time range; \"" << TimeString << "\"";
			throw Soy::AssertException( Error.str() );
		}
		std::Debug << "regex matched OKAY (" <<  PatternStr << ")" << std::endl;
	}
	catch(std::regex_error& e)
	{
		std::stringstream Error;
		Error << "regex error: " << RegexErrorToString(e);
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
	if ( !Buffer.Pop( DoubleLineFeed_n, SrtData, false ) )
	{
		if ( !Buffer.Pop( DoubleLineFeed_rn, SrtData, false ) )
		{
			//	should read some data quite quickly...
			static int WarningSize = 2000;
			if ( Buffer.GetBufferedSize() > WarningSize )
			{
				Array<char> SampleArray( 30 );
				Buffer.Peek( GetArrayBridge(SampleArray) );
				std::stringstream Sample;
				Soy::ArrayToString( GetArrayBridge(SampleArray), Sample );
				std::Debug << "Warning .srt buffer size is " << Buffer.GetBufferedSize() << " yet we haven't found any double-line feeds yet. First " << SampleArray.GetSize() << " chars; " << Sample.str() << std::endl;
			}
			return TProtocolState::Waiting;
		}
	}

	//	split lines
	Array<std::string> Lines;
	Soy::StringSplitByMatches( GetArrayBridge(Lines), SrtData, LineFeed_rn, false );

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
				mString += LineFeed_n;
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
	
	//	if the end time is invalid (or just less than start) correct it
	if ( mEnd < mStart )
		mEnd = mStart;
	
	Output << mIndex << OutputLineFeed;
	EncodeTimeRange( Output, mStart, mEnd );
	Output << OutputLineFeed;
	
	//	split the line so we don't get empty lines (double line feed=terminator)
	bool HasOutputTrailingLinefeed = false;
	auto OutputLine = [&Output,&HasOutputTrailingLinefeed](const std::string& Chunk,const char& Splitter)
	{
		Output << Chunk << OutputLineFeed;
		HasOutputTrailingLinefeed = true;
		return true;
	};
	Soy::StringSplitByMatches( OutputLine, mString, LineFeed_rn, false );

	if ( !HasOutputTrailingLinefeed )
		Output << OutputLineFeed;
	
	//	final empty line
	Output << OutputLineFeed;
	
	Buffer.Push( Output.str() );
}


