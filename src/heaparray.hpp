#pragma once


#include <cassert>
#include <cstddef>
#include "types.hpp"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...
#include "array.hpp"
#include "memheap.hpp"
	


//	gr: this is exactly the same as an Array type, but uses a prmem::Heap to allocate from.
//		As long as it doesn't have any repurcussions, the original Array type should
//		be able to be replaced by this, but with a default Heap
//	gr: This should always allocate from a heap now, by default the "pr-global heap"
//		with the exception of heaparrays that are allocated globally before the prcore::Heap
template <typename T,prmem::Heap& HEAP=prcore::Heap>
class Array
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)

public:

	Array()
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( HEAP );
	}

	explicit Array(prmem::Heap& Heap)
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( Heap );
	}

	Array(const int size)
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( HEAP );
		SetSize(size);
	}

	Array(const unsigned int size)
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( HEAP );
		SetSize(size);
	}

	//	need an explicit constructor of self-type
	Array(const Array& v)
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( HEAP );
		Copy( v );
	}

	//	explicit to avoid accidental implicit array-conversions (eg. when passing BufferArray to Array param)
	template<typename ARRAYTYPE>
	explicit Array(const ARRAYTYPE& v)
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		SetHeap( &HEAP );
		Copy( v );
	}

	//	construct from a C-array like int Hello[2]={0,1}; this automatically sets the size
	template<size_t CARRAYSIZE>
	explicit Array(const T(&CArray)[CARRAYSIZE])
	: mdata(NULL),mmaxsize(0),moffset(0),mHeap(NULL)
	{
		PushBackArray( CArray );
	}

	~Array()
	{
		//	gr; for safety, if there is no heap, we use normal new
		if ( mdata && mHeap )
			mHeap->FreeArray( mdata, mmaxsize );
		else if ( mdata )
			delete[] mdata;
	}

	template<typename ARRAYTYPE>
	void operator = (const ARRAYTYPE& v)
	{
		Copy(v);
	}

	//	need an explicit = operator for self-type
	void operator = (const Array& v)
	{
		Copy(v);
	}

	template<typename ARRAYTYPE>
	void Copy(const ARRAYTYPE& v)
	{
		SetSize(v.GetSize(),false,true);

		if ( DoComplexCopy(v) )
		{
			for ( int i=0; i<GetSize(); ++i )
				(*this)[i] = v[i];	//	use [] operator for bounds check
		}
		else if ( GetSize() > 0 )
		{
			memcpy( mdata, v.GetArray(), GetSize() * sizeof(T) );
		}
	}

	//	different type array, must do complex copy.
	template<class ARRAYTYPE>
	inline bool DoComplexCopy(const ARRAYTYPE& v) const
	{
		return true;
	}

	//	specialisation for an array of T (best case scenario T is non-complex)
	//	gr: unfortunately, cannot test BufferArray<T> here, (cyclic headers) but we would if we could
	inline bool DoComplexCopy(const Array& v) const
	{
		return Soy::IsComplexType<T>();
	}

	T& GetAt(int index)
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}

	const T& GetAtConst(int index) const
	{
		assert( index >= 0 && index < moffset );
		return mdata[index];
	}
	T&			operator [] (int index)				{	return GetAt(index);	}
	const T&	operator [] (int index) const		{	return GetAtConst(index);	}

	T&			GetBack()		{	return (*this)[GetSize()-1];	}
	const T&	GetBack() const	{	return (*this)[GetSize()-1];	}

	bool IsEmpty() const
	{
		return moffset == 0;
	}

	int GetSize() const
	{
		return moffset;
	}

	//	size of all the data in bytes
	int GetDataSize() const
	{
		return GetSize() * sizeof(T);
	}

	const T* GetArray() const
	{
		return mdata;
	}

	T* GetArray()
	{
		return mdata;
	}

	void SetSize(int size, bool preserve = false,bool AllowLess=false)
	{
		assert( size >= 0 );
		if ( size < 0 )	
			size = 0;

		if ( size == mmaxsize )
		{
			moffset = size;
			return;
		}

		if ( AllowLess && size <= mmaxsize )
		{
			moffset = size;
			return;
		}

		T* array = NULL;

		if ( size )
		{
			//	gr; for safety, if there is no heap, we use normal new
			if ( mHeap )
				array = mHeap->AllocArray<T>(size);
			else
				array = new T[size];

			if ( mdata && preserve )
			{
				int size0 = GetSize();
				int count = (size0 > size) ? size : size0;

				if ( DoComplexCopy(*this) )
				{
					for ( int i=0; i<count; ++i )
						array[i] = mdata[i];
				}
				else if ( count > 0 )
				{
					memcpy( array, mdata, count * sizeof(T) );
				}
			}
		}

		if ( mdata )
		{
			//	gr; for safety, if there is no heap, we use normal new
			if ( mHeap )
				mHeap->FreeArray( mdata, mmaxsize );
			else
				delete[] mdata;
		}
		mdata    = array;
		mmaxsize = size;
		moffset  = size;
	}

	void Reserve(int size,bool clear=true)
	{
		assert( size >= 0 );
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

		if ( endoff >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				int size = count * 2;

				//	gr; for safety, if there is no heap, we use normal new
				if ( mHeap )
					mdata = mHeap->AllocArray<T>( size );
				else
					mdata = new T[size];

				mmaxsize = size;
			}
			else
			{
				int size = mmaxsize * 2 + count;
				SetSize(size,true);
			}
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
		if ( moffset >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				//	gr; for safety, if there is no heap, we use normal new
				if ( mHeap )
					mdata = mHeap->AllocArray<T>( 4 );
				else
					mdata = new T[4];
				mmaxsize = 4;
				moffset = 0;
			}
			else
			{
				int offset = moffset;
				int size = mmaxsize * 2;
				SetSize(size,true);
				moffset = offset;
			}
		}

		T& ref = mdata[moffset++];
		ref = item;
		return ref;
	}

	T& PushBack()
	{
		if ( moffset >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				//	gr; for safety, if there is no heap, we use normal new
				if ( mHeap )
					mdata = mHeap->AllocArray<T>( 4 );
				else
					mdata = new T[4];
				mmaxsize = 4;
				moffset = 0;
			}
			else
			{
				int offset = moffset;
				int size = mmaxsize * 2;
				SetSize(size,true);
				moffset = offset;
			}
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

	T PopAt(int Index)
	{
		T Item = GetAt(Index);
		RemoveBlock( Index, 1 );
		return Item;
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
			assert( (moffset-count) == static_cast<int>(dest - mdata) );
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
		
	prmem::Heap&	GetHeap() const	{	return *mHeap;	}
	void			SetHeap(prmem::Heap& Heap)
	{
		//	if heap isn't valid yet (eg. if this has been allocated before the global heap)
		//	we'll use NULL (and revert to new/delete)
		prmem::Heap* pNewHeap = &Heap;
		if ( !Heap.IsHeapValid() )
			pNewHeap = NULL;

		//	if heap changes and we have allocated data we should re-allocate the data...
		bool ReAlloc = mdata && (pNewHeap!=mHeap);

		//	currently we don't support re-allocating to another heap. 
		//	It's quite trivial, but usually indicates an unexpected situation so leaving it for now
		assert( !ReAlloc );
		mHeap = pNewHeap;
	}

private:
	prmem::Heap*	mHeap;		//	where to alloc/free from
	T*				mdata;
	int				mmaxsize;
	int				moffset;
};
