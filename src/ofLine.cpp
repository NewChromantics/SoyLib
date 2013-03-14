#include "ofLine.h"


bool ofLine2::GetIntersection(const ofLine2& Line,vec2f& Intersection) const
{
	float IntersectionAlongThis,IntersectionAlongLine;
	if ( !GetIntersection( Line, IntersectionAlongThis, IntersectionAlongLine ) )
		return false;

	Intersection = GetPoint( IntersectionAlongThis );
	return true;
}

bool ofLine2::GetIntersection(const ofLine2& Line,float& IntersectionAlongThis,float& IntersectionAlongLine) const	
{
	const vec2f& v1 = this->mStart;
	const vec2f& v2 = this->mEnd;
	const vec2f& v3 = Line.mStart;
	const vec2f& v4 = Line.mEnd;

	vec2f v2MinusV1( v2-v1 );
	vec2f v1Minusv3( v1-v3 );
	vec2f v4Minusv3( v4-v3 );

	float denom =		((v4Minusv3.y) * (v2MinusV1.x)) - ((v4Minusv3.x) * (v2MinusV1.y));
    float numerator =	((v4Minusv3.x) * (v1Minusv3.y)) - ((v4Minusv3.y) * (v1Minusv3.x));
    float numerator2 =	((v2MinusV1.x) * (v1Minusv3.y)) - ((v2MinusV1.y) * (v1Minusv3.x));

    if ( denom == 0.0f )
    {
        if ( numerator == 0.0f && numerator2 == 0.0f )
        {
            return false;//COINCIDENT;
        }
        return false;// PARALLEL;
    }

	float& ua = IntersectionAlongThis;
	float& ub = IntersectionAlongLine;

    ua = numerator / denom;
    ub = numerator2/ denom;

	//	intersection will be past the ends of these lines
	if ( ua < 0.f || ua > 1.f )	return false;
	if ( ub < 0.f || ub > 1.f )	return false;

	return true;
}

//-----------------------------------------------------------
//	get nearest point on line
//-----------------------------------------------------------
vec2f ofLine2::GetNearestPoint(const vec2f& Position,float& Time) const
{
	vec2f LineDir = GetDirection();
	float LineDirDotProduct = LineDir.dot(LineDir);
	
	//	avoid div by zero
	if ( LineDirDotProduct == 0.f )
	{
		Time = 0.f;
		return mStart;
	}

	vec2f Dist = Position - mStart;

	float LineDirDotProductDist = LineDir.dot(Dist);

	Time = LineDirDotProductDist / LineDirDotProduct;

	if ( Time <= 0.f )
		return mStart;

	if ( Time >= 1.f )
		return mEnd;

	return mStart + (LineDir * Time);
}





bool ofLine3::GetIntersection(const ofLine3& Line,vec3f& Intersection) const
{
	float IntersectionAlongThis,IntersectionAlongLine;
	if ( !GetIntersection( Line, IntersectionAlongThis, IntersectionAlongLine ) )
		return false;

	Intersection = GetPoint( IntersectionAlongThis );
	return true;
}


bool ofLine3::GetIntersection(const ofLine3& Line,float& IntersectionAlongThis,float& IntersectionAlongLine) const
{
	vec3f da = this->mEnd - this->mStart; 
	vec3f db = Line.mEnd - Line.mStart;
    vec3f dc = Line.mStart - this->mStart;

	vec3f acrossb = da.cross(db);
	// lines are not coplanar
	if ( dc.dot( acrossb ) != 0.0 ) 
		return false;

	vec3f ccrossb = dc.cross(db);
	IntersectionAlongLine = ccrossb.dot(acrossb) / acrossb.lengthSquared();
	if ( IntersectionAlongLine < 0.f || IntersectionAlongLine > 1.f )
		return false;

	return true;
}

