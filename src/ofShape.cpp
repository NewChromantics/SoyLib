#include "ofShape.h"


bool ofShapeBox3::IsOutside(const vec2f& Pos) const
{
	if ( Pos.x < mMin.x )	return true;
	if ( Pos.y < mMin.y )	return true;

	if ( Pos.x > mMax.x )	return true;
	if ( Pos.y > mMax.y )	return true;

	return false;
}


