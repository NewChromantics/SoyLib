#include "SoyHttp.h"
#include <regex>
#include "SoyStream.h"
#include "SoyMedia.h"



std::string Http::GetDefaultResponseString(size_t ResponseCode)
{
	//	todo: find the HTTP spec defaults
	switch ( ResponseCode )
	{
		case Response_OK:			return "OK";
		case Response_FileNotFound:	return "File Not Found";
		default:
			break;
	}
	
	std::stringstream ss;
	ss << ResponseCode;
	return ss.str();
}


void Http::TCommonProtocol::SetContent(const std::string& Content,SoyMediaFormat::Type Format)
{
	Soy::Assert( mContent.IsEmpty(), "Content already set" );
	Soy::Assert( mWriteContent==nullptr, "Content has a write-content function set");
	
	mContentMimeType = SoyMediaFormat::ToMime( Format );
	Soy::StringToArray( Content, GetArrayBridge( mContent ) );
	mContentLength = mContent.GetDataSize();
}


void Http::TCommonProtocol::SetContent(const ArrayBridge<char>& Data,SoyMediaFormat::Type Format)
{
	Soy::Assert( mContent.IsEmpty(), "Content already set" );
	Soy::Assert( mWriteContent==nullptr, "Content has a write-content function set");
	
	mContent.Copy( Data );
	mContentMimeType = SoyMediaFormat::ToMime( Format );
	mContentLength = mContent.GetDataSize();
}


void Http::TCommonProtocol::SetContent(const ArrayBridge<char>& Data,const std::string& MimeFormat)
{
	if ( MimeFormat.empty() )
	{
		SetContent( Data, SoyMediaFormat::Text );
		return;
	}
	Soy::Assert( mContent.IsEmpty(), "Content already set" );
	Soy::Assert( mWriteContent==nullptr, "Content has a write-content function set");
	
	mContent.Copy( Data );
	mContentMimeType = MimeFormat;
	mContentLength = mContent.GetDataSize();
}


void Http::TCommonProtocol::SetContentType(SoyMediaFormat::Type Format)
{
	mContentMimeType = SoyMediaFormat::ToMime( Format );
}


void Http::TCommonProtocol::BakeHeaders()
{
	//	else?
	if ( mKeepAlive )
		mHeaders["Connection"] = "keep-alive";

	
	if ( mChunkedContent )
	{
		Soy::Assert( mContent.IsEmpty(), "Chunked Content, but also has content");
		Soy::Assert( mWriteContent==nullptr, "Chunked Content, but also has write content func");
		mHeaders["Transfer-Encoding"] = "chunked";

		//	http://stackoverflow.com/a/26009085/355753
		mHeaders["Cache-Control"] = "no-cache";
	}
	else if ( mWriteContent )	//	remove this ambiguity!
	{
		Soy::Assert( mContent.IsEmpty(), "WriteContent callback, but also has content");
		//Soy::Assert( mContentLength==0, "WriteContent callback, but also has content length");

		if ( mContentLength != 0 )
			mHeaders["Content-length"] = Soy::StreamToString( std::stringstream()<<mContentLength );
	}
	else if ( !mContent.IsEmpty() )
	{
		Soy::Assert( mContent.GetDataSize() == mContentLength, "Content length doesn't match length of content");
		mHeaders["Content-length"] = Soy::StreamToString( std::stringstream()<<mContent.GetDataSize() );
	}
	
	if ( !mContentMimeType.empty() )
		mHeaders["Content-Type"] = mContentMimeType;
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
	if ( mChunkedContent )
	{
		//	keep pushing data until eof
		auto& Content = *mChunkedContent;
		while ( true )
		{
			//	done!
			if ( Content.HasEndOfStream() && Content.GetBufferedSize() == 0 )
				break;
			
			try
			{
				Http::TChunkedProtocol Chunk( *mChunkedContent );
				Chunk.Encode( Buffer );
			}
			catch(std::exception& e)
			{
				std::stringstream Error;
				Error << "Exception whilst chunking HTTP content; " << e.what();
				std::Debug << Error.str() << std::endl;
				static bool PushExceptionData = true;
				if ( PushExceptionData )
					Buffer.Push( Error.str() );
				break;
			}
		}
	}
	else if ( mWriteContent )
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


Http::TResponseProtocol::TResponseProtocol(size_t ResponseCode) :
	TCommonProtocol		( )
{
	mResponseCode = ResponseCode;
}

Http::TResponseProtocol::TResponseProtocol(TStreamBuffer& ChunkedDataBuffer) :
	TCommonProtocol		( ChunkedDataBuffer )
{
}


Http::TResponseProtocol::TResponseProtocol(std::function<void(TStreamBuffer&)> WriteContentCallback,size_t ContentLength) :
	TCommonProtocol		( WriteContentCallback, ContentLength )
{
}


void Http::TResponseProtocol::Encode(TStreamBuffer& Buffer)
{
	size_t ResultCode = Response_OK;
	std::string ResultString;

	//	specific response
	if ( mResponseCode != 0 )
	{
		ResultCode = mResponseCode;
	}
	if ( !mUrl.empty() )
	{
		ResultString = mUrl;
	}
	
	if ( ResultString.empty() )
		ResultString = GetDefaultResponseString( ResultCode );

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


TProtocolState::Type Http::TCommonProtocol::Decode(TStreamBuffer& Buffer)
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

void Http::TCommonProtocol::PushHeader(const std::string& Header)
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
		std::regex ResponsePattern("^HTTP/[12].[01] ([0-9]+) (.*)$", std::regex::icase );
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
		std::regex RequestPattern("^(GET|POST) /(.*) HTTP/([12]).([01])$", std::regex::icase );
		std::smatch Match;
		if ( std::regex_match( Header, Match, RequestPattern ) )
		{
			Soy::Assert( !HasResponseHeader(), "Already matched response header" );
			Soy::Assert( !HasRequestHeader(), "Already matched request header" );
			
			mMethod = Match[1].str();
			mUrl = std::string(Url_Root) + Match[2].str();
			
			std::stringstream VersionString;
			VersionString << Match[3].str() << '.' << Match[4].str();
			mRequestProtocolVersion = Soy::TVersion( VersionString.str() );
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

bool Http::TCommonProtocol::ParseSpecificHeader(const std::string& Key,const std::string& Value)
{
	if ( Soy::StringMatches(Key,"Content-length", false ) )
	{
		Soy::StringToType( mContentLength, Value );
		return true;
	}
	
	if ( Soy::StringMatches(Key,"Content-Type", false ) )
	{
		mContentMimeType = Value;
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
	//	set a default if method not specified
	//	gr: this was set in the constructor, but now for decoding we want to initialise it blank
	if ( mMethod.empty() )
		mMethod = "GET";
	
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


Http::TChunkedProtocol::TChunkedProtocol(TStreamBuffer& Input,size_t MinChunkSize,size_t MaxChunkSize) :
	mInput			( Input ),
	mMinChunkSize	( MinChunkSize ),
	mMaxChunkSize	( MaxChunkSize )
{
	
}

void Http::TChunkedProtocol::Encode(TStreamBuffer& Output)
{
	//	add some conditional here to stop thread spinning
	//	also need something to break out when we want to stop the thread
	while ( true )
	{
		//	spin
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );
		
		//	wait for min data
		if ( !mInput.HasEndOfStream() && mInput.GetBufferedSize() < mMinChunkSize )
		{
			std::Debug << "Http chunked data waiting for data: " << mInput.GetBufferedSize() << "<" << mMinChunkSize << std::endl;
			continue;
		}
		
		//	eat chunk
		Array<uint8> ChunkData;
		
		//	gr: docs don't seem to mention a max
		auto EatSize = std::min( mMaxChunkSize, mInput.GetBufferedSize() );

		//	eof, so 0 is valid
		if ( EatSize > 0 )
		{
			if ( !mInput.Pop( EatSize, GetArrayBridge(ChunkData) ) )
			{
				std::Debug << "Http chunked data failed to pop " << EatSize << "/" << mInput.GetBufferedSize() << std::endl;
				continue;
			}
		}
		
		//	write to output
		//	size in hex LINEFEED
		//	data LINEFEED
		BufferArray<char,2> LineFeed;
		LineFeed.PushBack('\r');
		LineFeed.PushBack('\n');
		std::stringstream ChunkPrefix;
		ChunkPrefix << std::hex << EatSize;
		Output.Push( ChunkPrefix.str() );
		Output.Push( GetArrayBridge(LineFeed) );
		Output.Push( GetArrayBridge(ChunkData) );
		Output.Push( GetArrayBridge(LineFeed) );
	}
	
}


