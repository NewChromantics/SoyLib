#include "SoyWebSocket.h"
#include <regex>
#include "SoyDebug.h"
#include <string>
#include "SoyString.h"
#include "smallsha1/sha1.h"
#include "SoyApp.h"
#include "SoyEnum.h"
#include "SoyBase64.h"
#include "SoyStream.h"



template<typename TYPE>
class THex
{
public:
	TYPE	mValue;
	
	void	GetString(std::stringstream& String) const;
};





std::string WebSocket::THandshakeMeta::GetReplyKey() const
{
	//	http://en.wikipedia.org/wiki/WebSocket
	//	The client sends a Sec-WebSocket-Key which is a random value that has been base64 encoded.
	//	To form a response, the magic string 258EAFA5-E914-47DA-95CA-C5AB0DC85B11 is appended to this (undecoded) key.
	//	The resulting string is then hashed with SHA-1, then base64 encoded.
	//	Finally, the resulting reply occurs in the header Sec-WebSocket-Accept.
	
	//	gr: RFC says this key IS NOT decoded...
	//	http://tools.ietf.org/html/rfc6455
	
	
	std::string key = mWebSocketKey;
	//key.append(websocketpp::processor::constants::handshake_guid);
	key.append( "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	//	apply hashing
	BufferArray<char,20> Hash(20);
	sha1::calc( key.c_str(), size_cast<int>(key.length()), reinterpret_cast<uint8*>(Hash.GetArray()) );
	
	//	encode to base 64
	auto HashBridge = GetArrayBridge( Hash );
	Array<char> Encoded;
	auto EncodedBridge = GetArrayBridge( Encoded );
	Base64::Encode(EncodedBridge, HashBridge);
	std::string EncodedKey = Soy::ArrayToString( EncodedBridge );
	return EncodedKey;
}

//	insert handshake headers
void WebSocket::TRequestProtocol::Encode(TStreamBuffer& Buffer)
{
	//	gr: we should support urls but don't atm

	//	need a host or url to switch to http 1.1
	//	gr: test server's url doesn't require slash, but check other implementations (and spec!)
	//mUrl = "ws://echo.websocket.org/?encoding=text";
	mUrl = std::string("ws://");
	mUrl += mRequestHost;
	mUrlPrefix = "";
	mHost = mRequestHost;

	//	init handshake
	//	the client sends a Sec - WebSocket - Key header containing base64 - 
	//	encoded random bytes, and the server replies with a hash of the key 
	//	in the Sec - WebSocket - Accept header.
	//	this was failing against node's ws module
	//	https://github.com/websockets/ws/blob/62f71d1aa67f2803a121539580b3d4695a69c39c/lib/websocket-server.js#L12
	//	spec says its 16 bytes (should be 24 bytes as per regex), some systems were allowing 20 though
	Array<char> RandomBytes;
	for ( auto i=0;	i<16;	i++)
	{
		auto Rand = rand() % 256;
		RandomBytes.PushBack(Rand);
	}
	Array<char> EncodedRandomBytes;
	Base64::Encode( GetArrayBridge(EncodedRandomBytes), GetArrayBridge(RandomBytes) );
	std::string EncodedRandomBytesStr = Soy::ArrayToString( GetArrayBridge(EncodedRandomBytes) );
	mHandshake.mWebSocketKey = EncodedRandomBytesStr;

	//	todo: support this
	//std::string RequestProtocol = "echo";
	std::string RequestProtocol;

	std::string RequestVersion = "13";

	mHeaders["Upgrade"] = "websocket";
	mHeaders["Connection"] = "Upgrade";
	mHeaders["Sec-WebSocket-Key"] = mHandshake.mWebSocketKey;

	//	gr: these are optional apparently...
	if (!RequestProtocol.empty())
		mHeaders["Sec-WebSocket-Protocol"] = RequestProtocol;
	if (!RequestVersion.empty())
		mHeaders["Sec-WebSocket-Version"] = RequestVersion;

	Http::TRequestProtocol::Encode(Buffer);
}


TProtocolState::Type WebSocket::TRequestProtocol::Decode(TStreamBuffer& Buffer)
{
	//	gr: IsCompleted is true when we fill it in for the output
	//	if we haven't handshaken yet, the first request should be a regular HTTP asking for an upgrade
	if ( !mHandshake.IsCompleted() )
	{
		auto HttpResult = TCommonProtocol::Decode( Buffer );
		if ( HttpResult != TProtocolState::Finished )
		{
			//	extra debug
			if ( HttpResult != TProtocolState::Waiting )
				std::Debug << "Websocket handshake failed; HTTP code=" << mResponseCode << " mime=" << mContentMimeType << std::endl;
				
			return HttpResult;
		}

		//	gr: need to distinguish between outgoing handshake and incoming		
		//	make the auto-reply packet that the caller needs to send to complete the handshake
		if ( !mHandshake.IsCompleted() )
		{
			//	gr: would caller ever want the http reply if it's not a valid handshake?
			throw Soy::AssertException("Websocket recieving http request, but was not a valid handshake request");
		}
		
		mReplyMessage.reset( new WebSocket::THandshakeResponseProtocol(mHandshake,nullptr) );
		return HttpResult;
	}

	//	we're doing websocket (not http) now, so we need to decode incoming data
	try
	{
		//	we may have already read the header, but returned waiting for the body
		if ( !mHeaderDecoded )
		{
			if ( !mHeader.Decode( Buffer ) )
			{
				//	failed to decode header, but EOF (socket disconnected), so just quit
				if ( Buffer.mEof )
					return TProtocolState::Disconnect;
				return TProtocolState::Waiting;
			}
			mHeaderDecoded = true;
		}
		
		//	just a close command
		if ( mHeader.GetOpCode() == TOpCode::ConnectionCloseFrame )
			return TProtocolState::Disconnect;
		
		//	decode body
		return DecodeBody( mHeader, *mMessage, Buffer );
	}
	catch(std::exception& e)
	{
		std::Debug << e.what() << std::endl;
		return TProtocolState::Disconnect;
	}
}


bool WebSocket::TRequestProtocol::ParseSpecificHeader(const std::string& Key,const std::string& Value)
{
	//	extract web-socket special headers
	if ( Soy::StringMatches(Key,"Upgrade", false ) )
	{
		if ( Soy::StringMatches(Value,"websocket", false ) )
		{
			mHandshake.mIsWebSocketUpgrade = true;
			return true;
		}
	}
	
	if ( Soy::StringMatches(Key,"Sec-WebSocket-Key", false ) )
	{
		mHandshake.mWebSocketKey = Value;
		return true;
	}
	
	if ( Soy::StringMatches(Key,"Sec-WebSocket-Protocol", false ) )
	{
		mHandshake.mProtocol = Value;
		return true;
	}

	if ( Soy::StringMatches(Key,"Sec-WebSocket-Version", false ) )
	{
		mHandshake.mVersion = Value;
		return true;
	}
	
	return Http::TRequestProtocol::ParseSpecificHeader(Key,Value);
}


bool WebSocket::THandshakeResponseProtocol::ParseSpecificHeader(const std::string& Key, const std::string& Value)
{
	//	extract web-socket special headers
	if (Soy::StringMatches(Key, "Upgrade", false))
	{
		if (Soy::StringMatches(Value, "websocket", false))
		{
			mHandshake.mIsWebSocketUpgrade = true;
			return true;
		}
	}

	if (Soy::StringMatches(Key, "Sec-WebSocket-Key", false))
	{
		mHandshake.mWebSocketKey = Value;
		return true;
	}

	if (Soy::StringMatches(Key, "Sec-WebSocket-Accept", false))
	{
		mHandshake.mWebSocketAcceptedKey = Value;
		return true;
	}

	if (Soy::StringMatches(Key, "Sec-WebSocket-Protocol", false))
	{
		mHandshake.mProtocol = Value;
		return true;
	}

	if (Soy::StringMatches(Key, "Sec-WebSocket-Version", false))
	{
		mHandshake.mVersion = Value;
		return true;
	}

	return Http::TResponseProtocol::ParseSpecificHeader(Key, Value);
}



TProtocolState::Type WebSocket::TRequestProtocol::DecodeBody(TMessageHeader& Header,TMessageBuffer& MessageBuffer,TStreamBuffer& Buffer)
{
	//	gr: to handle mulitple messages, the data all goes into the payload param
	//	if the param is missing, we haven't loaded our payload yet
	//	gr: ^^^ old approach. Now, we probably need to store a payload buffer in the request protocol that's persistent
	
	
	//	work out the size of the data we still need to read
	auto MessageDataSize = Header.GetLength();
	//	^^payloadsize
	//	std::Debug << "Websocket header was " << HeaderData.GetSize() << " bytes" << std::endl;
	//	std::Debug << "Websocket length: " << Header.GetLength() << " bytes" << std::endl;
	//	std::Debug << "MessageDataSize: " << MessageDataSize << " bytes " << std::endl;

	/*
	//	setup the job and the meta we need (todo: serialise TWebSocketMessageHeader into a single param?)
	Job.mParams.mCommand = WebsocketMessageCommand;
	Job.mParams.AddParam("payloadsize", Soy::StreamToString(std::stringstream()<<MessageDataSize) );
	//	set format if it's text, otherwise binary
	if ( Header.IsText() )
		Job.mParams.AddParam("format", TJobFormat( "text" ).GetFormatString() );
	if ( Header.Masked )
		Job.mParams.AddParam(WebsocketMaskKeyParam, Header.GetMaskKeyString() );
	Job.mParams.AddParam(WebSocketFinParam, (Header.Fin==1) );
	Job.mParams.AddParam(WebsocketIsContinuationParam, (Header.OpCode == TWebSockets::TOpCode::ContinuationFrame) );
	
*/
	//	read the data we're expecting
	Array<char> PayloadData;
	if ( !Buffer.Pop( MessageDataSize, GetArrayBridge(PayloadData) ) )
		return TProtocolState::Waiting;
		
	//	unmask the data
	if ( !Header.MaskKey.IsEmpty() )
	{
		auto& MaskKey = Header.MaskKey;
		for ( int i=0;	i<PayloadData.GetSize();	i++ )
		{
			char Encoded = PayloadData[i];
			char Decoded = Encoded ^ MaskKey[i%4];
			PayloadData[i] = Decoded;
		}
	}
		
	//	opcode tells us if it's text/binary, but we only know that if we're the first packet
	//	but we do know when we're the last one.
	auto IsLastPayload = Header.IsLastMessage();
	MessageBuffer.PushMessageData( Header.GetOpCode(), IsLastPayload, GetArrayBridge(PayloadData) );
	return TProtocolState::Finished;
}
		
		
void WebSocket::TMessageBuffer::PushMessageData(TOpCode::Type PayloadFormat,bool IsLastPayload,const ArrayBridge<char>&& Payload)
{
	if ( mIsComplete )
	{
		throw Soy::AssertException("Pushing to already complete message");
	}
	
	//	if the payload format is text/binary then it's new data (additional data is always continuation)
	if ( PayloadFormat == TOpCode::TextFrame || PayloadFormat == TOpCode::BinaryFrame )
	{
		//	if there's existing data, error!
		if ( !mBinaryData.IsEmpty() || mTextData.length() > 0 )
		{
			throw Soy::AssertException("Got new payload, but old one didn't finish");
		}
	}
	
	if ( PayloadFormat == TOpCode::ContinuationFrame )
	{
		if ( !mBinaryData.IsEmpty() )
		{
			PayloadFormat = TOpCode::BinaryFrame;
		}
		else if ( mTextData.length() > 0 )
		{
			PayloadFormat = TOpCode::TextFrame;
		}
		else
		{
			throw Soy::AssertException("Got a contiuation frame, but don't know the [old] payload format");
		}
	}
	
	//	binary data
	if ( PayloadFormat == TOpCode::BinaryFrame )
	{
		PushBinaryMessageData( Payload, IsLastPayload );
	}
	else if ( PayloadFormat == TOpCode::TextFrame )
	{
		PushTextMessageData( Payload, IsLastPayload );
	}
	else
	{
		std::stringstream Error;
		Error << "Unexpected payload type: " << PayloadFormat;
		throw Soy::AssertException(Error.str());
	}
}


void WebSocket::TMessageBuffer::PushBinaryMessageData(const ArrayBridge<char>& Payload,bool IsLastPayload)
{
	mBinaryData.PushBackArray( Payload );
	mIsComplete = IsLastPayload;
}

void WebSocket::TMessageBuffer::PushTextMessageData(const ArrayBridge<char>& Payload,bool IsLastPayload)
{
	auto PayloadString = Soy::ArrayToString( Payload );
	mTextData += PayloadString;
	mIsComplete = IsLastPayload;
}

WebSocket::THandshakeResponseProtocol::THandshakeResponseProtocol(THandshakeMeta& Handshake, std::shared_ptr<TMessageBuffer> Message) :
	mHandshake	( Handshake ),
	mMessage	( Message ) 
{
}

void WebSocket::THandshakeResponseProtocol::Encode(TStreamBuffer& Buffer)
{
	//	setup http response for websocket acceptance
	
	//	add http headers we need to reply with
	if ( !mHandshake.mProtocol.empty() )
		mHeaders.insert( {"Sec-WebSocket-Protocol", mHandshake.mProtocol} );
	
	if ( mHandshake.mIsWebSocketUpgrade )
	{
		mHeaders.insert( {"Sec-WebSocket-Accept", mHandshake.GetReplyKey() } );
		mHeaders.insert( {"Upgrade", "websocket"} );
		mHeaders.insert( {"Connection", "Upgrade"} );
	}
	
	//	gr: need content length or chrome never displays...
	//	gr: ^^ I think this was related to png, let base class deal with that
	//Http.PushHeader( "Content-Length", std::stringstream() << DefaultParam.GetDataSize() );
	//Http.PushHeader( SoyHttpHeaderElement("Content-Type", SoyHttpHeader::FormatToHttpContentType(Reply.mData.mFormat).c_str() ) );
	
	mResponseCode = Http::Response_SwitchingProtocols;

	Http::TResponseProtocol::Encode(Buffer);
}


TProtocolState::Type WebSocket::THandshakeResponseProtocol::Decode(TStreamBuffer& Buffer)
{
	//	decode this handshake reply
	if (!mHandshake.IsCompleted())
	{
		auto HttpResult = TCommonProtocol::Decode(Buffer);
		if (HttpResult != TProtocolState::Finished)
		{
			//	extra debug
			if ( HttpResult != TProtocolState::Waiting )
				std::Debug << "Websocket handshake failed; HTTP code=" << mResponseCode << " Response=" << mResponseString << " mime=" << mContentMimeType << std::endl;
				
			return HttpResult;
		}

		//	gr: need to distinguish between outgoing handshake and incoming		
		//	make the auto-reply packet that the caller needs to send to complete the handshake
		if (!mHandshake.IsCompleted())
		{
			//	gr: would caller ever want the http reply if it's not a valid handshake?
			throw Soy::AssertException("Websocket recieving http request, but was not a valid handshake request");
		}

		//mReplyMessage.reset(new WebSocket::THandshakeResponseProtocol(mHandshake));
		return HttpResult;
	}

	//	we're doing websocket now, so we need to decode incoming data
	TMessageHeader Header;

	try
	{
		if (!Header.Decode(Buffer))
		{
			//	failed to decode header, but EOF (socket disconnected), so just quit
			if (Buffer.mEof)
				return TProtocolState::Disconnect;
			return TProtocolState::Waiting;
		}

		//	just a close command
		if (Header.GetOpCode() == TOpCode::ConnectionCloseFrame)
			return TProtocolState::Disconnect;

		//	decode body
		return WebSocket::TRequestProtocol::DecodeBody(Header, *mMessage, Buffer);
	}
	catch (std::exception& e)
	{
		std::Debug << e.what() << std::endl;
		return TProtocolState::Disconnect;
	}
}

size_t WebSocket::TMessageHeader::GetLength() const
{
	if ( Length64 != 0 )
	{
		if ( Length16 != 0 || Length != 127 )
			return -1;
		
		//	currently limited cos we're using 32bit lengths in arrays and stuff
		if ( Length64 > std::numeric_limits<int>::max() )
			return -1;
		return Length64;
	}
	
	if ( Length16 != 0 )
	{
		if ( Length64 != 0 || Length != 126 )
			return -1;
		return Length16;
	}

	if ( Length >= 127 )
	{
		std::stringstream Error;
		Error << "Length (" << this->Length << ") expected < 127";
		throw Soy::AssertException( Error.str() );
	}
	
	{
		if ( Length64 != 0 || Length16 != 0 )
		{
			std::stringstream Error;
			Error << "Expected length 16/64 to be non-zero";
			throw Soy::AssertException( Error.str() );
		}
	}
	
	//	length must be 0?
	if ( Length < 0 )
	{
		std::stringstream Error;
		Error << "Unexpected length: " << Length;
		throw Soy::AssertException( Error.str() );
	}

	return Length;
}



template<>
void THex<uint32>::GetString(std::stringstream& String) const
{
	int TypeNibbles = 2*sizeof(mValue);
	for ( int i=0;	i<TypeNibbles;	i++ )
	{
		char hex = mValue >> ((TypeNibbles-1-i)*4);
		hex &= (1<<4)-1;
		if ( hex >= 10 )
			hex += 'A' - 10;
		else
			hex += '0';
		String << hex;
	}
}


std::string WebSocket::TMessageHeader::GetMaskKeyString() const
{
	if ( MaskKey.IsEmpty() )
		return std::string();
	
	//	represent as int32 (hex might be nicer)
	THex<uint32> Mask32;
	Mask32.mValue = *reinterpret_cast<const uint32*>( MaskKey.GetArray() );

	std::stringstream s;
	Mask32.GetString(s);
	return s.str();
}

void WebSocket::TMessageHeader::IsValid(bool ExpectedNonZeroLength) const
{
	std::stringstream Error;

	if ( Reserved != 0 )
	{
		Error << "Reserved(" << Reserved << ")!=0";
		throw Soy::AssertException(Error.str());
	}
	
	//	most significant bit should always be 0 (ws can't handle full-64bit length)
	if ( LenMostSignificant != 0 )
	{
		Error << "LenMostSignificant(" << LenMostSignificant << ")!=0";
		throw Soy::AssertException(Error.str());
	}

	if ( MaskKey.GetSize() != 0 && MaskKey.GetSize() != 4 )
	{
		Error << "MaskKey size(" << MaskKey.GetSize() << ") invalid";
		throw Soy::AssertException(Error.str());
	}
	
	//	this checks the lengths for us
	//	gr: maybe 0 is okay?
	auto Length = GetLength();
	if ( Length == 0 )
	{
		if ( ExpectedNonZeroLength )
		{
			Error << "Length of " << OpCode << " message is 0";
			throw Soy::AssertException(Error.str());
		}
	}
	
	//	we only support some opcodes atm
	switch ( OpCode )
	{
		case TOpCode::TextFrame:
		case TOpCode::BinaryFrame:
		case TOpCode::ConnectionCloseFrame:
		case TOpCode::ContinuationFrame:
		case TOpCode::PingFrame:
		case TOpCode::PongFrame:		
			break;
			
		default:
			Error << "Unsupported opcode " << OpCode;
			throw Soy::AssertException(Error.str());
	};
}


//	return false if not enough data, throw on error
bool WebSocket::TMessageHeader::Decode(TStreamBuffer& Buffer)
{
	auto PeekByte = [&](size_t Index)
	{
		uint8_t Value = 0;
		if ( !Buffer.Peek(Value,Index) )
			throw std::out_of_range("Not enough data in streambuffer");
		return Value;
	};
	TBitReader_Lambda BitReader( PeekByte );

	//	return false & no data == error
	//	return false & MoreData == need more data

	//	if any of these fail, we need more data
	int MoreDataRequired = 1;
	if ( !BitReader.Read( Fin, 1 ) )
		return false;
	if ( !BitReader.Read( Reserved, 3 ) )
		return false;
	if ( !BitReader.Read( OpCode, 4 ) )
		return false;
	
	MoreDataRequired = 1;
	if ( !BitReader.Read( Masked, 1 ) )
		return false;
	
	Length16 = 0;
	Length64 = 0;
	if ( !BitReader.Read( Length, 7 ) )
		return false;
	
	if ( Length == 126 )	//	length is 16bit
	{
		MoreDataRequired = 16/8;	//	gr: not quite right = 16 bits - bytes unread
		if ( !BitReader.Read( Length16, 16 ) )
			return false;
	}
	else if ( Length == 127 )	//	length is 64 bit
	{
		MoreDataRequired = 64/8;	//	gr: not quite right = 16 bits - bytes unread
		if ( !BitReader.Read( Length64, 64 ) )
			return false;
	}
	

	
	MaskKey.Clear();
	if ( Masked )
	{
		int m0,m1,m2,m3;
		MoreDataRequired = 1;
		if ( !BitReader.Read( m0, 8 ) )		return false;
		if ( !BitReader.Read( m1, 8 ) )		return false;
		if ( !BitReader.Read( m2, 8 ) )		return false;
		if ( !BitReader.Read( m3, 8 ) )		return false;
		MaskKey.PushBack( m0 );
		MaskKey.PushBack( m1 );
		MaskKey.PushBack( m2 );
		MaskKey.PushBack( m3 );
	}
	
	//	check theres enough data left...
	auto BitPos = BitReader.BitPosition();
	//int BytesRead = BitPos / 8;
	//	gr: expecting this to align, no mentiuon in the RFC but pretty sure it does (and makes sense
	MoreDataRequired = 0;
	if ( BitPos % 8 != 0 )
	{
		throw Soy::AssertException("Bits read in websocket header not aligned to 8");
	}
	
	//	check integrity
	IsValid(true);
	
	//	pop out all the data we read
	Buffer.Pop( BitReader.BytesRead() );
	
	return true;
}



void WebSocket::TMessageHeader::Encode(TStreamBuffer& Buffer,ArrayBridge<uint8_t>&& PayloadData)
{
	//	should be valid here. with zero length
	IsValid(false);
	if ( GetLength() != 0 )
	{
		std::stringstream Error;
		Error << "expected length(" << GetLength() << ") to be zero";
		throw Soy::AssertException(Error.str());
	}
	
	//	write header to it's own array first
	BufferArray<char,14> HeaderData;
	auto HeaderDataBridge = GetArrayBridge( HeaderData );
	
	//	write message header
	TBitWriter BitWriter( HeaderDataBridge );
	BitWriter.WriteBit( Fin );		//	fin
	BitWriter.Write( (uint8)Reserved, 3 );	//	reserved
	
	BitWriter.Write( (uint8)OpCode, 4 );
	BitWriter.WriteBit(Masked);	//	masked
	
	uint64 PayloadLength = PayloadData.GetDataSize();
	//	write length
	if ( PayloadLength > 0xffff )
	{
		//	64 bit length
		BitWriter.Write( 127u, 7 );	//	"theres another 64 bit length"
		BitWriter.Write( PayloadLength, 64 );
	}
	else if ( PayloadLength > 125 )
	{
		BitWriter.Write( 126u, 7 );	//	"theres another 16 bit length"
		BitWriter.Write( PayloadLength, 16 );
	}
	else
	{
		BitWriter.Write( PayloadLength, 7 );
	}
	
	unsigned char _MaskKey[4] = {0,0,0,0};
	if ( Masked )
	{
		BufferArray<unsigned char,4> MaskKey(_MaskKey);
		BitWriter.Write( MaskKey[0], 8 );
		BitWriter.Write( MaskKey[1], 8 );
		BitWriter.Write( MaskKey[2], 8 );
		BitWriter.Write( MaskKey[3], 8 );
	}
	
	//	data left over should align!
	int Remainder = BitWriter.BitPosition() % 8;
	if ( Remainder != 0 )
	{
		std::stringstream Error;
		Error << "Websocket header data doesn't align to 8 bits (" << Remainder << ")";
		throw Soy::AssertException(Error.str());
	}

	Buffer.Push( GetArrayBridge(HeaderData) );
	
	//	mask all the data
	//	gr: quick hack, zero mask xor's to same value, so we can skip the masking
	auto MaskedZero = !_MaskKey[0] && !_MaskKey[1] && !_MaskKey[2] && !_MaskKey[3];
	if ( Masked && !MaskedZero )
	{
		Array<uint8_t> MaskedPayload;
		MaskedPayload.Copy(PayloadData);
		for ( int i=0;	i<MaskedPayload.GetSize();	i++ )
		{
			char Encoded = PayloadData[i];
			char Decoded = Encoded ^ MaskKey[i%4];
			PayloadData[i] = Decoded;
		}
	}
	else
	{
		Buffer.Push( PayloadData );
	}
}


void WebSocket::TMessageProtocol::Encode(TStreamBuffer& Buffer)
{
	TMessageHeader Header(mIsTextMessage ? TOpCode::TextFrame : TOpCode::BinaryFrame, mFromServer );

	if (mIsTextMessage)
	{
		Array<uint8_t> MessageData;
		Soy::StringToArray(mTextMessage, GetArrayBridge(MessageData));
		Header.Encode(Buffer, GetArrayBridge(MessageData));
	}
	else
	{
		Header.Encode(Buffer, GetArrayBridge(mBinaryMessage));
	}
}
	


