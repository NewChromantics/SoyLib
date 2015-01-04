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
		virtual T&			GetBack()						{	return (*this)[GetSize()-1];	}
		virtual const T&	GetBack() const					{	return (*this)[GetSize()-1];	}
		bool				IsEmpty() const					{	return GetSize() == 0;	}
		bool				IsFull() const					{	return GetSize() >= MaxSize();	}
		virtual int			GetSize() const=0;
		int					GetDataSize() const				{	return GetSize() * GetElementSize();	}
		int					GetElementSize() const			{	return sizeof(T);	}
		virtual const T*	GetArray() const=0;
		virtual T*			GetArray()=0;
		virtual void		Reserve(int size,bool clear=false)=0;
		virtual T*			InsertBlock(int index, int count)=0;
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

		//	this was NOT required before, because of sort array. so that just fails
		virtual T*			PushBlock(int count)=0;
		virtual bool		SetSize(int size,bool preserve=true,bool AllowLess=false)=0;

		template<class ARRAY>
		bool				Copy(const ARRAY& a)
		{
			Clear(false);
			return PushBackArray( a ) != nullptr;
		}

		template<class ARRAYTYPE>
		T*					PushBackArray(const ARRAYTYPE& v)
		{
			int NewDataIndex = GetSize();
			T* pNewData = PushBlock( v.GetSize() );
			if ( !pNewData )
				return nullptr;

			if ( Soy::DoComplexCopy<T,typename ARRAYTYPE::TYPE>() )
			{
				for ( int i=0; i<v.GetSize(); ++i )
					(*this)[i+NewDataIndex] = v[i];	//	use [] operator for bounds check
			}
			else if ( v.GetSize() > 0 )
			{
				//	re-fetch address for bounds check. technically unncessary
				pNewData = &((*this)[NewDataIndex]);
				//	note: lack of bounds check for all elements here
				memcpy( pNewData, v.GetArray(), v.GetSize() * sizeof(T) );
			}

			return pNewData;
		}

		//	pushback a c-array
		template<size_t CARRAYSIZE>
		T*	PushBackArray(const T(&CArray)[CARRAYSIZE])
		{
			int NewDataIndex = GetSize();
			T* pNewData = PushBlock( CARRAYSIZE );
			if ( !pNewData )
				return nullptr;

			if ( Soy::IsComplexType<T>() )
			{
				for ( int i=0; i<CARRAYSIZE; ++i )
					(*this)[i+NewDataIndex] = CArray[i];	//	use [] operator for bounds check
			}
			else if ( GetSize() > 0 )
			{
				//	re-fetch address for bounds check. technically unncessary
				pNewData = &((*this)[NewDataIndex]);
				//	note: lack of bounds check for all elements here
				memcpy( pNewData, CArray, CARRAYSIZE * sizeof(T) );
			}

			return pNewData;
		}
		
		//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
		template<typename THATTYPE>
		T*	PushBackReinterpret(const THATTYPE& OtherData)
		{
			int ThatDataSize = sizeof(OtherData);
			T* pData = PushBlock( ThatDataSize / sizeof(T) );
			if ( !pData )
				return nullptr;

			//	memcpy over the block
			memcpy( pData, &OtherData, ThatDataSize );
			return pData;
		}

		//	reinterpret push, mostly used for reverse endian
		template<typename THATTYPE>
		T*	PushBackReinterpretReverse(const THATTYPE& OtherData)
		{
			int ThatDataSize = sizeof(OtherData);
			T* pData = PushBlock( ThatDataSize / sizeof(T) );
			if ( !pData )
				return nullptr;

			//	memcpy over the block
			const T* OtherT = reinterpret_cast<const T*>( &OtherData );
			for ( int i=0;	i<ThatDataSize;	i++ )
				pData[i] = OtherT[ThatDataSize-1-i];
			return pData;
		}
		
		template<class ARRAYTYPE>
		T*					InsertArray(const ARRAYTYPE& v,int Index)
		{
			int NewDataIndex = Index;
			T* pNewData = InsertBlock( Index, v.GetSize() );
			if ( !pNewData )
				return nullptr;
			
			if ( Soy::DoComplexCopy<T,typename ARRAYTYPE::TYPE>() )
			{
				for ( int i=0; i<v.GetSize(); ++i )
					(*this)[i+NewDataIndex] = v[i];	//	use [] operator for bounds check
			}
			else if ( v.GetSize() > 0 )
			{
				//	re-fetch address for bounds check. technically unncessary
				pNewData = &((*this)[NewDataIndex]);
				//	note: lack of bounds check for all elements here
				memcpy( pNewData, v.GetArray(), v.GetSize() * sizeof(T) );
			}
			
			return pNewData;
		}

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

		inline ArrayBridge<T>&	operator=(const ArrayBridge<T>& That)
		{
			this->Copy( That );
			return *this;
		}

		//	interfaces not required by ArrayInterface
		virtual T&			PushBack(const T& item)=0;
		virtual T&			PushBack()=0;
		virtual bool		SetSize(int size,bool preserve=true,bool AllowLess=false)=0;
		virtual void		Reserve(int size,bool clear=false)=0;
		virtual void		RemoveBlock(int index, int count)=0;
		virtual T*			InsertBlock(int index, int count)=0;
		virtual void		Clear(bool Dealloc=true)=0;
		virtual int			MaxSize() const=0;

		//	compare two arrays of the same type
		//	gr: COULD turn this into a compare for sorting, but that would invoke a < and > operator call for each type.
		//		this is also why we don't use the != operator, only ==
		//		if we want that, make a seperate func!
		inline bool			Matches(const ArrayBridge<T>&& That) const
		{
			return Matches( That );
		}
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

		virtual T&			operator [] (int index) override			{	return mArray[index];	}
		virtual const T&	operator [] (int index) const override		{	return mArray[index];	}
		virtual T&			GetBack() override							{	return mArray.GetBack();	}
		virtual int			GetSize() const override					{	return mArray.GetSize();	}
		virtual const T*	GetArray() const override					{	return mArray.GetArray();	}
		virtual T*			GetArray() override							{	return mArray.GetArray();	}
		virtual bool		SetSize(int size,bool preserve=true,bool AllowLess=false) override	{	return mArray.SetSize(size,preserve,AllowLess);	}
		virtual void		Reserve(int size,bool clear=false) override	{	return mArray.Reserve(size,clear);	}
		virtual T*			PushBlock(int count) override				{	return mArray.PushBlock(count);	}
		virtual T&			PushBack(const T& item) override			{	return mArray.PushBack(item);	}
		virtual T&			PushBack() override							{	return mArray.PushBack();	}
		virtual void		RemoveBlock(int index, int count) override	{	mArray.RemoveBlock(index,count);	}
		virtual T*			InsertBlock(int index, int count) override	{	return mArray.InsertBlock(index,count);	}
		virtual void		Clear(bool Dealloc) override				{	return mArray.Clear(Dealloc);	}
		virtual int			MaxSize() const override					{	return mArray.MaxSize();	}
	
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



class TArrayReader
{
public:
	TArrayReader(const ArrayBridge<char>& Array) :
		mArray	( Array ),
		mOffset	( 0 )
	{
	}
	
	bool		Eod() const		{	return mOffset == mArray.GetDataSize();	}
	bool		Read(ArrayBridge<char>& Pop);			//	copy this-length of data
	bool		ReadCompare(ArrayBridge<char>& Match);	//	read array and make sure it matches Pop
	bool		Read(uint8& Pop);
	template<class BUFFERARRAYTYPE,typename TYPE>
	bool		ReadReinterpretReverse(TYPE& Pop);
	
private:
	bool		ReadReverse(ArrayBridge<char>& Pop);
	
public:
	int			mOffset;
	const ArrayBridge<char>&	mArray;
};



template<class BUFFERARRAYTYPE,typename TYPE>
inline bool TArrayReader::ReadReinterpretReverse(TYPE& Pop)
{
	BUFFERARRAYTYPE Buffer;
	Buffer.SetSize( sizeof(TYPE) );
	auto BufferBridge = GetArrayBridge( Buffer );
	if ( !ReadReverse( BufferBridge ) )
		return false;
	
	const TYPE* PopRev = reinterpret_cast<const TYPE*>( Buffer.GetArray() );
	Pop = *PopRev;
	return true;
}
