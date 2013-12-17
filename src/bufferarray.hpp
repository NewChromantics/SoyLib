/*
	Twilight Prophecy SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2003 Twilight 3D Finland Oy Ltd.
*/
#ifndef Soy_BUFFERARRAY_HPP
#define Soy_BUFFERARRAY_HPP


#include <cassert>
#include <cstddef>
#include "SoyTypes.h"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...

//namespace Soy
//{		
	template <typename T,uint32 mmaxsize>
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
		bool Copy(const ARRAYTYPE& v)
		{
			if ( !SetSize( v.GetSize(),false ) )
				return false;

			if ( DoComplexCopy(v) )
			{
				for ( int i=0; i<GetSize(); ++i )
					(*this)[i] = v[i];	//	use [] operator for bounds check
			}
			else if ( GetSize() > 0 )
			{
				memcpy( mdata, v.GetArray(), GetSize() * sizeof(T) );
			}
			return true;
		}

		//	different type array, must do complex copy.
		template<class ARRAYTYPE>
		inline bool DoComplexCopy(const ARRAYTYPE& v) const
		{
			return true;
		}

		//	specialisation for an array of T (best case scenario T is non-complex)
		//	gr: unfortunately, cannot test Array<T> here, (cyclic headers) but we would if we could
		template<uint32 SIZE>
		inline bool DoComplexCopy(const BufferArray<T,SIZE>& v) const
		{
			return Soy::IsComplexType<T>();
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
		const T*	GetArray() const		{	return mdata;	}
		T*			GetArray()				{	return mdata;	}
		bool		IsEmpty() const			{	return GetSize() == 0;		}
		bool		IsFull() const			{	return GetSize() >= MaxSize();		}
		int			GetSize() const			{	return moffset;		}
		int			GetDataSize() const		{	return GetSize() * sizeof(T);	}	//	size of all the data in bytes
		int			GetElementSize() const	{	return sizeof(T);	}	//	size of all the data in bytes

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
		T* PushReinterpretBlock(const THATTYPE& OtherData)
		{
			int ThatDataSize = sizeof(OtherData);
			T* pData = PushBlock( ThatDataSize / sizeof(T) );
			if ( !pData )
				return NULL;

			//	memcpy over the block
			memcpy( pData, &OtherData, ThatDataSize );
			return pData;
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
				return NULL;
			}
			
			moffset = endoff;
			//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
			for ( int i=curoff;	i<curoff+count;	i++ )
				mdata[i] = T();
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
		void PushBackArray(const ARRAYTYPE& v)
		{
			int NewDataIndex = GetSize();
			T* pNewData = PushBlock( v.GetSize() );

			if ( DoComplexCopy(v) )
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
		}

		//	pushback a c-array
		template<size_t CARRAYSIZE>
		void PushBackArray(const T(&CArray)[CARRAYSIZE])
		{
			int NewDataIndex = GetSize();
			T* pNewData = PushBlock( CARRAYSIZE );

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
			PushBlock(count);	// increase array mem 

			if ( DoComplexCopy(*this) )
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

			if ( DoComplexCopy(*this) )
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
			for ( int i=0;	i<GetSize();	i++ )
			{
				const T& Element = mdata[i];
				if ( Element == Match )
					return i;
			}
			return -1;
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
		inline bool	operator==(const ARRAYTYPE& Array)
		{
			auto ThisBridge = GetArrayBridge( *this );
			auto ThatBridge = GetArrayBridge( Array );
			return ThisBridge.Matches( ThatBridge );
		}

		template<class ARRAYTYPE>
		inline bool	operator!=(const ARRAYTYPE& Array)
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
		T			mdata[mmaxsize];
		int			moffset;
	};

//} // namespace Soy

//	any type that uses a BufferArray can be sped up when in use of an array with
//	DECLARE_NONCOMPLEX_TYPE( BufferArray<yourtype,SIZE> );



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
		if ( !SetSize( v.GetSize(),false ) )
			return false;

		if ( DoComplexCopy(v) )
		{
			for ( int i=0; i<GetSize(); ++i )
				(*this)[i] = v[i];	//	use [] operator for bounds check
		}
		else if ( GetSize() > 0 )
		{
			memcpy( mdata, v.GetArray(), GetSize() * sizeof(T) );
		}
		return true;
	}

	//	different type array, must do complex copy.
	template<class ARRAYTYPE>
	inline bool DoComplexCopy(const ARRAYTYPE& v) const
	{
		return true;
	}

	//	specialisation for an array of T (best case scenario T is non-complex)
	//	gr: unfortunately, cannot test Array<T> here, (cyclic headers) but we would if we could
	template<uint32 SIZE>
	inline bool DoComplexCopy(const BufferArray<T,SIZE>& v) const
	{
		return Soy::IsComplexType<T>();
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
	T* PushReinterpretBlock(const THATTYPE& OtherData)
	{
		int ThatDataSize = sizeof(OtherData);
		T* pData = PushBlock( ThatDataSize / sizeof(T) );
		if ( !pData )
			return NULL;

		//	memcpy over the block
		memcpy( pData, &OtherData, ThatDataSize );
		return pData;
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
			return NULL;
		}
			
		moffset = endoff;
		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		for ( int i=curoff;	i<curoff+count;	i++ )
			mdata[i] = T();
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
	void PushBackArray(const ARRAYTYPE& v)
	{
		int NewDataIndex = GetSize();
		T* pNewData = PushBlock( v.GetSize() );

		if ( DoComplexCopy(v) )
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
	}

	//	pushback a c-array
	template<size_t CARRAYSIZE>
	void PushBackArray(const T(&CArray)[CARRAYSIZE])
	{
		int NewDataIndex = GetSize();
		T* pNewData = PushBlock( CARRAYSIZE );

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
		PushBlock(count);	// increase array mem 

		if ( DoComplexCopy(*this) )
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

		if ( DoComplexCopy(*this) )
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
		for ( int i=0;	i<GetSize();	i++ )
		{
			const T& Element = mdata[i];
			if ( Element == Match )
				return i;
		}
		return -1;
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
	inline bool	operator==(const ARRAYTYPE& Array)
	{
		auto ThisBridge = GetArrayBridge( *this );
		auto ThatBridge = GetArrayBridge( Array );
		return ThisBridge.Matches( ThatBridge );
	}

	template<class ARRAYTYPE>
	inline bool	operator!=(const ARRAYTYPE& Array)
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

//	just so I can use auto
template <typename TYPE,unsigned int BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(TYPE (& Buffer)[BUFFERSIZE],int& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

//	just so I can use auto
template <typename TYPE,unsigned int BUFFERSIZE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE (& Buffer)[BUFFERSIZE],const int& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferCounter );
}

//	just so I can use auto
template <typename TYPE>
inline RemoteArray<TYPE>	GetRemoteArray(const TYPE* Buffer,const int BufferSize,const int& BufferCounter)
{
	return RemoteArray<TYPE>( Buffer, BufferSize, BufferCounter );
}



#endif
