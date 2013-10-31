#pragma once


#include "SoyStream.h"
#include "SoyNet.h"
#include "SoyPacket.h"


namespace SoyStream
{
	class TSocket;

	template<class SOCKETTYPE>
	class TWriteSocket;
	template<class SOCKETTYPE>
	class TReadSocket;
};

namespace SoyNet
{
	class TSocket;
	class TAddress;
};

class SoyPacketManager;






class SoyStream::TSocket : public SoyStream::TStream
{
public:
	TSocket()
	{
	}

	virtual bool	Write(const ofxXmlSettings& xml);
	virtual bool	Read(ofxXmlSettings& xml);

protected:
	virtual SoyNet::TSocket&	GetSocket()=0;
	virtual bool				ConnectSocket()=0;
	virtual bool				PushPacket(const ofxXmlSettings& xml,SoyPacketManager& PacketManager)=0;
};


template<class SOCKETTYPE>
class SoyStream::TWriteSocket : public SoyStream::TSocket
{
public:
	TWriteSocket(const SoyNet::TAddress& ServerAddress,SoyPacketMeta PacketMeta,bool IncludeMetaInPacket=true) :
		mServerAddress	( ServerAddress ),
		mSocket			( IncludeMetaInPacket ),
		mPacketMeta		( PacketMeta )
	{
	}

	virtual bool	IsValid()					{	return mServerAddress.IsValid();	}
	virtual bool	HasChanged()				{	return false;	}
	virtual bool	Read(ofxXmlSettings& xml)	{	return false;	}


protected:
	virtual SoyNet::TSocket&	GetSocket()				{	return mSocket;	}
	virtual bool				ConnectSocket();
	virtual bool				PushPacket(const ofxXmlSettings& xml,SoyPacketManager& PacketManager);

protected:
	SOCKETTYPE			mSocket;
	SoyNet::TAddress	mServerAddress;
	SoyPacketMeta		mPacketMeta;
};


template<class SOCKETTYPE>
class SoyStream::TReadSocket : public SoyStream::TSocket
{
public:
	TReadSocket(const uint16 ListenPort,bool IncludeMetaInPacket=true) :
		mPort	( ListenPort ),
		mSocket	( IncludeMetaInPacket )
	{
	}

	virtual bool	IsValid()							{	return ConnectSocket();	}
	virtual bool	HasChanged();
	virtual bool	Write(const ofxXmlSettings& xml)	{	return false;	}

protected:
	virtual SoyNet::TSocket&	GetSocket()				{	return mSocket;	}
	virtual bool				ConnectSocket();
	virtual bool				PushPacket(const ofxXmlSettings& xml,SoyPacketManager& PacketManager)	{	return false;	}

protected:
	uint16			mPort;
	SOCKETTYPE		mSocket;
};



template<class SOCKETTYPE>
bool SoyStream::TWriteSocket<SOCKETTYPE>::ConnectSocket()
{
	//	get the socket
	auto& Socket = GetSocket();
	if ( Socket.GetState() != SoyNet::TSocketState::ClientConnected )
	{
		//	make sure it's connected
		if ( !Socket.Connect( mServerAddress ) )
			return false;
	}
	return true;	
}

template<class SOCKETTYPE>
bool SoyStream::TWriteSocket<SOCKETTYPE>::PushPacket(const ofxXmlSettings& xml,SoyPacketManager& PacketManager)
{
	//	convert xml to string
	std::string xmlstdstring;
	auto& xmlmutable = const_cast<ofxXmlSettings&>( xml );
	xmlmutable.copyXmlToString( xmlstdstring );
	TString xmlstring = xmlstdstring.c_str();
	
	//	make packet
	PacketManager.PushPacket( mPacketMeta, xmlstring.GetArray(), mSocket.GetMyAddress() );

	return true;
}



template<class SOCKETTYPE>
bool SoyStream::TReadSocket<SOCKETTYPE>::ConnectSocket()
{
	//	get the socket
	auto& Socket = GetSocket();
	if ( Socket.GetState() != SoyNet::TSocketState::ServerListening )
	{
		//	make sure it's connected
		if ( !Socket.Listen( mPort ) )
			return false;
	}
	return true;	
}

template<class SOCKETTYPE>
bool SoyStream::TReadSocket<SOCKETTYPE>::HasChanged()
{
	if ( !ConnectSocket() )
		return false;

	auto& Socket = GetSocket();
	if ( Socket.mPacketsIn.IsEmpty() )
		return false;

	return true;
}

