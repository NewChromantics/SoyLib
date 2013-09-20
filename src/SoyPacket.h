#pragma once

#include "SoyNet.h"
#include "SoyPair.h"

#define SOYPACKET_PROOF	SoyRef("Soylent")
#define SOYPACKET_SIZE_MAX	(10*1024*1024)	//	10mb - used to catch corrupt packets and stop us allocating silly memory

							

namespace SoyNet
{
	class TAddress;
};

//-------------------------------------------
//	every packet should start with this
//-------------------------------------------
class SoyPacketMeta
{
public:
	SoyPacketMeta() :
		mSoylentProof	( SOYPACKET_PROOF ),
		mDataSize		( 0 )
	{
	}
	SoyPacketMeta(SoyRef Sender,SoyRef Type) :
		mSoylentProof	( SOYPACKET_PROOF ),
		mSender			( Sender ),
		mType			( Type ),
		mDataSize		( 0 )
	{
	}

	SoyPacketMeta&			GetMeta()			{	return *this;	}
	const SoyPacketMeta&	GetMeta() const		{	return *this;	}
	bool					IsValid() const		{	return (mSoylentProof == SOYPACKET_PROOF) && (mDataSize<=SOYPACKET_SIZE_MAX) && IsValidType();	}
	bool					IsValidType() const	{	return mType.IsValid();	}
	SoyRef					GetType() const		{	return mType;	}

public:
	SoyRef		mSoylentProof;	//	all packets should start with this
	SoyRef		mSender;		//	module that sent this packet
	SoyRef		mType;			//	enum -> int. Replace with SoyRef conversion or something
	uint32		mDataSize;		//	following data is N bytes long (does not include header)
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyPacketMeta& Value)
{
	str << "Packet_" << Value.mType << "[" << Value.mSender << "](" << Value.mDataSize << ")";
	return str;
}


class TArrayWriter
{
public:
	TArrayWriter(Array<char>& Data) :
		mData	( Data )
	{
	}

	template<typename TYPE>
	bool	Write(const TYPE& Value)	
	{
		mData.PushReinterpretBlock( Value );
		return true;
	}

	template<class ARRAY>
	bool WriteArray(const ARRAY& Array)
	{
		//	write element count
		int ArrayLength = Array.GetSize();
	
		//	sensible check here please!
		if ( ArrayLength < 0 || ArrayLength > SOYPACKET_SIZE_MAX )
		{
			assert( false );
			return false;
		}

		mData.PushReinterpretBlock( ArrayLength );

		//	alloc and copy raw data
		char* pData = mData.PushBlock( Array.GetDataSize() );
		memcpy( pData, Array.GetArray(), Array.GetDataSize() );
		return true;
	}

public:
	Array<char>&	mData;
};

class TArrayReader
{
public:
	TArrayReader(const Array<char>& Data) :
		mData			( Data ),
		mDataPosition	( 0 )
	{
	}

	template<typename TYPE>
	bool	Read(TYPE& Value)					
	{	
		return Read( reinterpret_cast<char*>(&Value), sizeof(TYPE) );	
	}
	bool	Read(char* pDest,int Size)		
	{
		//	check we can keep reading
		if ( mDataPosition + Size > mData.GetDataSize() )
			return false;

		//	grab data
		const char* pSrc = &mData[mDataPosition];
		memcpy( pDest, pSrc, Size );

		//	move along
		mDataPosition += Size;
		return true;
	}
	template<class ARRAY>
	bool	ReadArray(ARRAY& ValueArray)
	{
		//	read array length
		int ArrayLength;
		if ( !Read( ArrayLength ) )
			return false;

		//	sensible check here please!
		if ( ArrayLength < 0 || ArrayLength > SOYPACKET_SIZE_MAX )
		{
			assert( false );
			return false;
		}
			
		//	alloc
		ValueArray.SetSize( ArrayLength );

		//	copy
		if ( !Read( reinterpret_cast<char*>( ValueArray.GetArray() ), ValueArray.GetDataSize() ) )
			return false;

		return true;
	}

public:
	const Array<char>&	mData;
	int					mDataPosition;
};


namespace Soy
{
	template<class PACKETTYPE>
	inline bool	ToRawData(Array<char>& Data,const PACKETTYPE& Packet)
	{
		TArrayWriter Writer( Data );
		Writer.Write( Packet );
		return true;
	}

	template<class PACKETTYPE>
	inline bool	FromRawData(PACKETTYPE& Packet,const Array<char>& Data)
	{
		TArrayReader Reader( Data );
		return Reader.Read( Packet );
		/*
		if ( !sizeof(PACKETTYPE) == Data.GetDataSize() )
		{
			assert( false );
			return false;
		}
		Packet = *reinterpret_cast<const PACKETTYPE*>( Data.GetArray() );
		return true;
		*/
	}
};



class SoyPacketContainer
{
public:
	SoyPacketContainer(prmem::Heap& Heap=prcore::Heap) :
		mData	( Heap )
	{
	}
	explicit SoyPacketContainer(const Array<char>& PacketData,const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender,prmem::Heap& Heap=prcore::Heap) :
		mData	( Heap )
	{
		Set( PacketData, Meta, Sender );
	}

	template<class TYPE>
	explicit SoyPacketContainer(const TYPE& Packet,const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender,prmem::Heap& Heap=prcore::Heap) :
		mData	( Heap )
	{
		if ( !Set( Packet, Meta, Sender ) )
		{
			assert( false );
		}
	}

	bool Set(const Array<char>& PacketData,const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
	{
		mSender = Sender;
		mMeta = Meta;

		//	copy raw data
		mData.PushBackArray( PacketData );

		//	init meta
		mMeta.mDataSize = mData.GetDataSize();
	
		//	type needs to have already been set if we're using raw packet data
		assert( mMeta.IsValidType() );

		return true;
	}

	template<class TYPE>
	bool	Set(const TYPE& Packet,const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
	{
		mSender = Sender;
		mMeta = Meta;
	
		//	turn into raw data
		assert( mData.GetDataSize() == 0 );
		if ( !Soy::ToRawData( mData, Packet ) )
			return false;

		//	init meta
		mMeta.mDataSize = mData.GetDataSize();
		mMeta.mType = Packet.GetType();
		return true;
	}

	template<class TYPE>
	bool			GetPacketAs(TYPE& Packet) const
	{
		return Soy::FromRawData( Packet, mData );
	}
	template<class TYPE>
	TYPE			GetPacketAs() const
	{
		TYPE Packet;
		if ( !Soy::FromRawData( Packet, mData ) )
			return Packet;
		return Packet;
	}

	void			GetPacketRaw(Array<char>& RawData,bool IncludeMetaInPacket)
	{
		if ( IncludeMetaInPacket )
			RawData.PushReinterpretBlock( mMeta );
		RawData.PushBackArray( mData );
	}

	template<typename TYPETYPE>
	TYPETYPE				GetType() const		{	return mMeta.GetType<TYPETYPE>();	}

public:
	SoyPacketMeta			mMeta;
	Array<char>				mData;
	SoyNet::TAddress		mSender;	//	for incoming data, this is where the data came from
};

class SoyPacket
{
public:
	virtual SoyRef		GetType() const=0;
};


class SoyPacketManager : public ofMutex
{
public:
	SoyPacketManager(prmem::Heap& Heap=prcore::Heap) :
		mHeap				( Heap ),
		mPendingPackets		( mHeap ),
		mPackets			( mHeap )
	{
	}

	bool					IsEmpty() const	{	return mPackets.IsEmpty();	}	//	read is so minute we don't need a lock

	template<class PACKET>
	void					PushPacket(const SoyPacketMeta& Meta,const PACKET& Packet);
	void					PushPacket(const SoyPacketMeta& Meta,const Array<char>& Data,const SoyNet::TAddress& Sender);	//	push a raw packet onto the stack

	bool					PopPacket(SoyPacketContainer& Container);	//	pops the next packet. caller takes ownership of data
	bool					PopPacketRawData(Array<char>& PacketData,bool IncludeMetaInPacket);	//	pops the next packet into raw data.
	
	//	incomplete packets where we've read a header, but not the data [yet]
	bool					PeekPendingPacket(SoyPacketMeta& Meta,const SoyNet::TAddress& SenderAddress);				//	see if there is a pending packet for this sender
	bool					FinishPendingPacket(const Array<char>& PacketData,const SoyNet::TAddress& SenderAddress);	//	complete & push a pending packet
	bool					PushPendingPacket(const SoyPacketMeta& Meta,const SoyNet::TAddress& SenderAddress);			//	start a pending packet

protected:
	prmem::Heap&			mHeap;

	Array<SoyPair<SoyNet::TAddress,SoyPacketMeta> >	mPendingPackets;
	Array<SoyPacketContainer>						mPackets;
};



	
template<class PACKET>
void SoyPacketManager::PushPacket(const SoyPacketMeta& Meta,const PACKET& Packet)
{
	ofMutex::ScopedLock Lock(*this);
	SoyPacketContainer& Container = mPackets.PushBack( SoyPacketContainer(mHeap) );
	Container.Set( Packet, Meta, SoyNet::TAddress() );
}	

