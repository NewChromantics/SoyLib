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

bool SoyStream::TFile::Read(ofxXmlSettings& xml)
{
	BufferString<MAX_PATH> Filename = GetFilename();
	if ( !xml.loadFile( Filename.c_str() ) )
		return false;
	//	mark last-modified
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

	return true;
}



bool SoyStream::TSocket::Read(ofxXmlSettings& xml)
{
	//	connect the socket if it's not already "connected"
	if ( !ConnectSocket() )
		return false;
	/*
	//	push packet into socket
	auto& Socket = GetSocket();
	if ( !PushPacket( xml, Socket.mPacketsOut ) )
		return false;
		
	return true;
	*/
	return false;
}

