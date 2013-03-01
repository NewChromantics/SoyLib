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

	ofAddListener( mDiscoverySocket.mOnServerListening, this, &SoyModule::OnDiscoverySocketServerListening );
}

void SoyModule::OnDiscoverySocketServerListening(bool& Event)
{
	//	can stop trying to listen
	mTryListenDiscoveryPorts.Clear();

	//	send out discover-me packet
	SendPacketBroadcastHelloWorld();
	SendPacketBroadcastSearchWorld();
}

void SoyModule::OnClusterSocketServerListening(bool& Event)
{
	//	can stop trying to listen
	mTryListenClusterPorts.Clear();

	//	send out change of details
	SendPacketBroadcastHelloWorld();
}

void SoyModule::OnClusterSocketClientConnected(bool& Event)
{
	//	can stop trying to connect
	mTryConnectToClusterServer = SoyNet::TAddress();

	//	send register packet
	SoyModulePacket_RegisterPeer Packet;
	Packet.mPeerMeta = GetMeta();
	SendPacketToPeers( Packet );
}

void SoyModule::OnClusterSocketClosed(bool& Event)
{
	//	clear connection attempts
	DisconnectCluster();

	//	send out change of details
	SendPacketBroadcastHelloWorld();
}


void SoyModule::SendPacketBroadcastHelloWorld()
{
	SoyNet::TAddress ListenAddress = mClusterSocket.GetMyAddress();

	SoyModulePacket_HelloWorld Packet;
	Packet.mPeerMeta = GetMeta();
	Packet.mServerPort = ListenAddress.mPort;

	SoyPacketMeta Meta( mRef );
	Meta.mType = Packet.GetType();	//	only needed for debug
	mDiscoverySocket.mPacketsOut.PushPacket( Meta, Packet );

	BufferString<100> Debug;
	Debug << "SENT Discovery packet " << Meta;
	ofLogNotice( static_cast<const char*>( Debug ) );


}

void SoyModule::SendPacketBroadcastSearchWorld()
{
	SoyModulePacket_SearchWorld Packet;
	
	SoyPacketMeta Meta( mRef );
	Meta.mType = Packet.GetType();	//	only needed for debug
	mDiscoverySocket.mPacketsOut.PushPacket( Meta, Packet );

	BufferString<100> Debug;
	Debug << "SENT Discovery packet " << Meta;
	ofLogNotice( static_cast<const char*>( Debug ) );
}


void SoyModule::OpenDiscoveryServer()
{
	DisconnectDiscovery();
	
	auto Ports = GetDiscoveryPortRange();
	for ( int i=0;	i<Ports.GetSize();	i++ )
		mTryListenDiscoveryPorts.PushBackUnique( Ports[i] );
}


void SoyModule::DisconnectDiscovery()
{
	mDiscoverySocket.Close();
}


void SoyModule::DisconnectCluster()
{
	//	disconnect and cancel any listen/connect attempts
	mClusterSocket.Close();
	mTryListenClusterPorts.Clear();
	mTryConnectToClusterServer = SoyNet::TAddress();
}

void SoyModule::OpenClusterServer()
{
	DisconnectCluster();

	auto ClusterPorts = GetClusterPortRange();

	for ( int i=0;	i<ClusterPorts.GetSize();	i++ )
		mTryListenClusterPorts.PushBackUnique( ClusterPorts[i] );
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
	if ( mTryConnectToClusterServer == ServerAddress )
		return true;

	//	close current socket
	DisconnectCluster();

	//	set new connection attempt
	mTryConnectToClusterServer = ServerAddress;

	return true;
}

void SoyModule::UpdateDiscoverySocket()
{
	//	trying to open a server...
	if ( !mTryListenDiscoveryPorts.IsEmpty() )
	{
		if ( mDiscoverySocket.GetState() != SoyNet::TSocketState::ServerListening )
		{
			auto Port = mTryListenDiscoveryPorts.PopAt(0);
			mDiscoverySocket.Listen( Port );
		}
	}

	//	update discovery socket
	mDiscoverySocket.Update();

	auto& DiscoveryPackets = mDiscoverySocket.mPacketsIn;
	while ( !DiscoveryPackets.IsEmpty() )
	{
		SoyPacketContainer Packet;
		if ( !DiscoveryPackets.PopPacket( Packet ) )
			break;

		//	packet is from ourselves... ignore it
		if ( Packet.mMeta.mSender == mRef )
		{
			BufferString<100> Debug;
			Debug << __FUNCTION__ << ": Skipping packet from self (" << Packet.mMeta << " bytes)";
			ofLogNotice( static_cast<const char*>(Debug) );
			continue;
		}

		OnDiscoveryPacket( Packet );
	}
}

void SoyModule::UpdateClusterSocket()
{
	//	trying to open a server...
	if ( !mTryListenClusterPorts.IsEmpty() )
	{
		if ( mClusterSocket.GetState() != SoyNet::TSocketState::ServerListening )
		{
			auto Port = mTryListenClusterPorts.PopAt(0);
			mClusterSocket.Listen( Port );
		}
	}

	//	trying to connect to server...
	if ( mTryConnectToClusterServer.IsValid() )
	{
		if ( mClusterSocket.GetState() != SoyNet::TSocketState::ClientConnected )
		{
			if ( !mClusterSocket.Connect( mTryConnectToClusterServer ) )
				mTryConnectToClusterServer = SoyNet::TAddress();
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
		{
			BufferString<100> Debug;
			Debug << __FUNCTION__ << ": Skipping packet from self (" << Packet.mMeta << " bytes)";
			ofLogNotice( static_cast<const char*>(Debug) );
			continue;
		}

		OnPacket( Packet );
	}
}

void SoyModule::Update(float TimeStep)
{
	UpdateDiscoverySocket();
	UpdateClusterSocket();
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


BufferString<300> SoyModule::GetClusterNetworkStatus() const
{
	BufferString<300> Status = "Cluster: ";
	auto& Socket = mClusterSocket;

	//	show our current state
	Status << SoyEnum::ToString( Socket.GetState() ) << "; ";

	//	cluster info
	if ( Socket.GetState() == SoyNet::TSocketState::ServerListening )
	{
		Status << "Server: " << Socket.GetMyAddress() << "; ";
	}

	if ( Socket.GetState() == SoyNet::TSocketState::ClientConnected )
	{
		Status << "Client (" << Socket.GetMyAddress() << ") Connected to: " << mClusterSocket.GetServerAddress() << "; ";
	}

	//	show connections
	Array<SoyNet::TAddress> Connections;
	Socket.GetConnections( Connections );
	for( int i=0;	i<Connections.GetSize();	i++ )
	{
		Status << Connections[i] << ", ";
	}

	return Status;
}


BufferString<300> SoyModule::GetDiscoveryNetworkStatus() const
{
	BufferString<300> Status = "Discovery: ";
	auto& Socket = mDiscoverySocket;

	//	show our current state
	Status << SoyEnum::ToString( Socket.GetState() ) << "; ";

	//	cluster info
	if ( Socket.GetState() == SoyNet::TSocketState::ServerListening )
	{
		Status << "Server: " << Socket.GetMyAddress() << "; ";
	}

	if ( Socket.GetState() == SoyNet::TSocketState::ClientConnected )
	{
		Status << "Client (" << Socket.GetMyAddress() << ") Connected to: " << mClusterSocket.GetServerAddress() << "; ";
	}

	//	show connections
	Array<SoyNet::TAddress> Connections;
	Socket.GetConnections( Connections );
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

	//	if we've now got an address which is valid, and we have an old unidentified one (port 0)
	//	remove the unidentified one
	if ( Address.mPort != 0 )
	{
		SoyNet::TAddress NoPortAddress( Address.mAddress, 0 );
		mAddresses.Remove( NoPortAddress );
	}

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

//------------------------------------------------------
//	return true if handled
//------------------------------------------------------
bool SoyModule::OnDiscoveryPacket(const SoyPacketContainer& Packet)
{
	BufferString<100> Debug;
	Debug << "RECV Discovery packet " << Packet.mMeta;
	ofLogNotice( static_cast<const char*>( Debug ) );

	auto SoyType = static_cast<SoyModulePackets::Type>( Packet.mMeta.mType );
	switch ( SoyType )
	{
		case SoyModulePacket_HelloWorld::TYPE:	return OnPacket( Packet.GetAs<SoyModulePacket_HelloWorld>(), Packet.mSender );
		case SoyModulePacket_SearchWorld::TYPE:	return OnPacket( Packet.GetAs<SoyModulePacket_SearchWorld>(), Packet.mSender );
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


bool SoyModule::OnPacket(const SoyModulePacket_HelloWorld& Packet,const SoyNet::TAddress& Sender)
{
	//	if they are listening, then we can make up their address with the port in the packet
	SoyNet::TAddress ClusterAddress( Sender.mAddress, Packet.mServerPort );

	//	turn into peer
	RegisterPeer( Packet.mPeerMeta, ClusterAddress );
	return true;
}

bool SoyModule::OnPacket(const SoyModulePacket_SearchWorld& Packet,const SoyNet::TAddress& Sender)
{
	//	someone's requested hello's from everyone
	SendPacketBroadcastHelloWorld();
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



