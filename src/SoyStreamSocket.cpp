#include "SoyStreamSocket.h"
#include "SoySocket.h"
#include "ofxXmlSettings.h"





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
	
	//	pop packet
	auto& Socket = GetSocket();

	//	get data as string array
	Array<char> PacketXml;
	if ( !Socket.mPacketsIn.PopPacketRawData( PacketXml, false ) )
		return false;

	//	add a terminator, just in case
	PacketXml.PushBack('\0');

	//	parse xml
	if ( !xml.loadFromBuffer( PacketXml.GetArray() ) )
	{
		BufferString<1000> Debug;
		Debug << "Failed to parse XML packet... ";
		
		if ( PacketXml.GetSize() <= 1 )
		{
			Debug << "No content";
		}
		else
		{
			int MidPoint = PacketXml.GetSize() / 2;
			BufferString<100> XmlStart;
			XmlStart.CopyString( &PacketXml[0], ofMin<int>( 99, MidPoint ) );
			XmlStart.Replace('\n',' ');
			XmlStart.Replace('\t',' ');

			BufferString<100> XmlEnd;
			int Last = PacketXml.GetSize() - 1;
			int LastLen = ofMin<int>( 99, Last - MidPoint );
			XmlEnd.CopyString( &PacketXml[Last-LastLen], LastLen );
			XmlEnd.Replace('\n',' ');
			XmlEnd.Replace('\t',' ');

			Debug << "\n" << XmlStart << "\n...\n" << XmlEnd;
		}
		ofLogError( Debug.c_str() );
		return false;
	}

	return true;
}

