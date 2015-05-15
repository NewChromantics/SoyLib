#pragma once


#include "SoyArray.h"



//	gr: should this be a type of heap to be used on heaparray?...
//	remote array of fixed size (cannot be shrunk or grown, only accessed)
template<typename T>
class FixedRemoteArray
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)
	
public:
	template <size_t BUFFERSIZE>
	FixedRemoteArray(T (& Buffer)[BUFFERSIZE]) :
		mData			( Buffer ),
		mDataSize		( BUFFERSIZE )
	{
	}
	explicit FixedRemoteArray(T* Buffer,const size_t BufferSize) :
		mData		( Buffer ),
		mDataSize	( BufferSize )
	{
	}
	
	template<typename ARRAYTYPE>
	void operator = (const ARRAYTYPE& v)
	{
		Copy(v);
	}
	
	//	need an explicit = operator for self-type
	void operator = (const FixedRemoteArray& v)
	{
		Copy(v);
	}
	
	template<class ARRAYTYPE>
	bool Copy(const ARRAYTYPE& v)
	{
		return GetArrayBridge(*this).Copy(v);
	}
	
	T& operator [] (size_t Index)
	{
		SoyArray::CheckBounds( Index, *this );
		return mData[Index];
	}
	
	const T& operator [] (size_t Index) const
	{
		SoyArray::CheckBounds( Index, *this );
		return mData[Index];
	}
	
	size_t		GetSize() const			{	return mDataSize;		}
	size_t		MaxSize() const			{	return GetSize();	}
	const T*	GetArray() const		{	return mData;	}
	T*			GetArray()				{	return mData;	}
	
	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	bool SetSize(size_t size, bool preserve=true,bool AllowLess=true)
	{
		return size == GetSize();
	}
	
	T*			PushBlock(size_t count)				{	return nullptr;	}
	T*			InsertBlock(size_t Index,size_t Count)	{	return nullptr;	}
	bool		RemoveBlock(size_t Index,size_t Count)	{	return false;	}
	void		Clear(bool Dealloc)					{	}
	
	void		Reserve(size_t Size,bool Clear)
	{
		//	should we warn here for different size?
		Soy::Assert( Size != GetSize(), "Reserving different-size FixedRemoteArray" );
	}
	
	
private:
	T*			mData;
	size_t		mDataSize;	//	elements in mData
};



//	gr: should this inherit from array interface? or do we want it copyable?
template<typename T>
class RemoteArray
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)

public:
	template <size_t BUFFERSIZE>
	RemoteArray(T (& Buffer)[BUFFERSIZE],size_t& BufferCounter) :
		moffset		( BufferCounter ),
		mdata		( Buffer ),
		mmaxsize	( BUFFERSIZE )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	template <size_t BUFFERSIZE>
	RemoteArray(const T (& Buffer)[BUFFERSIZE],const size_t& BufferCounter) :
		moffset		( const_cast<size_t&>(BufferCounter) ),
		mdata		( const_cast<T*>(Buffer) ),
		mmaxsize	( BUFFERSIZE )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	explicit RemoteArray(T* Buffer,const size_t BufferSize,size_t& BufferCounter) :
		moffset		( BufferCounter ),
		mdata		( Buffer ),
		mmaxsize	( BufferSize )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	explicit RemoteArray(const T* Buffer,const size_t BufferSize,const size_t& BufferCounter) :
		moffset		( const_cast<size_t&>(BufferCounter) ),
		mdata		( const_cast<T*>(Buffer) ),
		mmaxsize	( BufferSize )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}


	template<typename ARRAYTYPE>
	void operator = (const ARRAYTYPE& v)
	{
		Copy(v);
	}

	//	need an explicit = operator for self-type
	void operator = (const RemoteArray& v)
	{
		Copy(v);
	}

	template<class ARRAYTYPE>
	bool Copy(const ARRAYTYPE& v)
	{
		return GetArrayBridge(*this).Copy(v);
	}

	T& operator [] (size_t index)
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}

	const T& operator [] (size_t index) const
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}

	T&			GetBack()				{	return (*this)[GetSize()-1];	}
	const T&	GetBack() const			{	return (*this)[GetSize()-1];	}

	bool		IsEmpty() const			{	return GetSize() == 0;		}
	bool		IsFull() const			{	return GetSize() >= MaxSize();	}
	size_t		GetSize() const			{	return moffset;		}
	size_t		GetDataSize() const		{	return GetSize() * GetElementSize();	}	//	size of all the data in bytes
	size_t		GetElementSize() const	{	return sizeof(T);	}	//	size of all the data in bytes
	const T*	GetArray() const		{	return mdata;	}
	T*			GetArray()				{	return mdata;	}

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
			moffset = 0;
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
			for ( auto i=curoff;	i<curoff+count;	i++ )
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
		if ( moffset >= mmaxsize )
		{
			assert( moffset < mmaxsize );
			return mdata[ moffset-1 ];
		}

		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		auto& ref = mdata[moffset++];
		ref = T();
		return ref;
	}

	template<class ARRAYTYPE>
	T*	PushBackArray(const ARRAYTYPE& v)
	{
		return GetArrayBridge(*this).PushBackArray(v);
	}

	//	pushback a c-array
	template<size_t CARRAYSIZE>
	void PushBackArray(const T(&CArray)[CARRAYSIZE])
	{
		return GetArrayBridge(*this).PushBackArray(CArray);
	}

	T& PopBack()
	{
		assert( GetSize() > 0 );
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
		assert( count >= 0 );
		if ( count == 0 )
			return IsEmpty() ? NULL : (mdata + index);

		if ( index >= moffset ) 
			return PushBlock( count );

//			int left = static_cast<int>((mdata+moffset) - (mdata+index));
		ssize_t left = (moffset - index);
		if ( !PushBlock(count) )
			return nullptr;

		if ( Soy::DoComplexCopy<T,T>() )
		{
			T* src = mdata + moffset - count - 1;
			T* dest = mdata + moffset - 1;
			for ( size_t i=0; i<left; ++i )
				*dest-- = *src--;
		}
		else if ( left > 0 )
		{
			memmove( &mdata[index+count], &mdata[index], left * sizeof(T) );
		}

		return mdata + index;
	}

	void RemoveBlock(size_t index,size_t count)
	{
		//	do nothing if nothing to remove
		assert( count >= 0 );
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mdata + index;
		T* src = mdata + index + count;
		ssize_t left = ((mdata+moffset) - src);

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( size_t i=0; i<left; ++i )
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

	size_t	MaxAllocSize() const
	{
		return MaxSize();
	}

	size_t	MaxSize() const
	{
		return mmaxsize;
	}

	size_t	MaxDataSize() const
	{
		return MaxSize() * sizeof(T);
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
				memset( mdata, pValue[0], GetDataSize() );
				return;
			}
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
	size_t		CopyToBuffer(TYPE (& Buffer)[BUFFERSIZE]) const
	{
		auto Count = std::min( GetSize(), BUFFERSIZE );
		for ( size_t i=0;	i<Count;	i++ )
		{
			Buffer[i] = mdata[i];
		}
		return Count;
	}
		

private:
	T*			mdata;
	size_t&		moffset;
	size_t		mmaxsize;	//	original buffer size
};


//	GetRemoteArray functions to allow use of auto;
//	auto Array = GetRemoteArray(....)
//	ideally this would return an array bridge... maybe we can merge ArrayInterface and ArrayBridge and just have some inherit, and some wrap
template <typename TYPE,size_t BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(TYPE (& Buffer)[BUFFERSIZE],size_t& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

template <typename TYPE,size_t BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE (& Buffer)[BUFFERSIZE],const size_t& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

template <typename TYPE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE* Buffer,const size_t BufferSize,const size_t& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferSize, BufferCounter );
}

template <typename TYPE,size_t BUFFERSIZE>
inline FixedRemoteArray<TYPE>	GetRemoteArray(TYPE (& Buffer)[BUFFERSIZE])
{
	return FixedRemoteArray<TYPE>( Buffer );
}

template <typename TYPE>
inline FixedRemoteArray<TYPE>	GetRemoteArray(TYPE* Buffer,const size_t BufferSize)
{
	return FixedRemoteArray<TYPE>( Buffer, BufferSize );
}

template <typename TYPE>
inline const FixedRemoteArray<TYPE>	GetRemoteArray(const TYPE* Buffer,const size_t BufferSize)
{
	auto Remote = GetRemoteArray( const_cast<TYPE*>(Buffer), BufferSize );
	return Remote;
}


