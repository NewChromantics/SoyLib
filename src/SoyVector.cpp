#include "SoyVector.h"
#if defined(TARGET_IOS)||defined(TARGET_OSX)
#include <CoreGraphics/CGAffineTransform.h>
#endif

#if defined(TARGET_IOS)||defined(TARGET_OSX)
float3x3 Soy::MatrixToVector(const CGAffineTransform& Transform, vec2f TransformNormalisation)
{
	//	CGAffineTransform is a 3x3 matrix with the z col as 0,0,1
	//	http://iphonedevelopment.blogspot.co.uk/2008/10/demystifying-cgaffinetransform.html
	auto& a = Transform.a;
	auto& b = Transform.b;
	auto& c = Transform.c;
	auto& d = Transform.d;
	auto tx = Transform.tx;
	auto ty = Transform.ty;

	//	normalise transform
	//	which needs to be rotated first
	
	vec2f NormRot;
	auto w = TransformNormalisation.x;
	auto h = TransformNormalisation.y;
	//	gr: use the Matrix classes... that's what they're for!
	NormRot.x = (a * w) + (b * h);
	NormRot.y = (c * w) + (d * h);
	
	//	gr: note, tx and ty are swapped here, and I'm pretty sure they shouldn't be
	ty = Transform.tx / NormRot.x;
	tx = Transform.ty / NormRot.y;

	//	gr: this is what I was expecting... but having the translate in 3rd COLUMN is correct...
	//return float3x3( a,b,0, c,d,0, tx,ty,1 );
	return float3x3( a,b,tx, c,d,ty, 0,0,0 );
}
#endif
