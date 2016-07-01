#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <scebase.h>
/*
#include <kernel.h>
#include <gnmx.h>
#include <video_out.h>
*/

#define __func__		__FUNCTION__

//	gr: consider undefining this to indicate no support (same as android)
#define __thread

// Attribute to make function be exported from a plugin
#define __export		extern "C" __declspec(dllexport)

//	clang seems to define these :D
//#define __pure
//#define __unused		//	can't find a declpec for this :/s

//	gr: delspec's need to go BEFORE function declarations on windows... find a nice workaround that isn't __deprecated(int myfunc());
#define __noexcept		throw()//	__declspec(nothrow)
#define __deprecated	//	__declspec(deprecated)
#define __noexcept_prefix	__declspec(nothrow)
#define __deprecated_prefix	__declspec(deprecated)

#if !_HAS_EXCEPTIONS
#error soy currently uses exceptions... everywhere
#endif

#define TARGET_POSIX