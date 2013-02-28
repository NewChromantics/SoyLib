#include "ofxSoylent.h"





SoyModule::SoyModule(const char* Name) :
	SoyModuleMeta	( SoyRef(Name) )
{
	//	gr: you must do this in your overloaded module. (vtable not ready here)
	//ChangeState<SoyModuleState_DiscoveryBind>();

	ofAddListener( mClusterSocket.mOnClosed, this, &SoyModule::OnClusterSocketClosed );
	ofAddListener( mClusterSocket.mOnClientConnected, this, &SoyModule::OnClusterSocketClientConnected );
	ofAddListener( mClusterSocket.mOnServerListening, this, &SoyModule::OnClusterSocketServerListening );
	ofAddListener( mClusterSocket.mOnClientJoin, this, &SoyModule::OnClusterSocketClientJoin );
	ofAddListener( mClusterSocket.mOnClientLeft, this, &SoyModule::OnClusterSocketClientLeft );
}

void SoyModule::OnClusterSocketServerListening(bool& Event)
{
	//	can stop trying to listen
	mTryListenPorts.Clear();
}

void SoyModule::OnClusterSocketClientConnected(bool& Event)
{
	//	can stop trying to connect
	mTryConnectToServer = SoyNet::TAddress();

	//	send register packet
	SoyModulePacket_RegisterPeer Packet;
	Packet.mPeerMeta = GetMeta();
	SendPacketToPeers( Packet );
}

void SoyModule::OnClusterSocketClosed(bool& Event)
{
	//	clear connection attempts
	DisconnectCluster();
}

void SoyModule::DisconnectCluster()
{
	//	disconnect and cancel any listen/connect attempts
	mClusterSocket.Close();
	mTryListenPorts.Clear();
	mTryConnectToServer = SoyNet::TAddress();
}

void SoyModule::OpenClusterServer()
{
	DisconnectCluster();

	auto ClusterPorts = GetClusterPortRange();

	for ( int i=0;	i<ClusterPorts.GetSize();	i++ )
		mTryListenPorts.PushBackUnique( ClusterPorts[i] );
}

bool SoyModule::ConnectToClusterServer(const SoyRef& Peer)
{
	//	grab address
	auto* pPeer = GetPeer( Peer );
	if ( !pPeer )
		return false;
	if ( pPeer->mAddresses.IsEmpty() )
		return false;
	auto ServerAddress = pPeer->mAddresses[0];

	//	already trying to connect to this...
	if ( mTryConnectToServer == ServerAddress )
		return true;

	//	close current socket
	DisconnectCluster();

	//	set new connection attempt
	mTryConnectToServer = ServerAddress;

	return true;
}


void SoyModule::Update(float TimeStep)
{
	//	trying to open a server...
	if ( !mTryListenPorts.IsEmpty() )
	{
		if ( mClusterSocket.GetState() != SoyNet::TSocketState::ServerListening )
		{
			auto Port = mTryListenPorts.PopAt(0);
			mClusterSocket.Listen( Port );
		}
	}

	//	trying to connect to server...
	if ( mTryConnectToServer.IsValid() )
	{
		if ( mClusterSocket.GetState() != SoyNet::TSocketState::ClientConnected )
		{
			if ( !mClusterSocket.Connect( mTryConnectToServer ) )
				mTryConnectToServer = SoyNet::TAddress();
		}
	}

	//	update socket
	mClusterSocket.Update();
	
	//	process the incoming packets
	auto& Packets = mClusterSocket.mPacketsIn;
	while ( !Packets.IsEmpty() )
	{
		SoyPacketContainer Packet;
		if ( !Packets.PopPacket( Packet ) )
			break;

		//	packet is from ourselves... ignore it
		if ( Packet.mMeta.mSender == mRef )
			continue;

		OnPacket( Packet );
	}
}


void SoyModule::OnClusterSocketClientJoin(const SoyNet::TAddress& Client)
{
}


void SoyModule::OnClusterSocketClientLeft(const SoyNet::TAddress& Client)
{
	//	if this client managed to become a peer, kill it
	int ActivePeerIndex = mPeers.FindIndex( Client );
	if ( ActivePeerIndex != -1 )
	{
		mPeers.RemoveBlock( ActivePeerIndex, 1 );
		OnPeersChanged();
	}
}


void SoyModule::RegisterPeer(const SoyModuleMeta& PeerMeta,const SoyNet::TAddress& Address)
{
	bool Changed = false;

	//	do we alreayd have a peer with this name?
	auto* Peer = mPeers.Find( PeerMeta.mRef );
	if ( !Peer )
	{
		Peer = &mPeers.PushBack( SoyModulePeer(PeerMeta) );
		Changed = true;
	}

	//	register address with peer
	Changed |= Peer->AddAddress( Address );

	//	notify changes
	if ( Changed )
		OnPeersChanged();
}


BufferString<300> SoyModule::GetNetworkStatus() const
{
	BufferString<300> Status;

	//	show our current state
	Status << SoyEnum::ToString( mClusterSocket.GetState() ) << "; ";

	//	cluster info
	if ( mClusterSocket.GetState() == SoyNet::TSocketState::ServerListening )
	{
		Status << "Server: " << mClusterSocket.GetMyAddress() << "; ";
	}

	if ( mClusterSocket.GetState() == SoyNet::TSocketState::ClientConnected )
	{
		Status << "Client (" << mClusterSocket.GetMyAddress() << ") Connected to: " << mClusterSocket.GetServerAddress() << "; ";
	}

	//	show connections
	Array<SoyNet::TAddress> Connections;
	mClusterSocket.GetConnections( Connections );
	for( int i=0;	i<Connections.GetSize();	i++ )
	{
		Status << Connections[i] << ", ";
	}

	return Status;
}



void SoyModule::OnPeersChanged()
{
	//	send notification
	Array<SoyRef> PeerRefs;
	for ( int i=0;	i<mPeers.GetSize();	i++ )
		PeerRefs.PushBack( mPeers[i].mRef );
	ofNotifyEvent( mOnPeersChanged, PeerRefs );
}


SoyModuleMemberBase::SoyModuleMemberBase(SoyModule& Parent,const char* Name) :
	mParent				( Parent ),
	SoyModuleMemberMeta	( Name )
{
	mParent.RegisterMember( *this );
}

bool SoyModuleMemberBase::OnDataChanged(const SoyTime& ModifiedTime)
{
	//	if no time specified, set to current module time
	SoyTime NewTime = (ModifiedTime.IsValid()) ? ModifiedTime : mParent.GetTime();

	//	only update if time is newer
	//	gr: TOO LATE HERE!
	mLastModified = NewTime;

	return mParent.OnMemberChanged( *this );
}


bool SoyModule::OnMemberChanged(const SoyModuleMemberBase& Member)
{
	//	send packet to clients
	SoyModulePacket_MemberChanged Packet;
	Packet.mMemberRef = Member.mRef;
	Packet.mModifiedTime = Member.mLastModified;
	Member.GetData( Packet.mData );
	SendPacketToPeers( Packet );

	//	notify listeners
	ofNotifyEvent( mOnMemberChanged, Member.mRef );

	return true;
}

	
//-------------------------------------------------
//	synchronised time
//-------------------------------------------------
SoyTime SoyModule::GetTime() const
{
	return SoyTime::Now();
}


SoyRef SoyModule::GetServerClientPeer(SoyNet::TClientRef ClientRef) const
{
	for ( int p=0;	p<mPeers.GetSize();	p++ )
	{
		auto& Peer = mPeers[p];
		for ( int a=0;	a<Peer.mAddresses.GetSize();	a++ )
		{
			auto& Address = Peer.mAddresses[a];
			if ( Address.mClientRef != ClientRef )
				continue;

			return Peer.mRef;
		}
	}
	return SoyRef();
}



bool SoyModulePeer::AddAddress(const SoyNet::TAddress& Address)	
{
	//	already exists
	if ( mAddresses.Find( Address ) )
		return false;

	mAddresses.PushBack( Address );

	return true;
}
	



//------------------------------------------------------
//	return true if handled
//------------------------------------------------------
bool SoyModule::OnPacket(const SoyPacketContainer& Packet)
{
	auto SoyType = static_cast<SoyModulePackets::Type>( Packet.mMeta.mType );
	switch ( SoyType )
	{
		case_OnSoyPacket( SoyModulePacket_MemberChanged );
		case SoyModulePacket_RegisterPeer::TYPE:	return OnPacket( Packet.GetAs<SoyModulePacket_RegisterPeer>(), Packet.mSender );

	}

	return false;
}

bool SoyModule::OnPacket(const SoyModulePacket_MemberChanged& Packet)
{
	//	get member
	auto* pMember = GetMember( Packet.mMemberRef );
	if ( !pMember )
		return false;
	auto& Member = *pMember;

	//	dont update if modified time is older than our last change
	if ( Member.mLastModified >= Packet.mModifiedTime )
		return true;

	Member.SetData( Packet.mData, Packet.mModifiedTime );
	return true;
}


bool SoyModule::OnPacket(const SoyModulePacket_RegisterPeer& Packet,const SoyNet::TAddress& Sender)
{
	//	turn into peer
	RegisterPeer( Packet.mPeerMeta, Sender );
	return true;
}

SoyModuleMemberBase* SoyModule::GetMember(const SoyRef& MemberRef)
{
	for ( int i=0;	i<mMembers.GetSize();	i++ )
	{
		auto& Member = *mMembers[i];
		if ( Member.mRef != MemberRef )
			continue;

		return &Member;
	}

	return NULL;
}

