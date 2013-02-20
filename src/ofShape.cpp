#include "ofShape.h"


bool ofShapeBox3::IsOutside(const vec2f& Pos) const
{
	if ( Pos.x < mMin.x )	return true;
	if ( Pos.y < mMin.y )	return true;

	if ( Pos.x > mMax.x )	return true;
	if ( Pos.y > mMax.y )	return true;

	return false;
}

TIntersection2 ofShapeCircle2::GetIntersection(const ofShapeCircle2& ShapeB) const
{
	auto& ShapeA = *this;
	TIntersection2 Intersection;

	Intersection.mDelta = ShapeB.mPosition - ShapeA.mPosition;
	Intersection.mDistanceSq = Intersection.mDelta.lengthSquared();
	float Distance = Intersection.mDelta.length();

	float RadTotal = ShapeA.mRadius + ShapeB.mRadius;

	Intersection.mIntersected = ( Intersection.mDistanceSq <= RadTotal*RadTotal );
	return Intersection;
}


ofShapeCircle2 ofShapePolygon2::GetBounds() const
{
	ofShapeCircle2 Circle;
	Circle.mPosition = GetCenter();

	//	find longest point from center
	for ( int i=0;	i<mTriangle.GetSize();	i++ )
	{
		vec2f Delta = mTriangle[i] - Circle.mPosition;
		Circle.mRadius = ofMax( Circle.mRadius, Delta.length() );
	}
	return Circle;
}
