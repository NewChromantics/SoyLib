#pragma once

#include "Array.hpp"


//	ascending (default, todo; rename!)
template<typename TYPEA>
class TSortPolicy
{
public:
	template<typename TYPEB>
	static int		Compare(const TYPEA& a,const TYPEB& b)
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
	static int		Compare(const TYPEA& a,const TYPEB& b)
	{
		if ( a > b ) return -1;
		if ( a < b ) return 1;
		return 0;
	}
};


/*
	Like array bridge, this is a wrapper for an array which adds insert-sorting and binary chop searching

*/


template<typename ARRAY,class TSORTPOLICY>
class SortArray : public ArrayInterface<typename ARRAY::TYPE>
{
public:
	typedef typename ARRAY::TYPE T;

public:
	explicit SortArray(ARRAY& Array) :
		mArray	( Array )
	{
	}
	explicit SortArray(const ARRAY& Array) :
		mArray	( const_cast<ARRAY&>(Array) )
	{
	}

	virtual T&			operator [] (int index)			{	return mArray[index];	}
	virtual const T&	operator [] (int index) const	{	return mArray[index];	}
	virtual T&			GetBack()						{	return mArray.GetBack();	}
	virtual const T&	GetBack(int index) const		{	return mArray.GetBack();	}
	virtual bool		IsEmpty() const					{	return mArray.IsEmpty();	}
	virtual int			GetSize() const					{	return mArray.GetSize();	}
	virtual int			GetDataSize() const				{	return mArray.GetDataSize();	}
	virtual int			GetElementSize() const			{	return mArray.GetElementSize();	}
	virtual const T*	GetArray() const				{	return mArray.GetArray();	}
	virtual T*			GetArray()						{	return mArray.GetArray();	}
	virtual void		Reserve(int size,bool clear=false)	{	return mArray.Reserve(size,clear);	}
	virtual void		RemoveBlock(int index, int count)	{	return mArray.RemoveBlock(index,count);	}
	virtual void		Clear(bool Dealloc)				{	return mArray.Clear(Dealloc);	}
	virtual int			MaxSize() const					{	return mArray.MaxSize();	}
	
	inline T			PopBack() const					{	return mArray.PopBack();	}
	
	virtual T*			PushBlock(int count)
	{
		//	can't push blocks in sort arrays!
		assert(false);
		return nullptr;
	}

	template<typename MATCHTYPE>
	bool				Remove(const MATCHTYPE& Match)
	{
		int Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}

	bool				IsAscending() const				{	return true;	}
	bool				IsDescending() const			{	return !IsAscending();	}
	bool				IsSorted() const	
	{
		for ( int i=1;	i<GetSize();	i++ )
		{
			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = TSORTPOLICY::Compare( a, b );
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
		int i = 0;
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
			int Compare = TSORTPOLICY::Compare( a, b );

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
		assert( IsSorted() );
		return Changed;
	}

	template<typename MATCHTYPE>
	T*					Find(const MATCHTYPE& item) const
	{
		int Index = FindIndex( item );
		if ( Index < 0 )
			return NULL;
		return &mArray[Index];
	}

	template<typename MATCHTYPE>
	int					FindIndex(const MATCHTYPE& item) const
	{
		int InsertIndex = FindInsertIndex( item );
		if ( InsertIndex >= GetSize() )
			return -1;

		//	found an index, if it's a different value, then it doesn't exist
		if ( TSORTPOLICY::Compare( mArray[InsertIndex], item ) != 0 )
			return -1;
		return InsertIndex;
	}

	template<typename MATCHTYPE>
	int					FindInsertIndex(const MATCHTYPE& item) const
	{
		if ( !GetSize() ) 
			return 0;
		
		int p1 = 0;
		int p2 = GetSize() - 1;
		int a = -1;
		
		while ( ( p2 - p1 ) > 1 )
		{
			int a = ( p1 + p2 ) / 2;
			int r = TSORTPOLICY::Compare( mArray[a], item );
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
		a = TSORTPOLICY::Compare( mArray[p1], item );
		if ( a >= 0 ) return p1;
		
		a = TSORTPOLICY::Compare( mArray[p2], item );
		if ( a >= 0 ) return p2;
		
		return p2 + 1;
	}
	
	T&	Push(const T& item)
	{
		int dbi = FindInsertIndex( item );
		T* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}

	//	adds item if it's unique. If item already exists, the original is returned
	T&	PushUnique(const T& item)
	{
		int dbi = FindInsertIndex( item );
		
		//	already exists
		if ( dbi < GetSize() )
			if ( TSORTPOLICY::Compare( mArray[dbi], item ) == 0 )
				return mArray[dbi];
		
		//	insert the unique item
		T* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}

public:
	ARRAY&		mArray;
};


//	with policy
template<typename ARRAY,class TSORTPOLICY>
inline SortArray<ARRAY,TSORTPOLICY> GetSortArray(ARRAY& Array,const TSORTPOLICY& Policy)
{
	return SortArray<ARRAY,TSORTPOLICY>( Array );
}

template<typename ARRAY,class TSORTPOLICY>
inline const SortArray<ARRAY,TSORTPOLICY> GetSortArrayConst(const ARRAY& Array,const TSORTPOLICY& Policy)
{
	return SortArray<ARRAY,TSORTPOLICY>( Array );
}


//	without policy
template<typename ARRAY>
inline SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>> GetSortArray(ARRAY& Array)
{
	return SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>>( Array );
}

template<typename ARRAY>
inline const SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>> GetSortArrayConst(const ARRAY& Array)
{
	return SortArray<ARRAY,TSortPolicy<typename ARRAY::TYPE>>( Array );
}


