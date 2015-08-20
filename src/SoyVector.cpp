#include "SoyVector.h"
#include <CoreGraphics/CGAffineTransform.h>

float3x3 Soy::MatrixToVector(const CGAffineTransform& Transform)
{
	//	CGAffineTransform is a 3x3 matrix with the z col as 0,0,1
	//	http://iphonedevelopment.blogspot.co.uk/2008/10/demystifying-cgaffinetransform.html
	auto& a = Transform.a;
	auto& b = Transform.b;
	auto& c = Transform.c;
	auto& d = Transform.d;
	auto& tx = Transform.tx;
	auto& ty = Transform.ty;
	
	return float3x3( a,b,0, c,d,0, tx,ty,1 );
}

