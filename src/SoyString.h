#pragma once


#include <vector>
#include <string>

namespace Soy
{
	void		StringToLower(std::string& String);
	
	bool		StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringEndsWith(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	
	std::string	Join(const std::vector<std::string>& Strings,const std::string& Glue);
};
