#pragma once

#include "Array.hpp"
#include "SoyArray.h"



//	ascending (default, todo; rename!)
template<typename TYPEA>
class TSortPolicy
{
public:
	template<typename TYPEB>
	static int		Compare(const TYPEA& a,const TYPEB& b,const TSortPolicy& This)
	{
		if ( a < b ) return -1;
		if ( a > b ) return 1;
		return 0;
	}
};

//	reverse of default
template<typename TYPEA>
class TSortPolicy_Descending
{
public:
	template<typename TYPEB>
	static int		Compare(const TYPEA& a,const TYPEB& b,const TSortPolicy_Descending& This)
	{
		if ( a > b ) return -1;
		if ( a < b ) return 1;
		return 0;
	}
};


/*
	Like array bridge, this is a wrapper for an array which adds insert-sorting and binary chop searching

*/


template<typename TYPE>
class SortArrayLambda : public ArrayInterface<TYPE>
{
public:
	SortArrayLambda(ArrayBridge<TYPE>& Array,std::function<int(const TYPE&,const TYPE&)> SortFunc) :
		mArray		( Array ),
		mSortFunc	( SortFunc )
	{
	}

	virtual TYPE&		operator[](size_t index) override		{	return mArray[index];	}
	virtual const TYPE&	operator[](size_t index) const override	{	return mArray[index];	}
	virtual TYPE&		GetBack() override						{	return mArray.GetBack();	}
	virtual const TYPE&	GetBack() const override				{	return mArray.GetBack();	}
	virtual size_t		GetSize() const override				{	return mArray.GetSize();	}
	virtual const TYPE*	GetArray() const override				{	return mArray.GetArray();	}
	virtual TYPE*		GetArray() override						{	return mArray.GetArray();	}
	virtual void		Reserve(size_t size,bool clear=false) override	{	return mArray.Reserve(size,clear);	}
	virtual void		RemoveBlock(size_t index,size_t count) override	{	return mArray.RemoveBlock(index,count);	}
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count) override	{	return mArray.MoveBlock( OldIndex, NewIndex, Count );	}
	virtual void		Clear(bool Dealloc) override			{	return mArray.Clear(Dealloc);	}
	virtual size_t		MaxSize() const override				{	return mArray.MaxSize();	}
	
	inline TYPE			PopBack() const							{	return mArray.PopBack();	}
	
	virtual TYPE*		PushBlock(size_t count) override
	{
		throw Soy::AssertException("Cannot allocate in sort array");
	}
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true) override
	{
		if ( size <= GetSize() && AllowLess && preserve )
			return mArray.SetSize( size, preserve, AllowLess );
		//	can't push blocks in sort arrays!
		throw Soy::AssertException("Cannot allocate in sort array");
	}
	virtual TYPE*			InsertBlock(size_t index,size_t Count) override
	{
		throw Soy::AssertException("Cannot allocate in sort array");
	}
	
	template<typename MATCHTYPE>
	bool				Remove(const MATCHTYPE& Match)
	{
		auto Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}
	
	bool				IsAscending() const				{	return true;	}
	bool				IsDescending() const			{	return !IsAscending();	}
	bool				IsSorted() const
	{
		for ( size_t i=1;	i<GetSize();	i++ )
		{
			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = mSortFunc( a, b );
			//	expecting -1 or 0
			if ( Compare > 0 )
				return false;
		}
		return true;
	}
	
	bool				Sort()
	{
		//	bubble sort for quick implementation
		bool Changed = false;
		size_t i = 0;
		while ( i < GetSize() )
		{
			//	i is at the top, move on
			if ( i == 0 )
			{
				i++;
				continue;
			}
			
			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = mSortFunc( a, b );
			
			//	i is in place
			if ( Compare == 0 || Compare == -1 )
			{
				i++;
				continue;
			}
			
			//	i needs to move up
			auto temp = b;
			b = a;
			a = temp;
			i--;	//	i is now at i-1
			Changed = true;
		}
		if ( !IsSorted() )
			throw Soy::AssertException("Array is unsorted after sort()");

		return Changed;
	}
	
	template<typename MATCHTYPE>
	TYPE*				Find(const MATCHTYPE& item) const
	{
		auto Index = FindIndex( item );
		if ( Index < 0 )
			return nullptr;
		return &mArray[Index];
	}
	
	template<typename MATCHTYPE>
	ssize_t				FindIndex(const MATCHTYPE& item) const
	{
		auto InsertIndex = FindInsertIndex( item );
		if ( InsertIndex >= GetSize() )
			return -1;
		
		//	found an index, if it's a different value, then it doesn't exist
		if ( mSortFunc( mArray[InsertIndex], item ) != 0 )
			return -1;
		return InsertIndex;
	}
	
	template<typename MATCHTYPE>
	ssize_t				FindInsertIndex(const MATCHTYPE& item) const
	{
		if ( !GetSize() )
			return 0;
		
		ssize_t p1 = 0;
		ssize_t p2 = GetSize() - 1;
		ssize_t a = -1;
		
		while ( ( p2 - p1 ) > 1 )
		{
			auto a = ( p1 + p2 ) / 2;
			int r = mSortFunc( mArray[a], item );
			if ( r > 0 )	//	item less than entry a
			{
				p2 = a;
			}
			else
				if ( r == 0 )
				{
					return a;
				}
				else
				{
					p1 = a;
				}
		}
		a = mSortFunc( mArray[p1], item );
		if ( a >= 0 )
			return p1;
		
		a = mSortFunc( mArray[p2], item );
		if ( a >= 0 )
			return p2;
		
		return p2 + 1;
	}
	
	TYPE&	Push(const TYPE& item)
	{
		auto dbi = FindInsertIndex( item );
		auto* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}
	
	//	adds item if it's unique. If item already exists, the original is returned
	TYPE&	PushUnique(const TYPE& item)
	{
		auto dbi = FindInsertIndex( item );
		
		//	already exists
		if ( dbi < GetSize() )
			if ( mSortFunc( mArray[dbi], item ) == 0 )
				return mArray[dbi];
		
		//	insert the unique item
		auto* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}
	
public:
	std::function<int(const TYPE&,const TYPE&)>	mSortFunc;
	ArrayBridge<TYPE>&							mArray;
};



template<typename ARRAY,class TSORTPOLICY>
class SortArray : public ArrayInterface<typename ARRAY::TYPE>
{
public:
	typedef typename ARRAY::TYPE TYPE;
public:
	explicit SortArray(ARRAY& Array,const TSORTPOLICY& SortPolicy) :
		mArray		( Array ),
		mPolicy		( SortPolicy )
	{
	}
	explicit SortArray(const ARRAY& Array,const TSORTPOLICY& SortPolicy) :
		mArray		( const_cast<ARRAY&>(Array) ),
		mPolicy		( SortPolicy )
	{
	}

	virtual TYPE&		operator[](size_t index) override		{	return mArray[index];	}
	virtual const TYPE&	operator[](size_t index) const override	{	return mArray[index];	}
	virtual TYPE&		GetBack() override						{	return mArray.GetBack();	}
	virtual const TYPE&	GetBack() const override				{	return mArray.GetBack();	}
	virtual size_t		GetSize() const override				{	return mArray.GetSize();	}
	virtual const TYPE*	GetArray() const override				{	return mArray.GetArray();	}
	virtual TYPE*		GetArray() override						{	return mArray.GetArray();	}
	virtual void		Reserve(size_t size,bool clear=false) override	{	return mArray.Reserve(size,clear);	}
	virtual void		RemoveBlock(size_t index,size_t count) override	{	return mArray.RemoveBlock(index,count);	}
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count) override	{	return mArray.MoveBlock( OldIndex, NewIndex, Count );	}
	virtual void		Clear(bool Dealloc) override			{	return mArray.Clear(Dealloc);	}
	virtual size_t		MaxSize() const override				{	return mArray.MaxSize();	}
	
	inline TYPE			PopBack() const							{	return mArray.PopBack();	}
	
	virtual TYPE*		PushBlock(size_t count) override
	{
		throw Soy::AssertException("Cannot allocate (PushBlock) in sort array");
	}
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true) override
	{
		throw Soy::AssertException("Cannot allocate (SetSize) in sort array");
	}
	virtual TYPE*			InsertBlock(size_t index,size_t Count) override
	{
		throw Soy::AssertException("Cannot allocate (InsertBlock) in sort array");
	}

	template<typename MATCHTYPE>
	bool				Remove(const MATCHTYPE& Match)
	{
		auto Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}

	bool				IsAscending() const				{	return true;	}
	bool				IsDescending() const			{	return !IsAscending();	}
	bool				IsSorted() const	
	{
		for ( size_t i=1;	i<GetSize();	i++ )
		{
			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = TSORTPOLICY::Compare( a, b, mPolicy );
			//	expecting -1 or 0
			if ( Compare > 0 )
				return false;
		}
		return true;
	}

	bool				Sort()
	{
		//	bubble sort for quick implementation
		bool Changed = false;
		size_t i = 0;
		while ( i < GetSize() )
		{
			//	i is at the top, move on
			if ( i == 0 )
			{
				i++;
				continue;
			}

			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = TSORTPOLICY::Compare( a, b, mPolicy );

			//	i is in place
			if ( Compare == 0 || Compare == -1 )
			{
				i++;
				continue;
			}

			//	i needs to move up
			auto temp = b;
			b = a;
			a = temp;
			i--;	//	i is now at i-1
			Changed = true;
		}
		if ( !IsSorted() )
			throw Soy::AssertException("Array is unsorted after sort()");
		return Changed;
	}

	template<typename MATCHTYPE>
	TYPE*				Find(const MATCHTYPE& item) const
	{
		auto Index = FindIndex( item );
		if ( Index < 0 )
			return nullptr;
		return &mArray[Index];
	}

	template<typename MATCHTYPE>
	ssize_t				FindIndex(const MATCHTYPE& item) const
	{
		auto InsertIndex = FindInsertIndex( item );
		if ( InsertIndex >= GetSize() )
			return -1;

		//	found an index, if it's a different value, then it doesn't exist
		if ( TSORTPOLICY::Compare( mArray[InsertIndex], item, mPolicy ) != 0 )
			return -1;
		return InsertIndex;
	}

	template<typename MATCHTYPE>
	ssize_t				FindInsertIndex(const MATCHTYPE& item) const
	{
		if ( !GetSize() ) 
			return 0;
		
		ssize_t p1 = 0;
		ssize_t p2 = GetSize() - 1;
		ssize_t a = -1;
		
		while ( ( p2 - p1 ) > 1 )
		{
			auto a = ( p1 + p2 ) / 2;
			int r = TSORTPOLICY::Compare( mArray[a], item, mPolicy );
			if ( r > 0 )	//	item less than entry a
			{
				p2 = a;
			}
			else
			if ( r == 0 )
			{
				return a;
			}
			else
			{
				p1 = a;
			}
		}
		a = TSORTPOLICY::Compare( mArray[p1], item, mPolicy );
		if ( a >= 0 )
			return p1;
		
		a = TSORTPOLICY::Compare( mArray[p2], item, mPolicy );
		if ( a >= 0 )
			return p2;
		
		return p2 + 1;
	}
	
	TYPE&	Push(const TYPE& item)
	{
		auto dbi = FindInsertIndex( item );
		auto* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}

	//	adds item if it's unique. If item already exists, the original is returned
	TYPE&	PushUnique(const TYPE& item)
	{
		auto dbi = FindInsertIndex( item );
		
		//	already exists
		if ( dbi < GetSize() )
			if ( TSORTPOLICY::Compare( mArray[dbi], item, mPolicy ) == 0 )
				return mArray[dbi];
		
		//	insert the unique item
		auto* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}

public:
	TSORTPOLICY				mPolicy;
	ArrayInterface<TYPE>&	mArray;
};


//	with policy
template<typename ARRAY,class TSORTPOLICY>
inline SortArray<ARRAY,TSORTPOLICY> GetSortArray(ARRAY& Array,const TSORTPOLICY& Policy)
{
	return SortArray<ARRAY,TSORTPOLICY>( Array, Policy );
}

template<typename ARRAY,class TSORTPOLICY>
inline const SortArray<ARRAY,TSORTPOLICY> GetSortArrayConst(const ARRAY& Array,const TSORTPOLICY& Policy)
{
	return SortArray<ARRAY,TSORTPOLICY>( Array, Policy );
}


//	without policy
//	gr: can't i just use defaults?
template<typename ARRAY>
inline SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>> GetSortArray(ARRAY& Array)
{
	return SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>>( Array, TSortPolicy<typename ARRAY::TYPE>() );
}

template<typename ARRAY>
inline const SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>> GetSortArrayConst(const ARRAY& Array)
{
	return SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>>( Array, TSortPolicy<typename ARRAY::TYPE>() );
}


