#pragma once

/*

	manager to hold onto stuff for exporting to c# and then releasing
	
*/
#include "SoyAssert.h"
#include "SoyPool.h"


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
		mPool		( ElementLimit )
	{
	}

	const EXTERNALTYPE*					Lock(const TYPE& Item);
	void								Unlock(const EXTERNALTYPE* Element);
	void								UnlockAll();

public:
	//	gr: to avoid using std::shared for strings (which realloc and then give us moving addresses)
	//		we pool stuff which manages it for us.
	//		that internally uses std::shared, but could change later, but not our concern
	TPool<TYPE>							mPool;
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
	auto Alloc = []()
	{
		return std::make_shared<std::string>();
	};
	
	auto& NewItem = mPool.Alloc( Alloc );
	NewItem = Item;
	
	//	gr: this should probably not allowed to be able to return null
	auto* External = Soy::ExternalCast<EXTERNALTYPE>(NewItem);
	if ( External == nullptr )
	{
		//	gr: not doing std::Debug for recursion in Unity stuff (fix that!)
		//	something to breakpoint
		static int x = 0;
		x++;
	}
	return External;
}

template<typename TYPE,typename EXTERNALTYPE>
void TExportManager<TYPE,EXTERNALTYPE>::Unlock(const EXTERNALTYPE* Element)
{
	auto Compare = [=](const TYPE& Value)
	{
		auto* CompareExternal = Soy::ExternalCast<EXTERNALTYPE>(Value);
		return CompareExternal == Element;
	};
	
	mPool.Release( Compare );
}

template<typename TYPE,typename EXTERNALTYPE>
void TExportManager<TYPE,EXTERNALTYPE>::UnlockAll()
{
	mPool.ReleaseAll();
}

