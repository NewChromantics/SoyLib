#pragma once


//	array forward declaration
template<typename TYPE>
class ArrayInterface;


#if defined(_MSC_VER)
#define TARGET_WINDOWS
#endif



#if !defined(NO_OPENFRAMEWORKS)

#include <ofMain.h>
#include <assert.h>
#include <type_traits>

typedef ofColor ofColour;
inline bool operator==(const ofColor& a,const ofColor& b)
{
	return (a.r==b.r) && (a.g==b.g) && (a.b==b.b) && (a.a==b.a);
}

#else





#if defined(TARGET_WINDOWS)

//	see ofConstants
#define WIN32_LEAN_AND_MEAN

#if (_MSC_VER)
#define NOMINMAX		
//http://stackoverflow.com/questions/1904635/warning-c4003-and-errors-c2589-and-c2059-on-x-stdnumeric-limitsintmax
#endif

#include <windows.h>
#include <process.h>
#include <vector>
#include <mmsystem.h>
#ifdef _MSC_VER
	#include <direct.h>
#endif
#pragma comment(lib,"winmm.lib")


#define __func__ __FUNCTION__
#define __thread __declspec( thread )
// Attribute to make function be exported from a plugin
#define STDCALL		__stdcall
#define EXPORT_API	__declspec(dllexport)

#elif defined(TARGET_OSX)

#include <sys/time.h>
#include <math.h>

#define MAX_PATH    256
#define STDCALL		
#define EXPORT_API

#elif defined(TARGET_ANDROID)


#endif

//	clang(?) macro for testing features missing on android
#if !defined(__has_feature)
#define __has_feature(x)	FALSE
#endif



//	all platforms
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <type_traits>
#include <sstream>



typedef	signed char			int8;
typedef	unsigned char		uint8;
typedef	signed short		int16;
typedef	unsigned short		uint16;
#if defined(TARGET_WINDOWS)
typedef signed __int32		int32;
typedef unsigned __int32	uint32;
typedef signed __int64		int64;
typedef unsigned __int64	uint64;
typedef SSIZE_T				ssize_t;
#else
typedef int32_t             int32;
typedef uint32_t             uint32;
typedef int64_t             int64;
typedef uint64_t             uint64;
#endif

//	when casting integers down, get rid of warnings using this, so we can add a check later if it EVER comes up as a problem
template<typename SMALLSIZE,typename BIGSIZE>
inline SMALLSIZE size_cast(BIGSIZE Size)
{
	//	gr: do value check
	return static_cast<SMALLSIZE>( Size );
}

namespace std
{
#define DISALLOW_EVIL_CONSTRUCTORS(x)
//#include "chromium/stack_container.h"
};
#define sizeofarray(ARRAY)	( sizeof(ARRAY)/sizeof((ARRAY)[0]) )

//	set a standard RTTI macro
#if defined(__cpp_rtti) || defined(GCC_ENABLE_CPP_RTTI) || __has_feature(cxx_rtti)
#define ENABLE_RTTI
#endif

#if !defined(_NOEXCEPT)
#define _NOEXCEPT
//	_GLIBCXX_USE_NOEXCEPT is something
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
#if !defined(ENABLE_RTTI)
	std::string		AllocTypeName();
#endif
	
	
	//	readable name for a type (alternative to RTTI) which means we don't need to use DECLARE_TYPE_NAME
	template<typename TYPE>
	inline const std::string& GetTypeName()
	{
#if defined(ENABLE_RTTI)
		static std::string TypeName = DemangleTypeName( typeid(TYPE).name() );
#else
		static std::string TypeName = AllocTypeName();
#endif
		return TypeName;
	}

	template<typename TYPE>
	inline const std::string& GetTypeName(const TYPE& Object)
	{
		return GetTypeName<TYPE>();		
	}
	

	
	//	auto-define the name for this type for use in the memory debug
#define DECLARE_TYPE_NAME(TYPE)	\
	namespace Soy \
	{	\
		template<>		\
		inline const std::string& GetTypeName<TYPE>()	{	static std::string TypeName = #TYPE;	return TypeName;	} \
	};
	
#define DECLARE_TYPE_NAME_AS(TYPE,NAME)	\
	namespace Soy \
	{	\
		template<>	\
		inline const std::string& GetTypeName<TYPE>()	{	static std::string TypeName = NAME;	return TypeName ;	} \
	};
	
	//	speed up use of this type in our arrays when resizing, allocating etc
	//	declare a type that can be memcpy'd (ie. no pointers or ref-counted objects that rely on =operators or copy constructors)
#define DECLARE_NONCOMPLEX_TYPE(TYPE)	\
	DECLARE_TYPE_NAME(TYPE)	\
	namespace Soy \
	{	\
		template<>	\
		inline bool IsComplexType<TYPE>()	{	return false;	} \
	};
	
	//	speed up allocation of this type in our heaps...
	//	declare a non-complex type that also requires NO construction (ie, will be memcpy'd over or fully initialised when required)
#define DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE(TYPE)	\
	DECLARE_NONCOMPLEX_TYPE(TYPE)	\
	namespace Soy \
	{	\
		template<>	\
		inline bool DoConstructType<TYPE>()	{	return false;	}	\
	};
	

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
    void AddData(const uint8_t* pData, const size_t length)
    {
        uint8_t* pCur = (uint8_t*)pData;
        auto remaining = length;
        for (; remaining--; ++pCur)
            _crc = ( _crc >> 8 ) ^ Crc32Table[(_crc ^ *pCur) & 0xff];
    }
    const uint32_t GetCrc32() { return ~_crc; }

private:
    uint32_t _crc;
};


namespace Soy
{
	namespace Private
	{
		uint32	GetCrc32(const char* Data,size_t DataSize);
	};
	
	template<typename TYPE>
	inline uint32	GetCrc32(const ArrayInterface<TYPE>& Array)
	{
		return Private::GetCrc32( Array.GetArray(), Array.GetDataSize() );
	}

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
	bool		FileToArray(ArrayBridge<char>& Data,std::string Filename,std::stringstream& Error);
	inline bool	FileToArray(ArrayBridge<char>&& Data,std::string Filename,std::stringstream& Error)	{	return FileToArray( Data, Filename, Error );	}
	inline bool	LoadBinaryFile(ArrayBridge<char>& Data,std::string Filename,std::stringstream& Error)	{	return FileToArray( Data, Filename, Error );	}
	bool		ReadStream(ArrayBridge<char>& Data, std::istream& Stream, std::stringstream& Error);
	bool		ReadStream(ArrayBridge<char>&& Data, std::istream& Stream, std::stringstream& Error);
	bool		ReadStreamChunk( ArrayBridge<char>& Data, std::istream& Stream );
	inline bool	ReadStreamChunk( ArrayBridge<char>&& Data, std::istream& Stream )	{	return ReadStreamChunk( Data, Stream );		}
	bool		StringToFile(std::string Filename,std::string String);
	bool		FileToString(std::string Filename,std::string& String);
	bool		FileToString(std::string Filename,std::string& String,std::stringstream& Error);
	bool		FileToStringLines(std::string Filename,ArrayBridge<std::string>& StringLines,std::stringstream& Error);
	inline bool	FileToStringLines(std::string Filename,ArrayBridge<std::string>&& StringLines,std::stringstream& Error)	{	return FileToStringLines( Filename, StringLines, Error );	}
}


namespace Soy
{
	//	http://www.adp-gmbh.ch/cpp/common/base64.html
	void		base64_encode(ArrayBridge<char>& Encoded,const ArrayBridge<char>& Decoded);
	void		base64_decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>& Decoded);
};

