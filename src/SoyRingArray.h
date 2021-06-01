#pragma once

#include "Array.hpp"
#include "HeapArray.hpp"
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
	RingArray(size_t InitialSize=0) :
		mHead	( 0 ),
		mTail	( 0 )
	{
		ResizeBuffer( InitialSize );
		//	init tail
	//	mTail = mBuffer.GetSize()-1;
	}

	void		PushBack(const TYPE& Element);
	void		PushBack(const ArrayBridge<TYPE>& Array);
	void		PushBack(const ArrayBridge<TYPE>&& Array)			{	return PushBack( Array );	}
	void		PopFront(size_t Elements,ArrayBridge<TYPE>& Array);
	void		PopFront(size_t Elements,ArrayBridge<TYPE>&& Array)	{	return PopFront( Elements, Array );	}
	void		PopFront(TYPE& Element);

private:
	void		ResizeBuffer(size_t NewSize);
	void		GetStats(size_t& SpaceAfterHead,size_t& SpaceBeforeTail,size_t& UsedAfterTail,size_t& UsedAfterStart);
	void		Verify();
	
private:
	std::recursive_mutex	mLock;	//	maybe lock head and tail seperately?
	size_t			mHead;	//	end of used ring (where to push to)
	size_t			mTail;	//	start of used ring (where to pop from)
	ARRAY			mBuffer;
};


template<typename TYPE>
inline void RingArray<TYPE>::ResizeBuffer(size_t NewSize)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	mBuffer.SetSize( static_cast<int>(NewSize) );
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
	if ( Used + Space != mBuffer.GetSize() )
	{
		throw Soy::AssertException("SoyRingBuffer Bad calculations");
	}
	//Soy::Assert( mHead != mTail, "Head and tail shouldn't clash");
}

template<typename TYPE>
inline void RingArray<TYPE>::Verify()
{
	size_t SpaceAfterHead,SpaceBeforeTail,UsedAfterTail,UsedAfterStart;
	GetStats(SpaceAfterHead,SpaceBeforeTail,UsedAfterTail,UsedAfterStart);
}

template<typename TYPE>
inline void RingArray<TYPE>::PushBack(const TYPE& Element)
{
	size_t Counter = 1;
	auto AsArray = GetRemoteArray( &Element, Counter );
	PushBack( GetArrayBridge( AsArray ) );
}

template<typename TYPE>
inline void RingArray<TYPE>::PopFront(TYPE& Element)
{
	//	fill this array-of-one by writing directly into the element
	int Counter = 0;
	auto AsArray = GetRemoteArray( &Element, 1, Counter );
	PopFront( 1, GetArrayBridge( AsArray ) );
}

template<typename TYPE>
inline void RingArray<TYPE>::PushBack(const ArrayBridge<TYPE>& Array)
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
		PushBack( Array );
		return;
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
		Verify();
	}

	//	do second chunk starts at start of buffer
	{
		auto ArrayRemain = Array.GetSize() - ArrayWritten;
		if ( ArrayRemain > 0 )
		{
			auto ArraySecondHalf = GetRemoteArray( &Array[ArrayWritten], ArrayRemain );
			PushBack( GetArrayBridge( ArraySecondHalf ) );
			return;
		}
	}
}

template<typename TYPE>
inline void RingArray<TYPE>::PopFront(size_t Elements,ArrayBridge<TYPE>& Array)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );

	size_t SpaceAfterHead,SpaceBeforeTail,UsedAfterTail,UsedAfterStart;
	GetStats( SpaceAfterHead, SpaceBeforeTail, UsedAfterTail, UsedAfterStart );

	//	not this much to pop
	if ( UsedAfterTail + UsedAfterStart < Elements )
		throw Soy::AssertException("Trying to pop more data than there is used in ring buffer");
	
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
		Verify();

		//	gr: add overlap check. will be needed if locks seperate
	}
	
	//	pop second chunk
	{
		if ( Elements > 0 )
		{
			PopFront( Elements, Array );
			return;
		}
	}
}

