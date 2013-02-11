/*
	


*/
#ifndef Soy_TYPES_HPP
#define Soy_TYPES_HPP


//	ensure this is included first so types are defined
#include "configure.hpp"




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
		//	"unregistered" type, return the size
		static BufferString<30> Name;
		if ( Name.IsEmpty() )
			Name << sizeof(TYPE) << "ByteType";
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
		inline bool Soy::IsComplexType<TYPE>()	{	return false;	}	\
	
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

#endif
