#include "SoyHttp.h"
#include <regex>
#include "SoyStream.h"



Http::TResponseProtocol::TResponseProtocol() :
	mContentLength		( 0 ),
	mResponseCode		( 0 ),
	mHeadersComplete	( false ),
	mKeepAlive			( true )
{
	
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
		Soy::Assert( HasResponseHeader(), "Finished http headers but never got response header" );
		return;
	}
	
	//	check for HTTP response header
	std::regex ResponsePattern("^HTTP/1.[01] ([0-9]+) (.*)$", std::regex::icase );
	std::smatch Match;
	if ( std::regex_match( Header, Match, ResponsePattern ) )
	{
		Soy::Assert( !HasResponseHeader(), "Already matched response header" );
		int ResponseCode;
		Soy::StringToType( ResponseCode, Match[1].str() );
		mResponseCode = size_cast<size_t>(ResponseCode);
		mResponseUrl = Match[2].str();
		return;
	}

	Soy::Assert( HasResponseHeader(), "Parsing http headers but never got response header" );

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
	
	if ( mKeepAlive )
		mHeaders["Connection"] = "keep-alive";
	
	if ( !mContent.IsEmpty() )
		mHeaders["Content-length"] = Soy::StreamToString( std::stringstream()<<mContent.GetDataSize() );

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
	
	//	write data
	Buffer.Push( GetArrayBridge(mContent) );
	
	/*
	//	note: this is DATA because we MUST NOT have a trailing terminator. Fine for HTTP, not for websocket/binary data!
	Soy::StringToArray( Http.str(), GetArrayBridge(RequestData) );
	if ( !Soy::Assert( RequestData.GetBack() != '\0', "Unwanted terminator in HTTP response" ) )
		RequestData.PopBack();
	*/
}

