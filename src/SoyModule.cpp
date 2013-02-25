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
}



SoyModuleState_DiscoveryBind::SoyModuleState_DiscoveryBind(SoyModule& Parent,uint16 Port) :
	SoyModuleState	( Parent, "Discovery Bind" ),
	mPort			( Port )
{
	auto& Server = GetParent().mDiscoveryServer;
	if ( mPort == 0 )
		mPort = GetParent().GetDiscoveryPortRange()[0];

}

void SoyModuleState_DiscoveryBind::Update(float TimeStep)
{
	auto& Server = GetParent().mDiscoveryServer;
	bool Success = false;

	if ( Server.Create() )
	{
		if ( Server.Bind(mPort) )
		{
			Server.SetEnableBroadcast(true);
			Server.SetNonBlocking(true);
			Success = true;
		}
	}

	//	failed to bind
	if ( !Success )
	{
		Server.Close();

		//	try next port
		auto Ports = GetParent().GetDiscoveryPortRange();
		int PortIndex = Ports.FindIndex( mPort );
		PortIndex++;
		if ( PortIndex < Ports.GetSize() )
			GetParent().ChangeState<SoyModuleState_DiscoveryBind>( Ports[PortIndex] );
		else
			GetParent().ChangeState<SoyModuleState_DiscoveryBindFailed>();
		return;
	}
	
	//	discovery bound okay...
	//	send out hello world packet
	//Server.Send()
	
	//	start binding main server	
	GetParent().ChangeState<SoyModuleState_TryBind>( GetParent().GetClusterPortRange()[0] );
}



SoyModuleState_TryBind::SoyModuleState_TryBind(SoyModule& Parent,uint16 Port) :
	SoyModuleState	( Parent, "Try Bind" ),
	mPort			( Port )
{
	auto& Server = GetParent().mClusterServer;
	if ( mPort == 0 )
		mPort = GetParent().GetClusterPortRange()[0];
	Server.setup( mPort, false );
}

void SoyModuleState_TryBind::Update(float TimeStep)
{
	//	failed to bind
	auto& Server = GetParent().mClusterServer;
	if ( !Server.isConnected() )
	{
		Server.close();

		//	try next port
		auto Ports = GetParent().GetClusterPortRange();
		int PortIndex = Ports.FindIndex( mPort );
		PortIndex++;
		if ( PortIndex < Ports.GetSize() )
			GetParent().ChangeState<SoyModuleState_TryBind>( Ports[PortIndex] );
		else
			GetParent().ChangeState<SoyModuleState_BindFailed>();
		return;
	}
	
	//	bound okay, listen
	GetParent().ChangeState<SoyModuleState_Listening>();
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


	//	discovery info
	Status << "Discovery Port: " << GetDiscoveryServer().getPort() << ". ";


	//	cluster info
	if ( GetClusterServer().isConnected() )
		Status << "Cluster Connected. ";
	Status << "Cluster Port: " << GetClusterServer().getPort() << ". ";
	Status << "Clients: " << GetClusterServer().getNumClients() << ". ";

	return Status;
}