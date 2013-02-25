#pragma once

#include <ofxNetwork.h>


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
	SoyModuleMemberBase(const char* Name) :
		SoyModuleMemberMeta	( Name )
	{
	}
};

template<class TDATA>
class SoyModuleMember : public TDATA, public SoyModuleMemberBase
{
public:
	SoyModuleMember(const char* Name) :
		SoyModuleMemberBase	( Name )
	{
	}
	template<typename ARG1>
	SoyModuleMember(const char* Name,const ARG1& arg1) :
		SoyModuleMemberBase	( Name ),
		TDATA				( arg1 )
	{
	}

	TDATA&					GetData()	{	return *this;	}
};


class SoyModuleMeta
{
public:
	SoyModuleMeta(const char* Name) :
		mRef	( Name )
	{
	}

public:
	SoyRef		mRef;
};



class SoyModulePacket_Hello
{
public:
	SoyModuleMeta	mMeta;
};


//-------------------------------------------------
//	base cluster module
//-------------------------------------------------
class SoyModule : public SoyModuleMeta, public TStateManager<SoyModule>
{
public:
	SoyModule(const char* Name);

	virtual BufferArray<uint16,10>	GetDiscoveryPortRange() const=0;
	virtual BufferArray<uint16,10>	GetClusterPortRange() const=0;
	BufferString<300>				GetNetworkStatus() const;	//	debug
	virtual void					Update(float TimeStep);

protected:
	virtual SoyModule&				GetStateParent()			{	return *this;	}
	ofxUDPManager& 					GetDiscoveryServer() const	{	return const_cast<ofxUDPManager&>( mDiscoveryServer );	}
	ofxTCPServer& 					GetClusterServer() const	{	return const_cast<ofxTCPServer&>( mClusterServer );	}
	
public:
	ofxUDPManager 					mDiscoveryServer;
	ofxTCPServer 					mClusterServer;
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
//	bind our discovery server
//------------------------------------------------
class SoyModuleState_DiscoveryBind : public SoyModuleState
{
public:
	SoyModuleState_DiscoveryBind(SoyModule& Parent,uint16 Port=0);

	virtual void		Update(float TimeStep);

protected:
	uint16				mPort;
};


//------------------------------------------------
//	bind our module's server
//------------------------------------------------
class SoyModuleState_TryBind : public SoyModuleState
{
public:
	SoyModuleState_TryBind(SoyModule& Parent,uint16 Port=0);

	virtual void		Update(float TimeStep);

protected:
	uint16				mPort;
};


//------------------------------------------------
//	module is now listening
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
//	discovery server couldn't bind
//------------------------------------------------
class SoyModuleState_DiscoveryBindFailed : public SoyModuleState
{
public:
	SoyModuleState_DiscoveryBindFailed(SoyModule& Parent) :
		SoyModuleState	( Parent, "Discovery Bind Failed" )
	{
	}
};

//------------------------------------------------
//	module couldn't bind
//------------------------------------------------
class SoyModuleState_BindFailed : public SoyModuleState
{
public:
	SoyModuleState_BindFailed(SoyModule& Parent) :
		SoyModuleState	( Parent, "Bind Failed" )
	{
	}
};
