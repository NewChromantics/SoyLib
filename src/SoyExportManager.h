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
	TExportManager(size_t ElementLimit) :
		mHeap			( true, true, "TExportManager" ),
		mElements		( mHeap ),
		mElementLimit	( ElementLimit )
	{
	}

	const EXTERNALTYPE*					Lock(const TYPE& Item);
	void								Unlock(const EXTERNALTYPE* Element);

public:
	size_t								mElementLimit;
	prmem::Heap							mHeap;
	std::mutex							mElementsLock;
	
	//	gr: change this to a ring buffer to stop reallocating and remove need for the lock
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
	if ( mElements.GetSize() >= mElementLimit )
	{
		std::stringstream Error;
		Error << Soy::GetTypeName(*this) << " hit element limit " << mElements.GetSize() << "/" << mElementLimit;
		throw Soy::AssertException( Error.str() );
	}
	
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

