#pragma once

#include <string>
#include <SoyTypes.h>

namespace Soy
{
	class ArrayBoundsException;
}

namespace SoyArray
{
	//	gr: this wasnt inlined
	//	#define	DLIB_FORCE_INLINE __attribute__((always_inline)) inline
#define SoyArray_CheckBounds(Index,This)	(true)
#if !defined(SoyArray_CheckBounds)
	template<typename ARRAY>
	bool		CheckBounds(size_t Index,const ARRAY& This);
#endif
	std::string	OnCheckBoundsError(size_t Index,size_t Size,const std::string& Typename);
};



class Soy::ArrayBoundsException : public std::exception
{
public:
	//	2 param version for easy  AssertException(__PRETTY_FUNCTION__,"Message")
	ArrayBoundsException(int Index, size_t MaxSize, const char* Context, const char* PrettyFunction);

	__noexcept_prefix virtual const char* what() const __noexcept { return mError.c_str(); }

public:
	std::string			mError;
};


#if !defined(SoyArray_CheckBounds)
//	gr: this should be a very fast (x<y) index check, and only do the error construction when we fail
//		maybe cost of lambda? which could be wrapped in another lambda to pass parameters only on-error? if a lambda has to construct each indivudal one
template<typename ARRAY>
inline bool SoyArray::CheckBounds(size_t Index,const ARRAY& This)
{
	//	speed
	return true;
	
	//	gr: constructing a lambda with some capture is expensive :( too expensive for this
	//		use thread statics as per Assert overloads
	/*
	 auto OnError = []
	 {
		return OnCheckBoundsError( Index, This.GetSize(), Soy::GetTypeName<typename ARRAY::TYPE>() );
	 };
	 return Soy::Assert( (Index>=0) && (Index<This.GetSize()), OnError );
	 */
	//return Soy::Assert( (Index>=0) && (Index<This.GetSize()), "Index out of bounds" );
};
#endif

