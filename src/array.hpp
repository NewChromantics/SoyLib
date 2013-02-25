/*
	Twilight Prophecy SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2003 Twilight 3D Finland Oy Ltd.
*/
#ifndef Soy_ARRAY_HPP
#define Soy_ARRAY_HPP


#include <cassert>
#include <cstddef>
#include "types.hpp"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...

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
	class ArrayBridge
	{
	public:
		ArrayBridge()	{}

		virtual T&			operator [] (int index)=0;
		virtual const T&	operator [] (int index) const=0;
		virtual bool		IsEmpty() const=0;
		virtual int			GetSize() const=0;
		virtual int			GetDataSize() const=0;
		virtual const T*	GetArray() const=0;
		virtual T*			GetArray()=0;
		virtual void		SetSize(int size,bool preserve=false,bool AllowLess=false)=0;
		virtual void		Reserve(int size,bool clear=true)=0;
		virtual T*			PushBlock(int count)=0;
		virtual T&			PushBack(const T& item)=0;
		virtual T&			PushBack()=0;
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
				for ( int i=0;	i<GetSize();	i++ )
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
		ArrayBridgeDef(const ARRAY& Array) :
			mArray	( const_cast<ARRAY&>(Array) )
		{
		}

		virtual T&			operator [] (int index)			{	return mArray[index];	}
		virtual const T&	operator [] (int index) const	{	return mArray[index];	}
		virtual bool		IsEmpty() const					{	return mArray.IsEmpty();	}
		virtual int			GetSize() const					{	return mArray.GetSize();	}
		virtual int			GetDataSize() const				{	return mArray.GetDataSize();	}
		virtual const T*	GetArray() const				{	return mArray.GetArray();	}
		virtual T*			GetArray()						{	return mArray.GetArray();	}
		virtual void		SetSize(int size,bool preserve,bool AllowLess)	{	return mArray.SetSize(size,preserve,AllowLess);	}
		virtual void		Reserve(int size,bool clear)	{	return mArray.Reserve(size,clear);	}
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


	template <typename T>
	class Array
	{
	public:
		typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)

	public:
		Array()
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
		}

		Array(const int size)
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
			SetSize(size);
		}

		Array(const unsigned int size)
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
			SetSize(size);
		}

		//	need an explicit constructor of self-type
		Array(const Array& v)
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
			Copy( v );
		}

		//	explicit to avoid accidental implicit array-conversions (eg. when passing BufferArray to HeapArray param)
		template<typename ARRAYTYPE>
		explicit Array(const ARRAYTYPE& v)
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
			Copy( v );
		}

		//	construct from a C-array like int Hello[2]={0,1}; this automatically sets the size
		template<size_t CARRAYSIZE>
		explicit Array(const T(&CArray)[CARRAYSIZE])
		: mdata(NULL),mmaxsize(0),moffset(0)
		{
			PushBackArray( CArray );
		}

		~Array()
		{
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

		void SetSize(int size,bool preserve=false,bool AllowLess=false)
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

			if ( mdata ) delete[] mdata;
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

			if ( IsComplexType<T>() )
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

		void Remove(const T& item)
		{
			int count = GetSize();
			T* src = mdata;
			T* dest = mdata;

			for ( int i=0; i<count; ++i )
			{
				if ( *src != item )
				{
					if ( src != dest )
						*dest = *src;

					++dest;
				}
				++src;
			}
			moffset = static_cast<int>(dest - mdata);
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
		
		private:

		T*		mdata;
		int		mmaxsize;
		int		moffset;
	};




//} // namespace Soy



//----------------------------------------
//----------------------------------------
template<typename T,bool AUTODEALLOC=false>
class PtrArray : public Array<T*>
{
public:
	~PtrArray()
	{
		if ( AUTODEALLOC )
			DeleteAll();
	}

	void		DeleteAll()
	{
		for ( int i=0;	i<GetSize();	i++ )
		{
			T*& pElement = GetAt(i);
			if ( !pElement )
				continue;
			delete pElement;
			pElement = NULL;
		}
		Clear();
	}

	template<typename MATCHTYPE>
	int			FindIndex(const MATCHTYPE& Match) const
	{
		for ( int i=0;	i<GetSize();	i++ )
		{
			T*const& pElement = GetAtConst(i);
			if ( !pElement )
				continue;
			const T& Element = *pElement;
			if ( Element == Match )
				return i;
		}
		return -1;
	}

	//	find an element - returns first matching element or NULL
	template<typename MATCH> T*			Find(const MATCH& Match)		{	int Index = FindIndex( Match );		return (Index < 0) ? NULL : GetAt(Index);	}
	template<typename MATCH> const T*	Find(const MATCH& Match) const	{	int Index = FindIndex( Match );		return (Index < 0) ? NULL : GetAtConst(Index);	}
	
	T& operator [] (int index)				{	return *GetAt(index);	}
	const T& operator [] (int index) const	{	return *GetAtConst(index);	}
};


#endif
