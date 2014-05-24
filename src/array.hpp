/*
	Twilight Prophecy SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2003 Twilight 3D Finland Oy Ltd.
*/
#pragma once

#include <cassert>
#include <cstddef>
#include "SoyTypes.h"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...

//namespace Soy
//{
	//	ArrayTest object forces .cpp to be built and not throw away the unittest symbols
	class ArrayTestDummy
	{
	public:
		// nothing class
		ArrayTestDummy();
	};


	//	An ArrayBridge<T> is an interface to an array object, without needing to specialise the array type (buffer, heap, etc)
	//	Why do we need this when we could template our function? because this allows us to put the definition somewhere away from
	//	the header! Although there is a (minor) overhead of using a vtable, it's much more useful for having more flexible functions
	//	in source (obviously not everything can go in headers) and reduces code-size (if we ever require that)
	//		usage;
	//	bool	MyFunc(ArrayBridge<int>& Array);	//	source NOT in header
	//	....
	//	HeapArray<int> h;
	//	Array<int> a;
	//	MyFunc( GetArrayBridge(h) );
	//	MyFunc( GetArrayBridge(a) );
	//	MyFunc( GetArrayBridge(s) );
	//	
	//	or for even more transparency (eg. no need to change lots of calls....)
	//	template<class ARRAY> MyFunc(ARRAY& Array)	{	MyFunc( GetArrayBridge(Array) );	}
	//	MyFunc( h );
	//	MyFunc( a );
	//	MyFunc( s );
	//	
	//	gr: can't currently use SortArray's because of the hidden/missing PushBack/PushBlock functions, which ideally we require here..
	template<typename T>
	class ArrayInterface
	{
	public:
		typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)
	public:
		ArrayInterface()	{}

		virtual T&			operator [] (int index)=0;
		virtual const T&	operator [] (int index) const=0;
		virtual T&			GetBack()=0;
		virtual const T&	GetBack(int index) const=0;
		bool				IsEmpty() const					{	return GetSize() == 0;	}
		bool				IsFull() const					{	return GetSize() >= MaxSize();	}
		virtual int			GetSize() const=0;
		virtual int			GetDataSize() const=0;
		virtual int			GetElementSize() const=0;
		virtual const T*	GetArray() const=0;
		virtual T*			GetArray()=0;
		virtual void		Reserve(int size,bool clear=false)=0;
		virtual void		RemoveBlock(int index, int count)=0;
		virtual void		Clear(bool Dealloc=true)=0;
		virtual int			MaxSize() const=0;

		//	simple iterator to find index of an element matching via == operator
		template<typename MATCHTYPE>
		int					FindIndex(const MATCHTYPE& Match) const
		{
			for ( int i=0;	i<GetSize();	i++ )
			{
				const T& Element = (*this)[i];
				if ( Element == Match )
					return i;
			}
			return -1;
		}

		//	find an element - returns first matching element or NULL
		template<typename MATCH> T*			Find(const MATCH& Match)		{	int Index = FindIndex( Match );		return (Index < 0) ? nullptr : &((*this)[Index]);	}
		template<typename MATCH> const T*	Find(const MATCH& Match) const	{	int Index = FindIndex( Match );		return (Index < 0) ? nullptr : &((*this)[Index]);	}


		uint32				GetCrc32() const
		{
			TCrc32 Crc;
			Crc.AddData( reinterpret_cast<const uint8_t*>(GetArray()), GetDataSize() );
			return Crc.GetCrc32();
		}
	};


	template<typename T>
	class ArrayBridge : public ArrayInterface<T>
	{
	public:
		typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)
	public:
		ArrayBridge()	{}

		//	interfaces not required by ArrayInterface
		virtual T*			PushBlock(int count)=0;
		virtual T&			PushBack(const T& item)=0;
		virtual T&			PushBack()=0;
		virtual bool		SetSize(int size,bool preserve=true,bool AllowLess=false)=0;
		virtual void		Reserve(int size,bool clear=false)=0;
		virtual void		RemoveBlock(int index, int count)=0;
		virtual void		Clear(bool Dealloc=true)=0;
		virtual int			MaxSize() const=0;

		//	compare two arrays of the same type
		//	gr: COULD turn this into a compare for sorting, but that would invoke a < and > operator call for each type.
		//		this is also why we don't use the != operator, only ==
		//		if we want that, make a seperate func!
		inline bool			Matches(const ArrayBridge<T>& That) const
		{
			if ( this->GetSize() != That.GetSize() )	
				return false;

			//	both empty
			if ( this->IsEmpty() )
				return true;

			//	do quick compare (memcmp) when possible
			auto* ThisData = this->GetArray();
			auto* ThatData = That.GetArray();
			if ( Soy::IsComplexType<T>() )
			{
				for ( int i=0;	i<this->GetSize();	i++ )
				{
					if ( ThisData[i] == ThatData[i] )
						continue;
					//	elements differ
					return false;
				}
				return true;
			}
			else
			{
				//	memory differs
				if ( memcmp( ThisData, ThatData, this->GetDataSize() ) != 0 )
					return false;
				return true;
			}
		}
	};


	//	actual BridgeObject, templated to your array.
	template<class ARRAY>
	class ArrayBridgeDef : public ArrayBridge<typename ARRAY::TYPE>
	{
	public:
		typedef typename ARRAY::TYPE T;
	public:
		//	const cast :( but only way to avoid it is to duplicate both ArrayBridge 
		//	types for a const and non-const version...
		//	...not worth it.
		explicit ArrayBridgeDef(const ARRAY& Array) :
			mArray	( const_cast<ARRAY&>(Array) )
		{
		}

		virtual T&			operator [] (int index)			{	return mArray[index];	}
		virtual const T&	operator [] (int index) const	{	return mArray[index];	}
		virtual T&			GetBack()						{	return mArray.GetBack();	}
		virtual const T&	GetBack(int index) const		{	return mArray.GetBack();	}
		virtual int			GetSize() const					{	return mArray.GetSize();	}
		virtual int			GetDataSize() const				{	return mArray.GetDataSize();	}
		virtual int			GetElementSize() const			{	return mArray.GetElementSize();	}
		virtual const T*	GetArray() const				{	return mArray.GetArray();	}
		virtual T*			GetArray()						{	return mArray.GetArray();	}
		virtual bool		SetSize(int size,bool preserve=true,bool AllowLess=false)	{	return mArray.SetSize(size,preserve,AllowLess);	}
		virtual void		Reserve(int size,bool clear=false)	{	return mArray.Reserve(size,clear);	}
		virtual T*			PushBlock(int count)			{	return mArray.PushBlock(count);	}
		virtual T&			PushBack(const T& item)			{	return mArray.PushBack(item);	}
		virtual T&			PushBack()						{	return mArray.PushBack();	}
		virtual void		RemoveBlock(int index, int count)	{	return mArray.RemoveBlock(index,count);	}
		virtual void		Clear(bool Dealloc)				{	return mArray.Clear(Dealloc);	}
		virtual int			MaxSize() const					{	return mArray.MaxSize();	}
	
	private:
		ARRAY&				mArray;
	};

	//	helper function to make syntax nicer;
	//		auto Bridge = GetArrayBridge( MyArray );
	//	instead of
	//		auto Bridge = ArrayBridgeDef<BufferArray<int,100>>( MyArray );
	template<class ARRAY>
	inline ArrayBridgeDef<ARRAY> GetArrayBridge(const ARRAY& Array)	
	{	
		return ArrayBridgeDef<ARRAY>( Array );
	};
