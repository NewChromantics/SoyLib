#include "SoyHttp.h"
#include <regex>
#include "SoyStream.h"



Http::TResponseProtocol::TResponseProtocol() :
	mContentLength		( 0 ),
	mResponseCode		( 0 ),
	mHeadersComplete	( false )
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
	
	return TProtocolState::Finished;
}

void Http::TResponseProtocol::PushHeader(const std::string& Header)
{
	auto& mResponseUrl = mUrl;
	
	if ( Header == "\r\n" )
	{
		mHeadersComplete = true;
		Soy::Assert( !mResponseUrl.empty(), "Finished http headers but never got response header" );
		return;
	}
	
	//	check for HTTP response header
	std::regex ResponsePattern("^HTTP/1.1 /(.*)$", std::regex::icase );
	std::smatch Match;
	if ( std::regex_match( Header, Match, ResponsePattern ) )
	{
		mResponseUrl = std::string("/") + Match[1].str();
		return;
	}

	Soy::Assert( !mResponseUrl.empty(), "Parsing http headers but never got response header" );

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

	return false;
}



void Http::TRequestProtocol::Encode(TStreamBuffer& Buffer)
{
	Soy::Assert( mMethod=="GET" || mMethod=="POST", "Invalid method for HTTP request" );

	//	write request header
	{
		std::stringstream RequestHeader;
		RequestHeader << mMethod << " /" << mUrl << " HTTP/1.1\r\n";
		Buffer.Push( RequestHeader.str() );
	}
	
	//	write headers
	for ( auto h=mHeaders.begin();	h!=mHeaders.end();	h++ )
	{
		auto& Key = h->first;
		auto& Value = h->second;

		std::stringstream Header;
		Header << Key << ": " << Value << "\r\n";
		Buffer.Push( Header.str() );
	}
	
	//	write special headers
	{
		std::stringstream ContentLengthHeader;
		ContentLengthHeader << "Content-length: " << mContent.GetDataSize() << "\r\n";
		Buffer.Push( ContentLengthHeader.str() );
	}
	
	//	write header terminator
	Buffer.Push("\r\n\r\n");
	
	//	write data
	Buffer.Push( GetArrayBridge(mContent) );
	
	/*
	//	note: this is DATA because we MUST NOT have a trailing terminator. Fine for HTTP, not for websocket/binary data!
	Soy::StringToArray( Http.str(), GetArrayBridge(RequestData) );
	if ( !Soy::Assert( RequestData.GetBack() != '\0', "Unwanted terminator in HTTP response" ) )
		RequestData.PopBack();
	*/
}

