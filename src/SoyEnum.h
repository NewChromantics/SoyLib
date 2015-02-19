#pragma once


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
\


#define DECLARE_SOYENUM_WITHINVALID(Namespace,INVALID)	\
	extern std::map<Namespace::Type,std::string> EnumMap;	\
	inline Type			ToType(const std::string& String)	{	return SoyEnum::ToType<Type>( String, EnumMap, INVALID );	}	\
	inline std::string	ToString(Type type)					{	return SoyEnum::ToString<Type>( type, EnumMap );	}	\
\


namespace SoyEnum
{
	template<typename ENUMTYPE,class ENUMMAP>
	std::string	ToString(ENUMTYPE Type,const ENUMMAP& EnumMap);
	
	template<typename ENUMTYPE,class ENUMMAP>
	ENUMTYPE	ToType(const std::string& String,const ENUMMAP& EnumMap,ENUMTYPE Default);
	
}

template<typename ENUMTYPE,class ENUMMAP>
inline std::string SoyEnum::ToString(ENUMTYPE Type,const ENUMMAP& EnumMap)
{
	//	stop bad cases being created in the map
	auto it = EnumMap.find( Type );

	__thread static ENUMTYPE LastErrorType;
	__thread static auto Error = []
	{
		std::stringstream Error;
		Error << "unhandled enum" << (int)LastErrorType;
		return Error.str();
	};
	LastErrorType = Type;

	if ( !Soy::Assert( it != EnumMap.end(), Error ) )
		it = EnumMap.begin();
	
	return it->second;
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


