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

inline bool ofIsValidFloat(double Value)
{
	if ( Value != Value )
		return false;
	if ( _isnan(Value) )
		return false;
	return true;
}

inline bool ofIsValidFloat(float Value)
{
	//	standard way of detecting invalid floats
	if ( Value != Value )
		return false;

#if (_MSC_VER) || defined (TARGET_ANDROID)
#ifndef isnan
#define isnan(a) ((a) != (a))
#endif
	if ( isnan(Value) )
		return false;
#else
	if ( std::isnan(Value) )
		return false;
#endif
	
	//	IEEE test
	if ( _isnan(Value) )
		return false;

	return true;
}

inline bool ofIsValidFloat(const ofVec2f& Value)
{
	return ofIsValidFloat(Value.x) && 
			ofIsValidFloat(Value.y);
}

inline bool ofIsValidFloat(const ofVec3f& Value)
{
	return ofIsValidFloat(Value.x) && 
			ofIsValidFloat(Value.y) && 
			ofIsValidFloat(Value.z);
}

inline bool ofIsValidFloat(const ofVec4f& Value)
{
	return ofIsValidFloat(Value.x) && 
			ofIsValidFloat(Value.y) && 
			ofIsValidFloat(Value.z) && 
			ofIsValidFloat(Value.w);
}

inline bool ofIsValidFloat(const float* Values,int Count)
{
	for ( int i=0;	i<Count;	i++ )
	{
		if ( !ofIsValidFloat( Values[i] ) )
			return false;
	}
	return true;
}

inline bool ofIsValidFloat(const ofMatrix4x4& Value)
{
	return ofIsValidFloat( Value.getRowAsVec4f(0) ) && 
			ofIsValidFloat( Value.getRowAsVec4f(1) ) && 
			ofIsValidFloat( Value.getRowAsVec4f(2) ) && 
			ofIsValidFloat( Value.getRowAsVec4f(3) );
}
