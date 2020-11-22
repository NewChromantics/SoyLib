#pragma once

/*
	Platform/OS functions which are not file related
*/
#include <string>

//	need to undef windows macro for anything that includes this
#include "SoyTypes.h"	//	TARGET_WINDOWS

//	bloody windows macros
#if defined(TARGET_WINDOWS)
#undef GetComputerName
#endif


namespace Platform
{
	//	maybe not file system? generic platform stuff...
	std::string	GetComputerName();
	void		SetEnvVar(const char* Key,const char* Value);
	std::string	GetEnvVar(const char* Key);
}

