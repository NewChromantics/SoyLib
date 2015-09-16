#pragma once

#include "SoyProtocol.h"
#include "HeapArray.hpp"

namespace Http
{
	class TResponseProtocol;
	class TRequestProtocol;
}



class Http::TResponseProtocol : public Soy::TReadProtocol
{
public:
	TResponseProtocol();
	
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override;
	
	void							PushHeader(const std::string& Header);
	virtual bool					ParseSpecificHeader(const std::string& Key,const std::string& Value);	//	check for specific headers
	
public:
	std::map<std::string,std::string>	mHeaders;
	bool								mHeadersComplete;
	size_t								mContentLength;
	size_t								mResponseCode;
	std::string							mUrl;
	Array<char>							mContent;
};


class Http::TRequestProtocol : public Soy::TWriteProtocol
{
public:
	TRequestProtocol() :
		mMethod	( "GET" )
	{
	}
	
	virtual void					Encode(TStreamBuffer& Buffer) override;
	
public:
	std::map<std::string,std::string>	mHeaders;
	std::string							mUrl;
	Array<char>							mContent;
	std::string							mMethod;
};




