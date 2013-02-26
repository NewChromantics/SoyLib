#pragma once

#include <ofxNetwork.h>
#include "SoyPacket.h"


class SoyModule;


namespace SoyModulePackets
{
	enum Type
	{
		MemberChanged,
	};
};


class SoyModulePacket : public SoyPacket<SoyModulePackets::Type>
{
public:
};

class SoyModulePacketManager : public SoyPacketManager<SoyModulePackets::Type>
{
public:
};

template<SoyModulePackets::Type PACKETTYPE>
class SoyModulePacketDerivitive : public SoyModulePacket
{
public:
	static const SoyModulePackets::Type TYPE = PACKETTYPE;

public:
	virtual SoyModulePackets::Type	GetType() const	{	return PACKETTYPE;	}
};

class SoyModulePacket_MemberChanged : public SoyModulePacketDerivitive<SoyModulePackets::MemberChanged>
{
public:
	SoyRef				mMemberRef;
	SoyTime				mModifiedTime;
	BufferString<100>	mData;			//	this will need to change...
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
class SoyModuleMemberBase : public SoyModuleMemberMeta
{
public:
	SoyModuleMemberBase(SoyModule& Parent,const char* Name);

	virtual void		GetData(BufferString<100>& String) const=0;
	virtual bool		SetData(const BufferString<100>& String,const SoyTime& ModifiedTime=SoyTime())=0;

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

	virtual void		GetData(BufferString<100>& String) const	{	String << GetData();	}
	TDATA&				GetData()									{	return *this;	}
	const TDATA&		GetData() const								{	return *this;	}
	virtual bool		SetData(const BufferString<100>& String,const SoyTime& ModifiedTime=SoyTime())	{	GetData() = String;	return OnDataChanged(ModifiedTime);	}
	bool				SetData(const TDATA& Data,const SoyTime& ModifiedTime=SoyTime())				{	GetData() = Data;	return OnDataChanged(ModifiedTime);	}
};


class SoyModuleMeta
{
public:
	SoyModuleMeta(const SoyRef& Ref) :
		mRef	( Ref )
	{
	}

	inline bool	operator==(const SoyRef& Ref) const	{	return mRef == Ref;	}
	inline bool	operator!=(const SoyRef& Ref) const	{	return !(*this == Ref);	}

public:
	SoyRef		mRef;
};

class SoyModulePeerAddress
{
private:
	static SoyRef	gUniqueRef;

public:
	SoyModulePeerAddress() :
		mPort		( 0 ),
		mClientId	( -1 )
	{
	}
	SoyModulePeerAddress(const char* Address,uint16 Port) :
		mPort		( Port ),
		mAddress	( Address ),
		mClientId	( -1 ),
		mRef		( ++gUniqueRef )
	{
	}

	const SoyRef&		GetRef() const	{	return mRef;	}
	inline bool			operator==(const SoyModulePeerAddress& That) const;		//	gr: test client id or not? address AND port should be unique enough...)

public:
	BufferString<100>	mAddress;
	uint16				mPort;
	int					mClientId;	//	clientid of ofxTCPServer's client. -1 if it's not a client (ie. ofxTCPClient's server or self)

private:
	SoyRef				mRef;		//	unique address reference
};

class SoyModulePeer : public SoyModuleMeta
{
public:
	SoyModulePeer(const SoyRef& Ref=SoyRef()) :
		SoyModuleMeta	( Ref )
	{
	}
	SoyModulePeer(const SoyRef& Ref,const SoyModulePeerAddress& Address) :
		SoyModuleMeta	( Ref )
	{
		AddAddress( Address );
	}

	bool		AddAddress(const SoyModulePeerAddress& Address);

public:
	BufferArray<SoyModulePeerAddress,10>	mAddresses;	//	ways we are/were connected to this peer
};


//-------------------------------------------------
//	packet from a server identifying itself
//-------------------------------------------------
class SoyModulePacket_Hello
{
public:
	SoyModulePeer	mPeer;
};


//-------------------------------------------------
//	base cluster module
//-------------------------------------------------
class SoyModule : public SoyModuleMeta, public TStateManager<SoyModule>
{
public:
	SoyModule(const char* Name);

	virtual BufferArray<uint16,100>	GetDiscoveryPortRange() const=0;
	virtual BufferArray<uint16,100>	GetClusterPortRange() const=0;
	BufferString<300>				GetNetworkStatus() const;	//	debug
	virtual void					Update(float TimeStep);
	SoyTime							GetTime() const;			//	synchronised time

	ofxTCPClient& 					GetClusterClient() const				{	return const_cast<ofxTCPClient&>( mClusterClient );	}
	SoyModulePeerAddress			GetClientServerPeerAddress() const;
	void							OnConnectedToServer(const SoyRef& Peer)	{	mServerPeer = Peer;	}

	ofxTCPServer& 					GetClusterServer() const				{	return const_cast<ofxTCPServer&>( mClusterServer );	}
	SoyModulePeerAddress			GetServerClientPeerAddress(int ClientId) const;
	SoyRef							GetServerClientPeer(int ClientId) const;

	void							OnFoundPeer(const SoyRef& Peer,const SoyModulePeerAddress& Address);	//	add to peer list
	SoyModulePeer*					GetPeer(const SoyRef& Peer)				{	return mPeers.Find( Peer );	}
	const SoyModulePeer*			GetPeer(const SoyRef& Peer) const		{	return mPeers.Find( Peer );	}

	void							RegisterMember(SoyModuleMemberBase& Member)			{	mMembers.PushBack( &Member );	}
	SoyModuleMemberBase*			GetMember(const SoyRef& MemberRef);
	bool							OnMemberChanged(const SoyModuleMemberBase& Member);

	virtual bool					OnPacket(const SoyModulePacket& Packet);	//	return true if handled
	bool							OnPacket(const SoyModulePacket_MemberChanged& Packet);

protected:
	virtual SoyModule&				GetStateParent()			{	return *this;	}

public:
	ofEvent<const Array<SoyRef>>	mOnPeersChanged;
	SoyModulePacketManager			mPacketsIn;
	SoyModulePacketManager			mPacketsOut;
	
private:
	Array<SoyModuleMemberBase*>		mMembers;			//	auto-registered members
	Array<SoyModulePeer>			mPeers;
	ofxTCPServer 					mClusterServer;
	ofxTCPClient 					mClusterClient;
	SoyRef							mServerPeer;		//	who we're connected to (master)
};

class SoyModuleState : public TState<SoyModule>
{
public:
	SoyModuleState(SoyModule& Parent,const char* Name) :
		TState	( Parent ),
		mName	( Name )
	{
	}

	const BufferString<100>&	GetStateName() const	{	return mName;	}

private:
	BufferString<100>	mName;
};



//------------------------------------------------
//	bind our module's server
//------------------------------------------------
class SoyModuleState_ServerBind : public SoyModuleState
{
public:
	SoyModuleState_ServerBind(SoyModule& Parent,uint16 Port=0);

	virtual void		Update(float TimeStep);

protected:
	uint16				mPort;
};




//------------------------------------------------
//	module is now listening (waiting to connect, if we're a client)
//------------------------------------------------
class SoyModuleState_Listening : public SoyModuleState
{
public:
	SoyModuleState_Listening(SoyModule& Parent) :
		SoyModuleState	( Parent, "Listening" )
	{
	}
};

//------------------------------------------------
//	connect to a server
//------------------------------------------------
class SoyModuleState_ClientConnect : public SoyModuleState
{
public:
	SoyModuleState_ClientConnect(SoyModule& Parent,const SoyRef& Peer);

	virtual void		Update(float TimeStep);

protected:
	SoyRef				mPeer;
};

//------------------------------------------------
//	module is connected
//------------------------------------------------
class SoyModuleState_Connected : public SoyModuleState
{
public:
	SoyModuleState_Connected(SoyModule& Parent) :
		SoyModuleState	( Parent, "Connected" )
	{
	}
};


//------------------------------------------------
//	couldn't bind
//------------------------------------------------
class SoyModuleState_BindFailed : public SoyModuleState
{
public:
	SoyModuleState_BindFailed(SoyModule& Parent) :
		SoyModuleState	( Parent, "Bind Failed" )
	{
	}
};
