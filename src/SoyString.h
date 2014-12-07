#pragma once


#include <vector>
#include <string>


template<typename TYPE>
class ArrayBridge;

namespace Soy
{
	void		StringToLower(std::string& String);
	std::string	StringToLower(const std::string& String);
	
	bool		StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringEndsWith(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	
	std::string	Join(const std::vector<std::string>& Strings,const std::string& Glue);
	void		StringSplitByString(ArrayBridge<std::string>& Parts,std::string String,std::string Delim,bool IncludeEmpty=true);
	void		StringSplitByString(ArrayBridge<std::string>&& Parts,std::string String,std::string Delim,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);

	std::string	ArrayToString(const ArrayBridge<char>& Array);
	void		ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String);
	
	void		StringToArray(std::string String,ArrayBridge<char>& Array);

	std::string	StreamToString(std::ostream& Stream);	//	windows
	std::string	StreamToString(std::stringstream&& Stream);	//	osx

	void		SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String);
	void		SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String);

	bool		IsUtf8String(const std::string& String);
	bool		IsUtf8Char(char c);
};

