#pragma once

#include <mutex>
#include "SoyTypes.h"
#include "RemoteArray.h"


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

	virtual T&			operator [] (size_t index)=0;
	virtual const T&	operator [] (size_t index) const=0;
	virtual T&			GetBack()						{	return (*this)[GetSize()-1];	}
	virtual const T&	GetBack() const					{	return (*this)[GetSize()-1];	}
	bool				IsEmpty() const					{	return GetSize() == 0;	}
	bool				IsFull() const					{	return GetSize() >= MaxSize();	}
	virtual size_t		GetSize() const=0;
	size_t				GetDataSize() const				{	return GetSize() * GetElementSize();	}
	size_t				GetElementSize() const			{	return sizeof(T);	}
	virtual const T*	GetArray() const=0;
	virtual T*			GetArray()=0;
	virtual void		Reserve(size_t size,bool clear=false)=0;
	virtual T*			InsertBlock(size_t index,size_t count)=0;
	virtual void		RemoveBlock(size_t index,size_t count)=0;
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count)=0;
	virtual void		Clear(bool Dealloc=true)=0;
	virtual size_t		MaxSize() const=0;

	//	simple iterator to find index of an element matching via == operator
	template<typename MATCHTYPE>
	ssize_t				FindIndex(const MATCHTYPE& Match) const
	{
		for ( size_t i=0;	i<GetSize();	i++ )
		{
			auto& Element = (*this)[i];
			if ( Element == Match )
				return i;
		}
		return -1;
	}

	//	find an element - returns first matching element or NULL
	template<typename MATCH> T*			Find(const MATCH& Match)		{	auto Index = FindIndex( Match );		return (Index < 0) ? nullptr : &((*this)[Index]);	}
	template<typename MATCH> const T*	Find(const MATCH& Match) const	{	auto Index = FindIndex( Match );		return (Index < 0) ? nullptr : &((*this)[Index]);	}

	template<typename MATCH>
	bool				Remove(const MATCH& Match)
	{
		auto Index = FindIndex( Match );
		if ( Index == -1 )
			return false;
		
		RemoveBlock( Index, 1 );
		return true;
	}
	
	//	this was NOT required before, because of sort array. so that just fails
	virtual T*			PushBlock(size_t count)=0;
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true)=0;

	template<class ARRAY>
	void				Copy(const ARRAY& a)
	{
		//	gr: old method, but didn't work for fixed arrays which fail on push...
		//Clear(false);
		//return PushBackArray( a ) != nullptr;
		SetSize( a.GetSize() );

		Copy<typename ARRAY::TYPE>( a.GetArray(), 0, a.GetSize() );
	}

	template<class ARRAY>
	void				Copy(const ARRAY& a,size_t CopyMax)
	{
		CopyMax = std::min( CopyMax, a.GetSize() );
		SetSize( CopyMax );
		Copy<typename ARRAY::TYPE>( a.GetArray(), 0, CopyMax );
	}


	template<class ARRAYTYPE>
	T*					PushBackArray(const ARRAYTYPE& v)
	{
		auto NewDataIndex = GetSize();
		auto* pNewData = PushBlock( v.GetSize() );
		if ( !pNewData )
			return nullptr;

		Copy<typename ARRAYTYPE::TYPE>( v.GetArray(), NewDataIndex, v.GetSize() );

		return pNewData;
	}

	//	pushback a c-array
	template<size_t CARRAYSIZE>
	T*	PushBackArray(const T(&CArray)[CARRAYSIZE])
	{
		auto NewDataIndex = GetSize();
		auto* pNewData = PushBlock( CARRAYSIZE );
		if ( !pNewData )
			return nullptr;

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

		return pNewData;
	}
	
	//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
	template<typename THATTYPE>
	T*	PushBackReinterpret(const THATTYPE& OtherData)
	{
		size_t ThatDataSize = sizeof(THATTYPE);
		auto* pData = PushBlock( ThatDataSize / sizeof(T) );
		if ( !pData )
			return nullptr;
		
		//	memcpy over the block
		memcpy( pData, &OtherData, ThatDataSize );
		return pData;
	}
	
	//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
	template<typename THATTYPE>
	T*	PushBackReinterpret(const THATTYPE* OtherData,size_t Count)
	{
		size_t ThatDataSize = sizeof(THATTYPE) * Count;
		auto* pData = PushBlock( ThatDataSize / sizeof(T) );
		if ( !pData )
			return nullptr;
		
		//	memcpy over the block
		memcpy( pData, OtherData, ThatDataSize );
		return pData;
	}
	
	//	reinterpret push, mostly used for reverse endian
	template<typename THATTYPE>
	T*	PushBackReinterpretReverse(const THATTYPE& OtherData)
	{
		auto ThatDataSize = sizeof(OtherData);
		auto* pData = PushBlock( ThatDataSize / sizeof(T) );
		if ( !pData )
			return nullptr;

		//	memcpy over the block
		auto* OtherT = reinterpret_cast<const T*>( &OtherData );
		for ( size_t i=0;	i<ThatDataSize;	i++ )
			pData[i] = OtherT[ThatDataSize-1-i];
		return pData;
	}
	
	template<class ARRAYTYPE>
	T*					InsertArray(const ARRAYTYPE& v,size_t Index)
	{
		auto NewDataIndex = Index;
		auto* pNewData = InsertBlock( Index, v.GetSize() );
		if ( !pNewData )
			return nullptr;
		
		if ( Soy::DoComplexCopy<T,typename ARRAYTYPE::TYPE>() )
		{
			for ( size_t i=0; i<v.GetSize(); ++i )
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
	
	
	//	if any case of the function returns false, this function breaks and returns false
	bool		ForEach(std::function<bool(const T&)> Function) const
	{
		for ( size_t i=0;	i<GetSize();	i++ )
		{
			auto& Element = (*this)[i];
			if ( !Function( Element ) )
				return false;
		}
		return true;
	}
	
	
	//	if any case of the function returns false, this function breaks and returns false
	bool		ForEach(std::function<bool(T&)> Function)
	{
		for ( size_t i=0;	i<GetSize();	i++ )
		{
			auto& Element = (*this)[i];
			if ( !Function( Element ) )
				return false;
		}
		return true;
	}
	
	
	T	PopAt(size_t Index)
	{
		T Item = (*this)[Index];
		RemoveBlock( Index, 1 );
		return Item;
	}
	
	//	set all elements to a value
	template<typename TYPE>
	void	SetAll(const TYPE& Value)
	{
		auto Size = GetSize();
		auto mdata = GetArray();
		
		for ( size_t i=0;	i<Size;	i++ )
			mdata[i] = Value;
	}
	
	void	SetAll(const T& Value)
	{
		auto Size = GetSize();
		auto mdata = GetArray();
		
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

		//	gr: can I re-use func above?
		for ( size_t i=0;	i<Size;	i++ )
			mdata[i] = Value;
	}


private:
	//	last-stage copy, vars should all be setup by now
	template<typename SOURCE_T>
	void	Copy(const SOURCE_T* Source,size_t NewDataIndex,size_t Length)
	{
		//	nothing to do
		if ( Length == 0 )
			return;
		if ( !Source )
			throw Soy::AssertException("Tried to copy Null data into array");
		if ( NewDataIndex >= GetSize() )
			throw Soy::AssertException(std::string(__func__) + " index out of range");
		if ( NewDataIndex+Length > GetSize() )
		{
			std::stringstream Error;
			Error << __func__ << " index+length out of range. (" << NewDataIndex << " + " << Length << " > " << GetSize() << ")";
			throw Soy::AssertException(Error.str());
		}
				
		if ( Soy::DoComplexCopy<T,SOURCE_T>() )
		{
			auto* pNewData = &((*this)[NewDataIndex]);
			for ( size_t i=0;	i<Length;	i++ )
				pNewData[i] = Source[i];
				//(*this)[i+NewDataIndex] = Source[i];	//	use [] operator for bounds check
		}
		else if ( Length > 0 )
		{
			//	re-fetch address for bounds check. technically unncessary
			auto* pNewData = &((*this)[NewDataIndex]);
			//	note: lack of bounds check for all elements here
			std::memcpy( pNewData, Source, Length * sizeof(T) );
		}
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
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true)=0;
	virtual void		Reserve(size_t size,bool clear=false)=0;
	virtual void		RemoveBlock(size_t index,size_t count)=0;
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count)=0;
	virtual T*			InsertBlock(size_t index,size_t count)=0;
	virtual void		Clear(bool Dealloc=true)=0;

	inline T&			InsertAt(size_t index,const T& item)
	{
		auto* Block = InsertBlock( index, 1 );
		*Block = item;
		return *Block;
	}
	
	T&					PushBackUnique(const T& item)
	{
		auto Index = this->FindIndex( item );
		if ( Index != -1 )
			return (*this)[Index];
		return PushBack(item);
	}

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
			for ( size_t i=0;	i<this->GetSize();	i++ )
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
	
	template<typename TYPE>
	FixedRemoteArray<TYPE>	GetSubArray(size_t ByteOffset,size_t OutputElements)
	{
		//	verify this combination is possible
		auto ThisDataSize = this->GetDataSize();
		if ( ByteOffset >= ThisDataSize )
			throw Soy::AssertException("Subarray start out of bounds");
		size_t ElementLast = ByteOffset + (OutputElements * sizeof(TYPE)) - 1;
		if ( ElementLast >= ThisDataSize )
			throw Soy::AssertException("Subarray end out of bounds");
		
		auto* Start = reinterpret_cast<uint8*>( this->GetArray() ) + ByteOffset;
		return FixedRemoteArray<TYPE>( reinterpret_cast<TYPE*>( Start ), OutputElements );
	}

	FixedRemoteArray<T>	GetSubArray(size_t IndexOffset,ssize_t OutputElements=-1)
	{
		auto ByteOffset = IndexOffset * this->GetElementSize();
		if ( OutputElements < 0 )
			OutputElements = this->GetSize() - IndexOffset;
		return GetSubArray<T>( ByteOffset, OutputElements );
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

	virtual T&			operator [](size_t index) override			{	return mArray[index];	}
	virtual const T&	operator [](size_t index) const override		{	return mArray[index];	}
	virtual T&			GetBack() override							{	return mArray[mArray.GetSize()-1];	}
	virtual size_t		GetSize() const override					{	return mArray.GetSize();	}
	virtual const T*	GetArray() const override					{	return mArray.GetArray();	}
	virtual T*			GetArray() override							{	return mArray.GetArray();	}
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true) override	{	return mArray.SetSize(size,preserve,AllowLess);	}
	virtual void		Reserve(size_t size,bool clear=false) override	{	return mArray.Reserve(size,clear);	}
	virtual T*			PushBlock(size_t count) override				{	return mArray.PushBlock(count);	}
	virtual T&			PushBack(const T& item) override			{	auto& Tail = PushBack();	Tail = item;	return Tail;	}
	virtual T&			PushBack() override							{	auto* Tail = PushBlock(1);	return *Tail;	}
	virtual void		RemoveBlock(size_t index,size_t count) override	{	mArray.RemoveBlock(index,count);	}
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count) override	{	mArray.MoveBlock( OldIndex, NewIndex, Count );	}
	virtual T*			InsertBlock(size_t index,size_t count) override	{	return mArray.InsertBlock(index,count);	}
	virtual void		Clear(bool Dealloc) override				{	return mArray.Clear(Dealloc);	}
	virtual size_t		MaxSize() const override					{	return mArray.MaxSize();	}

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


//	gr: really hacky, and locks aren't thought out, but this might work well.
//	may need implementing as a proper array type
template<class ARRAY>
class AtomicArrayBridge : public ArrayBridge<typename ARRAY::TYPE>
{
public:
	typedef typename ARRAY::TYPE T;
public:
	//	const cast :( but only way to avoid it is to duplicate both ArrayBridge
	//	types for a const and non-const version...
	//	...not worth it.
	explicit AtomicArrayBridge(const ARRAY& Array) :
	mArray	( const_cast<ARRAY&>(Array) )
	{
	}
	
	virtual T&			operator [](size_t index) override				{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray[index];	}
	virtual const T&	operator [](size_t index) const override		{	return mArray[index];	}
	virtual T&			GetBack() override								{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray[mArray.GetSize()-1];	}
	virtual size_t		GetSize() const override						{	return mArray.GetSize();	}
	virtual const T*	GetArray() const override						{	return mArray.GetArray();	}
	virtual T*			GetArray() override								{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.GetArray();	}
	virtual bool		SetSize(size_t size,bool preserve=true,bool AllowLess=true) override	{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.SetSize(size,preserve,AllowLess);	}
	virtual void		Reserve(size_t size,bool clear=false) override	{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.Reserve(size,clear);	}
	virtual T*			PushBlock(size_t count) override				{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.PushBlock(count);	}
	virtual T&			PushBack(const T& item) override				{	std::lock_guard<std::recursive_mutex> Lock(mLock);	auto& Tail = PushBack();	Tail = item;	return Tail;	}
	virtual T&			PushBack() override								{	std::lock_guard<std::recursive_mutex> Lock(mLock);	auto* Tail = PushBlock(1);	return *Tail;	}
	virtual void		RemoveBlock(size_t index,size_t count) override	{	std::lock_guard<std::recursive_mutex> Lock(mLock);	mArray.RemoveBlock(index,count);	}
	virtual void		MoveBlock(size_t OldIndex,size_t NewIndex,size_t Count) override	{	std::lock_guard<std::recursive_mutex> Lock(mLock);	mArray.MoveBlock( OldIndex, NewIndex, Count );	}
	virtual T*			InsertBlock(size_t index,size_t count) override	{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.InsertBlock(index,count);	}
	virtual void		Clear(bool Dealloc) override					{	std::lock_guard<std::recursive_mutex> Lock(mLock);	return mArray.Clear(Dealloc);	}
	virtual size_t		MaxSize() const override						{	return mArray.MaxSize();	}
	
private:
	std::recursive_mutex	mLock;
	ARRAY&				mArray;
};


class TArrayReader
{
public:
	TArrayReader(const ArrayBridge<char>& Array) :
		mArray	( Array )
	{
	}
	
	size_t		GetRemainingBytes() const	{	return mArray.GetDataSize() - mOffset;	}
	bool		Eod() const					{	return GetRemainingBytes() <= 0;	}
	//bool		ReadArray(ArrayBridge<char>& Pop);			//	copy this-length of data
	bool		ReadCompare(ArrayBridge<char>&& Match);	//	read array and make sure it matches Pop
	
	template<typename TYPE>
	bool		Read(TYPE& Pop);

	template<typename TYPE>
	bool		ReadArray(ArrayBridge<TYPE>& Pop);		//	read this-many
	template<typename TYPE>
	bool		ReadArray(ArrayBridge<TYPE>&& Pop)		{	return ReadArray( Pop );	}
	
	template<class BUFFERARRAYTYPE,typename TYPE>
	bool		ReadReinterpretReverse(TYPE& Pop);
	
private:
	bool		ReadReverse(ArrayBridge<char>& Pop);
	
public:
	const ArrayBridge<char>&	mArray;
	size_t						mOffset = 0;
};


template<typename TYPE>
inline bool TArrayReader::Read(TYPE& Pop)
{
	if ( mOffset+sizeof(TYPE) > mArray.GetDataSize() )
		return false;
	auto& Data = *reinterpret_cast<const TYPE*>( &mArray[mOffset] );
	Pop = Data;
	mOffset += sizeof(TYPE);
	return true;
}

template<typename TYPE>
inline bool TArrayReader::ReadArray(ArrayBridge<TYPE>& Pop)
{
	if ( mOffset+Pop.GetDataSize() > mArray.GetDataSize() )
		return false;
	auto* Data = reinterpret_cast<const TYPE*>( &mArray[mOffset] );
	memcpy( Pop.GetArray(), Data, Pop.GetDataSize() );
	mOffset += Pop.GetDataSize();
	return true;
}


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
