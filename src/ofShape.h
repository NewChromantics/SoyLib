#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"
#include "SoyMath.h"
#include "ofPlane.h"
#include "ofLine.h"

class ofShapeCircle2;
class ofShapeCapsule2;
class ofShapePolygon2;
class ofShapeTriangle2;

void Tesselate(ArrayBridge<ofShapeTriangle2>& Triangles,const ofPolyline& Polygon);
void Tesselate(ArrayBridge<ofShapeTriangle2>& Triangles,const ArrayBridge<vec2f>& Polygon);
float GetArea(ArrayBridge<ofShapeTriangle2>& Triangles);
float GetArea(const ofPolyline& Polygon);

class ofxSoyRectangle : public ofRectangle
{
public:
	ofxSoyRectangle()	{}
	
	void		setLeft(float x)	{	setX(x);	}
	void		setTop(float y)		{	setY(y);	}
	void		setRight(float x)	{	setWidth( x - getLeft() );	}
	void		setBottom(float y)	{	setHeight( y - getTop() );	}
};


class TTransform2
{
public:
	TTransform2(const vec2f& Position=vec2f()) :
		mPosition			( Position ),
		mRotationDegrees	( 0.f )
	{
	}

	void		Transform(vec2f& Position) const
	{
		//	rotate around 0,0
		Position.rotate( GetRotationDeg() );

		//	do translate
		Position.x += mPosition.x;
		Position.y += mPosition.y;
	}

	void		Transform(TTransform2& Child) const
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
	TIntersection(bool Valid=false) :
		mIsValid		( Valid ),
		mDistanceSq		( 0.f )
	{
	}

	bool		IsValid() const			{	return mIsValid;	}
	void		Flip()
	{
		ofSwap( mCollisionPointA, mCollisionPointB );
		mDelta *= -1.f;
	}
	
	operator	bool() const	{	return IsValid();	}

public:
	bool		mIsValid;			//	did intersect
	vec2f		mMidIntersection;
	vec2f		mCollisionPointA;
	vec2f		mCollisionPointB;
	vec2f		mDelta;
	float		mDistanceSq;
};



namespace ofShape
{
	TIntersection			GetIntersection(const ofShapeCircle2& a,const ofShapeCircle2& b);
	TIntersection			GetIntersection(const ofShapePolygon2& a,const ofShapeCircle2& b);
	TIntersection			GetIntersection(const ofShapePolygon2& a,const ofShapePolygon2& b);
	TIntersection			GetIntersection(const ofShapeCapsule2& a,const ofShapeCapsule2& b);
	TIntersection			GetIntersection(const ofShapeCapsule2& a,const ofShapePolygon2& b);
	TIntersection			GetIntersection(const ofShapeCapsule2& a,const ofShapeCircle2& b);

	inline TIntersection	GetIntersection(const ofShapeCircle2& a,const ofShapePolygon2& b)		{	TIntersection Int = GetIntersection( b, a );	Int.Flip();	return Int;	}
	inline TIntersection	GetIntersection(const ofShapePolygon2& a,const ofShapeCapsule2& b)		{	TIntersection Int = GetIntersection( b, a );	Int.Flip();	return Int;	}
	inline TIntersection	GetIntersection(const ofShapeCircle2& a,const ofShapeCapsule2& b)		{	TIntersection Int = GetIntersection( b, a );	Int.Flip();	return Int;	}
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

	bool			IsValid() const											{	return mRadius > 0.f;	}
	vec2f			GetCenter() const										{	return mPosition;	}
	bool			IsInside(const vec2f& Point) const;
	bool			IsInside(const ofShapeCircle2& Shape) const;
	TIntersection	GetIntersection(const ofShapeCircle2& Shape) const		{	return ofShape::GetIntersection( *this, Shape );	}
	TIntersection	GetIntersection(const ofShapePolygon2& Shape) const		{	return ofShape::GetIntersection( *this, Shape );	}

	void			Transform(const TTransform2& Trans)	{	Trans.Transform( mPosition );	}
	void			Scale(float Trans)					{	mRadius *= Trans;	}
	void			Accumulate(const ofShapeCircle2& That);

public:
	vec2f		mPosition;
	float		mRadius;
};


class ofShapeCapsule2
{
public:
	ofShapeCapsule2(float Radius=0.f,const ofLine2& Line=ofLine2()) :
		mRadius		( Radius ),
		mLine		( Line )
	{
	}
	ofShapeCapsule2(const ofLine2& Line,float Radius) :
		mRadius		( Radius ),
		mLine		( Line )
	{
	}

	bool			IsValid() const				{	return /*!mLine.IsZeroLength() && */(mRadius > 0.f);	}	//	gr: zero length capsule is fine. Just a sphere!
	vec2f			GetCenter() const			{	return mLine.GetPoint(0.5f);	}
	float			GetDiameter() const			{	return mRadius * 2.f;	}
	float			GetTotalLength() const		{	return mLine.GetLength() + GetDiameter();	}
	ofShapeCircle2	GetBoundsCircle() const		{	return ofShapeCircle2( GetCenter(), (mLine.GetLength()/2.f) + mRadius );	}
	ofRectangle		GetBoundsRect() const;
	float			GetArea() const;
	void			Transform(const TTransform2& Trans)	{	mLine.Transform( Trans );	}
	bool			IsInside(const vec2f& Point) const;
	vec2f			GetNearestPoint(const vec2f& Position) const				{	return mLine.GetNearestPoint( Position );	}
	vec2f			GetNearestPoint(const vec2f& Position,float& Time) const	{	return mLine.GetNearestPoint( Position, Time );	}
	void			Accumulate(const ArrayBridge<vec2f>& Points);
	void			Accumulate(const ofShapeCapsule2& Capsule);

public:
	ofLine2		mLine;
	float		mRadius;
};

template<>
inline ofShapeCapsule2 ofLerp(const ofShapeCapsule2& start,const ofShapeCapsule2& stop, float amt)
{
	ofShapeCapsule2 LerpCapsule;
	LerpCapsule.mRadius = ofLerp( start.mRadius, stop.mRadius, amt );
	LerpCapsule.mLine.mStart = ofLerp( start.mLine.mStart, stop.mLine.mStart, amt );
	LerpCapsule.mLine.mEnd = ofLerp( start.mLine.mEnd, stop.mLine.mEnd, amt );
	return LerpCapsule;
}


class ofShapePolygon2
{
public:
	ofShapePolygon2()
	{
		assert( !IsValid() );
	}
	bool			IsValid() const		{	return mTriangle.GetSize() >= 3;	}
	vec2f			GetCenter() const
	{
		vec2f Center;
		for ( int i=0;	i<mTriangle.GetSize();	i++ )
			Center += mTriangle[i];
		Center /= mTriangle.IsEmpty() ? 1.f : static_cast<float>(mTriangle.GetSize());
		return Center;
	}
	ofShapeCircle2	GetBounds() const;
	TIntersection	GetIntersection(const ofShapeCircle2& Shape) const		{	return ofShape::GetIntersection( *this, Shape );	}
	TIntersection	GetIntersection(const ofShapePolygon2& Shape) const		{	return ofShape::GetIntersection( *this, Shape );	}
	bool			IsInside(const vec2f& Pos) const;

	void			Transform(const TTransform2& Trans)	
	{	
		for ( int i=0;	i<mTriangle.GetSize();	i++ )
			Trans.Transform( mTriangle[i] );
	}


public:
	//ofPolyline		mContour;
	BufferArray<vec2f,3>	mTriangle;
};


class ofShapeTriangle2
{
public:
	bool			IsInside(const vec2f& Point) const;
	bool			IsValid() const		{	return mTriangle[0]!=mTriangle[1] && mTriangle[1]!=mTriangle[2];	}
	float			GetArea() const;
	vec2f			GetCenter() const
	{
		vec2f Center = mTriangle[0];
		Center += mTriangle[1];
		Center += mTriangle[2];
		Center /= 3.f;
		return Center;
	}
	inline ofShapeTriangle2&	operator+=(const vec2f& Offset)	{	mTriangle[0] += Offset;	mTriangle[1] += Offset;	mTriangle[2] += Offset;	return *this;	}
	inline ofShapeTriangle2&	operator-=(const vec2f& Offset)	{	*this += -Offset;	return *this;	}
	
public:
	vec2f	mTriangle[3];
};


class ofShapeTriangle3
{
public:
	ofShapeTriangle3() :
		mTriangle	( 3 )
	{
	}
	bool			IsValid() const		{	return mTriangle.GetSize() >= 3;	}
	vec2f			GetCenter() const
	{
		vec2f Center;
		for ( int i=0;	i<mTriangle.GetSize();	i++ )
			Center += mTriangle[i];
		Center /= mTriangle.IsEmpty() ? 1.f : static_cast<float>(mTriangle.GetSize());
		return Center;
	}

public:
	BufferArray<vec2f,3>	mTriangle;
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




//	gr: replace this with openframeworks path which does more clever stuff
//	maybe? ofPath is very rendering orientated....
class ofShapePath2
{
public:
	ofShapePath2(float MinSegmentLength=2.f) :
		mMinSegmentLength	( MinSegmentLength )
	{
	}

	bool					IsValid() const		{	return mPath.GetSize() >= 2;	}
	float					GetLength() const;
	void					PushTail(const vec2f& Pos);						//	extend the path
	vec2f					GetPosition(float PathPos) const;				//	get a pos along the path between two distances along path (time, but not normalised)
	SoyPair<vec2f,vec2f>	GetPosition(float PathPos,float NextPos) const;	//	get a pos along the path between two distances along path (time, but not normalised)
	vec2f					GetDelta(float PathPos,float NextPos) const;	//	get a delta along the path between two distances along path (time, but not normalised)
	vec2f					GetTailNormal(float TailDistance) const;		//	get the last delta (normalised)

public:
	Array<vec2f>	mPath;
	float			mMinSegmentLength;
};


class TCollisionShape
{
public:
	bool			IsValid() const		{	return mCircle.IsValid();	}	//	this should always be valid when a shape is set
	vec2f			GetCenter() const	{	return mCircle.mPosition;	}

	const ofShapeCircle2&	GetCircle() const	{	return mCircle;	}
	const ofShapePolygon2&	GetPolygon() const	{	return mPolygon;	}
	const ofShapeCapsule2&	GetCapsule() const	{	return mCapsule;	}

	void			Transform(const TTransform2& Trans)	
	{
		mCircle.Transform( Trans );	
		mCapsule.Transform( Trans );	
		mPolygon.Transform( Trans );	
	}

	void			SetPolygon(const ofShapePolygon2& Polygon)	
	{
		mPolygon = Polygon;	
		mCapsule = ofShapeCapsule2();	
		OnShapeChanged();	
	}
	void			SetCapsule(const ofShapeCapsule2& Capsule)	
	{
		mPolygon = ofShapePolygon2();	
		mCapsule = Capsule;	
		OnShapeChanged();	
	}
	void			SetCircle(const ofShapeCircle2& Circle)	
	{
		mPolygon = ofShapePolygon2();	
		mCapsule = ofShapeCapsule2();	
		mCircle = Circle;
		OnShapeChanged();
	}
	void			OnShapeChanged()		
	{
		if ( mPolygon.IsValid() )
			mCircle = mPolygon.GetBounds();	
		else if ( mCapsule.IsValid() )
			mCircle = mCapsule.GetBoundsCircle();	
	}

private:
	ofShapeCircle2		mCircle;
	ofShapePolygon2		mPolygon;
	ofShapeCapsule2		mCapsule;
};


