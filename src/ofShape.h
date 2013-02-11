#pragma once



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

	bool		IsValid() const	{	return mRadius > 0.f;	}

public:
	vec2f		mPosition;
	float		mRadius;
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

public:
	vec3f	mMin;
	vec3f	mMax;
};


class ofShapeLine2
{
public:
	ofShapeLine2()	{}
	ofShapeLine2(const vec2f& Start,const vec2f& End) :
		mStart	( Start ),
		mEnd	( End )
	{
	}

public:
	vec2f	mStart;
	vec2f	mEnd;
};

