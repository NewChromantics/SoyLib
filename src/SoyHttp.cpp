#include "SoyHttp.h"
#include <regex>
#include "SoyStream.h"



void Http::TCommonProtocol::BakeHeaders()
{
	//	else?
	if ( mKeepAlive )
		mHeaders["Connection"] = "keep-alive";

	//	remove this ambiguity!
	Soy::Assert( mContent.GetDataSize() == mContentLength, "Content length doesn't match length of content");
	if ( mWriteContent )
	{
		Soy::Assert( mContent.IsEmpty(), "WriteContent callback, but also has content");
		Soy::Assert( mContentLength==0, "WriteContent callback, but also has content length");
	}
	else if ( !mContent.IsEmpty() )
	{
		mHeaders["Content-length"] = Soy::StreamToString( std::stringstream()<<mContent.GetDataSize() );
	}
}

void Http::TCommonProtocol::WriteHeaders(TStreamBuffer& Buffer) const
{	
	//	write headers
	for ( auto h=mHeaders.begin();	h!=mHeaders.end();	h++ )
	{
		auto& Key = h->first;
		auto& Value = h->second;

		std::stringstream Header;
		Header << Key << ": " << Value << "\r\n";
		Buffer.Push( Header.str() );
	}
	
	//	write header terminator
	Buffer.Push("\r\n");
}

void Http::TCommonProtocol::WriteContent(TStreamBuffer& Buffer)
{
	//	write data
	if ( mWriteContent )
	{
		//	this could block for infinite streaming, but should be writing to buffer so data still gets out! woooo!
		mWriteContent( Buffer );
	}
	else
	{
		Buffer.Push( GetArrayBridge(mContent) );
	}

	
	/*
	//	note: this is DATA because we MUST NOT have a trailing terminator. Fine for HTTP, not for websocket/binary data!
	Soy::StringToArray( Http.str(), GetArrayBridge(RequestData) );
	if ( !Soy::Assert( RequestData.GetBack() != '\0', "Unwanted terminator in HTTP response" ) )
		RequestData.PopBack();
	*/
}


Http::TResponseProtocol::TResponseProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback) :
	TCommonProtocol		( WriteContentCallback ),
	mResponseCode		( 0 ),
	mHeadersComplete	( false )
{
}


void Http::TResponseProtocol::Encode(TStreamBuffer& Buffer)
{
	size_t ResultCode = 200;
	std::string ResultString = "OK";

	//	write request header
	{
		std::string HttpVersion;
		/*
		if ( mHost.empty() )
			HttpVersion = "HTTP/1.0";
		else
		*/
		HttpVersion = "HTTP/1.1";
				
		std::stringstream RequestHeader;
		RequestHeader << HttpVersion << " " << ResultCode << " " <<  ResultString << "\r\n";
		Buffer.Push( RequestHeader.str() );
	}

	BakeHeaders();
	WriteHeaders( Buffer );
	WriteContent( Buffer );
}


TProtocolState::Type Http::TResponseProtocol::Decode(TStreamBuffer& Buffer)
{
	//	read next header
	while ( !mHeadersComplete )
	{
		std::string Header;
		if ( !Buffer.Pop("\r\n", Header, false ) )
			return TProtocolState::Waiting;
		
		//	parse header
		PushHeader( Header );
	}
	
	//	read data
	if ( mContentLength == 0 )
		return TProtocolState::Finished;
	
	if ( !Buffer.Pop( mContentLength, GetArrayBridge(mContent) ) )
		return TProtocolState::Waiting;
	
	if ( !mKeepAlive )
		return TProtocolState::Disconnect;
	
	return TProtocolState::Finished;
}

void Http::TResponseProtocol::PushHeader(const std::string& Header)
{
	auto& mResponseUrl = mUrl;
	
	if ( Header.empty() )
	{
		mHeadersComplete = true;
		Soy::Assert( HasResponseHeader() || HasRequestHeader(), "Finished http headers but never got response/request header" );
		return;
	}
	
	//	check for HTTP response header
	{
		std::regex ResponsePattern("^HTTP/1.[01] ([0-9]+) (.*)$", std::regex::icase );
		std::smatch Match;
		if ( std::regex_match( Header, Match, ResponsePattern ) )
		{
			Soy::Assert( !HasResponseHeader(), "Already matched response header" );
			Soy::Assert( !HasRequestHeader(), "Already matched request header" );
			int ResponseCode;
			Soy::StringToType( ResponseCode, Match[1].str() );
			mResponseCode = size_cast<size_t>(ResponseCode);
			mResponseUrl = Match[2].str();
			return;
		}
	}

	//	check for HTTP request header
	{
		std::regex RequestPattern("^(GET|POST) /(.*) HTTP/1.([0-9])$", std::regex::icase );
		std::smatch Match;
		if ( std::regex_match( Header, Match, RequestPattern ) )
		{
			Soy::Assert( !HasResponseHeader(), "Already matched response header" );
			Soy::Assert( !HasRequestHeader(), "Already matched request header" );
			
			mRequestMethod = Match[1].str();
			mUrl = std::string("/") + Match[2].str();
			mRequestProtocolVersion = Match[3].str();
			return;
		}
	}

	Soy::Assert( HasResponseHeader() || HasRequestHeader(), "Parsing http headers but never got response header" );

	//	split
	BufferArray<std::string,2> Parts;
	Soy::StringSplitByMatches( GetArrayBridge(Parts), Header, ":" );
	
	//	odd case
	while ( Parts.GetSize() < 2 )
		Parts.PushBack();
	
	auto& Key = Parts[0];
	auto& Value = Parts[1];
	Soy::StringTrimLeft(Key,' ');
	Soy::StringTrimLeft(Value,' ');
	
	//	parse specific headers
	if ( ParseSpecificHeader( Key, Value ) )
		return;
	
	//	not expecting this... but dont throw
	auto Duplicate = mHeaders.find(Key);
	if ( Duplicate != mHeaders.end() )
	{
		std::Debug << "Duplicate HTTP header [" << Key << "] found. Prev value=[" << Duplicate->second << "] new value=" << Value << "] skipped." << std::endl;
		return;
	}
	
	mHeaders[Key] = Value;
}

bool Http::TResponseProtocol::ParseSpecificHeader(const std::string& Key,const std::string& Value)
{
	if ( Soy::StringMatches(Key,"Content-length", false ) )
	{
		Soy::StringToType( mContentLength, Value );
		return true;
	}
	
	if ( Soy::StringMatches(Key,"Connection", false ) )
	{
		if ( Value == "close" )
		{
			mKeepAlive = false;
			return true;
		}
	}

	return false;
}


void Http::TRequestProtocol::Encode(TStreamBuffer& Buffer)
{
	Soy::Assert( mMethod=="GET" || mMethod=="POST", "Invalid method for HTTP request" );

	//	write request header
	{
		std::string HttpVersion;
		if ( mHost.empty() )
			HttpVersion = "HTTP/1.0";
		else
			HttpVersion = "HTTP/1.1";
		
		std::stringstream RequestHeader;
		RequestHeader << mMethod << " /" << mUrl << " " << HttpVersion << "\r\n";
		Buffer.Push( RequestHeader.str() );
	}

	//mHeaders["Accept"] = "text/html";
	if ( !mHost.empty() )
		mHeaders["Host"] = mHost;
	//mHeaders["User-Agent"] = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36";
	BakeHeaders();
	WriteHeaders( Buffer );
	WriteContent( Buffer );

}

