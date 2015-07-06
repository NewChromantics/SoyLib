#pragma once

#include "SoyTypes.h"
#include <cmath>
#include "SoyVector.h"

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
	TRgb(float r,float g,float b) :
		mRgb	( r,g,b )
	{		
	}
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




