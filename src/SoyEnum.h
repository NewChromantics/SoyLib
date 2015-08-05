#pragma once


#include <string>
#include <map>
#include "SoyAssert.h"

/*
	gr: new SoyEnum system, usage:

namespace TDeviceType
{
	enum Type
	{
		Invalid,
		Speaker,
		Tv,
		TvLeft,
		TvRight,
		Phone,
	};
	
	DECLARE_SOYENUM( TDeviceType );
};


std::map<TDeviceType::Type,std::string> TDeviceType::EnumMap =
{
	{ TDeviceType::Invalid,	"invalid" },
	{ TDeviceType::Speaker,	"speaker" },
	{ TDeviceType::Tv,		"tv" },
	{ TDeviceType::TvLeft,	"tvleft" },
	{ TDeviceType::TvRight,	"tvright" },
	{ TDeviceType::Phone,	"phone" },
};
*/


#define DECLARE_SOYENUM(Namespace)	\
	extern std::map<Namespace::Type,std::string> EnumMap;	\
	inline Type			ToType(const std::string& String)	{	return SoyEnum::ToType<Type>( String, EnumMap, Invalid );	}	\
	inline std::string	ToString(Type type)					{	return SoyEnum::ToString<Type>( type, EnumMap );	}	\
	inline bool			IsValid(Type type)					{	return SoyEnum::Validate<Type>( type, EnumMap, Invalid ) != Invalid;	}	\
	template<typename T>inline Type			Validate(T type)	{	return SoyEnum::Validate<Type>( static_cast<Type>(type), EnumMap, Invalid );	}	\
	template<>inline Type					Validate(Type type)	{	return SoyEnum::Validate<Type>( type, EnumMap, Invalid );	}	\
\


#define DECLARE_SOYENUM_WITHINVALID(Namespace,INVALID)	\
	extern std::map<Namespace::Type,std::string> EnumMap;	\
	inline Type			ToType(const std::string& String)	{	return SoyEnum::ToType<Type>( String, EnumMap, INVALID );	}	\
	inline std::string	ToString(Type type)					{	return SoyEnum::ToString<Type>( type, EnumMap );	}	\
	inline bool			IsValid(Type type)					{	return SoyEnum::Validate<Type>( type, EnumMap, INVALID ) != INVALID;	}	\
	inline Type			Validate(Type type)					{	return SoyEnum::Validate<Type>( type, EnumMap, INVALID );	}	\
\


namespace SoyEnum
{
	template<typename ENUMTYPE,class ENUMMAP>
	std::string	ToString(ENUMTYPE Type,const ENUMMAP& EnumMap);
	
	template<typename ENUMTYPE,class ENUMMAP>
	ENUMTYPE	ToType(const std::string& String,const ENUMMAP& EnumMap,ENUMTYPE Default);
	
	template<typename ENUMTYPE,class ENUMMAP>
	ENUMTYPE	Validate(ENUMTYPE Type,const ENUMMAP& EnumMap,ENUMTYPE InvalidReturn);
	
}

template<typename ENUMTYPE,class ENUMMAP>
inline std::string SoyEnum::ToString(ENUMTYPE Type,const ENUMMAP& EnumMap)
{
	//	stop bad cases being created in the map
	auto it = EnumMap.find( Type );

	if ( it == EnumMap.end() )
	{
		std::stringstream Error;
		Error << "Value missing from enum map for " << Soy::GetTypeName<ENUMTYPE>() << ": " << static_cast<int>(Type);
		throw Soy::AssertException( Error.str() );
	}

	return it->second;
};


//	note: this only will succeed if it's in the enum map... could still be valid in code
template<typename ENUMTYPE,class ENUMMAP>
inline ENUMTYPE SoyEnum::Validate(ENUMTYPE Type,const ENUMMAP& EnumMap,ENUMTYPE InvalidReturn)
{
	//	stop bad cases being created in the map
	auto it = EnumMap.find( Type );
	
	return ( it != EnumMap.end() ) ? Type : InvalidReturn;
};

template<typename ENUMTYPE,class ENUMMAP>
inline ENUMTYPE SoyEnum::ToType(const std::string& String,const ENUMMAP& EnumMap,ENUMTYPE Default)
{
	for ( auto it=EnumMap.begin();	it!=EnumMap.end();	it++ )
	{
		if ( it->second == String )
			return it->first;
	}
	return Default;
};


