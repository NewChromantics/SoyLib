#pragma once


#include <sstream>
#include "SoyAssert.h"
#include "magic_enum/include/magic_enum.hpp"

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

*/


#define DECLARE_SOYENUM(Namespace)	\
	inline Type						ToType(const std::string& String)	{	return *magic_enum::enum_cast<Type>( String );	}	\
	inline Type						ToType(std::string_view String)		{	return *magic_enum::enum_cast<Type>( String );	}	\
	inline constexpr auto			ToStringView(Type type)				{	return magic_enum::enum_name( type );	}	\
	inline std::string				ToString(Type type)					{	return std::string(magic_enum::enum_name( type ) );	}	\
	template<typename T>inline bool	IsValid(T type)						{	auto CastValue = Validate(type);	return CastValue != Invalid;	}	\
	template<typename T>inline Type	Validate(T type)					{	auto CastValue = magic_enum::enum_cast<Type>( type );	SoyEnum::ThrowInvalid<Type>(type, CastValue, #Namespace );	return *CastValue;	}	\
	inline std::ostream& operator<<(std::ostream &out,const Namespace::Type &in)	{	out << ToStringView(in);	return out;	}	\
\


namespace SoyEnum
{
	template<typename EnumType,typename InputType>
	void	ThrowInvalid(InputType InputValue,const std::optional<EnumType>& OutputValue,const char* SoyEnumTypeName);
}


template<typename EnumType,typename InputType>
inline void SoyEnum::ThrowInvalid(InputType InputValue,const std::optional<EnumType>& OutputValue,const char* SoyEnumTypeName)
{
	if ( OutputValue.has_value() )
		return;
	std::stringstream Error;
	Error << "Tried to convert " << InputValue << " to " << SoyEnumTypeName << " enum, but invalid";
	throw Soy::AssertException(Error);
}


