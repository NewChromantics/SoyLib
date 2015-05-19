#pragma once

#include "array.hpp"
#include "heaparray.hpp"
#include "RemoteArray.h"

//	gr: adapt this to
//	a) have an array interface so we can use it as an arraybridge
//	b) use whatever underlying array type we want
template<typename ARRAY>
class RingArray
{
public:
	typedef typename ARRAY::TYPE TYPE;
	
public:
	RingArray(size_t InitialSize) :
		mHead	( 0 ),
		mTail	( 0 )
	{
		ResizeBuffer( InitialSize );
		//	init tail
	//	mTail = mBuffer.GetSize()-1;
	}

	bool		PushBack(const TYPE& Element);
	bool		PushBack(const ArrayBridge<TYPE>& Array);
	bool		PushBack(const ArrayBridge<TYPE>&& Array)			{	return PushBack( Array );	}
	bool		PopFront(size_t Elements,ArrayBridge<TYPE>& Array);
	bool		PopFront(size_t Elements,ArrayBridge<TYPE>&& Array)	{	return PopFront( Elements, Array );	}
	bool		PopFront(TYPE& Element);

private:
	bool		ResizeBuffer(size_t NewSize);
	void		GetStats(size_t& SpaceAfterHead,size_t& SpaceBeforeTail,size_t& UsedAfterTail,size_t& UsedAfterStart);
	
private:
	std::recursive_mutex	mLock;	//	maybe lock head and tail seperately?
	size_t			mHead;	//	end of used ring (where to push to)
	size_t			mTail;	//	start of used ring (where to pop from)
	ARRAY			mBuffer;
};


template<typename TYPE>
inline bool RingArray<TYPE>::ResizeBuffer(size_t NewSize)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	mBuffer.SetSize( static_cast<int>(NewSize) );

	if ( mBuffer.IsEmpty() )
		return false;

	return true;
}

template<typename TYPE>
inline void RingArray<TYPE>::GetStats(size_t& SpaceAfterHead,size_t& SpaceBeforeTail,size_t& UsedAfterTail,size_t& UsedAfterStart)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );

	SpaceAfterHead = (mTail > mHead) ? (mTail - mHead) : (mBuffer.GetSize() - mHead);
	SpaceBeforeTail = (mTail > mHead) ? (mTail - mHead ) : (mTail - 0);
	UsedAfterTail = (mHead >= mTail) ? (mHead - mTail) : (mBuffer.GetSize() - mTail);
	UsedAfterStart = (mHead > mTail) ? 0 : mHead;
	
	auto Used = UsedAfterTail+UsedAfterStart;
	auto Space = SpaceAfterHead+SpaceBeforeTail;
	Soy::Assert( Used + Space == mBuffer.GetSize(), "Bad calculations" );
	//Soy::Assert( mHead != mTail, "Head and tail shouldn't clash");
}

template<typename TYPE>
inline bool RingArray<TYPE>::PushBack(const TYPE& Element)
{
	size_t Counter = 1;
	auto AsArray = GetRemoteArray( &Element, Counter );
	return PushBack( GetArrayBridge( AsArray ) );
}

template<typename TYPE>
inline bool RingArray<TYPE>::PopFront(TYPE& Element)
{
	//	fill this array-of-one by writing directly into the element
	int Counter = 0;
	auto AsArray = GetRemoteArray( &Element, 1, Counter );
	return PopFront( 1, GetArrayBridge( AsArray ) );
}

template<typename TYPE>
inline bool RingArray<TYPE>::PushBack(const ArrayBridge<TYPE>& Array)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );

	size_t SpaceAfterHead,SpaceBeforeTail,UsedAfterTail,UsedAfterStart;
	GetStats( SpaceAfterHead, SpaceBeforeTail, UsedAfterTail, UsedAfterStart );
	
	//	not enough space in total?
	//	gr: if we completely fill the buffer, head and tail will collide
	if ( SpaceAfterHead + SpaceBeforeTail <= Array.GetSize() )
	{
		size_t Spacer = 1;
		ResizeBuffer( UsedAfterTail + UsedAfterStart + Spacer + Array.GetSize() );
		return PushBack( Array );
	}
	
	//	do first chunk (fills end of buffer)
	size_t ArrayWritten = 0;
	{
		auto ArraySize = Array.GetSize();
		auto DestSize = (ArraySize <= SpaceAfterHead) ? ArraySize : SpaceAfterHead;
		auto Destination = GetRemoteArray( &mBuffer[static_cast<int>(mHead)], DestSize, DestSize );
		auto SrcSize = DestSize;
		auto Source = GetRemoteArray( &Array[0], SrcSize, SrcSize );
		
		Destination.Copy( Source );
		ArrayWritten += SrcSize;
		mHead += Destination.GetSize();
		//	loop
		if ( mHead >= mBuffer.GetSize() )
			mHead -= mBuffer.GetSize();
		
		//	gr: add overlap check. will be needed if locks seperate
	}

	//	do second chunk starts at start of buffer
	{
		auto ArrayRemain = Array.GetSize() - ArrayWritten;
		if ( ArrayRemain > 0 )
		{
			auto ArraySecondHalf = GetRemoteArray( &Array[ArrayWritten], ArrayRemain );
			return PushBack( GetArrayBridge( ArraySecondHalf ) );
		}
	}
	
	return true;
}

template<typename TYPE>
inline bool RingArray<TYPE>::PopFront(size_t Elements,ArrayBridge<TYPE>& Array)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );

	size_t SpaceAfterHead,SpaceBeforeTail,UsedAfterTail,UsedAfterStart;
	GetStats( SpaceAfterHead, SpaceBeforeTail, UsedAfterTail, UsedAfterStart );

	//	not this much to pop
	if ( UsedAfterTail + UsedAfterStart < Elements )
		return false;
	
	//	pop first chunk
	{
		size_t PopAfterTail = (Elements <= UsedAfterTail) ? Elements : UsedAfterTail;
		auto* Popped = Array.PushBlock( static_cast<int>(PopAfterTail) );
		memcpy( Popped, &mBuffer[static_cast<int>(mTail)], sizeof(TYPE)*PopAfterTail );
		mTail += PopAfterTail;
		//	loop
		if ( mTail >= mBuffer.GetSize() )
			mTail -= mBuffer.GetSize();
		Elements -= PopAfterTail;

		//	gr: add overlap check. will be needed if locks seperate
	}
	
	//	pop second chunk
	{
		if ( Elements > 0 )
			return PopFront( Elements, Array );
	}

	return true;
}

