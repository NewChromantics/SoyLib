#pragma once

#include "Array.hpp"



template<typename TYPEA,typename TYPEB>
inline int SortCompare_Descending(const TYPEA& a,const TYPEB& b)
{
	if ( a < b ) return -1;
	if ( a > b ) return 1;
	return 0;
}


/*
	Like array bridge, this is a wrapper for an array which adds insert-sorting and binary chop searching

*/


template<typename ARRAY>
class SortArray : public ArrayInterface<typename ARRAY::TYPE>
{
public:
	typedef typename ARRAY::TYPE T;

public:
	explicit SortArray(ARRAY& Array) :
		mArray	( Array )
	{
	}

	virtual T&			operator [] (int index)			{	return mArray[index];	}
	virtual const T&	operator [] (int index) const	{	return mArray[index];	}
	virtual T&			GetBack()						{	return mArray.GetBack();	}
	virtual const T&	GetBack(int index) const		{	return mArray.GetBack();	}
	virtual bool		IsEmpty() const					{	return mArray.IsEmpty();	}
	virtual int			GetSize() const					{	return mArray.GetSize();	}
	virtual int			GetDataSize() const				{	return mArray.GetDataSize();	}
	virtual const T*	GetArray() const				{	return mArray.GetArray();	}
	virtual T*			GetArray()						{	return mArray.GetArray();	}
	virtual void		Reserve(int size,bool clear)	{	return mArray.Reserve(size,clear);	}
	virtual void		RemoveBlock(int index, int count)	{	return mArray.RemoveBlock(index,count);	}
	virtual void		Clear(bool Dealloc)				{	return mArray.Clear(Dealloc);	}
	virtual int			MaxSize() const					{	return mArray.MaxSize();	}
	
	inline T			PopBack() const					{	return mArray.PopBack();	}

	bool				IsAscending() const				{	return true;	}
	bool				IsDescending() const			{	return !IsAscending();	}
	bool				IsSorted() const	
	{
		for ( int i=1;	i<GetSize();	i++ )
		{
			auto& a = mArray[i-1];
			auto& b = mArray[i];
			int Compare = SortCompare_Descending( a, b );
			if ( Compare != -1 )
				return false;
		}
		return true;
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
		if ( SortCompare_Descending( item, mArray[InsertIndex] ) != 0 )
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
			int r = SortCompare_Descending( item, mArray[a] );
			if ( r < 0 )
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
		a = SortCompare_Descending( item, mArray[p1] );
		if ( a <= 0 ) return p1;
		
		a = SortCompare_Descending( item, mArray[p2] );
		if ( a <= 0 ) return p2;
		
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
			if ( SortCompare_Descending( item, mArray[dbi] ) == 0 )
				return mArray[dbi];
		
		//	insert the unique item
		T* dbr = mArray.InsertBlock( dbi, 1 );
		*dbr = item;
		return *dbr;
	}

public:
	ARRAY&		mArray;
};



template<typename ARRAY>
inline SortArray<ARRAY> GetSortArray(ARRAY& Array)
{
	return SortArray<ARRAY>( Array );
}


