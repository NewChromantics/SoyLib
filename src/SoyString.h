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
	void		StringSplit(ArrayBridge<std::string>& Parts,std::string String,std::string Delim,bool IncludeEmpty=true);

	std::string	ArrayToString(const ArrayBridge<char>& Array);
	void		ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String);
	
	void		StringToArray(std::string String,ArrayBridge<char>& Array);
};

