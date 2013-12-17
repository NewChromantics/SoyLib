#pragma once

#include "SoyEnum.h"
#include "SoyPacket.h"
#include "SoyNet.h"
#include "SoyThread.h"
#include <ofxNetwork.h>

#define UDP_MAX_BUFFERSIZE	1024*64

namespace SoyNet
{
	class TSocket;
	class TSocketTCP;	
	class TSocketUDP;
	class TSocketUDPMultiCast;	
};


namespace SoyNet
{
	namespace TSocketState
	{
		enum Type
		{
			Closed,
			ServerListening,
			ClientConnected,
		};

		BufferString<100>	ToString(TSocketState::Type Enum);
		void				GetArray(ArrayBridge<TSocketState::Type>& Array);
	};
};
SOY_DECLARE_ENUM( SoyNet::TSocketState );

class SoyNet::TSocket : public SoyThread
{
public:
	TSocket(bool IncludeMetaInPacket=true);
	virtual ~TSocket();

	void				StopThread();
	virtual void		GetConnections(Array<SoyNet::TAddress>& Addresses) const=0;
	virtual bool		Listen(uint16 Port)=0;
	virtual bool		Connect(const SoyNet::TAddress& ServerAddress)=0;
	virtual void		Close()=0;
	TSocketState::Type	GetState() const		{	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mStateLock ) );	return mState;	}

private:
	void				SetState(TSocketState::Type NewState);
	void				StartThread();			//	we don't want to start thread in constructor (do we?) so it's started first time we try to connect/listen etc
	virtual void		threadedFunction();
	
protected:
	//	callbacks which instigate events and update state
	virtual void		OnClosed();				//	will callback if listen or connect fail
	void				OnClientConnected();
	void				OnServerListening();
	void				OnClientJoin(const SoyNet::TAddress& Address);
	void				OnClientLeft(const SoyNet::TAddress& Address);
	void				OnRecievePacket(const SoyPacketContainer& Packet);

	virtual void		CheckState()=0;			//	check if we've been closed
	virtual void		CheckForClients()=0;	//	check clients have connected/disconnected
	virtual void		RecievePackets()=0;	
	virtual void		SendPackets()=0;	

public:
	bool								mIncludeMetaInPacket;
	SoyPacketManager					mPacketsIn;
	SoyPacketManager					mPacketsOut;
	ofEvent<bool>						mOnClosed;
	ofEvent<bool>						mOnClientConnected;
	ofEvent<bool>						mOnServerListening;
	ofEvent<const SoyNet::TAddress>		mOnClientJoin;
	ofEvent<const SoyNet::TAddress>		mOnClientLeft;
	ofEvent<const SoyPacketContainer*>	mOnRecievePacket;

private:
	ofMutex					mStateLock;
	TSocketState::Type		mState;
};


class SoyNet::TSocketTCP : public SoyNet::TSocket
{
public:
	TSocketTCP(bool IncludeMetaInPacket=true) :
		TSocket	( IncludeMetaInPacket )
	{
	}
	virtual void		GetConnections(Array<SoyNet::TAddress>& Addresses) const;
	virtual bool		Listen(uint16 Port);
	virtual bool		Connect(const SoyNet::TAddress& ServerAddress);
	virtual void		Close();
	
	TAddress			GetClientAddress(int ClientId) const;
	TAddress			GetServerAddress() const;
	TAddress			GetMyAddress() const;

protected:
	virtual void		OnClosed();

	virtual void		CheckState();
	virtual void		CheckForClients();
	virtual void		RecievePackets();	
	virtual void		SendPackets();	
	bool				CheckForClient(int ClientId);	//	check an individual client is newly connected/disconnected. Done individually as we cannot lock the connections. returns if client is connected

private:
	//	ofxNetwork stuff isn't very const compliant...
	ofxTCPServer&		GetServerConst() const		{	return const_cast<ofxTCPServer&>( mServer );	}
	ofxTCPClient&		GetClientConst() const		{	return const_cast<ofxTCPClient&>( mClient );	}

private:
	ofxTCPServer 		mServer;
	Array<TAddress>		mClientCache;	//	list of connected clients just so we know when they connect/disconnect
	ofxTCPClient 		mClient;
};



class SoyNet::TSocketUDP : public SoyNet::TSocket
{
public:
	TSocketUDP(bool IncludeMetaInPacket=true) :
		TSocket	( IncludeMetaInPacket )
	{
		//	this is not initialised in ofxUDPManager
		mSocket.SetTimeoutSend( NO_TIMEOUT );
		mSocket.SetTimeoutReceive( NO_TIMEOUT );
	}
	virtual void		GetConnections(Array<SoyNet::TAddress>& Addresses) const;
	virtual bool		Listen(uint16 Port);
	virtual bool		Connect(const SoyNet::TAddress& ServerAddress);
	virtual void		Close();
	
	TAddress			GetMyAddress() const;

protected:
	virtual void		OnClosed();

	virtual void		CheckState();
	virtual void		CheckForClients();
	virtual void		RecievePackets();	
	virtual void		SendPackets();	
	bool				CheckForClient(int ClientId);	//	check an individual client is newly connected/disconnected. Done individually as we cannot lock the connections. returns if client is connected

protected:
	virtual bool		BindUDP(uint16 Port);
	virtual bool		ConnectUDP(const SoyNet::TAddress& ServerAddress);

protected:
	ofMutexT<Array<SoyPair<SoyNet::TAddress,Array<char>>>>	mPacketData;	//	data we've recieved from various sources but not yet processed
	ofMutexT<ofxUDPManager>		mSocket;
};


class SoyNet::TSocketUDPMultiCast : public SoyNet::TSocketUDP
{
public:
	TSocketUDPMultiCast(const char* MultiCastAddress="255.255.255.255") :
		mMultiCastAddress	( MultiCastAddress )
	{
	}

protected:
	virtual bool		BindUDP(uint16 Port);
	virtual bool		ConnectUDP(const SoyNet::TAddress& ServerAddress);

private:
	BufferString<100>	mMultiCastAddress;
};
