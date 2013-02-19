#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"

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


class TTransform
{
public:
	TTransform(const vec2f& Position=vec2f()) :
		mPosition			( Position ),
		mRotationDegrees	( 0.f )
	{
	}

	void		Transform(vec2f& Position) const
	{
		float s = sinf( GetRotationRad() );
		float c = cosf( GetRotationRad() );
		vec2f OldPosition( Position );

		// rotate point around center (0,0)
		float xnew = OldPosition.x * c - OldPosition.y * s;
		float ynew = OldPosition.x * s + OldPosition.y * c;

		//	do transform
		Position.x = mPosition.x + xnew;
		Position.y = mPosition.y + ynew;
	}

	void		Transform(TTransform& Child) const
	{
		//	re-position child transform
		Transform( Child.mPosition );

		//	rotate it some more
		Child.SetRotationDeg( Child.mRotationDegrees + this->mRotationDegrees );
	}

	float		GetRotationDeg() const			{	return mRotationDegrees;	}
	float		GetRotationRad() const			{	return ofDegToRad( GetRotationDeg() );	}
	void		SetRotationDeg(float Angle)		{	mRotationDegrees = ofWrapDegrees( Angle );	}

public:
	vec2f		mPosition;

private:
	float		mRotationDegrees;	//	rotate around Z (2D)
};

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

	void			Transform(const TTransform& Trans)	{	Trans.Transform( mPosition );	}

public:
	vec2f		mPosition;
	float		mRadius;
};


class ofShapePolygon2
{
public:
	bool			IsValid() const		{	return mContour.size() >= 3;	}

public:
	ofPolyline		mContour;
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

	float	GetLength() const		{	return GetDirection().length();	}
	vec2f	GetDirection() const	{	return mEnd-mStart;	}
	vec2f	GetNormal() const		{	return GetDirection().getNormalized();	}

public:
	vec2f	mStart;
	vec2f	mEnd;
};


class TCollisionShape
{
public:
	bool			IsValid() const		{	return mCircle.IsValid();	}
	vec2f			GetCenter() const	{	return mCircle.mPosition;	}

	void			Transform(const TTransform& Trans)	{	mCircle.Transform( Trans );	}

public:
	ofShapeCircle2		mCircle;
};


