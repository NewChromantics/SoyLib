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
	Type		ToType(const std::string& String)	{	return SoyEnum::ToType<Type>( String, EnumMap );	}	\
	std::string	ToString(Type type)				{	return SoyEnum::ToString<Type>( type, EnumMap );	}	\
\



namespace SoyEnum
{
	template<typename ENUMTYPE,class ENUMMAP>
	std::string	ToString(ENUMTYPE Type,const ENUMMAP& EnumMap);
	
	template<typename ENUMTYPE,class ENUMMAP>
	ENUMTYPE	ToType(const std::string& String,const ENUMMAP& EnumMap);
	
}

template<typename ENUMTYPE,class ENUMMAP>
inline std::string SoyEnum::ToString(ENUMTYPE Type,const ENUMMAP& EnumMap)
{
	//	stop bad cases being created in the map
	auto it = EnumMap.find( Type );
	if ( !Soy::Assert( it != EnumMap.end(), std::stringstream() << "unhandled enum" << (int)Type ) )
		it = EnumMap.begin();
	
	return it->second;
};


template<typename ENUMTYPE,class ENUMMAP>
inline ENUMTYPE SoyEnum::ToType(const std::string& String,const ENUMMAP& EnumMap)
{
	for ( auto it=EnumMap.begin();	it!=EnumMap.end();	it++ )
	{
		if ( it->second == String )
			return it->first;
	}
	return ENUMTYPE::Invalid;
};


