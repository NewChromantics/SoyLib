#pragma once


#include "ofxSoylent.h"
#include "mathfu/vector.h"
#include "mathfu/vector_2.h"
#include "mathfu/vector_3.h"
#include "mathfu/vector_4.h"
#include <cmath>

//	gr: dumb types with nice transparent accessors, then for complex stuff, use mathfu::Vector<float,3>
#define SWIZZLE2(A,B)	vec2x<TYPE>	A##B()		{	return vec2x<TYPE>(A,B);	}
#define SWIZZLE3(A,B,C)	vec3x<TYPE>	A##B##C()	{	return vec3x<TYPE>(A,B,C);	}

template<typename TYPE>
class vec2x
{
public:
	vec2x(TYPE _x,TYPE _y) :
		x	( _x ),
		y	( _y )
	{
	}
	vec2x() :
		x	( 0 ),
		y	( 0 )
	{
	}

	vec2x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;	return *this;	}
	vec2x&	operator*=(const vec2x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	return *this;	}
	
	bool	operator==(const vec2x& That) const	{	return x==That.x && y==That.y;	}
	bool	operator!=(const vec2x& That) const	{	return x!=That.x || y!=That.y;	}
	
	vec2x	xy() const	{	return vec2x(x,y);	}
	vec2x	yx() const	{	return vec2x(y,x);	}
	vec2x	xx() const	{	return vec2x(x,x);	}
	vec2x	yy() const	{	return vec2x(y,y);	}
	
public:
	TYPE	x;
	TYPE	y;
};


template<typename TYPE>
class vec3x
{
public:
	vec3x(TYPE _x,TYPE _y,TYPE _z) :
	x	( _x ),
	y	( _y ),
	z	( _z )
	{
	}
	vec3x() :
	x	( 0 ),
	y	( 0 ),
	z	( 0 )
	{
	}
	
	vec3x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;		z*=Scalar;	return *this;	}
	vec3x&	operator*=(const vec3x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	z*=Scalar.z;	return *this;	}
	
	SWIZZLE2(x,x);
	SWIZZLE2(x,y);
	SWIZZLE2(y,x);
	SWIZZLE2(y,y);
	
	SWIZZLE3(x,x,x);
	SWIZZLE3(y,y,y);
	SWIZZLE3(z,z,z);
	SWIZZLE3(x,y,z);
	
public:
	TYPE	x;
	TYPE	y;
	TYPE	z;
};


template<typename TYPE>
class vec4x
{
public:
	vec4x(TYPE _x,TYPE _y,TYPE _z,TYPE _w) :
	x	( _x ),
	y	( _y ),
	z	( _z ),
	w	( _w )
	{
	}
	vec4x() :
	x	( 0 ),
	y	( 0 ),
	z	( 0 ),
	w	( 0 )
	{
	}
	
	vec4x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;		z*=Scalar;		w*=Scalar;	return *this;	}
	vec4x&	operator*=(const vec4x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	z*=Scalar.z;	w*=Scalar.w;	return *this;	}
	
	SWIZZLE2(x,x);
	SWIZZLE2(x,y);
	SWIZZLE2(y,x);
	SWIZZLE2(y,y);
	
	SWIZZLE3(x,x,x);
	SWIZZLE3(y,y,y);
	SWIZZLE3(z,z,z);
	SWIZZLE3(x,y,z);
	
public:
	TYPE	x;
	TYPE	y;
	TYPE	z;
	TYPE	w;
};


typedef vec2x<float> vec2f;
typedef vec3x<float> vec3f;
typedef vec4x<float> vec4f;

DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec2f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec3f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec4f );



//	gr: maybe not soymath?
class TColourHsl
{
public:
	TColourHsl(float Hue,float Sat,float Lightness) :
		mHsl	( Hue, Sat, Lightness )
	{
	}
#if !defined(NO_OPENFRAMEWORKS)
	TColourHsl(const ofColour& Rgb=ofColour::black);
#endif
	
	float		GetHue() const			{	return mHsl.x;	}
	float		GetSaturation() const	{	return mHsl.y;	}
	float		GetLightness() const	{	return mHsl.z;	}
	vec3f		GetHsl() const			{	return mHsl;	}
#if !defined(NO_OPENFRAMEWORKS)
	ofColour	GetRgb() const;
#endif
public:
	vec3f		mHsl;
};
DECLARE_NONCOMPLEX_TYPE( TColourHsl );


