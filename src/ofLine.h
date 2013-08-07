#pragma once


#include "ofxSoylent.h"
#include "SoyMath.h"


class TTransform2;

class ofLine2
{
public:
	ofLine2()	{}
	ofLine2(const vec2f& Start,const vec2f& End) :
		mStart	( Start ),
		mEnd	( End )
	{
	}

	bool	IsZeroLength() const		{	return GetDirection().lengthSquared() <= 0.f;	}
	float	GetLength() const			{	return GetDirection().length();	}
	void	SetLength(float Length)		{	mEnd = mStart + (GetNormal() * Length);	}
	vec2f	GetDirection() const		{	return mEnd-mStart;	}
	vec2f	GetNormal() const			{	return GetDirection().getNormalized();	}
	vec2f	GetPoint(float Time) const	{	return ofLerp( mStart, mEnd, Time );	}
	float	GetDistance(const vec2f& Position) const	{	float ta;	return GetDistance( Position, ta );	}
	float	GetDistance(const ofLine2& Line) const		{	float ta,tb;	return GetDistance( Line, ta, tb );	}
	float	GetDistance(const vec2f& Position,float& ThisTime) const;
	float	GetDistance(const ofLine2& Line,float& ThisTime,float& ThatTime) const;
	vec2f	GetNearestPoint(const vec2f& Position) const				{	float Time;	return GetNearestPoint( Position, Time );	}
	vec2f	GetNearestPoint(const vec2f& Position,float& Time) const;	//	get nearest point on line
	void	GetNearestPoints(const ofLine2& That,vec2f& ThisIntersection,vec2f& ThatIntersection) const;	//	get nearest points on each line to each other
	bool	GetIntersection(const ofLine2& Line,float& IntersectionAlongThis,float& IntersectionAlongLine) const;
	bool	GetIntersection(const ofLine2& Line,vec2f& Intersection) const;
	void	Transform(const TTransform2& Trans);
	
	void	MakeYDescending()		{	if ( mStart.y < mEnd.y )	ofSwap( mStart, mEnd );	}

public:
	vec2f	mStart;
	vec2f	mEnd;
};
DECLARE_NONCOMPLEX_TYPE(ofLine2);

class ofLine3
{
public:
	ofLine3()	{}
	ofLine3(const vec3f& Start,const vec3f& End) :
		mStart	( Start ),
		mEnd	( End )
	{
	}

	float	GetLength() const			{	return GetDirection().length();	}
	vec3f	GetDirection() const		{	return mEnd-mStart;	}
	vec3f	GetNormal() const			{	return GetDirection().getNormalized();	}
	vec3f	GetPoint(float Time) const	{	return ofLerp( mStart, mEnd, Time );	}
	vec3f	GetNearestPoint(const vec3f& Position) const				{	float Time;	return GetNearestPoint( Position, Time );	}
	vec3f	GetNearestPoint(const vec3f& Position,float& Time) const;	//	get nearest point on line
	bool	GetIntersection(const ofLine3& Line,float& IntersectionAlongThis,float& IntersectionAlongLine) const;
	bool	GetIntersection(const ofLine3& Line,vec3f& Intersection) const;
	void	GetNearestPoints(const ofLine3& That,vec3f& ThisIntersection,vec3f& ThatIntersection) const;	//	get nearest points on each line to each other

public:
	vec3f	mStart;
	vec3f	mEnd;
};
DECLARE_NONCOMPLEX_TYPE(ofLine3);