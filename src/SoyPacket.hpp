//	included from SoyPacket.h

	
	
template<typename PACKETENUM>
template<class PACKET>
void SoyPacketManager<PACKETENUM>::PushPacket(const SoyPacketMeta& Meta,const PACKET& Packet)
{
	ofMutex::ScopedLock Lock(*this);
	SoyPacketContainer Container( Packet, Meta, SoyNet::TAddress() );
	mPackets.PushBack( Container );
}	

template<typename PACKETENUM>
void SoyPacketManager<PACKETENUM>::PushPacket(const SoyPacketMeta& Meta,const Array<char>& Data,const SoyNet::TAddress& Sender)
{
	ofMutex::ScopedLock Lock(*this);
	SoyPacketContainer Container( Data, Meta, Sender );
	mPackets.PushBack( Container );
}




template<typename PACKETENUM>
bool SoyPacketManager<PACKETENUM>::PopPacket(SoyPacketContainer& Container)
{
	ofMutex::ScopedLock Lock(*this);
	if ( mPackets.IsEmpty() )
		return false;

	Container = mPackets[0];
	mPackets.RemoveBlock( 0, 1 );
	return true;
}

template<typename PACKETENUM>
bool SoyPacketManager<PACKETENUM>::PopPacketRawData(Array<char>& PacketData)
{
	ofMutex::ScopedLock Lock(*this);
	if ( mPackets.IsEmpty() )
		return false;

	auto& Container = mPackets[0];
	Container.GetPacketRaw( PacketData );
	mPackets.RemoveBlock( 0, 1 );
	return true;
}




template<typename PACKETENUM>
inline bool SoyPacketManager<PACKETENUM>::PeekPendingPacket(SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
{
	auto* pPending = mPendingPackets.Find( Sender );
	if ( !pPending )
		return false;

	Meta = pPending->mSecond;
	return true;
}


template<typename PACKETENUM>
inline bool SoyPacketManager<PACKETENUM>::FinishPendingPacket(const Array<char>& PacketData,const SoyNet::TAddress& Sender)
{
	int PendingIndex = mPendingPackets.FindIndex( Sender );
	//	expecting a pending packet!
	assert( PendingIndex >= 0 );
	if ( PendingIndex < 0 )
		return false;

	//	push the complete packet
	auto& PendingPacket = mPendingPackets[PendingIndex];
	PushPacket( PendingPacket.mSecond, PacketData, Sender );

	//	no longer pending
	mPendingPackets.RemoveBlock( PendingIndex, 1 );
	return true;
}

template<typename PACKETENUM>
inline bool SoyPacketManager<PACKETENUM>::PushPendingPacket(const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
{
	//	check there isn't one already!
	if ( mPendingPackets.Find( Sender ) )
	{
		assert( false );
		return false;
	}

	//	make a new pair and add to the list
	SoyPair<SoyNet::TAddress,SoyPacketMeta> PendingPacket( Sender, Meta );
	mPendingPackets.PushBack( PendingPacket );
	return true;
}
