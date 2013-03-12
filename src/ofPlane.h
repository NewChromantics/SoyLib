#pragma once


#include "ofxSoylent.h"
#include "SoyMath.h"
#include "ofLine.h"


class ofPlane
{
public:
	ofPlane() :
		mDistance	( 0.f )
	{
	}

	ofPlane(const ofVec3f& n, const float& d) :
		mNormal		(n), 
		mDistance	(d)
	{
	}

	ofPlane(const ofVec3f& n, const ofVec3f& point) :
		mNormal		( n ),
		mDistance	( n.dot(point) )
	{
	}

	ofPlane(const ofVec3f& v1, const ofVec3f& v2, const ofVec3f& v3) :
		mNormal		( (v2-v1).cross(v3-v1).normalized() ), 
		mDistance	( v1.dot(mNormal) )
	{
	}

	ofPlane(const ofPlane& plane) : 
		mNormal		( plane.mNormal ), 
		mDistance	( plane.mDistance )
	{
	}


	float GetOffset(const ofVec3f& point) const
	{
		return mNormal.dot(point) - mDistance;
	}

	bool IsFront(const ofVec3f& point) const
	{
		return GetOffset(point) > 0;
	}

	bool IsBack(const ofVec3f& point) const
	{
		return GetOffset(point) < 0;
	}
		
	bool GetIntersection(const ofLine3& Line,float& Time)
	{
		const ofVec3f& origin = Line.mStart;
		ofVec3f direction = Line.mEnd - Line.mStart;

		float ndot = mDistance - origin.dot( mNormal );
		float vdot = direction.dot( mNormal );

		// is line parallel to the plane? if so, even if the line is
		// at the plane it is not considered as intersection because
		// it would be impossible to determine the point of intersection
		if ( vdot == 0 )
			return false;

		// the resulting intersection is in the line segment from a - b
		// if the intersect time is between 0 and 1
		Time = ndot / vdot;

		return true;
	}
	
	bool GetIntersection(const ofLine3& Line,vec3f& Intersection)
	{
		float Time;
		if ( !GetIntersection( Line, Time ) )
			return false;

		Intersection = Line.GetPoint( Time );
		return true;
	}
	
public:
	ofVec3f	mNormal;
	float	mDistance;
};


