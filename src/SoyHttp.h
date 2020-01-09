#pragma once

#include "SoyProtocol.h"
#include "HeapArray.hpp"
#include "SoyMedia.h"


namespace Http
{
	class TResponseProtocol;
	class TRequestProtocol;
	class TCommonProtocol;
	class TChunkedProtocol;
	
	//	decide how to lay these out and whether to be strict
	const size_t	Response_Invalid = 0;
	const size_t	Response_OK = 200;
	const size_t	Response_FileNotFound = 404;
	const size_t	Response_Forbidden = 403;
	const size_t	Response_SwitchingProtocols = 101;
	const size_t	Response_Error = 500;

	std::string		GetDefaultResponseString(size_t ResponseCode);
}




class Http::TCommonProtocol
{
public:
	TCommonProtocol() :
		mKeepAlive			( false ),
		mContentLength		( 0 ),
		mWriteContent		( nullptr ),
		mHeadersComplete	( false ),
		mResponseCode		( Response_Invalid ),
		mChunkedContent		( nullptr )
	{
	}
	//	gr: this may be response only?
	TCommonProtocol(TStreamBuffer& ChunkedDataBuffer) :
		mKeepAlive			( false ),
		mContentLength		( 0 ),
		mWriteContent		( nullptr ),
		mHeadersComplete	( false ),
		mResponseCode		( Response_Invalid ),
		mChunkedContent		( &ChunkedDataBuffer )
	{
	}
	TCommonProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength) :
		mKeepAlive			( false ),
		mContentLength		( ContentLength ),
		mWriteContent		( WriteContentCallback ),
		mHeadersComplete	( false ),
		mResponseCode		( Response_Invalid ),
		mChunkedContent		( nullptr )
	{
	}

	void					SetContent(const std::string& Content,SoyMediaFormat::Type Format=SoyMediaFormat::Text);
	void					SetContent(const ArrayBridge<char>& Data,SoyMediaFormat::Type Format);
	void					SetContent(const ArrayBridge<char>& Data,const std::string& MimeFormat);
	void					SetContentType(SoyMediaFormat::Type Format);
	
	//	common code atm
	TProtocolState::Type	Decode(TStreamBuffer& Buffer);

	//	we have an extra HasVariable() as variables can be empty
	bool					HasVariable(const std::string& Name) const	{	return mVariables.find(Name) != mVariables.end();	}
	std::string				GetVariable(const std::string& Name) const	{	auto it = mVariables.find(Name);	return (it == mVariables.end()) ? std::string() : it->second;	}
	
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
	std::map<std::string,std::string>	mVariables;			//	GET url vars in requests
	const char*							mUrlPrefix = "/";	//	inserted into GET url, now a variable so it can be removed in specific cases (eg. websocket url is ws://xyz)
	std::string							mUrl;				//	could be "Bad Request" or "OK" for responses
	Array<char>							mContent;
	std::string							mContentMimeType;	//	change this to SoyMediaFormat
	bool								mKeepAlive;

	//	encoding
private:
	TStreamBuffer*						mChunkedContent;
	std::function<void(TStreamBuffer&)>	mWriteContent;		//	if this is present, we use a callback to write the content and don't know ahead of time the content length
	
	//	decoding
private:
	size_t								mContentLength;		//	need this for when reading headers... maybe ditch if possible
	bool								mHeadersComplete;
	Soy::TVersion						mRequestProtocolVersion;
public:
	size_t								mResponseCode;
	std::string							mMethod;
};




class Http::TResponseProtocol : public Http::TCommonProtocol, public Soy::TReadProtocol, public Soy::TWriteProtocol
{
public:
	TResponseProtocol(size_t ResponseCode=Http::Response_OK);
	TResponseProtocol(TStreamBuffer& ChunkedDataBuffer);	
	TResponseProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength);
	
protected:
	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override	{	return TCommonProtocol::Decode( Buffer );	}
	
public:
	std::string&					mResponseString = mUrl;	//	could be "Bad Request" or "OK" for responses
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
	
protected:
	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override	{	return TCommonProtocol::Decode( Buffer );	}
	
public:
	std::string						mHost;		//	if empty forces us to http1.0
};


class Http::TChunkedProtocol : public Soy::TWriteProtocol
{
public:
	TChunkedProtocol(TStreamBuffer& Input,size_t MinChunkSize=100,size_t MaxChunkSize=1024*1024*1);
	
	virtual void		Encode(TStreamBuffer& Output) override;
	
public:
	size_t				mMinChunkSize;
	size_t				mMaxChunkSize;
	TStreamBuffer&		mInput;
};

