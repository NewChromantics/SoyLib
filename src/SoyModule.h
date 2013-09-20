#pragma once

#include "SoyPacket.h"
#include "SoySocket.h"
#include "SoyTime.h"


class SoyModule;



class SoyModuleMeta
{
public:
	SoyModuleMeta()
	{
	}
	SoyModuleMeta(const SoyRef& Ref) :
		mRef	( Ref )
	{
	}

	const SoyRef&			GetRef() const		{	return mRef;	}
	SoyModuleMeta&			GetMeta()			{	return *this;	}
	const SoyModuleMeta&	GetMeta() const		{	return *this;	}

	inline bool				operator==(const SoyRef& Ref) const	{	return mRef == Ref;	}
	inline bool				operator!=(const SoyRef& Ref) const	{	return !(*this == Ref);	}

private:
	SoyRef		mRef;
};

namespace SoyModulePackets
{
	enum Type
	{
		Invalid,
		HelloWorld,			//	discovery hello-world
		SearchWorld,		//	request everyone to send out a hello world
		RegisterPeer,
		MemberChanged,
	};

	//	intermediate replacement as type is now ref
	inline SoyRef	GetRef(Type t)
	{
		switch( t )
		{
		case HelloWorld:	return SoyRef("HelloWld");
		case SearchWorld:	return SoyRef("HelloWld");
		case RegisterPeer:	return SoyRef("RegPeer");
		case MemberChanged:	return SoyRef("MemChang");
		}
		return SoyRef();
	}
	
	inline Type		GetType(SoyRef Ref)
	{
		if ( Ref == GetRef( HelloWorld ) )		return HelloWorld;
		if ( Ref == GetRef( SearchWorld ) )		return SearchWorld;
		if ( Ref == GetRef( RegisterPeer ) )	return RegisterPeer;
		if ( Ref == GetRef( MemberChanged ) )	return MemberChanged;
		assert(false);
		return Invalid;
	}
};


template<SoyModulePackets::Type PACKETTYPE>
class SoyModulePacket : public SoyPacket
{
public:
	static const SoyModulePackets::Type TYPE = PACKETTYPE;

public:
	virtual SoyRef		GetType() const	{	return SoyModulePackets::GetRef( PACKETTYPE );	}
};


class SoyModulePacket_MemberChanged : public SoyModulePacket<SoyModulePackets::MemberChanged>
{
public:
	SoyRef				mMemberRef;
	SoyTime				mModifiedTime;
	BufferString<100>	mData;			//	this will need to change...
};

//--------------------------------------------
//	packet to send after connecting to verify self
//--------------------------------------------
class SoyModulePacket_RegisterPeer : public SoyModulePacket<SoyModulePackets::RegisterPeer>
{
public:
	SoyModuleMeta		mPeerMeta;
};


//--------------------------------------------
//	discovery packet to encourage others to connect to us
//--------------------------------------------
class SoyModulePacket_HelloWorld : public SoyModulePacket<SoyModulePackets::HelloWorld>
{
public:
	SoyModuleMeta		mPeerMeta;		//	sender
	uint16				mServerPort;	//	if we're listening on a server, it's this one
};

//--------------------------------------------
//	discovery packet to encourage others to connect to us
//--------------------------------------------
class SoyModulePacket_SearchWorld : public SoyModulePacket<SoyModulePackets::SearchWorld>
{
public:
	//	gr: MIGHT require a varaible...
};






class SoyModuleMemberMeta
{
public:
	SoyModuleMemberMeta(const char* Name) :
		mRef	( Name )
	{
	}

	SoyModuleMemberMeta&	GetMeta()	{	return *this;	}

public:
	SoyRef		mRef;
	SoyTime		mLastModified;
};

//-------------------------------------------------
//	base [enumeratable] member-of-cluster 
//-------------------------------------------------
class SoyModuleMemberBase : public SoyModuleMemberMeta, public ofMutex
{
public:
	SoyModuleMemberBase(SoyModule& Parent,const char* Name);

	virtual void		GetData(BufferString<100>& String) const=0;
	virtual bool		SetData(const BufferString<100>& String,const SoyTime& ModifiedTime=SoyTime())=0;

	ofMutex&			GetMutex() const 	{	return const_cast<ofMutex&>( static_cast<const ofMutex&>(*this) );	}

protected:
	bool				OnDataChanged(const SoyTime& ModifiedTime);

protected:
	SoyModule&			mParent;
};

template<class TDATA>
class SoyModuleMember : public TDATA, public SoyModuleMemberBase
{
public:
	SoyModuleMember(SoyModule& Parent,const char* Name) :
		SoyModuleMemberBase	( Parent, Name )
	{
	}
	template<typename ARG1>
	SoyModuleMember(SoyModule& Parent,const char* Name,const ARG1& arg1) :
		SoyModuleMemberBase	( Parent, Name ),
		TDATA				( arg1 )
	{
	}

	virtual void		GetData(BufferString<100>& String) const	{	ofMutex::ScopedLock Lock(GetMutex());	String << GetData();	}
	TDATA&				GetData()									{	return *this;	}
	const TDATA&		GetData() const								{	return *this;	}
	TDATA				GetDataCopy() const							{	ofMutex::ScopedLock Lock(GetMutex());	return *this;	}
	virtual bool		SetData(const BufferString<100>& String,const SoyTime& ModifiedTime=SoyTime())	{	ofMutex::ScopedLock Lock(GetMutex());	GetData() = TDATA(String);	return OnDataChanged(ModifiedTime);	}
	bool				SetData(const TDATA& Data,const SoyTime& ModifiedTime=SoyTime())				{	ofMutex::ScopedLock Lock(GetMutex());	GetData() = Data;			return OnDataChanged(ModifiedTime);	}
};




class SoyModulePeer : public SoyModuleMeta
{
public:
	SoyModulePeer()
	{
	}
	explicit SoyModulePeer(const SoyRef& Ref) :
		SoyModuleMeta	( Ref )
	{
	}
	explicit SoyModulePeer(const SoyModuleMeta& Meta) :
		SoyModuleMeta	( Meta )
	{
	}
	explicit SoyModulePeer(const SoyRef& Ref,const SoyNet::TAddress& Address) :
		SoyModuleMeta	( Ref )
	{
		AddAddress( Address );
	}

	bool		AddAddress(const SoyNet::TAddress& Address);
	inline bool	operator==(const SoyNet::TAddress& Address)const		{	return mAddresses.Find(Address)!=NULL;	}
	inline bool	operator==(const SoyRef& PeerRef)const					{	return GetMeta() == PeerRef;	}
	inline bool	operator==(const SoyNet::TClientRef& ClientRef)const	{	return mAddresses.Find(ClientRef)!=NULL;	}

public:
	BufferArray<SoyNet::TAddress,10>	mAddresses;		//	ways we are/were connected to this peer
};


//-------------------------------------------------
//	base cluster module
//-------------------------------------------------
class SoyModule : public SoyModuleMeta
{
public:
	SoyModule(SoyRef ModuleRef);
	~SoyModule();

	bool							IsServer() const			{	return mClusterSocket.GetState() == SoyNet::TSocketState::ServerListening;	}
	bool							IsClient() const			{	return mClusterSocket.GetState() == SoyNet::TSocketState::ClientConnected;	}
	void							OpenDiscoveryServer();
	void							DisconnectDiscovery();
	void							OpenClusterServer();
	bool							ConnectToClusterServer(const SoyRef& Peer);	//	returns false if immediate fail
	void							DisconnectCluster();		//	disconnect and cancel any listen/connect attempts

	virtual BufferArray<uint16,100>	GetDiscoveryPortRange() const=0;
	virtual BufferArray<uint16,100>	GetClusterPortRange() const=0;
	BufferString<300>				GetClusterNetworkStatus() const;	//	debug
	BufferString<300>				GetDiscoveryNetworkStatus() const;	//	debug
	virtual void					Update(float TimeStep);
	SoyTime							GetTime() const;			//	synchronised time

	SoyRef							GetServerClientPeer(SoyNet::TClientRef ClientRef) const;

	void							RegisterPeer(const SoyModuleMeta& PeerMeta,const SoyNet::TAddress& Address);
	SoyModulePeer*					GetPeer(const SoyRef& Peer)				{	return mPeers.Find( Peer );	}
	const SoyModulePeer*			GetPeer(const SoyRef& Peer) const		{	return mPeers.Find( Peer );	}

	void							RegisterMember(SoyModuleMemberBase& Member)			{	mMembers.PushBack( &Member );	}
	SoyModuleMemberBase*			GetMember(const SoyRef& MemberRef);
	bool							OnMemberChanged(const SoyModuleMemberBase& Member);

	template<class PACKET>
	bool							SendPacketToPeers(const PACKET& Packet);
	void							SendPacketBroadcastSearchWorld();	//	request info from all clients

protected:
	virtual SoyModule&				GetStateParent()			{	return *this;	}

	virtual bool					OnPacket(const SoyPacketContainer& Packet);	//	return true if handled
	bool							OnPacket(const SoyModulePacket_MemberChanged& Packet);
	bool							OnPacket(const SoyModulePacket_RegisterPeer& Packet,const SoyNet::TAddress& Sender);
	bool							OnPacket(const SoyModulePacket_HelloWorld& Packet,const SoyNet::TAddress& Sender);
	bool							OnPacket(const SoyModulePacket_SearchWorld& Packet,const SoyNet::TAddress& Sender);

	void							OnClusterSocketClosed(bool& Event);
	void							OnClusterSocketClientConnected(bool& Event);
	void							OnClusterSocketServerListening(bool& Event);
	void							OnClusterSocketClientJoin(const SoyNet::TAddress& Client);
	void							OnClusterSocketClientLeft(const SoyNet::TAddress& Client);
	void							OnDiscoverySocketClientJoin(const SoyNet::TAddress& Client);
	void							OnDiscoverySocketClientLeft(const SoyNet::TAddress& Client);
	void							OnDiscoverySocketClosed(bool& Event);

private:
	void							OnDiscoverySocketServerListening(bool& Event);
	bool							OnDiscoveryPacket(const SoyPacketContainer& Packet);
	void							SendPacketBroadcastHelloWorld();	//	send out my info to clients when it changes

	bool							OnServerClientConnected(SoyNet::TClientRef ClientRef);		//	returns if changed
	bool							OnServerClientDisconnected(SoyNet::TClientRef ClientRef);	//	returns if changed
	void							OnPeersChanged();
	
	void							UpdateDiscoverySocket();
	void							UpdateClusterSocket();

public:
	ofEvent<const Array<SoyRef>>	mOnPeersChanged;
	ofEvent<const SoyRef>			mOnMemberChanged;
	ofEvent<const SoyNet::TSocketState::Type>	mOnClusterSocketChanged;
	ofEvent<const SoyNet::TSocketState::Type>	mOnDiscoverySocketChanged;
	
private:
	Array<uint16>					mTryListenDiscoveryPorts;	//	trying to setup a discovery server...
	Array<uint16>					mTryListenClusterPorts;		//	trying to setup a server...
	SoyNet::TAddress				mTryConnectToClusterServer;	//	trying to connect to this server....

	Array<SoyModuleMemberBase*>		mMembers;			//	auto-registered members
	Array<SoyModulePeer>			mPeers;				//	connected peers
	SoyNet::TSocketTCP				mClusterSocket;
	SoyNet::TSocketUDPMultiCast		mDiscoverySocket;
};



template<class PACKET>
bool SoyModule::SendPacketToPeers(const PACKET& Packet)
{
	SoyPacketMeta Meta( GetRef(), Packet.GetType() );
	mClusterSocket.mPacketsOut.PushPacket( Meta, Packet );
	return true;
}

