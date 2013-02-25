#include "ofxSoylent.h"


SoyModule::SoyModule(const char* Name) :
	SoyModuleMeta	( Name )
{
	//	gr: you must do this in your overloaded module. (vtable not ready here)
	//ChangeState<SoyModuleState_DiscoveryBind>();
}


void SoyModule::Update(float TimeStep)
{
	UpdateStates( TimeStep );

	//	look out for packets
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
		Client.setup( static_cast<const char*>(pPeer->mAddress), pPeer->mPort, false );
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


void SoyModule::OnFoundPeer(const SoyModulePeer& Peer)
{
	bool Changed = true;

	//	remove existing peer
	mPeers.Remove( Peer.mRef );
	mPeers.PushBack( Peer );

	//	send notification
	if ( Changed )
	{
		Array<SoyRef> PeerRefs;
		for ( int i=0;	i<mPeers.GetSize();	i++ )
			PeerRefs.PushBack( mPeers[i].mRef );
		ofNotifyEvent( mOnPeersChanged, PeerRefs );
	}
}


bool SoyModuleMemberBase::OnDataChanged()
{
	return mParent.OnMemberChanged( *this );
}


bool SoyModule::OnMemberChanged(const SoyModuleMemberBase& Member)
{
	return false;
}

