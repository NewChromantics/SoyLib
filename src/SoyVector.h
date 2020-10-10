#pragma once


#include "SoyTypes.h"

//	gr: if we want to reduce dependancies, put a GetArrayBridge(vecN<T>) func in another file
#include "RemoteArray.h"
#include "BufferArray.hpp"

//	for soy: optimised/full implementations are called "matrix's"
//	the simple POD types are called floatX's
namespace Soy
{
	template<typename TYPE>
	class Rectx;

	//	minmax or bounds...
	template<typename TYPE>
	class Boundsx;
	
	//	delineators for string. Was using multiple chars for input parsing, but that's gone
	static const char* VecNXDelins = ",x";
};

struct CGAffineTransform;



//	gr: dumb types with nice transparent accessors, then for complex stuff, use mathfu::Vector<float,3>
#define SWIZZLE2(A,B)	vec2x<TYPE>	A##B() const	{	return vec2x<TYPE>(A,B);	}
#define SWIZZLE3(A,B,C)	vec3x<TYPE>	A##B##C() const	{	return vec3x<TYPE>(A,B,C);	}

template<typename TYPE>
class vec2x
{
public:
	vec2x(TYPE _x,TYPE _y) :
		x	( _x ),
		y	( _y )
	{
	}
	vec2x() :
		x	( 0 ),
		y	( 0 )
	{
	}

	vec2x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;	return *this;	}
	vec2x&	operator*=(const vec2x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	return *this;	}
	vec2x&	operator+=(const vec2x& Scalar)	{	x+=Scalar.x;	y+=Scalar.y;	return *this;	}
	vec2x&	operator-=(const vec2x& Scalar)	{	x-=Scalar.x;	y-=Scalar.y;	return *this;	}
	vec2x&	operator/=(const vec2x& Scalar)	{	x/=Scalar.x;	y/=Scalar.y;	return *this;	}

	vec2x	operator*(const vec2x& Scalar) const	{	return vec2x(x*Scalar.x, y* Scalar.y);	}

	bool	operator==(const vec2x& That) const	{	return x==That.x && y==That.y;	}
	bool	operator!=(const vec2x& That) const	{	return x!=That.x || y!=That.y;	}
	
	vec2x	xy() const	{	return vec2x(x,y);	}
	vec2x	yx() const	{	return vec2x(y,x);	}
	vec2x	xx() const	{	return vec2x(x,x);	}
	vec2x	yy() const	{	return vec2x(y,y);	}
	
	const TYPE&	operator[](size_t i) const
	{
		const TYPE* Elements[] = { &x,&y };
		return *Elements[i];
	}
	TYPE&	operator[](size_t i)
	{
		TYPE* Elements[] = { &x,&y };
		return *Elements[i];
	}
	
public:
	TYPE	x;
	TYPE	y;
};


template<typename TYPE>
class vec3x
{
public:
	vec3x(TYPE _x,TYPE _y,TYPE _z) :
		x	( _x ),
		y	( _y ),
		z	( _z )
	{
	}
	vec3x() :
		x	( 0 ),
		y	( 0 ),
		z	( 0 )
	{
	}
	
	vec3x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;		z*=Scalar;	return *this;	}
	vec3x&	operator*=(const vec3x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	z*=Scalar.z;	return *this;	}
	
	SWIZZLE2(x,x);
	SWIZZLE2(x,y);
	SWIZZLE2(y,x);
	SWIZZLE2(y,y);
	
	SWIZZLE3(x,x,x);
	SWIZZLE3(y,y,y);
	SWIZZLE3(z,z,z);
	SWIZZLE3(x,y,z);

	FixedRemoteArray<TYPE>		GetArray() const 	{	return FixedRemoteArray<TYPE>( const_cast<TYPE*>(&x), 3);	}

	const TYPE&	operator[](size_t i) const
	{
		const TYPE* Elements[] = { &x,&y,&z };
		return *Elements[i];
	}
	TYPE&	operator[](size_t i)
	{
		TYPE* Elements[] = { &x,&y,&z };
		return *Elements[i];
	}

	bool	operator==(const vec3x& that) const	{	return x==that.x && y==that.y && z==that.z;	}
	
public:
	TYPE	x;
	TYPE	y;
	TYPE	z;
};


template<typename TYPE>
class vec4x
{
public:
	vec4x(TYPE _x,TYPE _y,TYPE _z,TYPE _w) :
		x	( _x ),
		y	( _y ),
		z	( _z ),
		w	( _w )
	{
	}
	vec4x() :
		x	( 0 ),
		y	( 0 ),
		z	( 0 ),
		w	( 0 )
	{
	}
	
	vec4x&	operator*=(const TYPE& Scalar)	{	x*=Scalar;		y*=Scalar;		z*=Scalar;		w*=Scalar;	return *this;	}
	vec4x&	operator*=(const vec4x& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	z*=Scalar.z;	w*=Scalar.w;	return *this;	}
	
	SWIZZLE2(x,x);
	SWIZZLE2(x,y);
	SWIZZLE2(y,x);
	SWIZZLE2(y,y);
	
	SWIZZLE3(x,x,x);
	SWIZZLE3(y,y,y);
	SWIZZLE3(z,z,z);
	SWIZZLE3(w,w,w);
	SWIZZLE3(x,y,z);
	
	FixedRemoteArray<TYPE>		GetArray() const 	{	return FixedRemoteArray<TYPE>(&x, 4);	}


	const TYPE&	operator[](size_t i) const
	{
		const TYPE* Elements[] = { &x,&y,&z,&w };
		return *Elements[i];
	}
	TYPE&	operator[](size_t i)
	{
		TYPE* Elements[] = { &x,&y,&z,&w };
		return *Elements[i];
	}
	
public:
	TYPE	x;
	TYPE	y;
	TYPE	z;
	TYPE	w;
};



template<typename TYPE>
class vec4x4
{
public:
	vec4x4() :
		vec4x4	( 1,0,0,0,	0,1,0,0,	0,0,1,0,	0,0,0,1 )
	{
	}
	vec4x4(const TYPE* m44)
	{
		std::copy(m44, m44 + 16, &rows[0][0]);
	}
	vec4x4(TYPE a,TYPE b,TYPE c,TYPE d,
		   TYPE e,TYPE f,TYPE g,TYPE h,
		   TYPE i,TYPE j,TYPE k,TYPE l,
		   TYPE m,TYPE n,TYPE o,TYPE p)
#if !defined(OLD_VISUAL_STUDIO)
		   :
		rows	{	vec4x<TYPE>(a,b,c,d), vec4x<TYPE>(e,f,g,h), vec4x<TYPE>(i,j,k,l), vec4x<TYPE>(m,n,o,p)	}
#endif
	{
#if defined(OLD_VISUAL_STUDIO)
		rows[0] = vec4x<TYPE>(a,b,c,d);
		rows[1] = vec4x<TYPE>(e,f,g,h);
		rows[2] = vec4x<TYPE>(i,j,k,l);
		rows[3] = vec4x<TYPE>(m,n,o,p);
#endif
	}

	const TYPE&	operator()(size_t c,size_t r) const
	{
		return rows[r][c];
	}
	
	const TYPE&	operator[](size_t i) const
	{
		const TYPE* Elements[] =
		{
			&rows[0][0],
			&rows[1][0],
			&rows[2][0],
			&rows[3][0],
			&rows[0][1],
			&rows[1][1],
			&rows[2][1],
			&rows[3][1],
			&rows[0][2],
			&rows[1][2],
			&rows[2][2],
			&rows[3][2],
			&rows[0][3],
			&rows[1][3],
			&rows[2][3],
			&rows[3][3],
		};
		return *Elements[i];
	}
	
	TYPE&	operator[](size_t i)
	{
		TYPE* Elements[] =
		{
			&rows[0][0],
			&rows[1][0],
			&rows[2][0],
			&rows[3][0],
			&rows[0][1],
			&rows[1][1],
			&rows[2][1],
			&rows[3][1],
			&rows[0][2],
			&rows[1][2],
			&rows[2][2],
			&rows[3][2],
			&rows[0][3],
			&rows[1][3],
			&rows[2][3],
			&rows[3][3],
		};
		return *Elements[i];
	}

	//	gr: may need a more float-error friendly version
	bool					IsIdentity() const		{	return *this == vec4x4();	}
	
	FixedRemoteArray<TYPE>	GetArray() const 
	{
		auto* m00 = const_cast<TYPE*>(&rows[0][0]);
		return FixedRemoteArray<TYPE>(m00, 4 * 4);
	}

	bool	operator==(const vec4x4& that) const
	{
		return
		rows[0]==that.rows[0] &&
		rows[1]==that.rows[1] &&
		rows[2]==that.rows[2] &&
		rows[3]==that.rows[3];
	}
	
public:
	vec4x<TYPE>	rows[4];
};


template<typename TYPE>
class vec3x3
{
public:
	vec3x3() :
		vec3x3(1, 0, 0, 0, 1, 0, 0, 0, 1)
	{
	}
	vec3x3(TYPE a,TYPE b,TYPE c,
		   TYPE d,TYPE e,TYPE f,
		   TYPE g,TYPE h,TYPE i)
#if !defined(OLD_VISUAL_STUDIO)
	:
	m	{	a,b,c,d,e,f,g,h,i	}
#endif
	{
		#if defined(OLD_VISUAL_STUDIO)
		m[0] = a;
		m[1] = b;
		m[2] = c;
		m[3] = d;
		m[4] = e;
		m[5] = f;
		m[6] = g;
		m[7] = h;
		m[8] = i;
#endif
	}
	
	vec3x<TYPE>	GetRow(size_t r) const
	{
		return vec3x<TYPE>( m[(r*3)+0], m[(r*3)+1], m[(r*3)+2] );
	}
	
	const TYPE&	operator()(size_t c,size_t r) const
	{
		return m[(r*3)+c];
	}
	
	TYPE&	operator()(size_t c,size_t r)
	{
		return m[(r*3)+c];
	}
	
	const TYPE&	operator[](size_t i) const
	{
		return m[i];
	}
	
	//	gr: may need a more float-error friendly version
	bool	IsIdentity() const		{	return *this == vec3x3();	}
	
	bool	operator==(const vec3x3& that) const
	{
		return
		m[0]==that.m[0] &&
		m[1]==that.m[1] &&
		m[2]==that.m[2] &&
		m[3]==that.m[3] &&
		m[4]==that.m[4] &&
		m[5]==that.m[5] &&
		m[6]==that.m[6] &&
		m[7]==that.m[7] &&
		m[8]==that.m[8];
	}
	bool	operator!=(const vec3x3& that) const
	{
		return
		m[0]!=that.m[0] ||
		m[1]!=that.m[1] ||
		m[2]!=that.m[2] ||
		m[3]!=that.m[3] ||
		m[4]!=that.m[4] ||
		m[5]!=that.m[5] ||
		m[6]!=that.m[6] ||
		m[7]!=that.m[7] ||
		m[8]!=that.m[8];
	}
	
public:
	TYPE	m[3*3];
};

//	gr: this should be renamed to RectT, not X (X should be dimensions)
template<typename TYPE>
class Soy::Rectx
{
public:
	Rectx() :
		x	( 0 ),
		y	( 0 ),
		w	( 0 ),
		h	( 0 )
	{
	}
	
	Rectx(TYPE _x,TYPE _y,TYPE _w,TYPE _h) :
		x	(_x),
		y	(_y),
		w	(_w),
		h	(_h)
	{
	}
	
	template<typename OTHERTYPE>
	Rectx(const Rectx<OTHERTYPE>& r) :
		x	(r.x),
		y	(r.y),
		w	(r.w),
		h	(r.h)
	{
	}
	
	TYPE	Left() const		{	return x;	}
	TYPE	Right() const		{	return x+w;	}
	TYPE	Top() const			{	return y;	}
	TYPE	Bottom() const		{	return y+h;	}
	TYPE	GetWidth() const	{	return w;	}
	TYPE	GetHeight() const	{	return h;	}
	TYPE	GetCenterX() const	{	return x + (w/2.0f);	}
	TYPE	GetCenterY() const	{	return y + (h/2.0f);	}
	vec2x<TYPE>	GetCenter() const	{	return vec2x<TYPE>( GetCenterX(), GetCenterY() );	}
	vec4x<TYPE>	GetVec4() const	{	return vec4x<TYPE>(x,y,w,h);	}
	void	ScaleTo(const Rectx& Parent);		//	assume this is normalised
	void	Normalise(const Rectx& Parent);		//	make this the normalised
	void	Accumulate(TYPE x,TYPE y);
	
	bool	operator==(const Rectx& that) const	{	return (x==that.x) && (y==that.y) && (w==that.w) && (h==that.h);	}
	bool	operator!=(const Rectx& that) const	{	return !(*this == that);	}
	
	
public:
	TYPE	x;
	TYPE	y;
	TYPE	w;
	TYPE	h;
};



template<typename TYPE>
class Soy::Boundsx
{
public:
	Boundsx()
	{
	}
	Boundsx(TYPE Minx,TYPE MinY,TYPE MaxX,TYPE MaxY) :
		min	( Minx, MinY ),
		max	( MaxX, MaxY )
	{
	}
	Boundsx(const TYPE& _min,const TYPE& _max) :
		min	(_min),
		max	(_max)
	{
	}
	template<typename OTHERTYPE>
	Boundsx(const Boundsx<OTHERTYPE>& r) :
		min	(r.min),
		max	(r.max)
	{
	}
	
	const TYPE	Min() const		{	return min;	}
	const TYPE	Max() const		{	return min;	}
	const TYPE	Size() const	{	return max-min;	}
	
public:
	TYPE	min;
	TYPE	max;
};






//	gr: rename these types float2, float3, float4
typedef vec2x<float> vec2f;
typedef vec3x<float> vec3f;
typedef vec4x<float> vec4f;
typedef vec4x4<float> float4x4;
typedef vec3x3<float> float3x3;

namespace Soy
{
	typedef Rectx<float> Rectf;
	typedef Boundsx<vec3f> Bounds3f;
};

DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec2f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec3f );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( vec4f );
DECLARE_NONCOMPLEX_TYPE( float4x4 );
DECLARE_NONCOMPLEX_TYPE( Soy::Rectf );
DECLARE_NONCOMPLEX_TYPE( Soy::Bounds3f );




namespace Soy
{
	inline vec4f	RectToVector(const Rectf& v)	{	return vec4f( v.x, v.y, v.w, v.h );	}

	float3x3		MatrixToVector(const CGAffineTransform& Transform,vec2f TransformNormalisation);
};



#if defined(__OBJC__) && defined(TARGET_OSX)
inline Soy::Rectf NSRectToRect(NSRect Rect)
{
	return Soy::Rectf( Rect.origin.x, Rect.origin.y, Rect.size.width, Rect.size.height );
}
#endif


template<typename TYPE>
inline void Soy::Rectx<TYPE>::ScaleTo(const Rectx& Parent)
{
	auto Lerp = [](float Min,float Max,float Time)
	{
		return Min + ( Time * (Max-Min) );
	};

	auto& Normalised = *this;
	auto l = Lerp( Parent.Left(), Parent.Right(), Normalised.Left() );
	auto r = Lerp( Parent.Left(), Parent.Right(), Normalised.Right() );
	auto t = Lerp( Parent.Top(), Parent.Bottom(), Normalised.Top() );
	auto b = Lerp( Parent.Top(), Parent.Bottom(), Normalised.Bottom() );
	auto w = r-l;
	auto h = b-t;
	Normalised = Soy::Rectf( l, t, w, h );
}

template<typename TYPE>
inline void Soy::Rectx<TYPE>::Normalise(const Rectx& Parent)
{
	auto Range = [](float Min, float Max, float Value )
	{
		return (Value-Min) / (Max-Min);
	};

	auto& Child = *this;
	auto l = Range( Parent.Left(), Parent.Right(), Child.Left() );
	auto r = Range( Parent.Left(), Parent.Right(), Child.Right() );
	auto t = Range( Parent.Top(), Parent.Bottom(), Child.Top() );
	auto b = Range( Parent.Top(), Parent.Bottom(), Child.Bottom() );
	auto w = r-l;
	auto h = b-t;
	Child = Soy::Rectf( l, t, w, h );
}


template<typename TYPE>
inline void Soy::Rectx<TYPE>::Accumulate(TYPE Newx,TYPE Newy)
{
	{
		auto r = x + w;
		if ( Newx < x )
		{
			w = r - Newx;
			x = Newx;
		}
		else if ( Newx > r )
		{
			w = Newx - x;
		}
	}
	{
		auto b = y + h;
		if ( Newy < y )
		{
			h = b - Newy;
			y = Newy;
		}
		else if ( Newy > b )
		{
			h = Newy - y;
		}
	}
}


template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out, const vec2x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y;
	return out;
}

template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out, const vec3x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.z;
	return out;
}


template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out, const vec4x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.z << Soy::VecNXDelins[0] << in.w;
	return out;
}

namespace Soy
{
	template<typename TYPE>
	inline std::ostream& operator<<(std::ostream &out, const Soy::Rectx<TYPE> &in)
	{
		out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.w << Soy::VecNXDelins[0] << in.h;
		return out;
	}
}


namespace Soy
{
	template<typename TYPE>
	inline std::ostream& operator<<(std::ostream &out, const Soy::Boundsx<TYPE> &in)
	{
		out << in.min << Soy::VecNXDelins[1] << in.max;
		return out;
	}
}


inline std::ostream& operator<<(std::ostream &out, const float4x4&in)
{
	out << in.rows[0] << Soy::VecNXDelins[0]
		<< in.rows[1] << Soy::VecNXDelins[0]
		<< in.rows[2] << Soy::VecNXDelins[0]
		<< in.rows[3];
	return out;
}

inline std::ostream& operator<<(std::ostream &out, const float3x3&in)
{
	out << in.GetRow(0) << Soy::VecNXDelins[0]
		<< in.GetRow(1) << Soy::VecNXDelins[0]
		<< in.GetRow(2);
	return out;
}
