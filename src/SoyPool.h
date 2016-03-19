#pragma once

#include "HeapArray.hpp"



//	todo; make threadsafe and make the destructor wait for all elements to be released
template<typename TYPE>
class TPool
{
public:
	template<typename MATCHTYPE>
	std::shared_ptr<TYPE>			AllocPtr(const MATCHTYPE& Meta,std::function<std::shared_ptr<TYPE>()> RealAlloc);
	template<typename MATCHTYPE>
	TYPE&							Alloc(const MATCHTYPE& Meta,std::function<std::shared_ptr<TYPE>()> RealAlloc);
	void							Release(TYPE* Object);
	void							Release(TYPE& Object)		{	Release(&Object);	}
	void							ReleaseAll();
	
public:
	Array<std::shared_ptr<TYPE>>	mPool;
	Array<std::shared_ptr<TYPE>>	mUsed;
};




template<typename TYPE>
template<typename MATCHTYPE>
inline std::shared_ptr<TYPE> TPool<TYPE>::AllocPtr(const MATCHTYPE& Meta,std::function<std::shared_ptr<TYPE>()> RealAlloc)
{
	//	find free one
	for ( int i=0;	i<mPool.GetSize();	i++ )
	{
		auto pFree = mPool[i];
		if ( !(*pFree == Meta ) )
			continue;
		
		//	match!
		mUsed.PushBack( pFree );
		mPool.RemoveBlock( i, 1 );
		return pFree;
	}
	
	//	no appropriate match, alloc
	auto pNew = RealAlloc();
	if ( !pNew )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " failed to allocate new item";
		throw Soy::AssertException( Error.str() );
	}
	
	//	double check the meta == works
	//	maybe this should be a warning rather than throwing, but we'll probably make a giant pool and never reuse
	if ( !( *pNew == Meta ) )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " allocated new item, but didn't match original meta (" << Meta << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	mUsed.PushBack( pNew );
	return pNew;
}

template<typename TYPE>
template<typename MATCHTYPE>
inline TYPE& TPool<TYPE>::Alloc(const MATCHTYPE& Meta,std::function<std::shared_ptr<TYPE>()> RealAlloc)
{
	return *AllocPtr( Meta, RealAlloc );
}

template<typename TYPE>
void TPool<TYPE>::Release(TYPE* Object)
{
	for ( int i=0;	i<mUsed.GetSize();	i++ )
	{
		if ( mUsed[i].get() != Object )
			continue;
		
		mPool.PushBack( mUsed[i] );
		mUsed.RemoveBlock( i, 1 );
		return;
	}
	//	for safety
	std::stringstream Error;
	Error << "Tried to release object back to pool<" << Soy::GetTypeName<TYPE>() << " that wasn't used";
	throw Soy::AssertException( Error.str() );
}

template<typename TYPE>
void TPool<TYPE>::ReleaseAll()
{
	mPool.PushBackArray( mUsed );
	mUsed.Clear();
}


