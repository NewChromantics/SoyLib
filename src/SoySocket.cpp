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

	template<typename DATATYPE>
	int					receiveRawBytes(ArrayBridge<DATATYPE>& Array)	
	{
		return mServer.receiveRawBytes( mClientId, reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	
	}
	template<typename DATATYPE>
	int					peekReceiveRawBytes(ArrayBridge<DATATYPE>& Array)	
	{
		return mServer.peekReceiveRawBytes( mClientId, reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	
	}

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

	template<typename DATATYPE>
	int					receiveRawBytes(ArrayBridge<DATATYPE>& Array)	
	{
		return mClient.receiveRawBytes( reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	
	}
	template<typename DATATYPE>
	int					peekReceiveRawBytes(ArrayBridge<DATATYPE>& Array)	
	{
		return mClient.peekReceiveRawBytes( reinterpret_cast<char*>( Array.GetArray() ), Array.GetDataSize() );	
	}

public:
	SoyNet::TAddress	mSender;
	ofxTCPClient&		mClient;
};


class TDataReadWrapper
{
public:
	TDataReadWrapper(Array<char>& Data,const SoyNet::TAddress& Sender) :
		mData		( Data ),
		mSender		( Sender )
	{
	}

	template<typename DATATYPE>
	int					receiveRawBytes(ArrayBridge<DATATYPE>& Array)	
	{
		//	should be enough space...
		if ( Array.GetDataSize() > mData.GetDataSize() )
		{
			assert( Array.GetDataSize() <= mData.GetDataSize() );
			return 0;
		}

		//	pop data from mData into Array
		char* pDestData = reinterpret_cast<char*>( Array.GetArray() );
		int CopySize = Array.GetDataSize();
		memcpy( pDestData, mData.GetArray(), Array.GetDataSize() );
		mData.RemoveBlock( 0, CopySize );
		return CopySize;
	}
	template<typename DATATYPE>
	int					peekReceiveRawBytes(ArrayBridge<DATATYPE>& Array)
	{
		return mData.GetDataSize();
	}

public:
	SoyNet::TAddress	mSender;
	Array<char>&		mData;
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

	return TPacketReadResult::Okay;
}

template<class READWRAPPER>
bool RecvPackets(SoyPacketManager& PacketManager,READWRAPPER& ReadWrapper,bool ExpectingPacketMeta)
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
		Array<char> PacketData;

		if ( ExpectingPacketMeta )
		{
			auto HeaderResult = ReadChunk( GetArrayBridge( PacketHeaderData ), ReadWrapper );
			if ( HeaderResult == TPacketReadResult::Error )
				return false;
			if ( HeaderResult == TPacketReadResult::NotEnoughData )
				return true;

			//	check the packet header is okay
			if ( !PacketHeader.IsValid() )
				return false;

			//	read the rest of the packet data
			PacketData.SetSize( PacketHeader.mDataSize );
			auto DataResult = ReadChunk( GetArrayBridge( PacketData ), ReadWrapper );
			if ( DataResult == TPacketReadResult::Error )
				return false;

			//	still waiting for data (but we've read the header)
			if ( DataResult == TPacketReadResult::NotEnoughData )
			{
				PacketManager.PushPendingPacket( PacketHeader, ReadWrapper.mSender );
				return true;
			}
		}
		else
		{
			//	dumb read
			PacketHeader.mDataSize = ReadWrapper.peekReceiveRawBytes( GetArrayBridge( PacketData ) );
			if ( PacketHeader.mDataSize == 0 )
				return false;

			PacketData.SetSize( PacketHeader.mDataSize );
			int BytesRecieved = ReadWrapper.receiveRawBytes( GetArrayBridge( PacketData ) );
			assert( BytesRecieved == PacketHeader.mDataSize );
			if ( BytesRecieved != PacketHeader.mDataSize )
				return false;
		}

		//	push new packet
		PacketManager.PushPacket( PacketHeader, PacketData, ReadWrapper.mSender );
	}
	
}



TSocket::TSocket(bool IncludeMetaInPacket) :
	SoyThread				( "TSocket" ),
	mIncludeMetaInPacket	( IncludeMetaInPacket ),
	mState					( TSocketState::Closed )
{
}
	
TSocket::~TSocket()
{
	//	gr: bit late here :/ parent should have already called the blocking StopThread
	assert( !isThreadRunning() );
	SoyThread::waitForThread();
}

void TSocket::StopThread()
{
	SoyThread::waitForThread();
}

void TSocket::StartThread()
{
	//	gr: if(isrunning) just for debugging
	if ( !isThreadRunning() )
		SoyThread::startThread( true, true );
}

void TSocket::threadedFunction()
{
	while ( isThreadRunning() )
	{
		sleep(1);

		//	check in case we've been disconnected
		CheckState();

		//	check for new clients
		CheckForClients();

		//	send/recieve packets
		RecievePackets();
		SendPackets();
	}
}
 
void TSocket::SetState(TSocketState::Type NewState)
{
	ofMutex::ScopedLock Lock( mStateLock );
	mState = NewState;
}

void TSocket::OnClosed()
{
	SetState( TSocketState::Closed );
	bool Event;
	ofNotifyEvent( mOnClosed, Event );
}

void TSocket::OnClientConnected()
{
	StartThread();
	SetState( TSocketState::ClientConnected );
	bool Event;
	ofNotifyEvent( mOnClientConnected, Event );
}

void TSocket::OnServerListening()
{
	StartThread();
	SetState( TSocketState::ServerListening );
	bool Event;
	ofNotifyEvent( mOnServerListening, Event );
}
	
void TSocket::OnClientJoin(const SoyNet::TAddress& Address)
{
	StartThread();
	ofNotifyEvent( mOnClientJoin, Address );
}

void TSocket::OnClientLeft(const SoyNet::TAddress& Address)
{
	StartThread();
	ofNotifyEvent( mOnClientLeft, Address );
}

void TSocket::OnRecievePacket(const SoyPacketContainer& Packet)
{
	StartThread();
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
		if ( !RecvPackets( mPacketsIn, Client, mIncludeMetaInPacket ) )
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
			if ( !RecvPackets( mPacketsIn, Client, mIncludeMetaInPacket ) )
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
		if ( !mPacketsOut.PopPacketRawData( PacketRaw, mIncludeMetaInPacket ) )
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







void TSocketUDP::GetConnections(Array<SoyNet::TAddress>& Addresses) const
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mPacketData.GetMutex() ) );

	//	look in our cache of addresses. Some of these might be dangling, but haven't given us an error
	for ( int i=0;	i<mPacketData.GetSize();	i++ )
	{
		auto& Address = mPacketData[i].mFirst;
		Addresses.PushBack( Address );
	}
}

bool TSocketUDP::Listen(uint16 Port)
{
	//	close old server if it's open
	Close();

	ofMutex::ScopedLock Lock( mSocket );

	if ( !mSocket.Create() )
		return false;

	//	gr: we need multiplexing some how... if UDP is on different ports, we can't find each other
	//		if we share a port, only the first/earlier sockets recieve a broadcast packet (C sends to A&B, A doesn't send to B or C)
	/*
	if ( !mSocket.SetReuseAddress(false) )
	{
		Close();
		return false;
	}
	*/

	if ( !BindUDP( Port ) )
	{
		Close();
		return false;
	}

	if ( !mSocket.SetNonBlocking( true ) )
	{
		Close();
		return false;
	}

	OnServerListening();
	return true;
}

bool TSocketUDP::Connect(const SoyNet::TAddress& ServerAddress)
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );

	if ( !mSocket.Create() )
		return false;
	
	//	must bind to recv on same port
	if ( !BindUDP(0) )
	{
		Close();
		return false;
	}

	if ( !ConnectUDP( ServerAddress ) )
	{
		Close();
		return false;
	}
	if ( !mSocket.SetNonBlocking( true ) )
	{
		Close();
		return false;
	}

	OnClientConnected();
	return true;
}

bool TSocketUDP::BindUDP(uint16 Port)
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );
	if ( !mSocket.Bind( Port ) )
		return false;

	//	make buffer recv size bigger than default
	mSocket.SetReceiveBufferSize( 1024*64 );	//	udp max
	return true;
}

bool TSocketUDP::ConnectUDP(const SoyNet::TAddress& ServerAddress)
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );
	const char* Address = static_cast<const char*>(ServerAddress.mAddress);
	return mSocket.Connect( Address, ServerAddress.mPort );
}


void TSocketUDP::Close()
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );
	if ( mSocket.Close() )
	{
		OnClosed();
	}
}


void TSocketUDP::CheckState()
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );

	if ( GetState() == TSocketState::ClientConnected && !mSocket.HasSocket() )
	{
		OnClosed();
	}

	if ( GetState() == TSocketState::ServerListening && !mSocket.HasSocket() )
	{
		OnClosed();
	}
}

void TSocketUDP::CheckForClients()
{
}

void TSocketUDP::RecievePackets()
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );

	//	recieve latest chunk into our buffer
	while ( mSocket.HasSocket() )
	{
		//	do a recv into our pending buffer and grab the source of the data
		int MaxHardareBufferSize = mSocket.GetReceiveBufferSize();
		int MaxBufferSize = mSocket.PeekReceive();
		if ( MaxBufferSize == SOCKET_ERROR )
		{
			Close();
			break;
		}
		//	nothing to recieve
		if ( MaxBufferSize == 0 )
			break;
		Array<char> Buffer( MaxBufferSize );
		int BufferSize = (MaxBufferSize<=0) ? MaxBufferSize : mSocket.Receive( Buffer.GetArray(), Buffer.GetDataSize() );

		//	socket error
		if ( BufferSize == SOCKET_ERROR )
		{
			Close();
			break;
		}

		//	nothing to do
		if ( BufferSize == 0 )
			break;

		//	fix buffer size according to how much data we recieved
		Buffer.SetSize( BufferSize, true, true );

		//	grab source of data
		string AddressString;
		int Port;
		//	discard if we can't find address
		if ( !mSocket.GetRemoteAddr( AddressString, Port ) )
		{
			BufferString<100> Debug;
			Debug << __FUNCTION__ << ": Couldn't get remote address for packet (" << Buffer.GetDataSize() << " bytes)";
			ofLogNotice( static_cast<const char*>(Debug) );
			continue;	
		}

		//	get the existing data array (or add a new one if it's a new client)
		TAddress Address( AddressString.c_str(), Port );
		ofMutex::ScopedLock PacketDataLock( const_cast<ofMutex&>( mPacketData.GetMutex() ) );
		auto* pDataContainer = mPacketData.Find( Address );
		if ( !pDataContainer )
		{
			//	new connection!
			OnClientJoin( Address );
			pDataContainer = &mPacketData.PushBack( Address );
		}

		//	append new data
		pDataContainer->mSecond.PushBackArray( Buffer );
	}

	ofMutex::ScopedLock PacketDataLock( const_cast<ofMutex&>( mPacketData.GetMutex() ) );
	for ( int i=mPacketData.GetSize()-1;	i>=0;	i-- )
	{
		auto& Address = mPacketData[i].mFirst;
		auto& Data = mPacketData[i].mSecond;

		TDataReadWrapper Client( Data, Address );
		if ( RecvPackets( mPacketsIn, Client, mIncludeMetaInPacket ) )
			continue;

		//	fatal error, "disconnect client" by removing the data stored for the address
		//	copy address before we delete it
		TAddress AddressCopy = Address;
		mPacketData.RemoveBlock( i, 1 );
		OnClientLeft( AddressCopy );
	}
}


void TSocketUDP::SendPackets()
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );
	while ( !mPacketsOut.IsEmpty() )
	{
		Array<char> PacketRaw;
		if ( !mPacketsOut.PopPacketRawData( PacketRaw, mIncludeMetaInPacket ) )
			break;

		if ( !mSocket.HasSocket() )
			continue;

		int SendResult = mSocket.SendAll( PacketRaw.GetArray(), PacketRaw.GetDataSize() );

	}
}


SoyNet::TAddress SoyNet::TSocketUDP::GetMyAddress() const
{
	ofMutex::ScopedLock Lock( const_cast<ofMutex&>( mSocket.GetMutex() ) );
	if ( mSocket.HasSocket() )
	{
		string Address;
		int Port;
		if ( mSocket.GetListenAddr( Address, Port ) )
			return TAddress( Address.c_str(), Port );
	}

	return SoyNet::TAddress();
}


void TSocketUDP::OnClosed()
{
	//	clear client cache
	//mClientCache.Clear();
	TSocket::OnClosed();
}



bool TSocketUDPMultiCast::BindUDP(uint16 Port)
{
	mSocket.SetEnableBroadcast( true );
	//return mSocket.BindMcast( mMultiCastAddress, Port );
	return mSocket.ConnectMcast( mMultiCastAddress, Port );
}

bool TSocketUDPMultiCast::ConnectUDP(const SoyNet::TAddress& ServerAddress)
{
	return mSocket.ConnectMcast( mMultiCastAddress, ServerAddress.mPort );
}