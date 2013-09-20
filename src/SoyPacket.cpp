#include "SoyPacket.h"

	

void SoyPacketManager::PushPacket(const SoyPacketMeta& Meta,const Array<char>& Data,const SoyNet::TAddress& Sender)
{
	ofMutex::ScopedLock Lock(*this);
	SoyPacketContainer& Container = mPackets.PushBack( SoyPacketContainer(mHeap) );
	Container.Set( Data, Meta, Sender );
}



bool SoyPacketManager::PopPacket(SoyPacketContainer& Container)
{
	ofMutex::ScopedLock Lock(*this);
	if ( mPackets.IsEmpty() )
		return false;

	Container = mPackets[0];
	mPackets.RemoveBlock( 0, 1 );
	return true;
}

bool SoyPacketManager::PopPacketRawData(Array<char>& PacketData,bool IncludeMetaInPacket)
{
	ofMutex::ScopedLock Lock(*this);
	if ( mPackets.IsEmpty() )
		return false;

	auto& Container = mPackets[0];
	Container.GetPacketRaw( PacketData, IncludeMetaInPacket );
	mPackets.RemoveBlock( 0, 1 );
	return true;
}




bool SoyPacketManager::PeekPendingPacket(SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
{
	auto* pPending = mPendingPackets.Find( Sender );
	if ( !pPending )
		return false;

	Meta = pPending->mSecond;
	return true;
}


bool SoyPacketManager::FinishPendingPacket(const Array<char>& PacketData,const SoyNet::TAddress& Sender)
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

bool SoyPacketManager::PushPendingPacket(const SoyPacketMeta& Meta,const SoyNet::TAddress& Sender)
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
