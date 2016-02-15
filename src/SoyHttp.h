#pragma once

#include "SoyProtocol.h"
#include "HeapArray.hpp"
#include "SoyMedia.h"


namespace Http
{
	class TResponseProtocol;
	class TRequestProtocol;
	class TCommonProtocol;
}


class Http::TCommonProtocol
{
public:
	TCommonProtocol() :
		mKeepAlive			( false ),
		mContentLength		( 0 ),
		mWriteContent		( nullptr ),
		mHeadersComplete	( false ),
		mResponseCode		( 0 )
	{
	}
	TCommonProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength) :
		mKeepAlive			( false ),
		mContentLength		( ContentLength ),
		mWriteContent		( WriteContentCallback ),
		mHeadersComplete	( false ),
		mResponseCode		( 0 )
	{
	}

	void					SetContent(const std::string& Content,SoyMediaFormat::Type Format=SoyMediaFormat::Text);
	void					SetContent(const ArrayBridge<char>& Data,SoyMediaFormat::Type Format);
	
	//	common code atm
	TProtocolState::Type	Decode(TStreamBuffer& Buffer);

protected:
	//	encoding
	void			BakeHeaders();		//	inject headers
	void			WriteHeaders(TStreamBuffer& Buffer) const;
	void			WriteContent(TStreamBuffer& Buffer);


	//	decoding
	void							PushHeader(const std::string& Header);
	virtual bool					ParseSpecificHeader(const std::string& Key,const std::string& Value);	//	check for specific headers
	bool							HasResponseHeader() const	{	return mResponseCode != 0;	}
	bool							HasRequestHeader() const	{	return !mMethod.empty();	}

public:
	std::map<std::string,std::string>	mHeaders;
	std::string							mUrl;				//	could be "Bad Request" or "OK" for responses
	std::function<void(TStreamBuffer&)>	mWriteContent;		//	if this is present, we use a callback to write the content and don't know ahead of time the content length
	Array<char>							mContent;
	std::string							mContentMimeType;	//	change this to SoyMediaFormat
	size_t								mContentLength;		//	need this for when reading headers... maybe ditch if possible
	bool								mKeepAlive;

private:	//	decoding
	bool								mHeadersComplete;
	Soy::TVersion						mRequestProtocolVersion;
public:
	size_t								mResponseCode;
	std::string							mMethod;
};




class Http::TResponseProtocol : public Http::TCommonProtocol, public Soy::TReadProtocol, public Soy::TWriteProtocol
{
public:
	TResponseProtocol();
	TResponseProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength);
	
	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override	{	return TCommonProtocol::Decode( Buffer );	}
	
	
public:
};


class Http::TRequestProtocol : public Http::TCommonProtocol, public Soy::TReadProtocol, public Soy::TWriteProtocol
{
public:
	TRequestProtocol()
	{
	}
	TRequestProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength) :
		TCommonProtocol	( WriteContentCallback, ContentLength )
	{
	}
	
	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override	{	return TCommonProtocol::Decode( Buffer );	}
	
public:
	std::string						mHost;		//	if empty forces us to http1.0
};




