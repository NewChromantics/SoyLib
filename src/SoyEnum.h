#pragma once


namespace SoyEnum
{
	template<typename ENUM>
	BufferString<100>	GetName()	{	return "???";	}

	template<typename ENUM_TYPE>
	inline ENUM_TYPE	ToType(const BufferString<100>& Name,const ENUM_TYPE& Default);
}

#define SOY_DECLARE_ENUM(ENUM)	\
	namespace SoyEnum		\
	{						\
		inline BufferString<100>			ToString(ENUM::Type Button)						{	return ENUM::ToString( Button );	}	\
		inline void							GetArray(ArrayBridge<ENUM::Type>& Array)		{	ENUM::GetArray( Array );	}			\
		template<> inline BufferString<100>	GetName<ENUM::Type>()							{	return #ENUM;	}			\
	}	

//		inline ENUM::Type					ToType(const BufferString<100>& Name)			{	return ENUM::ToType( Name );	}		\


//  gr: not compiling on OSX
#if defined(TARGET_WINDOWS)
template<typename ENUM_TYPE>
inline ENUM_TYPE SoyEnum::ToType(const BufferString<100>& Name,const ENUM_TYPE& Default)
{
	BufferArray<ENUM_TYPE,100> Types;
	SoyEnum::GetArray( GetArrayBridge(Types) );
	
	for ( int i=0;	i<Types.GetSize();	i++ )
	{
		auto& Type = Types[i];
		BufferString<100> TypeAsString = SoyEnum::ToString( Type );
		if ( Name == TypeAsString )
			return Type;
	}
	
	return Default;
};
#endif