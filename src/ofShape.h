#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"


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

	
inline float ofGetMathTime(float z,float Min,float Max) 
{
	return (z-Min) / (Max-Min);	
}

//-----------------------------------------------
//	different kind of intersection... for physics
//-----------------------------------------------
class TIntersection
{
public:
	TIntersection() :
		mIsValid		( false )
	{
	}

	bool		IsValid() const			{	return mIsValid;	}

public:
	bool		mIsValid;
	vec2f		mMidIntersection;
	vec2f		mCollisionPointA;
	vec2f		mCollisionPointB;
};


//-----------------------------------------------
//	shape intersection
//-----------------------------------------------
class TIntersection2
{
public:
	TIntersection2() :
		mIntersected	( false )
	{
	}

	operator	bool() const	{	return mIntersected;	}

public:
	bool		mIntersected;	//	did intersect
	vec2f		mDelta;
	float		mDistanceSq;
};


class ofShapeCircle2
{
public:
	ofShapeCircle2(float Radius=0.f,const vec2f& Pos=vec2f()) :
		mRadius		( Radius ),
		mPosition	( Pos )
	{
	}
	ofShapeCircle2(const vec2f& Pos,float Radius) :
		mRadius		( Radius ),
		mPosition	( Pos )
	{
	}

	bool			IsValid() const	{	return mRadius > 0.f;	}
	
	TIntersection2	GetIntersection(const ofShapeCircle2& Shape) const;

public:
	vec2f		mPosition;
	float		mRadius;
};


class ofShapeBox3
{
public:
	ofShapeBox3(const vec3f& Min,const vec3f& Max) :
		mMin	( Min ),
		mMax	( Max )
	{
	}

	vec3f	GetRandomPosInside() const		{	return vec3f( ofRandom(mMin.x,mMax.x), ofRandom(mMin.y,mMax.y), ofRandom(mMin.z,mMax.z) );	}
	vec3f	GetCenter() const				{	return vec3f( ofLerp(mMin.x,mMax.x,0.5f), ofLerp(mMin.y,mMax.y,0.5f), ofLerp(mMin.z,mMax.z,0.5f) );	}
	float	GetTimeZ(float z) const			{	return ofGetMathTime( z, mMin.z, mMax.z );	}
	bool	IsOutside(const vec2f& Pos) const;

public:
	vec3f	mMin;
	vec3f	mMax;
};


class ofLine2
{
public:
	ofLine2()	{}
	ofLine2(const vec2f& Start,const vec2f& End) :
		mStart	( Start ),
		mEnd	( End )
	{
	}

	float	GetLength() const	{	return (mEnd-mStart).length();	}

public:
	vec2f	mStart;
	vec2f	mEnd;
};

