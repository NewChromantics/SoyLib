#pragma once


namespace SoyEnum
{
	template<typename ENUM>
	BufferString<100>	GetName()	{	return "???";	}
}

#define SOY_DECLARE_ENUM(ENUM)	\
	namespace SoyEnum		\
	{						\
		inline BufferString<100>			ToString(ENUM::Type Button)						{	return ENUM::ToString( Button );	}	\
		inline ENUM::Type					ToType(const BufferString<100>& Name)			{	return ENUM::ToType( Name );	}		\
		inline void							GetArray(ArrayBridge<ENUM::Type>& Array)		{	ENUM::GetArray( Array );	}			\
		template<> inline BufferString<100>	GetName<ENUM::Type>()							{	return #ENUM;	}			\
	}	


