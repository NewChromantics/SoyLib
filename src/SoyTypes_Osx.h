#pragma once

#include <stdint.h>

typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;


//	todo: remove this, only depend in win32 specific code
#define __stdcall
#define __export	extern "C"
