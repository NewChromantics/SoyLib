#pragma once


#include "ofxSoylent.h"
#include "SoyMath.h"


class ofLine2
{
public:
	ofLine2()	{}
	ofLine2(const vec2f& Start,const vec2f& End) :
		mStart	( Start ),
		mEnd	( End )
	{
	}

	float	GetLength() const			{	return GetDirection().length();	}
	vec2f	GetDirection() const		{	return mEnd-mStart;	}
	vec2f	GetNormal() const			{	return GetDirection().getNormalized();	}
	vec2f	GetPoint(float Time) const	{	return ofLerp( mStart, mEnd, Time );	}
	vec2f	GetNearestPoint(const vec2f& Position) const				{	float Time;	return GetNearestPoint( Position, Time );	}
	vec2f	GetNearestPoint(const vec2f& Position,float& Time) const;	//	get nearest point on line
	bool	GetIntersection(const ofLine2& Line,float& IntersectionAlongThis,float& IntersectionAlongLine) const;
	bool	GetIntersection(const ofLine2& Line,vec2f& Intersection) const;

public:
	vec2f	mStart;
	vec2f	mEnd;
};

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
	
public:
	vec3f	mStart;
	vec3f	mEnd;
};
