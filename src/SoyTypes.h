#pragma once


#if !defined(NO_OPENFRAMEWORKS)

#include <ofMain.h>
#include <assert.h>
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
//this is for TryEnterCriticalSection
//http://www.zeroc.com/forums/help-center/351-ice-1-2-tryentercriticalsection-problem.html
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x400
#endif
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
#ifdef _MSC_VER
	#include <direct.h>
#endif
#pragma comment(lib,"winmm.lib")

#elif defined(TARGET_OSX)

#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <stdint.h>

#endif


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


void					ofLogNotice(const std::string& Message)		{	ofLogNotice( Message.c_str() );	}
void					ofLogWarning(const std::string& Message)	{	ofLogWarning( Message.c_str() );	}
void					ofLogError(const std::string& Message)		{	ofLogError( Message.c_str() );	}
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
std::string			ofBufferFromFile(const char* Filename);
inline std::string	ofBufferFromFile(const std::string& Filename)	{	return ofBufferFromFile( Filename.c_str() );	}

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
#if defined(TARGET_WINDOWS)//gr: depends on MSCV version
#define std_experimental    std::tr1
#else
#define std_experimental    std
#endif
template <typename T>
class ofPtr: public std_experimental::shared_ptr<T>
{
public:

	ofPtr()
	  : std_experimental::shared_ptr<T>() { }

	  template<typename Tp1>
		explicit
		ofPtr(Tp1* __p)
	: std_experimental::shared_ptr<T>(__p) { }

	  template<typename Tp1, typename _Deleter>
		ofPtr(Tp1* __p, _Deleter __d)
	: std_experimental::shared_ptr<T>(__p, __d) { }

	  template<typename Tp1, typename _Deleter, typename _Alloc>
		ofPtr(Tp1* __p, _Deleter __d, const _Alloc& __a)
	: std_experimental::shared_ptr<T>(__p, __d, __a) { }

	  // Aliasing constructor
	  template<typename Tp1>
		ofPtr(const ofPtr<Tp1>& __r, T* __p)
	: std_experimental::shared_ptr<T>(__r, __p) { }

	  template<typename Tp1>
		ofPtr(const ofPtr<Tp1>& __r)
	: std_experimental::shared_ptr<T>(__r) { }

	  /*ofPtr(ofPtr&& __r)
	  : std_experimental::shared_ptr<T>(std::move(__r)) { }

	  template<typename Tp1>
		ofPtr(ofPtr<Tp1>&& __r)
		: std_experimental::shared_ptr<T>(std::move(__r)) { }*/

	  template<typename Tp1>
		explicit
		ofPtr(const std_experimental::weak_ptr<Tp1>& __r)
	: std_experimental::shared_ptr<T>(__r) { }

	// tgfrerer: extends ofPtr facade to allow dynamic_pointer_cast, pt.1
#if (_MSC_VER)
	template<typename Tp1>
	ofPtr(const ofPtr<Tp1>& __r, std_experimental::_Dynamic_tag)
	: std::tr1::shared_ptr<T>(__r, std_experimental::_Dynamic_tag()) { }
#elif !defined(TARGET_OSX)
	template<typename Tp1>
	ofPtr(const ofPtr<Tp1>& __r, std::__dynamic_cast_tag)
	: std_experimental::shared_ptr<T>(__r, std::__dynamic_cast_tag()) { }
#endif
	  /*template<typename Tp1, typename Del>
		explicit
		ofPtr(const std::tr1::unique_ptr<Tp1, Del>&) = delete;

	  template<typename Tp1, typename Del>
		explicit
		ofPtr(std::tr1::unique_ptr<Tp1, Del>&& __r)
	: std::tr1::shared_ptr<T>(std::move(__r)) { }*/
};

// tgfrerer: extends ofPtr facade to allow dynamic_pointer_cast, pt. 2
#if (_MSC_VER)
template<typename _Tp, typename _Tp1>
ofPtr<_Tp>
	dynamic_pointer_cast(const ofPtr<_Tp1>& __r)
{ return ofPtr<_Tp>(__r, std_experimental::_Dynamic_tag()); }
#elif !defined(TARGET_OSX)
template<typename _Tp, typename _Tp1>
ofPtr<_Tp>
	dynamic_pointer_cast(const ofPtr<_Tp1>& __r)
{ return ofPtr<_Tp>(__r, std_experimental::__dynamic_cast_tag()); }
#endif

inline std::string ofToDataPath(const std::string& LocalPath,bool FullPath=false)	{	return LocalPath;	}


#endif


#define sizeofarray(ARRAY)	( sizeof(ARRAY)/sizeof((ARRAY)[0]) )

// Attribute to make function be exported from a plugin
#define EXPORT_API __declspec(dllexport)

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


inline Poco::Timestamp ofFileLastModified(const char* Path)
{
	std::string FullPath = ofToDataPath( Path );
	Poco::File File( FullPath );
	if ( !File.exists() )
		return Poco::Timestamp(0);

	return File.getLastModified();
}

inline bool ofFileExists(const char* Path)
{
	Poco::Timestamp LastModified = ofFileLastModified( Path );
	if ( LastModified == 0 )
		return false;
	return true;
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

	//	readable name for a type (alternative to RTTI)
	template<typename TYPE>
	inline const char* GetTypeName()	
	{
		//	"unregistered" type, return the size as name
        //  gr: OSX/gcc template can't use BufferString without declaration
#if defined(TARGET_OSX)
        static const char* Name = "Non-soy-declared type";
#else
		static BufferString<30> Name;
		if ( Name.IsEmpty() )
			Name << sizeof(TYPE) << "ByteType";
#endif
		return Name;
	}

	//	auto-define the name for this type for use in the memory debug
	#define DECLARE_TYPE_NAME(TYPE)								\
		template<>															\
		inline const char* Soy::GetTypeName<TYPE>()	{	return #TYPE ;	}

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







