#pragma once

#include "array.hpp"
#include "HeapArray.hpp"
#include "SoyRef.h"
#include "SoyThread.h"
#include "RemoteArray.h"


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

namespace MemFileAccess
{
	enum Type
	{
		Create,
		Writable,	//	read/write but don't create
		ReadOnly,
	};
};


class MemFileHandle
{
public:
#if defined(TARGET_OSX)
	MemFileHandle(const std::string& Filename,int CreateFlags,int Mode);
#endif
	~MemFileHandle();
	
public:
#if defined(TARGET_WINDOWS)
	HANDLE				mHandle;
#endif
#if defined(TARGET_OSX)
	int					mHandle;
	std::string			mFilename;	//	need filename to close
#endif

};


class SoyMemFile
{
public:
	SoyMemFile(const std::string& Filename,MemFileAccess::Type Access,size_t Size);
	
	FixedRemoteArray<uint8>	GetArray()		{	return FixedRemoteArray<uint8>( reinterpret_cast<uint8*>(mMap), mMapSize );	}
	const std::string&		GetFilename() const	{	return mFilename;	}
	void					Close();
	
private:
	std::string			mFilename;
	std::shared_ptr<MemFileHandle>	mHandle;
	void*				mMap;
	size_t				mMapSize;
};



class MemFileArray : public ArrayInterface<char>
{
public:
	typedef ArrayInterface<char>::TYPE T;
	typedef ArrayInterface<char>::TYPE TYPE;
	
public:
	MemFileArray(std::string Filename,bool AllowOtherFilename);
	MemFileArray(std::string Filename,bool AllowOtherFilename,size_t Size,bool ReadOnly);

	bool				Init(size_t Size,bool ReadOnly);
	bool				Init(size_t Size,bool ReadOnly,std::stringstream& Error);	//	size must be known beforehand!
	
	std::string			GetFilename() const				{	return mFilename;	}
	bool				IsValid() const;

	virtual T&			operator[](size_t index) override		{	return GetArray()[index];	}
	virtual const T&	operator[](size_t index) const override	{	return GetArray()[index];	}
	virtual T&			GetBack() override				{	return (*this)[GetSize()-1];	}
	virtual const T&	GetBack() const override		{	return (*this)[GetSize()-1];	}
	virtual size_t		GetSize() const override		{	return mOffset;	}
	virtual const T*	GetArray() const override		{	return CreateMap();	}
	virtual T*			GetArray() override				{	return CreateMap();	}
	virtual size_t		MaxSize() const override		{	return mMapSize;	}
	virtual void		Reserve(size_t size,bool clear=false) override
	{
		if ( clear )
		{
			SetSize(size,false);
			mOffset = 0;
		}
		else
		{
			//	grow the array as neccessary, then restore the current size again
			auto CurrentSize = GetSize();
			SetSize( CurrentSize + size, true, true );
			SetSize( CurrentSize, true, true );
		}
	}
	
	virtual T* InsertBlock(size_t index, size_t count) override
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

	virtual void		RemoveBlock(size_t index,size_t count) override
	{
		T*& mData = reinterpret_cast<T*&>(mMap);

		//	do nothing if nothing to remove
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mData + index;
		T* src = mData + index + count;
		ssize_t left = (mData+mOffset) - src;

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( ssize_t i=0; i<left; ++i )
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
		
	virtual void		Clear(bool Dealloc=true) override
	{
		bool AllowLess = !Dealloc;
		SetSize(0,false,AllowLess);
	}

	//	gr: AllowLess does nothing here, but the parameter is kept to match other Array types (in case it's used in template funcs for example)
	virtual bool SetSize(size_t size, bool preserve=true,bool AllowLess=true) override;

	virtual T*			PushBlock(size_t count) override
	{
		if ( !Soy::Assert( count > 0, "Can't allocate 0" ) )
			return nullptr;
		
		//	if we haven't allocated, try now
		if ( !Init(count,false) )
			return nullptr;
				 
		T*& mData = reinterpret_cast<T*&>(mMap);
		auto curoff = GetSize();
		auto endoff = GetSize() + count;

		//	fail if we try and "allocate" too much
		if ( endoff > MaxSize() )
		{
			if ( !Soy::Assert( endoff <= MaxSize(), "Cannot allocate more data" ) )
				return nullptr;
		}
			
		mOffset = endoff;

		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		if ( Soy::IsComplexType<TYPE>() )
		{
			for ( size_t i=curoff;	i<curoff+count;	i++ )
				mData[i] = T();
		}
		return mData + curoff;
	}


private:
	const T*			CreateMap() const				{	return const_cast<MemFileArray*>(this)->CreateMap();	}
	T*					CreateMap()						{	return reinterpret_cast<T*>(mMap);	}

#if defined(TARGET_OSX)
	bool				OpenWriteFile(size_t Size,std::stringstream& Error);
	bool				OpenReadOnlyFile(size_t Size,std::stringstream& Error);
#endif

private:
	std::shared_ptr<SoyMemFile>	mMemFile;
	std::string			mFilename;
	bool				mAllowOtherFilename;	//	if we cannot create file, we can try other filenames
	void*				mMap;
	size_t				mMapSize;

	size_t				mOffset;	//	for running data like bufferarray
};


class SoyMemFileManager
{
public:
	SoyMemFileManager(std::string FilenamePrefix,SoyRef FirstRef=SoyRef("File")) :
		mFilenamePrefix		( FilenamePrefix ),
		mFilenameRef		( FirstRef )
	{
	}

	std::shared_ptr<MemFileArray>			AllocFile(const ArrayBridge<char>& Data);
	std::shared_ptr<MemFileArray>			AllocFile(const ArrayBridge<char>&& Data)
	{
		return AllocFile( Data );
	}

private:
	std::mutex					mFileLock;
	Array<std::shared_ptr<MemFileArray>>	mFiles;
	std::string					mFilenamePrefix;
	SoyRef						mFilenameRef;
};




/*
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
*/

