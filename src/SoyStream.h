#pragma once


namespace SoyStream
{
	class TStream;
	class TFile;
	class TSocket;
};

namespace SoyNet
{
	class TSocket;
	class TAddress;
};

class ofxXmlSettings;
class SoyPacketManager;





class SoyStream::TStream
{
public:
	virtual ~TStream()	{}

	virtual bool	IsValid() const=0;
	virtual bool	Write(const ofxXmlSettings& xml)=0;
};


class SoyStream::TFile : public SoyStream::TStream
{
public:
	TFile(const char* Filename) :
		mFilename	( Filename )
	{
	}

	virtual bool	IsValid() const						{	return true;	}
	virtual bool	Write(const ofxXmlSettings& xml);

protected:
	virtual BufferString<MAX_PATH>	GetFilename()		{	return mFilename;	}

protected:
	BufferString<MAX_PATH>	mFilename;
};



class SoyStream::TSocket : public SoyStream::TStream
{
public:
	TSocket(const SoyNet::TAddress& ServerAddress);

	virtual bool	IsValid() const;
	virtual bool	Write(const ofxXmlSettings& xml);

protected:
	virtual SoyNet::TSocket&	GetSocket()=0;
	virtual bool	PushPacket(const ofxXmlSettings& xml,SoyPacketManager& PacketManager);
};

