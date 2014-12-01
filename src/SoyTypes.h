#pragma once

#if defined(_MSC_VER)
#define TARGET_WINDOWS
#endif


#if !defined(NO_OPENFRAMEWORKS)

#include <ofMain.h>
#include <assert.h>
#include <type_traits>

typedef ofVec2f				vec2f;
typedef ofVec3f				vec3f;
typedef ofVec4f				vec4f;

typedef ofColor ofColour;
inline bool operator==(const ofColor& a,const ofColor& b)
{
	return (a.r==b.r) && (a.g==b.g) && (a.b==b.b) && (a.a==b.a);
}

#else


//	see ofConstants
#define WIN32_LEAN_AND_MEAN

#if (_MSC_VER)
	#define NOMINMAX		
	//http://stackoverflow.com/questions/1904635/warning-c4003-and-errors-c2589-and-c2059-on-x-stdnumeric-limitsintmax
#endif

#if defined(TARGET_WINDOWS)

#include <windows.h>
#include <process.h>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <mmsystem.h>
#include <memory>
#ifdef _MSC_VER
	#include <direct.h>
#endif
#pragma comment(lib,"winmm.lib")


#elif defined(TARGET_OSX)

#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <memory>
#include <stdint.h>

#define MAX_PATH    256

#endif

#include <string>
#include <assert.h>
#include <type_traits>
#include <sstream>
namespace std
{
#define DISALLOW_EVIL_CONSTRUCTORS(x)
//#include "chromium/stack_container.h"
};

//	openframeworks functions
inline unsigned long long	ofGetSystemTime()
{
#if defined(TARGET_WINDOWS)
	return timeGetTime();
#elif defined(TARGET_OSX)
    struct timeval now;
    gettimeofday( &now, NULL );
    return
    (unsigned long long) now.tv_usec/1000 +
    (unsigned long long) now.tv_sec*1000;
#endif
}
inline unsigned long long	ofGetElapsedTimeMillis()	{	return ofGetSystemTime();	}	//	gr: offrameworks does -StartTime
inline float				ofGetElapsedTimef()			{	return static_cast<float>(ofGetElapsedTimeMillis()) / 1000.f;	}


void					ofLogNotice(const std::string& Message);
void					ofLogWarning(const std::string& Message);
void					ofLogError(const std::string& Message);
std::string				ofToString(int Integer);

//	gr: repalce uses of this with SoyTime
namespace Poco
{
	class Timestamp
	{
	public:
		Timestamp(int Value=0)
		{
		}
		inline bool		operator==(const int v) const			{	return false;	}
		inline bool		operator==(const Timestamp& t) const	{	return false;	}
		inline bool		operator!=(const Timestamp& t) const	{	return false;	}
	};
	class File
	{
	public:
		File(const char* Filename)	{}
		File(const std::string& Filename)	{}
		bool		exists() const	{	return false;	}
		Timestamp	getLastModified() const	{	return Timestamp();	}
	};
};

class ofFilePath
{
public:
	static std::string		getFileName(const std::string& Filename,bool bRelativeToData=true);
};

template<typename TYPE>
class ofEvent
{
public:
};

template<typename TYPE,typename ARG>
void ofNotifyEvent(ofEvent<TYPE>& Event,ARG& Arg)
{
}

class ofTexture
{
public:
	int		getWidth() const	{	return 0;	}
	int		getHeight() const	{	return 0;	}
};

//----------------------------------------------------------
// ofPtr
//----------------------------------------------------------
template<typename T>
using ofPtr = std::shared_ptr<T>;

inline std::string ofToDataPath(const std::string& LocalPath,bool FullPath=false)	{	return LocalPath;	}


#endif


#define sizeofarray(ARRAY)	( sizeof(ARRAY)/sizeof((ARRAY)[0]) )

// Attribute to make function be exported from a plugin
#if defined(TARGET_WINDOWS)
	#define STDCALL		__stdcall
	#define EXPORT_API	__declspec(dllexport)
#elif defined(TARGET_OSX)
	#define STDCALL		
	#define EXPORT_API
#endif

typedef	signed char			int8;
typedef	unsigned char		uint8;
typedef	signed short		int16;
typedef	unsigned short		uint16;
#if defined(TARGET_WINDOWS)
typedef signed __int32		int32;
typedef unsigned __int32	uint32;
typedef signed __int64		int64;
typedef unsigned __int64	uint64;
#else
typedef int32_t             int32;
typedef uint32_t             uint32;
typedef int64_t             int64;
typedef uint64_t             uint64;
#endif


typedef void(*ofDebugPrintFunc)(const std::string&);


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
T ofLimit(const T& v,const T& Min,const T& Max)
{
	assert( Min <= Max );
	if ( v < Min )
		return Min;
	else if ( v > Max )
		return Max;
	else
		return v;
}

template<typename T>
void ofSwap(T& a,T& b)
{
	T Temp = a;
	a = b;
	b = Temp;
}

template<typename T>
T ofLerp(const T& start,const T& stop, float amt)
{
	return start + ((stop-start) * amt);
}


inline float ofGetMathTime(float z,float Min,float Max) 
{
	return (z-Min) / (Max-Min);	
}


namespace Soy
{
	//	speed up large array usage with non-complex types (structs, floats, etc) by overriding this template function (or use the macro)
	template<typename TYPE>
	inline bool IsComplexType()	
	{
		//	default complex for classes
		return true;	
	}

	//	speed up allocations of large arrays of non-complex types by skipping construction& destruction (placement new, means data is always uninitialised)
	template<typename TYPE>
	inline bool DoConstructType()	
	{
		//	by default we want to construct classes, structs etc. 
		//	Only types with no constructor we don't want constructed for speed reallys
		return true;	
	}

	template<typename A,typename B>
	inline bool DoComplexCopy()
	{
		if ( std::is_same<A,B>::value )
			return IsComplexType<A>();
		else
			return true;
	}
	
	std::string		DemangleTypeName(const char* name);
	
	//	readable name for a type (alternative to RTTI)
	template<typename TYPE>
	inline std::string GetTypeName()
	{
		return DemangleTypeName( typeid(TYPE).name() );
	}

	//	auto-define the name for this type for use in the memory debug
	#define DECLARE_TYPE_NAME(TYPE)								\
		template<>															\
		inline std::string Soy::GetTypeName<TYPE>()	{	return #TYPE ;	}
	
	#define DECLARE_TYPE_NAME_AS(TYPE,NAME)								\
		template<>															\
		inline std::string Soy::GetTypeName<TYPE>()	{	return NAME ;	}
	
	//	speed up use of this type in our arrays when resizing, allocating etc
	//	declare a type that can be memcpy'd (ie. no pointers or ref-counted objects that rely on =operators or copy constructors)
	#define DECLARE_NONCOMPLEX_TYPE(TYPE)								\
		DECLARE_TYPE_NAME(TYPE)												\
		template<>															\
		inline bool Soy::IsComplexType<TYPE>()	{	return false;	}		
	
	//	speed up allocation of this type in our heaps...
	//	declare a non-complex type that also requires NO construction (ie, will be memcpy'd over or fully initialised when required)
	#define DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE(TYPE)					\
		DECLARE_NONCOMPLEX_TYPE(TYPE)										\
		template<>															\
		inline bool Soy::DoConstructType<TYPE>()	{	return false;	}	
	
	
} // namespace Soy


//	some generic noncomplex types 
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( float );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( char );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int8 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint8 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int16 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint16 );
//DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int32 );	//	int
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint32 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int64 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint64 );

DECLARE_TYPE_NAME_AS( std::string, "text" );



class TCrc32
{
public:
	static const uint32_t	Crc32Table[256];

public:
    TCrc32() { Reset(); }
    ~TCrc32() throw() {}
    void Reset() { _crc = (uint32_t)~0; }
    void AddData(const uint8_t* pData, const uint32_t length)
    {
        uint8_t* pCur = (uint8_t*)pData;
        uint32_t remaining = length;
        for (; remaining--; ++pCur)
            _crc = ( _crc >> 8 ) ^ Crc32Table[(_crc ^ *pCur) & 0xff];
    }
    const uint32_t GetCrc32() { return ~_crc; }

private:
    uint32_t _crc;
};

namespace Soy
{
	namespace Platform
	{
		int					GetLastError();
		std::string			GetErrorString(int Error);
		inline std::string	GetLastErrorString()	{	return GetErrorString( GetLastError() );	}
	}
};

template<typename TYPE>
class ArrayBridge;

namespace Soy
{
	bool	LoadBinaryFile(ArrayBridge<char>& Data,std::string Filename,std::stringstream& Error);
	bool	ReadStream(ArrayBridge<char>& Data, std::istream& Stream, std::stringstream& Error);
	bool	ReadStream(ArrayBridge<char>&& Data, std::istream& Stream, std::stringstream& Error);
	bool	ReadStreamChunk( ArrayBridge<char>& Data, std::istream& Stream );
	bool	StringToFile(std::string Filename,std::string String);
	bool	FileToString(std::string Filename,std::string& String);
}


namespace Soy
{
	//	http://www.adp-gmbh.ch/cpp/common/base64.html
	void		base64_encode(ArrayBridge<char>& Encoded,const ArrayBridge<char>& Decoded);
	void		base64_decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>& Decoded);
};

namespace Soy
{
	class AssertException;
};


class Soy::AssertException : public std::exception
{
public:
	AssertException(std::string Message) :
	mError	( Message )
	{
	}
	virtual const char* what() const _NOEXCEPT	{	return mError.c_str();	}
std::string			mError;
};

namespace Soy
{
#pragma warning(disable:4290)
	//	replace asserts with exception. If condition fails false is returned to save code
	bool		Assert(bool Condition,std::string ErrorMessage) throw(AssertException);
	inline bool	Assert(bool Condition, std::stringstream&& ErrorMessage ) throw( AssertException )
	{
		return Assert( Condition, ErrorMessage.str() );
	}
	inline bool	Assert(bool Condition, std::stringstream& ErrorMessage ) throw( AssertException )
	{
		return Assert( Condition, ErrorMessage.str() );
	}
	bool	Assert(bool Condition, std::ostream& ErrorMessage ) throw( AssertException );
};



class vec2f
{
public:
	vec2f(float _x,float _y) :
	x	( _x ),
	y	( _y )
	{
	}
	vec2f() :
	x	( 0 ),
	y	( 0 )
	{
	}
	
	float	LengthSq() const	{	return (x*x)+(y*y);	}
	float	Length() const		{	return sqrtf( LengthSq() );	}
	
	vec2f&	operator*=(const float& Scalar)	{	x*=Scalar;		y*=Scalar;	return *this;	}
	vec2f&	operator*=(const vec2f& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;	return *this;	}
	
public:
	float	x;
	float	y;
};


class vec3f
{
public:
	vec3f(float _x,float _y,float _z) :
	x	( _x ),
	y	( _y ),
	z	( _z )
	{
	}
	vec3f() :
	x	( 0 ),
	y	( 0 ),
	z	( 0 )
	{
	}
	
	float	LengthSq() const	{	return (x*x)+(y*y)+(z*z);	}
	float	Length() const		{	return sqrtf( LengthSq() );	}

	vec3f&	operator*=(const float& Scalar)	{	x*=Scalar;		y*=Scalar;		z*=Scalar;	return *this;	}
	vec3f&	operator*=(const vec3f& Scalar)	{	x*=Scalar.x;	y*=Scalar.y;		z*=Scalar.z;	return *this;	}
	
public:
	float	x;
	float	y;
	float	z;
};

