#pragma once

#include "SoyProtocol.h"
#include "HeapArray.hpp"

namespace Http
{
	class TResponseProtocol;
	class TRequestProtocol;
	class TCommonProtocol;
}


class Http::TCommonProtocol
{
public:
	TCommonProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback=nullptr) :
		mKeepAlive		( false ),
		mContentLength	( 0 ),
		mWriteContent	( WriteContentCallback )
	{
	}
	
	void			BakeHeaders();		//	inject headers
	void			WriteHeaders(TStreamBuffer& Buffer) const;
	void			WriteContent(TStreamBuffer& Buffer);

public:
	std::map<std::string,std::string>	mHeaders;
	std::string							mUrl;			//	could be "Bad Request" or "OK" for responses
	std::function<void(TStreamBuffer&)>	mWriteContent;		//	if this is present, we use a callback to write the content and don't know ahead of time the content length
	Array<char>							mContent;
	size_t								mContentLength;		//	need this for when reading headers... maybe ditch if possible
	bool								mKeepAlive;
};


class Http::TResponseProtocol : public Http::TCommonProtocol, public Soy::TReadProtocol, public Soy::TWriteProtocol
{
public:
	TResponseProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback=nullptr);
	
	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override;
	
	void							PushHeader(const std::string& Header);
	virtual bool					ParseSpecificHeader(const std::string& Key,const std::string& Value);	//	check for specific headers
	bool							HasResponseHeader() const	{	return mResponseCode != 0;	}
	bool							HasRequestHeader() const	{	return !mRequestMethod.empty();	}
	
public:
	bool							mHeadersComplete;
	size_t							mResponseCode;

	//	reading requests
	std::string						mRequestMethod;
	std::string						mRequestProtocolVersion;
};


class Http::TRequestProtocol : public Http::TCommonProtocol, public Soy::TWriteProtocol
{
public:
	TRequestProtocol() :
		mMethod		( "GET" )
	{
	}
	
	virtual void					Encode(TStreamBuffer& Buffer) override;
	
public:
	std::string						mMethod;
	std::string						mHost;		//	if empty forces us to http1.0
};




