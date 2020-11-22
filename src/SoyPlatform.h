#pragma once

/*
	Platform/OS functions which are not file related
*/
#include <string>

namespace Platform
{
	//	maybe not file system? generic platform stuff...
	std::string	GetComputerName();
	void		SetEnvVar(const char* Key,const char* Value);
	std::string	GetEnvVar(const char* Key);
}

