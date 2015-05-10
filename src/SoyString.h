#pragma once


#include <vector>
#include <string>
#include <sstream>

template<typename TYPE>
class ArrayBridge;

namespace Soy
{
	void		StringToLower(std::string& String);
	std::string	StringToLower(const std::string& String);
	
	bool		StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringEndsWith(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	bool		StringMatches(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	
	std::string	Join(const std::vector<std::string>& Strings,const std::string& Glue);
	void		StringSplitByString(ArrayBridge<std::string>& Parts,std::string String,std::string Delim,bool IncludeEmpty=true);
	void		StringSplitByString(ArrayBridge<std::string>&& Parts,std::string String,std::string Delim,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);

	void		StringTrimLeft(std::string& String,char TrimChar);
	
	std::string	ArrayToString(const ArrayBridge<char>& Array);
	void		ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String);
	
	void		StringToArray(std::string String,ArrayBridge<char>& Array);
	inline void	StringToArray(std::string String,ArrayBridge<char>&& Array)	{	StringToArray( String, Array );	}

	std::string	StreamToString(std::ostream& Stream);	//	windows
	std::string	StreamToString(std::stringstream&& Stream);	//	osx

	void		SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String);
	void		SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String);

	bool		IsUtf8String(const std::string& String);
	bool		IsUtf8Char(char c);
	
	template<typename TYPE>
	bool		StringToType(TYPE& Out,const std::string& String);
	template<typename TYPE>
	TYPE		StringToType(const std::string& String,const TYPE& Default);
};

#if defined(__OBJC__)
@class NSString;
namespace Soy
{
	NSString*	StringToNSString(const std::string& String);
	std::string	NSStringToString(NSString* String);
};
#endif

template<typename TYPE>
inline bool Soy::StringToType(TYPE& Out,const std::string& String)
{
	std::stringstream s( String );
	s >> Out;
	if ( s.fail() )
		return false;

	return true;
}


template<>
inline bool Soy::StringToType(int& Out,const std::string& String)
{
	Out = std::stoi( String );
	return true;
}

	
template<typename TYPE>
inline TYPE Soy::StringToType(const std::string& String,const TYPE& Default)
{
	TYPE Value;
	if ( !StringToType( Value, String ) )
		return Default;
	return Value;
}

