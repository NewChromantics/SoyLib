#pragma once

#include "SoyPacket.h"
#include "SoyNet.h"


namespace SoyNet
{
	class TSocket;
	class TSocketTCP;	
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

class SoyNet::TSocket
{
public:
	TSocket();
	virtual ~TSocket();

	void				Update();				//	todo: thread this
	virtual void		GetConnections(Array<SoyNet::TAddress>& Addresses) const=0;
	virtual bool		Listen(uint16 Port)=0;
	virtual bool		Connect(const SoyNet::TAddress& ServerAddress)=0;
	virtual void		Close()=0;
	TSocketState::Type	GetState() const		{	return mState;	}
	
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
	SoyPacketManager					mPacketsIn;
	SoyPacketManager					mPacketsOut;
	ofEvent<bool>						mOnClosed;
	ofEvent<bool>						mOnClientConnected;
	ofEvent<bool>						mOnServerListening;
	ofEvent<const SoyNet::TAddress>		mOnClientJoin;
	ofEvent<const SoyNet::TAddress>		mOnClientLeft;
	ofEvent<const SoyPacketContainer*>	mOnRecievePacket;

private:
	TSocketState::Type			mState;
};


class SoyNet::TSocketTCP : public SoyNet::TSocket
{
public:
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

