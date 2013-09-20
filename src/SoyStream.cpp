#include "ofxSoylent.h"
#include "SoyStream.h"
#include "SoySocket.h"
#include "ofxXmlSettings.h"


bool SoyStream::TFile::Write(const ofxXmlSettings& xml)
{
	BufferString<MAX_PATH> Filename = GetFilename();
	auto& xmlmutable = const_cast<ofxXmlSettings&>( xml );
	return xmlmutable.saveFile( Filename.c_str() );
}



SoyStream::TSocket::TSocket(const SoyNet::TAddress& ServerAddress) :
	mServerAddress	( ServerAddress )
{
}

bool SoyStream::TSocket::IsValid() const
{
	return mServerAddress.IsValid();
}

bool SoyStream::TSocket::ConnectSocket()
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

bool SoyStream::TSocket::Write(const ofxXmlSettings& xml)
{
	//	connect the socket if it's not already "connected"
	if ( !ConnectSocket() )
		return false;

	//	push packet into socket
	auto& Socket = GetSocket();
	if ( !PushPacket( xml, Socket.mPacketsOut ) )
		return false;

	Socket.Update();

	return true;
}


