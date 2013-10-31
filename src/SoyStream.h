#pragma once

#include "String.hpp"


namespace SoyStream
{
	class TStream;
	class TFile;
};

class ofxXmlSettings;





class SoyStream::TStream
{
public:
	virtual ~TStream()	{}

	virtual bool	IsValid()=0;
	virtual bool	Write(const ofxXmlSettings& xml)=0;
	virtual bool	HasChanged()=0;
	virtual bool	Read(ofxXmlSettings& xml)=0;
};


class SoyStream::TFile : public SoyStream::TStream
{
public:
	TFile(const char* Filename) :
		mFilename	( Filename )
	{
	}

	virtual bool	IsValid()							{	return true;	}
	virtual bool	Write(const ofxXmlSettings& xml);
	virtual bool	HasChanged()						{	return true;	}
	virtual bool	Read(ofxXmlSettings& xml);

protected:
	virtual BufferString<MAX_PATH>	GetFilename()		{	return mFilename;	}

protected:
	BufferString<MAX_PATH>	mFilename;
};


