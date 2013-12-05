#pragma once


#include "ofxSoylent.h"


class ofShapeCircle2;
class ofShapePolygon2;



//	gr: maybe not soymath?
class TColourHsl
{
public:
	TColourHsl(float Hue,float Sat,float Lightness) :
		mHsl	( Hue, Sat, Lightness )
	{
	}
	TColourHsl(const ofColour& Rgb=ofColour::black);

	float		GetHue() const			{	return mHsl.x;	}
	float		GetSaturation() const	{	return mHsl.y;	}
	float		GetLightness() const	{	return mHsl.z;	}
	vec3f		GetHsl() const			{	return mHsl;	}
	ofColour	GetRgb() const;

public:
	vec3f		mHsl;
};
DECLARE_NONCOMPLEX_TYPE( TColourHsl );





#define ofNearZero				0.00001f
#define SCREEN_UP2	vec2f(0,-1)




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


template<class STRING>
inline STRING& operator<<(STRING& str,const ofVec2f& Value)
{
	str << Value.x << ' ' << Value.y;
	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,ofVec2f& Value)
{
	BufferArray<float,2> Floats;
	str.GetFloatArray( Floats );
	
	for ( int i=0;	i<Floats.GetSize();	i++ )
		Value[i] = Floats[i];

	return str;
}


template<class STRING>
inline STRING& operator<<(STRING& str,const ofRectangle& Value)
{
	str << Value.getLeft() << ',' << Value.getTop() << ',' << Value.getWidth() << ',' << Value.getHeight();
	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,ofRectangle& Value)
{
	BufferArray<float,4> Floats;
	str.GetFloatArray( Floats );
	
	//	pad out
	for ( int i=Floats.GetSize();	i<Floats.MaxSize();	i++ )
		Floats.PushBack( -1.f );
	
	Value.setX( Floats[0] );
	Value.setX( Floats[1] );
	Value.setWidth( Floats[2] );
	Value.setHeight( Floats[3] );

	return str;
}

template<class STRING>
inline STRING& operator<<(STRING& str,const ofColour& Value)
{
	str << static_cast<int>(Value.r) << ',' << static_cast<int>(Value.g) << ',' << static_cast<int>(Value.b);
	return str;
}

template<class STRING>
inline STRING& operator<<(STRING& str,const ofColor_<float>& Value)
{
	str << Value.r << ',' << Value.g << ',' << Value.b;
	return str;
}

template<class STRING>
inline STRING& operator<<(STRING& str,const TColourHsl& Value)
{
	str << Value.GetHue() << ',' << Value.GetSaturation() << ',' << Value.GetLightness();
	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,ofColour& Value)
{
	//	gr: should be GetIntArray?
	BufferArray<float,4> Floats;
	str.GetFloatArray( Floats );
	
	Value.r = (Floats.GetSize() >= 1) ? Floats[0] : 0;
	Value.g = (Floats.GetSize() >= 2) ? Floats[1] : 0;
	Value.b = (Floats.GetSize() >= 3) ? Floats[2] : 0;
	Value.a = (Floats.GetSize() >= 4) ? Floats[3] : 0;

	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,ofColor_<float>& Value)
{
	BufferArray<float,4> Floats;
	str.GetFloatArray( Floats );
	
	Value.r = (Floats.GetSize() >= 1) ? Floats[0] : 0;
	Value.g = (Floats.GetSize() >= 2) ? Floats[1] : 0;
	Value.b = (Floats.GetSize() >= 3) ? Floats[2] : 0;
	Value.a = (Floats.GetSize() >= 4) ? Floats[3] : 0;

	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,TColourHsl& Value)
{
	BufferArray<float,3> Floats;
	str.GetFloatArray( Floats );
	
	Value.mHsl.x = (Floats.GetSize() >= 1) ? Floats[0] : 0;
	Value.mHsl.y = (Floats.GetSize() >= 2) ? Floats[1] : 0;
	Value.mHsl.z = (Floats.GetSize() >= 3) ? Floats[2] : 0;

	return str;
}

template<class STRING>
inline const STRING& operator>>(const STRING& str,float& Value)
{
	str.GetFloat( Value );
	return str;
}



template<class STRING>
inline STRING& operator<<(STRING& str,const ofMatrix4x4& Value)
{
	for ( int r=0;	r<4;	r++ )
	for ( int c=0;	c<4;	c++ )
	{
		auto f = Value( r, c );
		str << f << ' ';
	}
	str.TrimRight(' ');
	return str;
}


template<class STRING>
inline const STRING& operator>>(const STRING& str,ofMatrix4x4& Value)
{
	BufferArray<float,16> Floats;
	str.GetFloatArray( Floats );
	
	int i=0;
	for ( int r=0;	r<4;	r++ )
	for ( int c=0;	c<4;	c++,i++ )
	{
		Value( r, c ) = Floats[i];
	}

	return str;
}


inline ofRectangle GetOverlap(const ofRectangle& ra,const ofRectangle& rb,bool AllowNegativeSize)
{
	ofRectangle overlap;
	overlap.x = ofMax( ra.getLeft(), rb.getLeft() );
	overlap.y = ofMax( ra.getTop(), rb.getTop() );
	overlap.setWidth( ofMin( ra.getRight(), rb.getRight() ) - overlap.x );
	overlap.setHeight( ofMin( ra.getBottom(), rb.getBottom() ) - overlap.y );

	if ( !AllowNegativeSize )
	{
		overlap.width = ofMax( overlap.width, 0.f );
		overlap.height = ofMax( overlap.height, 0.f );
	}

	return overlap;
}

inline bool GetOverlapCoverage(vec2f& CoverageA,vec2f& CoverageB,const ofRectangle& ra,const ofRectangle& rb)
{
	ofRectangle overlap = GetOverlap( ra, rb, true );

	//	if we have a negative width or height, the boxes do not overlap
	if ( overlap.getWidth() <= 0 || overlap.getHeight() <= 0 )
		return false;

	CoverageA = vec2f( overlap.getWidth()/ra.getWidth(), overlap.getHeight()/ra.getHeight() );
	CoverageB = vec2f( overlap.getWidth()/rb.getWidth(), overlap.getHeight()/rb.getHeight() );
	return true;
}



inline ofRectangle GetMerged(const ofRectangle& ra,const ofRectangle& rb)
{
	if ( ra == ofRectangle() )
		return rb;
	if ( rb == ofRectangle() )
		return ra;

	ofRectangle merged;
	merged.x = ofMin( ra.getLeft(), rb.getLeft() );
	merged.y = ofMin( ra.getTop(), rb.getTop() );
	merged.setWidth( ofMax( ra.getRight(), rb.getRight() ) - merged.x );
	merged.setHeight( ofMax( ra.getBottom(), rb.getBottom() ) - merged.y );
	return merged;
}

inline void FitInsideRectagle(ofRectangle& Child,const ofRectangle& Parent)
{
	//	move to top left edge
	Child.x = ofMax( Child.x, Parent.x );
	Child.y = ofMax( Child.y, Parent.y );

	//	crop width
	if ( Child.getRight() > Parent.getRight() )
		Child.setWidth( Parent.getRight() - Child.getLeft() );
	if ( Child.getBottom() > Parent.getBottom() )
		Child.setHeight( Parent.getBottom() - Child.getTop() );
	
}

inline void AccumulateRectangle(ofRectangle& Rectangle,const vec2f& Point)
{
	//	rect is invalid...
	if ( Rectangle == ofRectangle() )
	{
		Rectangle = ofRectangle( Point, 0, 0 );
		return;
	}

	//	accumulate...
	if ( Point.x < Rectangle.x )
	{
		float Diff = Rectangle.x - Point.x;
		Rectangle.x -= Diff;
		Rectangle.width += Diff;
	}
	else if ( Point.x > Rectangle.getRight() )
	{
		float Diff = Point.x - Rectangle.getRight();
		Rectangle.width += Diff;
	}

	if ( Point.y < Rectangle.y )
	{
		float Diff = Rectangle.y - Point.y;
		Rectangle.y -= Diff;
		Rectangle.height += Diff;
	}
	else if ( Point.x > Rectangle.getBottom() )
	{
		float Diff = Point.y - Rectangle.getBottom();
		Rectangle.height += Diff;
	}
}
