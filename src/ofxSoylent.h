#pragma once


#include "ofMain.h"

#ifndef OF_USING_POCO
#error no events
#endif

//	gr: this include list is NOT required, just easier to include.

#include "types.hpp"
typedef ofColor ofColour;

#include "memheap.hpp"
#include "array.hpp"
#include "heaparray.hpp"
#include "bufferarray.hpp"
#include "string.hpp"
typedef Soy::String2<char,Array<char> >		TString;

#include "SoyPair.h"
#include "SoyEnum.h"
#include "SoyTime.h"
#include "SoyRef.h"
#include "SoyState.h"
#include "SoyApp.h"
#include "SoyMath.h"

//	gr: these are annoying names...
typedef SoyRef TRef;


