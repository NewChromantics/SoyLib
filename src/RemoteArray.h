#pragma once

namespace SoyArray
{
	template<typename ARRAY>
	bool		CheckBounds(int Index,const ARRAY& This);
	std::string	OnCheckBoundsError(int Index,size_t Size,const std::string& Typename);
};


//	gr: this should be a very fast (x<y) index check, and only do the error construction when we fail
//		maybe cost of lambda? which could be wrapped in another lambda to pass parameters only on-error? if a lambda has to construct each indivudal one
template<typename ARRAY>
inline bool SoyArray::CheckBounds(int Index,const ARRAY& This)
{
	//	speed
	return true;
	
	//	gr: constructing a lambda with some capture is expensive :( too expensive for this
	//		use thread statics as per Assert overloads
	/*
	auto OnError = []
	{
		return OnCheckBoundsError( Index, This.GetSize(), Soy::GetTypeName<typename ARRAY::TYPE>() );
	};
	return Soy::Assert( (Index>=0) && (Index<This.GetSize()), OnError );
	 */
	return Soy::Assert( (Index>=0) && (Index<This.GetSize()), "Index out of bounds" );
};


//	gr: should this be a type of heap to be used on heaparray?...
//	remote array of fixed size (cannot be shrunk or grown, only accessed)
template<typename T>
class FixedRemoteArray
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)
	
public:
	template <unsigned int BUFFERSIZE>
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
	
	T& operator [] (int Index)
	{
		SoyArray::CheckBounds( Index, *this );
		return mData[Index];
	}
	
	const T& operator [] (int Index) const
	{
		SoyArray::CheckBounds( Index, *this );
		return mData[Index];
	}
	
	size_t		GetSize() const			{	return mDataSize;		}
	size_t		MaxAllocSize() const	{	return GetSize();	}
	size_t		MaxSize() const			{	return GetSize();	}
	const T*	GetArray() const		{	return mData;	}
	T*			GetArray()				{	return mData;	}
	
	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	bool SetSize(int size, bool preserve=true,bool AllowLess=true)
	{
		return size == GetSize();
	}
	
	T*			PushBlock(int count)				{	return nullptr;	}
	T*			InsertBlock(int Index,int Count)	{	return nullptr;	}
	bool		RemoveBlock(int Index,int Count)	{	return false;	}
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
	template <unsigned int BUFFERSIZE>
	RemoteArray(T (& Buffer)[BUFFERSIZE],int& BufferCounter) :
		moffset		( BufferCounter ),
		mdata		( Buffer ),
		mmaxsize	( BUFFERSIZE )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	template <unsigned int BUFFERSIZE>
	RemoteArray(const T (& Buffer)[BUFFERSIZE],const int& BufferCounter) :
		moffset		( const_cast<int&>(BufferCounter) ),
		mdata		( const_cast<T*>(Buffer) ),
		mmaxsize	( BUFFERSIZE )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	explicit RemoteArray(T* Buffer,const int BufferSize,int& BufferCounter) :
		moffset		( BufferCounter ),
		mdata		( Buffer ),
		mmaxsize	( BufferSize )
	{
		//	can't really handle stuff that isn't setup
		assert( moffset <= mmaxsize );
	}
	explicit RemoteArray(const T* Buffer,const int BufferSize,const int& BufferCounter) :
		moffset		( const_cast<int&>(BufferCounter) ),
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

	T& operator [] (int index)
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}

	const T& operator [] (int index) const
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}

	T&			GetBack()				{	return (*this)[GetSize()-1];	}
	const T&	GetBack() const			{	return (*this)[GetSize()-1];	}

	bool		IsEmpty() const			{	return GetSize() == 0;		}
	bool		IsFull() const			{	return GetSize() >= MaxSize();	}
	int			GetSize() const			{	return moffset;		}
	int			GetDataSize() const		{	return GetSize() * GetElementSize();	}	//	size of all the data in bytes
	int			GetElementSize() const	{	return sizeof(T);	}	//	size of all the data in bytes
	const T*	GetArray() const		{	return mdata;	}
	T*			GetArray()				{	return mdata;	}

	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	bool SetSize(int size, bool preserve=true,bool AllowLess=true)
	{
		assert( size >= 0 );
		if ( size < 0 )	
			size = 0;

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

	void Reserve(int size,bool clear=false)
	{
		if ( clear )
		{
			SetSize(size,false);
			ResetIndex();
		}
		else
		{
			//	grow the array as neccessary, then restore the current size again
			int CurrentSize = GetSize();
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

	T* PushBlock(int count)
	{
		assert( count >= 0 );
		if ( count < 0 )	count = 0;
		int curoff = moffset;
		int endoff = moffset + count;

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
			for ( int i=curoff;	i<curoff+count;	i++ )
				mdata[i] = T();
		}
		return mdata + curoff;
	}
			
	T& PushBackUnique(const T& item)
	{
		T* pExisting = Find( item );
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

		T& ref = mdata[moffset++];
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
		T& ref = mdata[moffset++];
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
		int Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}


	T* InsertBlock(int index, int count)
	{
		//	do nothing if nothing to add
		assert( count >= 0 );
		if ( count == 0 )
			return IsEmpty() ? NULL : (mdata + index);

		if ( index >= moffset ) 
			return PushBlock( count );

//			int left = static_cast<int>((mdata+moffset) - (mdata+index));
		int left = static_cast<int>(moffset - index);
		if ( !PushBlock(count) )
			return nullptr;

		if ( Soy::DoComplexCopy<T,T>() )
		{
			T* src = mdata + moffset - count - 1;
			T* dest = mdata + moffset - 1;
			for ( int i=0; i<left; ++i )
				*dest-- = *src--;
		}
		else if ( left > 0 )
		{
			memmove( &mdata[index+count], &mdata[index], left * sizeof(T) );
		}

		return mdata + index;
	}

	void RemoveBlock(int index, int count)
	{
		//	do nothing if nothing to remove
		assert( count >= 0 );
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mdata + index;
		T* src = mdata + index + count;
		int left = static_cast<int>((mdata+moffset) - src);

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( int i=0; i<left; ++i )
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

	void SetIndex(int index)
	{
		assert( index >= 0 && index < mmaxsize );
		moffset = index;
	}

	void ResetIndex()
	{
		moffset = 0;
	}

	void TrimArray()
	{
		int size = GetSize();
		SetSize(size,true);
	}

	void Clear(bool Dealloc=true)
	{
		bool AllowLess = !Dealloc;
		SetSize(0,false,AllowLess);
	}

	int	MaxAllocSize() const
	{
		return MaxSize();
	}

	int	MaxSize() const
	{
		return mmaxsize;
	}

	int	MaxDataSize() const
	{
		return MaxSize() * sizeof(T);
	}

	//	simple iterator to find index of an element matching via == operator
	template<typename MATCHTYPE>
	int			FindIndex(const MATCHTYPE& Match) const
	{
		return GetArrayBridge(*this).FindIndex( Match );
	}

	//	find an element - returns first matching element or NULL
	template<typename MATCH> T*			Find(const MATCH& Match)		{	int Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}
	template<typename MATCH> const T*	Find(const MATCH& Match) const	{	int Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}

	//	swap two elements
	void	Swap(int a,int b)
	{
		assert( a >= 0 && a < GetSize() && b >= 0 && b < GetSize() );
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
		for ( int i=0;	i<GetSize();	i++ )
			mdata[i] = Value;
	}
	void	SetAll(const T& Value)
	{
		//	attempt non-complex memset if element can be broken into a byte
		if ( !Soy::IsComplexType<T>() )
		{
			bool AllSame = true;
			const uint8* pValue = reinterpret_cast<const uint8*>( &Value );
			for ( int i=1;	i<sizeof(Value);	i++ )
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
	template <typename TYPE,unsigned int BUFFERSIZE>
	int		CopyToBuffer(TYPE (& Buffer)[BUFFERSIZE]) const
	{
		int Count = ofMin<int>( GetSize(), static_cast<int>(BUFFERSIZE) );
		for ( int i=0;	i<Count;	i++ )
		{
			Buffer[i] = mdata[i];
		}
		return Count;
	}
		

private:
	T*			mdata;
	int&		moffset;
	int			mmaxsize;	//	original buffer size
};


//	GetRemoteArray functions to allow use of auto;
//	auto Array = GetRemoteArray(....)
//	ideally this would return an array bridge... maybe we can merge ArrayInterface and ArrayBridge and just have some inherit, and some wrap
template <typename TYPE,unsigned int BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(TYPE (& Buffer)[BUFFERSIZE],int& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

template <typename TYPE,unsigned int BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE (& Buffer)[BUFFERSIZE],const int& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

template <typename TYPE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE* Buffer,const int BufferSize,const int& BufferCounter)
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


