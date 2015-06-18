#pragma once

#include "SoyTypes.h"
#include "mathfu/vector.h"
#include "mathfu/vector_2.h"
#include "mathfu/vector_3.h"
#include "mathfu/vector_4.h"
#include "mathfu/matrix_4x4.h"
#include <cmath>


namespace Soy
{
	template<typename TYPE>
	bool		IsValidFloat(const TYPE& f)
	{
		return std::isfinite(f);	//	not infinite and not nan
	};

	inline float RadToDeg(float Radians)
	{
		return Radians * (180.f/M_PI);
	}
	
	inline float DegToRad(float Degrees)
	{
		return Degrees * (M_PI / 180.f);
	}
}

//	for soy: optimised/full implementations are called "matrix's"
//	the simple POD types are called floatX's
namespace Soy
{
	typedef mathfu::Vector<float,2>		Matrix2x1;
	typedef mathfu::Vector<float,3>		Matrix3x1;
	typedef mathfu::Vector<float,4>		Matrix4x1;
	typedef mathfu::Matrix<float,4,4>	Matrix4x4;
	typedef mathfu::Matrix<float,3,3>	Matrix3x3;
};

//	expanded std functions
namespace std
{
	template<typename T>
	inline const T&	min(const T& a,const T& b,const T& c)
	{
		return std::min<T>( a, std::min<T>( b, c ) );
	}
	
	template<typename T>
	inline const T&	max(const T& a,const T& b,const T& c)
	{
		return std::max<T>( a, std::max<T>( b, c ) );
	}
	
	template<typename T>
	inline void			clamp(T& a,const T& Min,const T& Max)
	{
		//	gr: simple comparisons, not min/max funcs in case this is complex
		if ( a < Min )
			a = Min;
		if ( a > Max )
			a = Max;
	}
	
	template<typename T>
	inline const T&		clamped(const T& a,const T& Min,const T& Max)
	{
		//	gr: simple comparisons, not min/max funcs in case this is complex
		if ( a < Min )
			return Min;
			
		if ( a > Max )
			return Max;
		
		return a;
	}
}

namespace Soy
{
	template<typename T>
	T	Lerp(const T& Start,const T& End,float Time)
	{
		return Start + ((End-Start) * Time);
	}

	//	gr: was "GetMathTime". Range doesn't scream "unlerp" to me, but still, this is the conventional name (I think it's in glsl too)
	template<typename T>
	float	Range(const T& Value,const T& Start,const T& End)
	{
		return (Value-Start) / (End-Start);
	}
}




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



template<typename TYPE>
class vec4x4
{
public:
	vec4x4() :
		rows	{	vec4x<TYPE>(1,0,0,0), vec4x<TYPE>(0,1,0,0), vec4x<TYPE>(0,0,1,0), vec4x<TYPE>(0,0,0,1)	}
	{
	}
	vec4x4(TYPE a,TYPE b,TYPE c,TYPE d,
		   TYPE e,TYPE f,TYPE g,TYPE h,
		   TYPE i,TYPE j,TYPE k,TYPE l,
		   TYPE m,TYPE n,TYPE o,TYPE p) :
		rows	{	vec4x<TYPE>(a,b,c,d), vec4x<TYPE>(e,f,g,h), vec4x<TYPE>(i,j,k,l), vec4x<TYPE>(m,n,o,p)	}
	{
	}
	
public:
	vec4x<TYPE>	rows[4];
};



//	gr: rename these types float2, float3, float4
typedef vec2x<float> vec2f;
typedef vec3x<float> vec3f;
typedef vec4x<float> vec4f;
typedef vec4x4<float> float4x4;

DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec2f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec3f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec4f );
DECLARE_NONCOMPLEX_TYPE( float4x4 );


namespace Soy
{
	inline Matrix2x1 VectorToMatrix(const vec2f& v)	{	return Matrix2x1( v.x, v.y );	}
	inline Matrix3x1 VectorToMatrix(const vec3f& v)	{	return Matrix3x1( v.x, v.y, v.z );	}
	inline Matrix4x1 VectorToMatrix(const vec4f& v)	{	return Matrix4x1( v.x, v.y, v.z, v.w );	}
	
	inline vec2f MatrixToVector(const Matrix2x1& v)	{	return vec2f( v.x(), v.y() );	}
	inline vec3f MatrixToVector(const Matrix3x1& v)	{	return vec3f( v.x(), v.y(), v.z() );	}
	inline vec4f MatrixToVector(const Matrix4x1& v)	{	return vec4f( v.x(), v.y(), v.z(), v.w() );	}
	inline float4x4 MatrixToVector(const Matrix4x4& v)
	{
		return float4x4( v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15] );
	}
};














//	gr: maybe not in SoyMath
namespace Soy
{
	class THsl;
	class TRgb;
};




class Soy::THsl
{
public:
	THsl()					{}
	THsl(const TRgb& rgb);
	
	float&			h()			{	return mHsl.x;	}
	float&			s()			{	return mHsl.y;	}
	float&			l()			{	return mHsl.z;	}
	const float&	h() const	{	return mHsl.x;	}
	const float&	s() const	{	return mHsl.y;	}
	const float&	l() const	{	return mHsl.z;	}
	
	TRgb			rgb() const;

public:
	vec3f		mHsl;
};
DECLARE_NONCOMPLEX_TYPE( Soy::THsl );



class Soy::TRgb
{
public:
	TRgb();
	TRgb(const THsl& Hsl);
	
	float&			r()			{	return mRgb.x;	}
	float&			g()			{	return mRgb.y;	}
	float&			b()			{	return mRgb.z;	}
	const float&	r() const	{	return mRgb.x;	}
	const float&	g() const	{	return mRgb.y;	}
	const float&	b() const	{	return mRgb.z;	}
	THsl			hsl() const;
	
public:
	vec3f		mRgb;
};
DECLARE_NONCOMPLEX_TYPE( Soy::TRgb );






namespace Soy
{
	inline	TRgb	THsl::rgb() const	{	return TRgb(*this);	}
	inline	THsl	TRgb::hsl() const	{	return THsl(*this);	}
}




