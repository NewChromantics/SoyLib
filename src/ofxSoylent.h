#pragma once

#if defined(WIN32) && !defined(_DEBUG) && !defined(NDEBUG)
#error neither DEBUG or non-DDEBUG preprocessor specified on windows
#endif


#if !defined(NO_OPENFRAMEWORKS)
#include <ofMain.h>
#define OPENFRAMEWORKS
#endif


//	gr: this include list is NOT required, just easier to include.

#include "SoyTypes.h"

#include "MemHeap.hpp"
#include "Array.hpp"
#include "HeapArray.hpp"
#include "BufferArray.hpp"
#include "String.hpp"

//	yuck
typedef Soy::String2<char,Array<char> >		TString;

#include "SoyEnum.h"
#include "SoyTime.h"
#include "SoyRef.h"
//#include "SoyState.h"
//#include "SoyApp.h"
//#include "SoyMath.h"
//#include "SoyOpenCl.h"
#include "SoyThread.h"
#include "SoyEvent.h"


#if OF_VERSION_MINOR > 7
	#define SOY_FONT_FILENAME	"NewMedia Fett.ttf"
#else
	#define SOY_FONT_FILENAME	"data/NewMedia Fett.ttf"
#endif


