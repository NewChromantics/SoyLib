#pragma once


//	earlier as used by some TARGET_X specific smart pointers
struct NonCopyable {
	NonCopyable & operator=(const NonCopyable&) = delete;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable() = default;
};


#if defined(TARGET_WINDOWS)
	#error Compiler now defines TARGET_WINDOWS
#endif
#if defined(_MSC_VER) && !defined(TARGET_PS4)
	#if defined(TARGET_UWP)
		#define TARGET_WINDOWS	TARGET_UWP
	#elif defined(_WIN64)
		#define TARGET_WINDOWS 64
	#else
		#define TARGET_WINDOWS 32
	#endif
#endif

#if defined(TARGET_IOS)
#include "SoyTypes_CoreFoundation.h"
#elif defined(TARGET_ANDROID)
#include "SoyTypes_Android.h"
#elif defined(TARGET_OSX)
#include "SoyTypes_CoreFoundation.h"
#elif defined(TARGET_WINDOWS)
#include "SoyTypes_Windows.h"
#elif defined(TARGET_PS4)
#include "SoyTypes_Ps4.h"
#else
#error no TARGET_XXX defined
#endif


typedef	int8_t		sint8;
typedef	uint8_t		uint8;
typedef	int16_t		sint16;
typedef	uint16_t	uint16;
typedef	int32_t		sint32;
typedef	uint32_t	uint32;
typedef	int64_t		sint64;
typedef	uint64_t	uint64;


//	all platforms
#include <stdio.h>
#include <assert.h>
#include <memory>
#include <string>
#include <type_traits>
#include <sstream>


//	clang(?) macro for testing features missing on android
#if !defined(__has_feature)
#define __has_feature(x)	FALSE
#endif


//	forward declarations
namespace Soy
{
	class TVersion;
}

//	array forward declaration
template<typename TYPE>
class ArrayInterface;


namespace Soy
{
	void		EndianSwap(uint32_t& Value);
}

namespace Soy
{
	void	SizeAssert_TooBig(uint64 Value,uint64 Max,const std::string& SmallType,const std::string& BigType);
	void	SizeAssert_TooSmall(sint64 Value,sint64 Min,const std::string& SmallType,const std::string& BigType);
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
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( double );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( int );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( char );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( sint8 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint8 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( sint16 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint16 );
//DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( sint32 );	//	int
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint32 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( sint64 );
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( uint64 );

DECLARE_TYPE_NAME_AS( std::string, "text" );


//	gr: alternative
//	http://codereview.stackexchange.com/questions/5515/c-int-cast-function-for-checked-casts
//	when casting integers down, get rid of warnings using this, so we can add a check later if it EVER comes up as a problem
template<typename SMALLSIZE,typename BIGSIZE>
inline SMALLSIZE size_cast(BIGSIZE Size)
{
#if defined(TARGET_ANDROID)
	
#else
	auto Min = std::numeric_limits<SMALLSIZE>::min();
	auto Max = std::numeric_limits<SMALLSIZE>::max();
	if ( Size > Max )
	{
		Soy::SizeAssert_TooBig( Size, Max, Soy::GetTypeName<SMALLSIZE>(), Soy::GetTypeName<BIGSIZE>() );
	}
	//	note: be careful about left & right, left is cast to right
	if ( static_cast<ssize_t>(Size) < static_cast<ssize_t>(Min) )
	{
		Soy::SizeAssert_TooSmall( Size, Min, Soy::GetTypeName<SMALLSIZE>(), Soy::GetTypeName<BIGSIZE>() );
	}
#endif
	return static_cast<SMALLSIZE>( Size );
}


//	remove annoying warning C4800 in windows without turning it off
template<typename TYPE>
inline bool		bool_cast(const TYPE& Value)
{
	return Value != 0;
}




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


class Soy::TVersion
{
public:
	TVersion(size_t Major=0,size_t Minor=0,size_t Patch=0) :
	mMajor	( Major ),
	mMinor	( Minor ),
	mPatch	( Patch )
	{
	}
	explicit TVersion(std::string VersionStr,const std::string& Prefix="");
	
	//	gr: throw if the minor is going to overflow
	size_t	GetHundred() const;
	size_t	GetMillion() const;	//	int representation xxx.xxx.xxx
	
	bool	operator<(const TVersion& that) const	{	return GetMillion() < that.GetMillion();	}
	bool	operator<=(const TVersion& that) const	{	return GetMillion() <= that.GetMillion();	}
	bool	operator>(const TVersion& that) const	{	return GetMillion() > that.GetMillion();	}
	bool	operator>=(const TVersion& that) const	{	return GetMillion() >= that.GetMillion();	}
	bool	operator==(const TVersion& that) const	{	return GetMillion() == that.GetMillion();	}
	bool	operator!=(const TVersion& that) const	{	return GetMillion() != that.GetMillion();	}
	
public:
	size_t	mMajor = 0;
	size_t	mMinor = 0;
	size_t	mPatch = 0;
};

namespace Soy
{
	inline std::ostream& operator<<(std::ostream &out,const Soy::TVersion& in)
	{
		//	gr: this may want to include patch, but maybe some things depend on it?
		out << in.mMajor << '.' << in.mMinor << '.' << in.mPatch;
		return out;
	}
}





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


class SoyThread;


namespace Platform
{
	bool				Init();

	void				FlushLastError();
	int					GetLastError(bool Flush=true);	//	gr: default on OSX was flushing for winsock...
	std::string			GetErrorString(int Error);
#if defined(TARGET_WINDOWS)
	std::string			GetErrorString(HRESULT Error);
#endif
	inline std::string	GetLastErrorString()	{	return GetErrorString( GetLastError() );	}

	//	gr: would like to make this a bit more generic, hence here rather than in window specific header
#if defined(TARGET_WINDOWS)
	void				IsOkay(HRESULT Error,const std::string& Context);
#endif
	void				IsOkay(int Error,const std::string& Context);
	void				IsOkay(const std::string& Context);	//	checks last error
	void				ThrowLastError(const std::string& Context);	//	throws an exception with the last error (even if there is none)

	//	gr: is this the right place? I put it with OS errors
	Soy::TVersion		GetOsVersion();
};




inline void Soy::EndianSwap(uint32_t& Value)
{
	//	https://stackoverflow.com/a/48793665/355753
#if defined(TARGET_WINDOWS)
	Value = _byteswap_ulong(Value);
#else
	//	gcc
	Value = __builtin_bswap32(Value);
#endif
}
