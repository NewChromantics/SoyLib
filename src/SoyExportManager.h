#pragma once

/*

	manager to hold onto stuff for exporting to c# and then releasing
	
*/
#include "SoyAssert.h"



namespace Soy
{
	template<typename EXTERNALTYPE,typename TYPE>
	const EXTERNALTYPE*	ExternalCast(const TYPE& Item);
}


template<typename TYPE,typename EXTERNALTYPE>
class TExportManager
{
public:
	TExportManager() :
		mHeap		( true, true, "TExportManager" ),
		mElements	( mHeap )
	{
	}

	const EXTERNALTYPE*					Lock(const TYPE& Item);
	void								Unlock(const EXTERNALTYPE* Element);

public:
	prmem::Heap							mHeap;
	std::mutex							mElementsLock;
	Array<TYPE>							mElements;
};

template<typename EXTERNALTYPE,typename TYPE>
const EXTERNALTYPE* Soy::ExternalCast(const TYPE& Item)
{
	throw Soy::AssertException("Unhandled case");
}

//	example for c type to c++ type
template<>
inline const char* Soy::ExternalCast<char>(const std::string& Item)
{
	return Item.c_str();
}




template<typename TYPE,typename EXTERNALTYPE>
const EXTERNALTYPE* TExportManager<TYPE,EXTERNALTYPE>::Lock(const TYPE& Item)
{
	std::lock_guard<std::mutex> Lock( mElementsLock );

	auto& NewItem = mElements.PushBack( Item );
	return Soy::ExternalCast<EXTERNALTYPE>(NewItem);
}

template<typename TYPE,typename EXTERNALTYPE>
void TExportManager<TYPE,EXTERNALTYPE>::Unlock(const EXTERNALTYPE* Element)
{
	std::lock_guard<std::mutex> Lock( mElementsLock );

	auto Index = mElements.FindIndex( Element );
	if ( Index == -1 )
		return;

	mElements.RemoveBlock( Index, 1 );
}

