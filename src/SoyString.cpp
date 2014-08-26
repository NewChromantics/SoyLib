#include "SoyString.h"
#include <regex>
#include <sstream>
#include "array.hpp"
#include "bufferarray.hpp"


void Soy::StringToLower(std::string& String)
{
	std::transform( String.begin(), String.end(), String.begin(), ::tolower );
}

std::string Soy::StringToLower(const std::string& String)
{
	std::string LowerString = String;
	StringToLower( LowerString );
	return LowerString;
}


bool Soy::StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		return (Haystack.find(Needle) != std::string::npos);
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringContains(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		return (Haystack.find(Needle) == 0 );
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringBeginsWith(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringEndsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	std::regex_constants::syntax_option_type Flags = std::regex_constants::ECMAScript;
	if ( CaseSensitive )
		Flags |= std::regex::icase;
	
	//	escape some chars...
	std::stringstream Pattern;
	Pattern << ".*";
	for ( auto it=Needle.begin();	it!=Needle.end();	it++ )
	{
		auto Char = *it;
		if ( Char == '.' )
			Pattern << "\\";
		Pattern << Char;
	}
	Pattern << "$";
	
	std::smatch Match;
	std::string HaystackStr = Haystack;
	std::regex Regex( Pattern.str(), Flags );
	if ( std::regex_match( HaystackStr, Match, Regex ) )
		return true;

	return false;
}

std::string	Soy::Join(const std::vector<std::string>& Strings,const std::string& Glue)
{
	//	gr: consider a lambda here?
	std::stringstream Stream;
	for ( auto it=Strings.begin();	it!=Strings.end();	it++ )
	{
		Stream << (*it);
		auto itLast = Strings.end();
		itLast--;
		if ( it != itLast )
			Stream << Glue;
	}
	return Stream.str();
}



std::string Soy::ArrayToString(const ArrayBridge<char>& Array)
{
	std::stringstream Stream;
	ArrayToString( Array, Stream );
	return Stream.str();
}

void Soy::ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String)
{
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		if ( Array[i] == '\0' )
			continue;
		String << Array[i];
	}
}

void Soy::StringToArray(std::string String,ArrayBridge<char>& Array)
{
	const int Size = String.length();
	auto CommandStrArray = GetRemoteArray( String.c_str(), Size, Size );
	Array.PushBackArray( CommandStrArray );
}

void Soy::StringSplit(ArrayBridge<std::string>& Parts,std::string String,std::string Delim,bool IncludeEmpty)
{
	std::string::size_type Start = 0;
	while ( Start < String.length() )
	{
		auto End = String.find( Delim, Start );
		if ( End == std::string::npos )
			End = String.length();
		
		auto Part = String.substr( Start, End-Start );
		//std::Debug << "found [" << Part << "] in [" << String << "]" << std::endl;

		if ( IncludeEmpty || !Part.empty() )
			Parts.PushBack( Part );
	
		Start = End+1;
	}
}

