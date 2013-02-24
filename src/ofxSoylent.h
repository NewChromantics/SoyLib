#pragma once


#include "ofMain.h"

#ifndef OF_USING_POCO
#error no events
#endif

//	gr: this include list is NOT required, just easier to include.

#include "types.hpp"
#include "array.hpp"
#include "bufferarray.hpp"
#include "string.hpp"

#include "SoyPair.h"
#include "SoyEnum.h"
#include "SoyTime.h"
#include "SoyRef.h"
#include "SoyApp.h"

#include "ofShape.h"
#include "SoyNet.h"


//	gr: these are annoying names...
typedef SoyRef TRef;
typedef ofColor ofColour;


