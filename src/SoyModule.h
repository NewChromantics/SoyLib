#pragma once

#include <ofxNetwork.h>


class SoyModule;



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
	SoyModuleMemberBase(SoyModule& Parent,const char* Name) :
		mParent				( Parent ),
		SoyModuleMemberMeta	( Name )
	{
	}

	virtual void		GetData(BufferString<100>& String) const=0;
	virtual bool		SetData(const BufferString<100>& String)=0;

protected:
	bool				OnDataChanged();

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
	virtual bool		SetData(const BufferString<100>& String)	{	GetData() = String;	return OnDataChanged();	}
	bool				SetData(const TDATA& Data)					{	GetData() = Data;	return OnDataChanged();	}
};


class SoyModuleMeta
{
public:
	SoyModuleMeta(const char* Name) :
		mRef	( Name )
	{
	}

	inline bool	operator==(const SoyRef& Ref) const	{	return mRef == Ref;	}
	inline bool	operator!=(const SoyRef& Ref) const	{	return !(*this == Ref);	}

public:
	SoyRef		mRef;
};




class SoyModulePeer : public SoyModuleMeta
{
public:
	SoyModulePeer() :
		SoyModuleMeta	( NULL )
	{
	}

public:
	BufferString<100>	mAddress;
	int					mPort;
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

	ofxTCPServer& 					GetClusterServer() const	{	return const_cast<ofxTCPServer&>( mClusterServer );	}
	ofxTCPClient& 					GetClusterClient() const	{	return const_cast<ofxTCPClient&>( mClusterClient );	}
	void							OnConnectedToServer(const SoyRef& Peer)	{	mServerPeer = Peer;	}
	void							OnFoundPeer(const SoyModulePeer& Peer);	//	add to peer list
	const SoyModulePeer*			GetPeer(const SoyRef& Peer) const	{	return mPeers.Find( Peer );	}

	bool							OnMemberChanged(const SoyModuleMemberBase& Member);

protected:
	virtual SoyModule&				GetStateParent()			{	return *this;	}

public:
	ofEvent<const Array<SoyRef>>	mOnPeersChanged;
	
private:
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
