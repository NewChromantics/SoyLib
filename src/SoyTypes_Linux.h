#pragma once


#define __func__	__PRETTY_FUNCTION__
#define __noexcept	_GLIBCXX_USE_NOEXCEPT
#define __noexcept_prefix

#define __export	extern "C"

#define __thread				//	thread local not supported on android
#define __stdcall
//#define __pure		__attribute__((pure))
#define __deprecated	__attribute__((deprecated))
#define __deprecated_prefix

#include <stdint.h>

//#define TARGET_POSIX
