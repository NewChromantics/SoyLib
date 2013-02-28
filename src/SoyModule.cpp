#include "ofxSoylent.h"





SoyModule::SoyModule(const char* Name) :
	SoyModuleMeta	( SoyRef(Name) )
{
	//	gr: you must do this in your overloaded module. (vtable not ready here)
	//ChangeState<SoyModuleState_DiscoveryBind>();
}


namespace TPacketReadResult
{
	enum Type
	{
		Error,
		Okay,
		NotEnoughData,
	};
};

class TServerReadWrapper
{
public:
	TServerReadWrapper(ofxTCPServer& Server,int ClientId,const SoyNet::TAddress& Sender) :
		mServer		( Server ),
		mClientId	( ClientId ),
		mSender		( Sender )
	{
	}

	BufferString<100>	GetName()										{	BufferString<100> Str;	Str << "server (" << mClientId << ")";	return Str;	}
	template<typename DATATYPE>
	int					receiveRawBytes(ArrayBridge<DATATYPE>& Array)	{	return mServer.receiveRawBytes( mClientId, reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	}
	template<typename DATATYPE>
	int					peekReceiveRawBytes(ArrayBridge<DATATYPE>& Array)	{	return mServer.peekReceiveRawBytes( mClientId, reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	}

public:
	SoyNet::TAddress	mSender;
	ofxTCPServer&		mServer;
	int					mClientId;
};
class TClientReadWrapper
{
public:
	TClientReadWrapper(ofxTCPClient& Client,const SoyNet::TAddress& Sender) :
		mClient		( Client ),
		mSender		( Sender )
	{
	}

	BufferString<100>	GetName()										{	return "client";	}
	template<typename DATATYPE>
	int					receiveRawBytes(ArrayBridge<DATATYPE>& Array)	{	return mClient.receiveRawBytes( reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	}
	template<typename DATATYPE>
	int					peekReceiveRawBytes(ArrayBridge<DATATYPE>& Array)	{	return mClient.peekReceiveRawBytes( reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	}

public:
	SoyNet::TAddress	mSender;
	ofxTCPClient&		mClient;
};

template<class READWRAPPER,typename DATATYPE>
TPacketReadResult::Type ReadChunk(ArrayBridge<DATATYPE>& Array,READWRAPPER& ReadWrapper)
{
	//	got enough recv data for this buffer?
	if ( ReadWrapper.peekReceiveRawBytes( Array ) < Array.GetDataSize() )
		return TPacketReadResult::NotEnoughData;

	//	read the data
	int BytesRecieved = ReadWrapper.receiveRawBytes( Array );

	//	socket error
	if ( BytesRecieved != Array.GetDataSize() )
		return TPacketReadResult::Error;

	BufferString<1000> Debug;
	Debug << ReadWrapper.GetName() << "read " << BytesRecieved << " bytes\n";
	printf( Debug );


	return TPacketReadResult::Okay;
}

template<class READWRAPPER>
bool RecvPackets(SoyModulePacketManager& PacketManager,READWRAPPER& ReadWrapper)
{
	//	have we got a pending packet to finish?
	{
		SoyPacketMeta PendingPacketMeta;
		if ( PacketManager.PeekPendingPacket( PendingPacketMeta, ReadWrapper.mSender ) )
		{
			//	read the rest of the packet
			Array<char> PacketData( PendingPacketMeta.mDataSize );
			auto Result = ReadChunk( GetArrayBridge( PacketData ), ReadWrapper );
			if ( Result == TPacketReadResult::Error )
				return false;

			//	still waiting for data
			if ( Result == TPacketReadResult::NotEnoughData )
				return true;

			//	finish this pending packet
			PacketManager.FinishPendingPacket( PacketData, ReadWrapper.mSender );
		}
	}

	//	look for next packet header
	while ( true )
	{
		BufferArray<SoyPacketMeta,1> PacketHeaderData(1);
		auto& PacketHeader = PacketHeaderData[0];
		auto HeaderResult = ReadChunk( GetArrayBridge( PacketHeaderData ), ReadWrapper );
		if ( HeaderResult == TPacketReadResult::Error )
			return false;
		if ( HeaderResult == TPacketReadResult::NotEnoughData )
			return true;

		//	check the packet header is okay
		if ( !PacketHeader.IsValid() )
			return false;

		//	read the rest of the packet data
		Array<char> PacketData( PacketHeader.mDataSize );
		auto DataResult = ReadChunk( GetArrayBridge( PacketData ), ReadWrapper );
		if ( DataResult == TPacketReadResult::Error )
			return false;

		//	still waiting for data (but we've read the header)
		if ( DataResult == TPacketReadResult::NotEnoughData )
		{
			PacketManager.PushPendingPacket( PacketHeader, ReadWrapper.mSender );
			return true;
		}

		//	push new packet
		PacketManager.PushPacket( PacketHeader, PacketData, ReadWrapper.mSender );
	}
	
}


void SoyModule::Update(float TimeStep)
{
	UpdateStates( TimeStep );

	//	send out packets
	while ( true )
	{
		Array<char> PacketRaw;
		if ( !mPacketsOut.PopPacketRawData( PacketRaw ) )
			break;

		//	send packet out
		if ( mClusterServer.isConnected() )
		{
			if ( mClusterServer.sendRawBytesToAll( PacketRaw.GetArray(), PacketRaw.GetDataSize() ) )
			{
				BufferString<1000> Debug;
				Debug << "Server Sent " << PacketRaw.GetDataSize() << " bytes\n";
				printf( Debug );
			}
		}

		if(  mClusterClient.isConnected() )
		{
			if ( mClusterClient.sendRawBytes( PacketRaw.GetArray(), PacketRaw.GetDataSize() ) )
			{
				BufferString<1000> Debug;
				Debug << "Client Sent " << PacketRaw.GetDataSize() << " bytes\n";
				printf( Debug );
			}
		}
	}

	//	read in packets from server
	if ( mClusterClient.isConnected() )
	{
		TClientReadWrapper Client( mClusterClient, GetClientServerPeerAddress() );
		if ( !RecvPackets( mPacketsIn, Client ) )
		{
			//	fatal error
			//	disconnect client
			assert( false );
			mClusterClient.close();
		}
	}
	
	//	check for new/closed clients
	for ( int i=0;	i<mClusterServer.getLastID();	i++ )
	{
		int ClientId = i;
		if ( mClusterServer.isClientConnected(ClientId) )
			OnServerClientConnected( ClientId );
		else
			OnServerClientDisconnected( ClientId );
	}

	//	read in packets from each client
	for ( int i=0;	i<mClusterServer.getLastID();	i++ )
	{
		int ClientId = i;
		if ( !mClusterServer.isClientConnected(ClientId) )
			continue;
		
		TServerReadWrapper Client( mClusterServer, ClientId, GetServerClientPeerAddress(ClientId) );
		if ( !RecvPackets( mPacketsIn, Client ) )
		{
			//	fatal error
			//	disconnect client
			assert( false );
			mClusterServer.disconnectClient( ClientId );
		}
	}
	
	//	process the incoming packets
	while ( !mPacketsIn.IsEmpty() )
	{
		SoyPacketContainer Packet;
		if ( !mPacketsIn.PopPacket( Packet ) )
			break;

		//	packet is from ourselves... ignore it
		if ( Packet.mMeta.mSender == mRef )
			continue;

		OnPacket( Packet );
	}
}


bool SoyModule::OnServerClientConnected(SoyNet::TClientRef ClientRef)
{
	//	already an active peer...
	if ( mPeers.Find( ClientRef ) )
		return false;

	//	already pending...
	if ( mPendingPeers.Find( ClientRef ) )
		return false;

	//	add to pending list
	auto ClientAddress = GetServerClientPeerAddress( ClientRef );
	mPendingPeers.PushBack( ClientAddress );
	return true;
}


bool SoyModule::OnServerClientDisconnected(SoyNet::TClientRef ClientRef)
{
	bool Changed = false;

	//	if in the pending list, remove it
	Changed |= mPendingPeers.Remove( ClientRef );

	//	if we have an active peer, move it to the dead peers
	int ActivePeerIndex = mPeers.FindIndex( ClientRef );
	if ( ActivePeerIndex != -1 )
	{
		//	move to dead peers
		assert( !mDeadPeers.Find( ClientRef ) );
		mDeadPeers.PushBack( mPeers[ActivePeerIndex] );
		mPeers.RemoveBlock( ActivePeerIndex, 1 );
		OnPeersChanged();
		return true;
	}

	return Changed;
}


void SoyModule::RegisterPeer(const SoyModuleMeta& PeerMeta,const SoyNet::TAddress& Address)
{
	//	do we alreayd have a peer with this name?
	auto* Peer = mPeers.Find( PeerMeta.mRef );
	if ( !Peer )
	{
		Peer = &mPeers.PushBack( SoyModulePeer(PeerMeta) );
	}

	//	register address with peer
	Peer->AddAddress( Address );

	//	if peer was dead, remove (now alive!)
	//	todo: merge with new peer
	mDeadPeers.Remove( PeerMeta.mRef );

	//	remove from pending list
	mPendingPeers.Remove( Address );

	//	notify changes
	OnPeersChanged();
}


SoyModuleState_ServerBind::SoyModuleState_ServerBind(SoyModule& Parent,uint16 Port) :
	SoyModuleState	( Parent, "Server Bind" ),
	mPort			( Port )
{
	auto& Server = GetParent().GetClusterServer();
	if ( mPort == 0 )
		mPort = GetParent().GetClusterPortRange()[0];
	Server.setup( mPort, false );
}

void SoyModuleState_ServerBind::Update(float TimeStep)
{
	//	failed to bind
	auto& Server = GetParent().GetClusterServer();
	if ( !Server.isConnected() )
	{
		Server.close();

		//	try next port
		auto Ports = GetParent().GetClusterPortRange();
		int PortIndex = Ports.FindIndex( mPort );
		PortIndex++;
		if ( PortIndex < Ports.GetSize() )
			GetParent().ChangeState<SoyModuleState_ServerBind>( Ports[PortIndex] );
		else
			GetParent().ChangeState<SoyModuleState_BindFailed>();
		return;
	}
	
	//	bound okay, listen
	GetParent().ChangeState<SoyModuleState_Listening>();
}


SoyModuleState_ClientConnect::SoyModuleState_ClientConnect(SoyModule& Parent,const SoyRef& Peer) :
	SoyModuleState	( Parent, "Client Connect" ),
	mPeer			( Peer )
{
	//	find server we're trying to connect to...
	const SoyModulePeer* pPeer = GetParent().GetPeer( mPeer );

	//	try to connect if it's valid
	if ( pPeer )
	{
		auto& Client = GetParent().GetClusterClient();
		auto& PeerAddress = pPeer->mAddresses[0];
		Client.setup( static_cast<const char*>(PeerAddress.mAddress), PeerAddress.mPort, false );
	}
}

void SoyModuleState_ClientConnect::Update(float TimeStep)
{
	//	failed to connect
	auto& Client = GetParent().GetClusterClient();
	if ( !Client.isConnected() )
	{
		Client.close();
		GetParent().ChangeState<SoyModuleState_Listening>();
		return;
	}
	
	//	all connected okay..
	GetParent().OnConnectedToServer( mPeer );
	GetParent().ChangeState<SoyModuleState_Connected>();
}


SoyModuleState_Connected::SoyModuleState_Connected(SoyModule& Parent) :
	SoyModuleState	( Parent, "Connected" )
{
	//	send registration packet
	SoyModulePacket_RegisterPeer Packet;
	Packet.mPeerMeta = GetParent().GetMeta();
	GetParent().SendPacketToPeers( Packet );
}



BufferString<300> SoyModule::GetNetworkStatus() const
{
	BufferString<300> Status;

	//	show our current state
	SoyModuleState* pState = static_cast<SoyModuleState*>( mState );
	if ( pState )
		Status << pState->GetStateName();
	else
		Status << "No state";
	Status << "; ";

	//	cluster info
	if ( GetClusterServer().isConnected() )
	{
		Status << "Server Port: " << GetClusterServer().getPort() << ". ";
		Status << "Clients: " << GetClusterServer().getNumClients() << ". ";
	}

	if ( GetClusterClient().isConnected() )
	{
		Status << "Client Connected to " << mServerPeer << ". ";
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

SoyNet::TAddress SoyModule::GetClientServerPeerAddress() const
{
	auto* pPeer = GetPeer( mServerPeer );
	if ( !pPeer )
	{
		assert( pPeer );
		return SoyNet::TAddress();
	}

	//	what to do with multiple addresses??
	assert( pPeer->mAddresses.GetSize() == 1 );
	return pPeer->mAddresses[0];
}

SoyNet::TAddress SoyModule::GetServerClientPeerAddress(SoyNet::TClientRef ClientRef) const
{
	auto& Server = GetClusterServer();
	BufferString<100> Address = Server.getClientIP( ClientRef.mClientId ).c_str();
	uint16 Port = Server.getClientPort( ClientRef.mClientId );

	return SoyNet::TAddress( Address, Port, ClientRef );
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

