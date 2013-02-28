#include "ofxSoylent.h"
#include "SoySocket.h"


using namespace SoyNet;

BufferString<100> TSocketState::ToString(TSocketState::Type Enum)
{
	switch ( Enum )
	{
	case Closed:			return "Closed";
	case ClientConnected:	return "ClientConnected";
	case ServerListening:	return "ServerListening";
	};
	assert(false);
	return NULL;
}

void TSocketState::GetArray(ArrayBridge<TSocketState::Type>& Array)
{
	Array.PushBack( TSocketState::Closed );
	Array.PushBack( TSocketState::ClientConnected );
	Array.PushBack( TSocketState::ServerListening );
}


namespace TPacketReadResult
{
	enum Type
	{
		NotEnoughData,
		Error,
		Okay,
	};
}

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
	int Size = ReadWrapper.peekReceiveRawBytes( Array );
	if ( Size == SOCKET_ERROR )
		return TPacketReadResult::Error;
	if ( Size < Array.GetDataSize() )
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
bool RecvPackets(SoyPacketManager& PacketManager,READWRAPPER& ReadWrapper)
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



TSocket::TSocket() :
	mState	( TSocketState::Closed )
{
}
	
TSocket::~TSocket()
{
}

void TSocket::Update()
{
	//	check in case we've been disconnected
	CheckState();

	//	check for new clients
	CheckForClients();

	//	send/recieve packets
	RecievePackets();
	SendPackets();
}


void TSocket::OnClosed()
{
	mState = TSocketState::Closed;
	bool Event;
	ofNotifyEvent( mOnClosed, Event );
}

void TSocket::OnClientConnected()
{
	mState = TSocketState::ClientConnected;
	bool Event;
	ofNotifyEvent( mOnClientConnected, Event );
}

void TSocket::OnServerListening()
{
	mState = TSocketState::ServerListening;
	bool Event;
	ofNotifyEvent( mOnServerListening, Event );
}
	
void TSocket::OnClientJoin(const SoyNet::TAddress& Address)
{
	ofNotifyEvent( mOnClientJoin, Address );
}

void TSocket::OnClientLeft(const SoyNet::TAddress& Address)
{
	ofNotifyEvent( mOnClientLeft, Address );
}

void TSocket::OnRecievePacket(const SoyPacketContainer& Packet)
{
	const SoyPacketContainer* pPacket = &Packet;
	ofNotifyEvent( mOnRecievePacket, pPacket );
}








void TSocketTCP::GetConnections(Array<SoyNet::TAddress>& Addresses) const
{
	Addresses.PushBackArray( mClientCache );
}

bool TSocketTCP::Listen(uint16 Port)
{
	//	close client/old server if it's open
	Close();

	if ( !mServer.setup( Port, false ) )
	{
		mServer.close();	//	ofxNetwork doesnt cleanup properly
		return false;
	}

	OnServerListening();
	return true;
}

bool TSocketTCP::Connect(const SoyNet::TAddress& ServerAddress)
{
	//	close old client/server if it's open
	Close();

	const char* Address = static_cast<const char*>(ServerAddress.mAddress);
	if ( !mClient.setup( Address, ServerAddress.mPort, false ) )
	{
		mClient.close();	//	ofxNetwork doesnt cleanup properly?
		return false;
	}

	OnClientConnected();
	return true;
}

void TSocketTCP::Close()
{
	if ( mClient.isConnected() )
	{
		mClient.close();
		OnClosed();
	}

	if ( mServer.isConnected() )
	{
		mServer.close();
		OnClosed();
	}
}


void TSocketTCP::CheckState()
{
	if ( GetState() == TSocketState::ClientConnected && !mClient.isConnected() )
	{
		OnClosed();
	}

	if ( GetState() == TSocketState::ServerListening && !mServer.isConnected() )
	{
		OnClosed();
	}
}

void TSocketTCP::CheckForClients()
{
	//	check for new/closed clients
	for ( int i=0;	i<mServer.getLastID();	i++ )
	{
		int ClientId = i;
		CheckForClient( ClientId );
	}
}

//	check an individual client is newly connected/disconnected. Done individually as we cannot lock the connections
//	returns if-connected
bool TSocketTCP::CheckForClient(int ClientId)
{
	TAddress ClientAddress = GetClientAddress( ClientId );
	bool Connected = mServer.isClientConnected( ClientId );
	int CachedClientIndex = mClientCache.FindIndex( ClientAddress );

	//	probbaly lost address. check for client id instead of address
	if ( !Connected && CachedClientIndex==-1 )
		CachedClientIndex = mClientCache.FindIndex( TClientRef(ClientId) );

	bool ExistingClient = (CachedClientIndex!=-1);

	if ( Connected && !ExistingClient )
	{
		//	new client!
		mClientCache.PushBack( ClientAddress );
		OnClientJoin( ClientAddress );
	}
	else if ( !Connected && ExistingClient )
	{
		//	client left!
		mClientCache.RemoveBlock( CachedClientIndex, 1 );
		OnClientLeft( ClientAddress );
	}

	return Connected;
}


void TSocketTCP::RecievePackets()
{
	//	read in packets from server
	if ( mClient.isConnected() )
	{
		TClientReadWrapper Client( mClient, GetServerAddress() );
		if ( !RecvPackets( mPacketsIn, Client ) )
		{
			//	fatal error, disconnect client
			Close();
		}
	}

	//	read in packets from each client
	if ( mServer.isConnected() )
	{
		for ( int i=0;	i<mServer.getLastID();	i++ )
		{
			int ClientId = i;
			if ( !CheckForClient( ClientId ) )
				continue;
		
			TServerReadWrapper Client( mServer, ClientId, GetClientAddress(ClientId) );
			if ( !RecvPackets( mPacketsIn, Client ) )
			{
				//	fatal error, disconnect client (should we disconnect ourselves? maybe depends on error)
				mServer.disconnectClient( ClientId );
			}
		}
	}
}


void TSocketTCP::SendPackets()
{
	while ( !mPacketsOut.IsEmpty() )
	{
		Array<char> PacketRaw;
		if ( !mPacketsOut.PopPacketRawData( PacketRaw ) )
			break;

		//	send packet out
		if ( mServer.isConnected() )
		{
			mServer.sendRawBytesToAll( PacketRaw.GetArray(), PacketRaw.GetDataSize() );
		}

		if(  mClient.isConnected() )
		{
			mClient.sendRawBytes( PacketRaw.GetArray(), PacketRaw.GetDataSize() );
		}
	}
}


SoyNet::TAddress SoyNet::TSocketTCP::GetMyAddress() const
{
	if ( GetServerConst().isConnected() )
		return SoyNet::TAddress("localhost", GetServerConst().getPort() );

	if ( GetClientConst().isConnected() )
		return SoyNet::TAddress("localhost", GetClientConst().getPort() );

	return SoyNet::TAddress();
}

SoyNet::TAddress SoyNet::TSocketTCP::GetClientAddress(int ClientId) const
{
	if ( GetServerConst().isClientConnected( ClientId ) )
	{
		SoyNet::TAddress Address;
		Address.mAddress = GetServerConst().getClientIP( ClientId ).c_str();
		Address.mPort = GetServerConst().getClientPort( ClientId );
		Address.mClientRef = TClientRef( ClientId );
		return Address;
	}

	return TAddress();
}

SoyNet::TAddress SoyNet::TSocketTCP::GetServerAddress() const
{
	SoyNet::TAddress Address;
	Address.mAddress = GetClientConst().getIP().c_str();
	Address.mPort = GetClientConst().getPort();
	return Address;
}


void TSocketTCP::OnClosed()
{
	//	clear client cache
	mClientCache.Clear();
	TSocket::OnClosed();
}
