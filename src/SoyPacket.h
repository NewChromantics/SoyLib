#pragma once

#define SOYPACKET_PROOF	SoyRef("Soylent")
#define SOYPACKET_SIZE_MAX	(1*1024*1024)	//	1mb - used to catch corrupt packets and stop us allocating silly memory


#define case_OnSoyPacket(PACKETTYPE)	case PACKETTYPE::TYPE:	return OnPacket( Packet.GetAs<PACKETTYPE>() );


//-------------------------------------------
//	every packet should start with this
//-------------------------------------------
class SoyPacketMeta
{
private:
	static const uint32	InvalidType = -1;

public:
	SoyPacketMeta(const SoyRef& Sender=SoyRef()) :
		mSoylentProof	( SOYPACKET_PROOF ),
		mSender			( Sender ),
		mType			( InvalidType ),
		mDataSize		( 0 )
	{
	}

	SoyPacketMeta&			GetMeta()			{	return *this;	}
	const SoyPacketMeta&	GetMeta() const		{	return *this;	}
	bool					IsValid() const		{	return (mSoylentProof == SOYPACKET_PROOF) && (mDataSize<=SOYPACKET_SIZE_MAX) && IsValidType();	}
	bool					IsValidType() const	{	return mType != InvalidType;	}

public:
	SoyRef		mSoylentProof;	//	all packets should start with this
	SoyRef		mSender;		//	module that sent this packet
	uint32		mType;			//	enum -> int. Replace with SoyRef conversion or something
	uint32		mDataSize;		//	following data is N bytes long (does not include header)
};


class SoyPacketContainer
{
public:
	SoyPacketContainer()
	{
	}
	explicit SoyPacketContainer(const Array<char>& PacketData,const SoyPacketMeta& Meta) :
		mMeta	( Meta )
	{
		//	copy raw data
		mData.PushBackArray( PacketData );

		//	init meta
		mMeta.mDataSize = mData.GetDataSize();
	
		//	type needs to have already been set if we're using raw packet data
		assert( mMeta.IsValidType() );
	}

	template<class TYPE>
	explicit SoyPacketContainer(const TYPE& Packet,const SoyPacketMeta& Meta) :
		mMeta	( Meta )
	{
		//	copy raw data
		mData.PushReinterpretBlock( Packet );

		//	init meta
		mMeta.mDataSize = mData.GetDataSize();
		mMeta.mType = Packet.GetType();
	}

	template<class TYPE>
	TYPE&			GetAs()
	{
		assert( sizeof(TYPE) == mData.GetDataSize() );
		return *reinterpret_cast<TYPE*>( mData.GetArray() );
	}
	template<class TYPE>
	const TYPE&			GetAs() const
	{
		assert( sizeof(TYPE) == mData.GetDataSize() );
		return *reinterpret_cast<const TYPE*>( mData.GetArray() );
	}

	void			GetPacketRaw(Array<char>& RawData)
	{
		RawData.PushReinterpretBlock( mMeta );
		RawData.PushBackArray( mData );
	}

public:
	SoyPacketMeta	mMeta;
	Array<char>		mData;
};

template<typename PACKETENUM>
class SoyPacket
{
public:
	virtual PACKETENUM	GetType() const=0;
};


template<typename PACKETENUM>
class SoyPacketManager : public ofMutex
{
public:
	typedef SoyPacket<PACKETENUM> TPacket;

public:
	bool					IsEmpty() const	{	return mPackets.IsEmpty();	}	//	read is so minute we don't need a lock

	template<class PACKET>
	void					PushPacket(const SoyPacketMeta& Meta,const PACKET& Packet);
	void					PushPacket(const SoyPacketMeta& Meta,const Array<char>& Data);	//	push a raw packet onto the stack

	bool					PopPacket(SoyPacketContainer& Container);	//	pops the next packet. caller takes ownership of data
	bool					PopPacketRawData(Array<char>& PacketData);	//	pops the next packet into raw data.
	
	//	incomplete packets where we've read a header, but not the data [yet]
	bool					PeekPendingPacket(SoyPacketMeta& Meta,const SoyRef& SenderAddress);				//	see if there is a pending packet for this sender
	bool					FinishPendingPacket(const Array<char>& PacketData,const SoyRef& SenderAddress);	//	complete & push a pending packet
	bool					PushPendingPacket(const SoyPacketMeta& Meta,const SoyRef& SenderAddress);			//	start a pending packet

protected:
	Array<SoyPair<SoyRef,SoyPacketMeta> >	mPendingPackets;
	Array<SoyPacketContainer>				mPackets;
};


#include "SoyPacket.hpp"
