#pragma once

#include "array.hpp"
#include "HeapArray.hpp"
#include "SoyRef.h"
#include "SoyThread.h"

namespace Soy
{
	namespace Platform
	{
#if defined(TARGET_WINDOWS)
		const static HANDLE		InvalidFileHandle = nullptr;
#endif
#if defined(TARGET_OSX)
		const static int		InvalidFileHandle = -1;
#endif
	}
}

class MemFileArray : public ArrayInterface<char>
{
public:
	typedef ArrayInterface<char>::TYPE T;
	typedef ArrayInterface<char>::TYPE TYPE;
	
public:
	MemFileArray(std::string Filename,int Size,bool ReadOnly=false);	//	size must be known beforehand!
	~MemFileArray()
	{
		Close();
	}

	std::string			GetFilename() const				{	return mFilename;	}
	void				Close();						//	close handle but don't destroy file
	void				Destroy();						//	destroy shared memory
	bool				IsValid() const;

	virtual T&			operator [] (int index)			{	return GetArray()[index];	}
	virtual const T&	operator [] (int index) const	{	return GetArray()[index];	}
	virtual T&			GetBack()						{	return (*this)[GetSize()-1];	}
	virtual const T&	GetBack() const					{	return (*this)[GetSize()-1];	}
	bool				IsEmpty() const					{	return GetSize() == 0;	}
	bool				IsFull() const					{	return GetSize() >= MaxSize();	}
	virtual int			GetSize() const					{	return mOffset;	}
	virtual const T*	GetArray() const				{	return CreateMap();	}
	virtual T*			GetArray()						{	return CreateMap();	}
	virtual int			MaxSize() const					{	return mMapSize;	}
	virtual void		Reserve(int size,bool clear=false)
	{
		if ( clear )
		{
			SetSize(size,false);
			mOffset = 0;
		}
		else
		{
			//	grow the array as neccessary, then restore the current size again
			int CurrentSize = GetSize();
			SetSize( CurrentSize + size, true, true );
			SetSize( CurrentSize, true, true );
		}
	}
	
	virtual T* InsertBlock(int index, int count)
	{
		 T*& mData = reinterpret_cast<T*&>(mMap);
		//	do nothing if nothing to add
		assert( count >= 0 );
		if ( count == 0 )
			return IsEmpty() ? NULL : (mData + index);

		if ( index >= mOffset ) 
			return PushBlock( count );

//			int left = static_cast<int>((mdata+moffset) - (mdata+index));
		int left = static_cast<int>(mOffset - index);
		PushBlock(count);	// increase array mem 

		if ( Soy::DoComplexCopy<T,T>() )
		{
			T* src = mData + mOffset - count - 1;
			T* dest = mData + mOffset - 1;
			for ( int i=0; i<left; ++i )
				*dest-- = *src--;
		}
		else if ( left > 0 )
		{
			memmove( &mData[index+count], &mData[index], left * sizeof(T) );
		}

		return mData + index;
	}

	virtual void		RemoveBlock(int index, int count)
	{
		 T*& mData = reinterpret_cast<T*&>(mMap);

		//	do nothing if nothing to remove
		assert( count >= 0 );
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mData + index;
		T* src = mData + index + count;
		int left = static_cast<int>((mData+mOffset) - src);

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( int i=0; i<left; ++i )
				*dest++ = *src++;
			mOffset = static_cast<int>(dest - mData);
		}
		else
		{
			if ( left > 0 )
				memmove( dest, src, left * sizeof(T) );
			mOffset -= count;
		}			
	}
		
	virtual void		Clear(bool Dealloc=true)
	{
		bool AllowLess = !Dealloc;
		SetSize(0,false,AllowLess);
	}

	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	bool SetSize(int size, bool preserve=true,bool AllowLess=true)
	{
		assert( size >= 0 );
		if ( size < 0 )	
			size = 0;

		//	limit size
		//	gr: assert, safely alloc, and return error. Maybe shouldn't "safely alloc"
		//	gr: assert over limit, don't silently fail
		assert( size <= MaxSize() );
		if ( size > MaxSize() )
		{
			size = MaxSize();
			mOffset = size;
			return false;
		}
				
		mOffset = size;
		return true;
	}
		
	virtual T*			PushBlock(int count)
	{
		T*& mData = reinterpret_cast<T*&>(mMap);
		assert( count >= 0 );
		if ( count < 0 )	count = 0;
		int curoff = GetSize();
		int endoff = GetSize() + count;

		//	fail if we try and "allocate" too much
		if ( endoff > MaxSize() )
		{
			assert( endoff <= MaxSize() );
			return NULL;
		}
			
		mOffset = endoff;
		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		for ( int i=curoff;	i<curoff+count;	i++ )
			mData[i] = T();
		return mData + curoff;
	}


	//	needed for arraybridge
	T&			PushBack(const T& item);
	T&			PushBack();


private:
	const T*			CreateMap() const				{	return const_cast<MemFileArray*>(this)->CreateMap();	}
	T*					CreateMap()						{	return reinterpret_cast<T*>(mMap);	}

	bool				CreateFile(size_t Size,std::stringstream& Error);		//	fails if already exists
	bool				OpenWriteFile(std::stringstream& Error);
	bool				OpenReadOnlyFile(std::stringstream& Error);
	
private:
#if defined(TARGET_WINDOWS)
	HANDLE				mHandle;
#endif
#if defined(TARGET_OSX)
	int					mHandle;
#endif
	std::string			mFilename;
	void*				mMap;
	int					mMapSize;

	int					mOffset;	//	for running data like bufferarray
};


class SoyMemFileManager
{
public:
	SoyMemFileManager(std::string FilenamePrefix,SoyRef FirstRef=SoyRef("File")) :
		mFilenamePrefix		( FilenamePrefix ),
		mFilenameRef		( FirstRef )
	{
	}

	ofPtr<MemFileArray>			AllocFile(const ArrayBridge<char>& Data);
	ofPtr<MemFileArray>			AllocFile(const ArrayBridge<char>&& Data)
	{
		return AllocFile( Data );
	}

private:
	ofMutex						mFileLock;
	Array<ofPtr<MemFileArray>>	mFiles;
	std::string					mFilenamePrefix;
	SoyRef						mFilenameRef;
};





inline MemFileArray::T& MemFileArray::PushBack(const T& item)
{
	T*& mData = reinterpret_cast<T*&>(mMap);
	
	//	out of space
	if ( mOffset >= MaxSize() )
	{
		assert( mOffset < MaxSize() );
		return mData[ mOffset-1 ];
	}

	T& ref = mData[mOffset++];
	ref = item;
	return ref;
}

inline MemFileArray::T& MemFileArray::PushBack()
{
	T*& mData = reinterpret_cast<T*&>(mMap);

	//	out of space
	if ( mOffset >= MaxSize() )
	{
		assert( mOffset < MaxSize() );
		return mData[ mOffset-1 ];
	}

	//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
	T& ref = mData[mOffset++];
	ref = T();
	return ref;
}
