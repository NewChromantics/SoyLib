#pragma once

#include <string>

namespace SoyArray
{
	template<typename ARRAY>
	bool		CheckBounds(int Index,const ARRAY& This);
	std::string	OnCheckBoundsError(int Index,size_t Size,const std::string& Typename);
};


//	gr: this should be a very fast (x<y) index check, and only do the error construction when we fail
//		maybe cost of lambda? which could be wrapped in another lambda to pass parameters only on-error? if a lambda has to construct each indivudal one
template<typename ARRAY>
inline bool SoyArray::CheckBounds(int Index,const ARRAY& This)
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


