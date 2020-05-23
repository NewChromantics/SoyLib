#pragma once


#include <jni.h>

#if defined(_NOEXCEPT)
#define __noexcept	_NOEXCEPT
#else
#define __noexcept	_GLIBCXX_USE_NOEXCEPT	//	when using gnu_static
#endif
#define __noexcept_prefix

#define __func__	__PRETTY_FUNCTION__
#define __export	extern "C"
//	gr: consider undefining this to indicate no support
#define __thread				//	thread local not supported on android
#define __stdcall
//#define __pure		__attribute__((pure))
#define __deprecated	__attribute__((deprecated))
#define __deprecated_prefix

#include <stdint.h>

typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;

#define TARGET_POSIX
