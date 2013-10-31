#pragma once

#if !defined(NO_OPENFRAMEWORKS)
#include "ofMain.h"
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

#include "SoyPair.h"
#include "SoyEnum.h"
#include "SoyTime.h"
#include "SoyRef.h"
//#include "SoyState.h"
//#include "SoyApp.h"
//#include "SoyMath.h"
//#include "SoyOpenCl.h"



