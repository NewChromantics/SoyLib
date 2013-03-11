#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"


class ofShapeCircle2;
class ofShapePolygon2;

#define ofNearZero				0.0001f
#define SCREEN_UP2	vec2f(0,-1)


template<typename T>
const T& ofMin(const T& a,const T& b)
{
	return (a < b) ? a : b;
}
template<typename T>
const T& ofMax(const T& a,const T& b)
{
	return (a > b) ? a : b;
}

template<typename T>
void ofLimit(T& v,const T& Min,const T& Max)
{
	assert( Min <= Max );
	if ( v < Min )
		v = Min;
	else if ( v > Max )
		v = Max;
}

template<typename T>
void ofSwap(T& a,T& b)
{
	T Temp = a;
	a = b;
	b = Temp;
}

template<typename T>
T ofLerp(const T& start,const T& stop, float amt)
{
	return start + ((stop-start) * amt);
}


inline float ofGetMathTime(float z,float Min,float Max) 
{
	return (z-Min) / (Max-Min);	
}


inline vec2f ofNormalFromAngle(float AngleDegrees)
{
	float AngleRad = ofDegToRad( AngleDegrees );
	vec2f Normal = SCREEN_UP2;
	return Normal.rotated( AngleDegrees );
}

inline float ofAngleFromNormal(const vec2f& Normal)
{
	static bool usenormalised = false;
	vec2f n = usenormalised ? Normal.normalized() : Normal;
	return SCREEN_UP2.angle( n );
}


