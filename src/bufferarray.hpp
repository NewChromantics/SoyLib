#pragma once


#include <cassert>
#include <cstddef>
#include "SoyTypes.h"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...
#include "SoyArray.h"


template <typename T,size_t mmaxsize>
class BufferArray
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)

public:

	BufferArray()
	: moffset(0)
	{
	}

	BufferArray(const int size)
	: moffset(size)
	{
	}
	
	BufferArray(const unsigned int size)
	: moffset(size)
	{
	}
	
	//	need an explicit constructor of self-type
	BufferArray(const BufferArray& v)
	: moffset(0)
	{
		Copy( v );
	}
	
	template<class ARRAYTYPE>
	explicit BufferArray(const ARRAYTYPE& v)	//	explicit to avoid accidental implicit array-conversions (eg. when passing BufferArray to HeapArray param)
	: moffset(0)
	{
		Copy( v );
	}

	//	construct from a C-array like int Hello[2]={0,1}; this automatically sets the size
	template<size_t CARRAYSIZE>
	explicit BufferArray(const T(&CArray)[CARRAYSIZE])
	: moffset(0)
	{
		PushBackArray( CArray );
	}

	//	construct from a c++11 initialiser list. BufferArray<T,N> x = {a,b};
	explicit BufferArray(const std::initializer_list<T>& List)
	: moffset(0)
	{
		PushBackArray( List );
	}

	~BufferArray()
	{
	}

	template<typename ARRAYTYPE>
	void operator = (const ARRAYTYPE& v)
	{
		Copy(v);
	}

	//	need an explicit = operator for self-type
	void operator = (const BufferArray& v)
	{
		Copy(v);
	}

	template<class ARRAYTYPE>
	void Copy(const ARRAYTYPE& v)
	{
		SetSize(v.GetSize(), false);
		
		if ( Soy::DoComplexCopy<T,typename ARRAYTYPE::TYPE>() )
		{
			for ( size_t i=0; i<GetSize(); ++i )
				(*this)[i] = v[i];	//	use [] operator for bounds check
		}
		else if ( GetSize() > 0 )
		{
			memcpy( mdata, v.GetArray(), GetSize() * sizeof(T) );
		}
	}

	T& operator[](size_t index)
	{
		SoyArray_CheckBounds( index, *this );
		return mdata[index];
	}

	const T& operator[](size_t index) const
	{
		SoyArray_CheckBounds( index, *this );
		return mdata[index];
	}

	T&			GetBack()				{	return (*this)[GetSize()-1];	}
	const T&	GetBack() const			{	return (*this)[GetSize()-1];	}
	const T*	GetArray() const		{	return IsEmpty() ? nullptr : mdata;	}
	T*			GetArray()				{	return IsEmpty() ? nullptr : mdata;	}
	bool		IsEmpty() const			{	return GetSize() == 0;		}
	bool		IsFull() const			{	return GetSize() >= MaxSize();		}
	size_t		GetSize() const			{	return moffset;		}
	size_t		GetDataSize() const		{	return GetSize() * sizeof(T);	}	//	size of all the data in bytes
	size_t		GetElementSize() const	{	return sizeof(T);	}	//	size of all the data in bytes

	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	bool SetSize(size_t size, bool preserve=true,bool AllowLess=true)
	{
		//	limit size
		//	gr: assert, safely alloc, and return error. Maybe shouldn't "safely alloc"
		//	gr: assert over limit, don't silently fail
		assert( size <= mmaxsize );
		if ( size > mmaxsize )
		{
			size = mmaxsize;
			moffset = size;
			return false;
		}
			
		moffset = size;
		return true;
	}

	void Reserve(size_t size,bool clear=false)
	{
		if ( clear )
		{
			SetSize(size,false);
			ResetIndex();
		}
		else
		{
			//	grow the array as neccessary, then restore the current size again
			auto CurrentSize = GetSize();
			SetSize( CurrentSize + size, true, true );
			SetSize( CurrentSize, true, true );
		}
	}

	//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
	template<typename THATTYPE>
	T* PushBackReinterpret(const THATTYPE& OtherData)
	{
		return GetArrayBridge(*this).PushBackReinterpret( OtherData );
	}

	T* PushBlock(size_t count)
	{
		if ( count == 0 )
			return nullptr;
		
		auto curoff = moffset;
		auto endoff = moffset + count;

		//	fail if we try and "allocate" too much
		if ( endoff > mmaxsize )
		{
			assert( endoff <= mmaxsize );
			return nullptr;
		}
		
		moffset = endoff;
		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		if ( Soy::IsComplexType<TYPE>() )
		{
			for ( size_t i=curoff;	i<curoff+count;	i++ )
				mdata[i] = T();
		}
		return mdata + curoff;
	}
		
	T& PushBackUnique(const T& item)
	{
		auto* pExisting = Find( item );
		if ( pExisting )
			return *pExisting;

		return PushBack( item );
	}

	T& PushBack(const T& item)
	{
		//	out of space
		if ( moffset >= mmaxsize )
		{
			assert( moffset < mmaxsize );
			return mdata[ moffset-1 ];
		}

		auto& ref = mdata[moffset++];
		ref = item;
		return ref;
	}

	T& PushBack()
	{
		//	out of space
		if ( moffset >= MaxSize() )
		{
			assert( moffset < MaxSize() );
			return mdata[ moffset-1 ];
		}

		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		auto& ref = mdata[moffset++];
		ref = T();
		return ref;
	}

	template<class ARRAYTYPE>
	void PushBackArray(const ARRAYTYPE& v)
	{
		GetArrayBridge( *this ).PushBackArray(v);
	}

	//	pushback a c-array
	template<size_t CARRAYSIZE>
	void PushBackArray(const T(&CArray)[CARRAYSIZE])
	{
		auto NewDataIndex = GetSize();
		auto* pNewData = PushBlock( CARRAYSIZE );

		if ( Soy::IsComplexType<T>() )
		{
			for ( size_t i=0; i<CARRAYSIZE; ++i )
				(*this)[i+NewDataIndex] = CArray[i];	//	use [] operator for bounds check
		}
		else if ( GetSize() > 0 )
		{
			//	re-fetch address for bounds check. technically unncessary
			pNewData = &((*this)[NewDataIndex]);
			//	note: lack of bounds check for all elements here
			memcpy( pNewData, CArray, CARRAYSIZE * sizeof(T) );
		}
	}
	
	void PushBackArray(const std::initializer_list<T>& List)
	{
		//	gr: if we ever need massive lists maybe we can do non-complex copies
		for ( auto Element : List )
			PushBack( Element );
	}


	T& PopBack()
	{
		SoyArray_CheckBounds( moffset, *this );
		return mdata[--moffset];
	}

	template<typename MATCHTYPE>
	bool	Remove(const MATCHTYPE& Match)
	{
		auto Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}


	T* InsertBlock(size_t index,size_t count)
	{
		//	do nothing if nothing to add
		if ( count == 0 )
			return IsEmpty() ? nullptr : (mdata + index);

		if ( index >= moffset ) 
			return PushBlock( count );

//			int left = static_cast<int>((mdata+moffset) - (mdata+index));
		ssize_t left = moffset - index;
		PushBlock(count);	// increase array mem 

		if ( Soy::DoComplexCopy<T,T>() )
		{
			T* src = mdata + moffset - count - 1;
			T* dest = mdata + moffset - 1;
			for ( ssize_t i=0; i<left; ++i )
				*dest-- = *src--;
		}
		else if ( left > 0 )
		{
			memmove( &mdata[index+count], &mdata[index], left * sizeof(T) );
		}

		return mdata + index;

	}

	void RemoveBlock(size_t index, size_t count)
	{
		//	do nothing if nothing to remove
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mdata + index;
		T* src = mdata + index + count;
		ssize_t left = static_cast<int>((mdata+moffset) - src);

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( ssize_t i=0; i<left; ++i )
				*dest++ = *src++;
			moffset = static_cast<int>(dest - mdata);
		}
		else
		{
			if ( left > 0 )
				memmove( dest, src, left * sizeof(T) );
			moffset -= count;
		}			
	}

	void SetIndex(size_t index)
	{
		SoyArray_CheckBounds( index, *this );
		moffset = index;
	}

	void ResetIndex()
	{
		moffset = 0;
	}

	void TrimArray()
	{
		auto size = GetSize();
		SetSize(size,true);
	}

	void Clear(bool Dealloc=true)
	{
		bool AllowLess = !Dealloc;
		SetSize(0,false,AllowLess);
	}

	size_t	MaxSize() const
	{
		return mmaxsize;
	}

	size_t	MaxDataSize() const
	{
		return MaxSize() * sizeof(T);
	}

	//	simple iterator to find index of an element matching via == operator
	template<typename MATCHTYPE>
	ssize_t		FindIndex(const MATCHTYPE& Match) const
	{
		for ( size_t i=0;	i<GetSize();	i++ )
		{
			const T& Element = mdata[i];
			if ( Element == Match )
				return i;
		}
		return -1;
	}

	//	find an element - returns first matching element or NULL
	template<typename MATCH> T*			Find(const MATCH& Match)		{	auto Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}
	template<typename MATCH> const T*	Find(const MATCH& Match) const	{	auto Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}

	//	swap two elements
	void	Swap(size_t a,size_t b)
	{
		if ( !SoyArray_CheckBounds(a,*this) || !SoyArray_CheckBounds(b,*this) )
			return;
		T& ElementA = (*this)[a];
		T& ElementB = (*this)[b];
		T Temp = ElementA;
		ElementA = ElementB;
		ElementB = Temp;
	}

	//	set all elements to a value
	template<typename TYPE>
	void	SetAll(const TYPE& Value)
	{
		for ( size_t i=0;	i<GetSize();	i++ )
			mdata[i] = Value;
	}
	
	void	SetAll(const T& Value)
	{
		//	attempt non-complex memset if element can be broken into a byte
		if ( !Soy::IsComplexType<T>() )
		{
			bool AllSame = true;
			const uint8* pValue = reinterpret_cast<const uint8*>( &Value );
			for ( size_t i=1;	i<sizeof(Value);	i++ )
			{
				if ( pValue[i] == pValue[i-1] )
					continue;
				AllSame = false;
			}
			if ( AllSame )
			{
				//	gr: xcode7.3 gives us a nice warning here, but shouldn't get hit... see if we can do the complex-type check at compile time
				memset( mdata, pValue[0], GetDataSize() );
				return;
			}
		}
		else
		{
			for ( size_t i=0;	i<GetSize();	i++ )
				mdata[i] = Value;
		}
	}

	template<class ARRAYTYPE>
	inline bool	operator==(const ARRAYTYPE& Array) const
	{
		auto ThisBridge = GetArrayBridge( *this );
		auto ThatBridge = GetArrayBridge( Array );
		return ThisBridge.Matches( ThatBridge );
	}

	template<class ARRAYTYPE>
	inline bool	operator!=(const ARRAYTYPE& Array) const
	{
		auto ThisBridge = GetArrayBridge( *this );
		auto ThatBridge = GetArrayBridge( Array );
		return ThisBridge.Matches( ThatBridge )==false;
	}
	
	//	copy data to a Buffer[BUFFERSIZE] c-array. (lovely template syntax! :)
	//	returns number of elements copied
	template <typename TYPE,size_t BUFFERSIZE>
	int		CopyToBuffer(TYPE (& Buffer)[BUFFERSIZE]) const
	{
		auto Count = std::min( GetSize(), BUFFERSIZE );
		for ( int i=0;	i<Count;	i++ )
		{
			Buffer[i] = mdata[i];
		}
		return Count;
	}
	

private:
	T			mdata[mmaxsize];
	size_t		moffset;
};


//	any type that uses a BufferArray can be sped up when in use of an array with
//	DECLARE_NONCOMPLEX_TYPE( BufferArray<yourtype,SIZE> );

