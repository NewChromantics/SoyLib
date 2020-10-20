#pragma once

#include "SoyProtocol.h"
#include "SoyHttp.h"


namespace WebSocket
{
	class TRequestProtocol;
	class THandshakeMeta;
	class THandshakeResponseProtocol;
	class TMessageHeader;
	class TMessageBuffer;
	class TMessageProtocol;
	
	namespace TOpCode
	{
		enum Type
		{
			Invalid					= -1,
			ContinuationFrame		= 0,
			TextFrame				= 1,
			BinaryFrame				= 2,
			ConnectionCloseFrame	= 8,
			PingFrame				= 9,
			PongFrame				= 10,
		};
		DECLARE_SOYENUM( WebSocket::TOpCode );
	}

}



//	gr: name is a little misleading, it's the websocket connection meta
class WebSocket::THandshakeMeta
{
public:
	std::string			GetReplyKey() const;
	//	gr: version is optional?
	//bool				IsCompleted() const	{	return mIsWebSocketUpgrade && mWebSocketKey.length()!=0 && mVersion.length()!=0;	}
	bool				IsCompleted() const	{	return mIsWebSocketUpgrade && mWebSocketKey.length()!=0;	}
	
public:
	//	protocol and version are optional
	std::string			mProtocol;
	std::string			mVersion;
	bool				mIsWebSocketUpgrade = false;	//	true once we get the upgrade reply
	std::string			mWebSocketKey;
	
	bool				mHasSentAcceptReply = false;
};


class WebSocket::TMessageBuffer
{
public:
	TMessageBuffer() :
		mIsComplete	(false )
	{
	}
	
	void				PushMessageData(TOpCode::Type PayloadFormat,bool IsLastPayload,const ArrayBridge<char>&& Payload);
	void				PushTextMessageData(const ArrayBridge<char>& Payload,bool IsLastPayload);
	void				PushBinaryMessageData(const ArrayBridge<char>& Payload,bool IsLastPayload);
	
	bool				IsCompleteTextMessage() const	{	return mIsComplete && mTextData.length() > 0;	}
	bool				IsCompleteBinaryMessage() const	{	return mIsComplete && mBinaryData.GetSize() > 0;	}
	
	
public:
	bool				mIsComplete;
	Array<uint8_t>		mBinaryData;	//	binary message
	std::string			mTextData;		//	text message
};

class WebSocket::TMessageHeader
{
public:
	TMessageHeader() :
		Length		( 0 ),
		Length16	( 0 ),
		LenMostSignificant	( 0 ),
		Length64	( 0 ),
		Fin			( 1 ),
		Reserved	( 0 ),
		OpCode		( TOpCode::Invalid ),
		Masked		( false )
	{
	}
	explicit TMessageHeader(TOpCode::Type Opcode) :
		Length		( 0 ),
		Length16	( 0 ),
		LenMostSignificant	( 0 ),
		Length64	( 0 ),
		Fin			( 1 ),
		Reserved	( 0 ),
		OpCode		( Opcode ),
		Masked		( false )
	{
	}
	
public:
	TOpCode::Type	GetOpCode() const	{	return TOpCode::Validate( OpCode );	}
	bool			IsText() const			{	return OpCode == TOpCode::TextFrame;	}
	size_t			GetLength() const;
	std::string		GetMaskKeyString() const;
	bool			IsLastMessage() const	{	return Fin==1;	}
	void			IsValid(bool ExpectedNonZeroLength) const;					//	throws if not valid
	bool			Decode(TStreamBuffer& Data);		//	returns false if not got enough data. throws on error
	void			Encode(TStreamBuffer& Buffer,ArrayBridge<uint8_t>&& PayloadData);

public:
	BufferArray<unsigned char,4> MaskKey;	//	store & 32 bit int

private:
	int		Fin;
	int		Reserved;
	int		OpCode;
	int		Masked;
	int		Length;
	int		Length16;
	int		LenMostSignificant;
	uint64	Length64;
};


//	a websocket client connecting to us (via http)
//	we should THEN switch to websocket::TMessageProtocol as all messages recieved will be that afterwards
class WebSocket::TRequestProtocol : public Http::TRequestProtocol
{
public:
	TRequestProtocol() : mHandshake(* new THandshakeMeta() ) 	{	throw Soy::AssertException("Should not be called");	}
	TRequestProtocol(THandshakeMeta& Handshake,std::shared_ptr<TMessageBuffer> Message,const std::string& Host) :
		mHandshake		( Handshake ),
		mMessage		( Message ),
		mRequestHost	( Host )
	{
	}

	virtual void					Encode(TStreamBuffer& Buffer) override;
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override;
	virtual bool					ParseSpecificHeader(const std::string& Key,const std::string& Value) override;
	
protected:
	static TProtocolState::Type	DecodeBody(TMessageHeader& Header,TMessageBuffer& Message,TStreamBuffer& Buffer);

public:
	//	if we've generated a "reply with this message" its the HTTP-reply to allow websocket (and not an actual packet)
	//	if this is present, send it to the socket :)
	std::shared_ptr<Soy::TWriteProtocol>	mReplyMessage;
	
	THandshakeMeta&		mHandshake;	//	persistent handshake data etc
	std::shared_ptr<TMessageBuffer>		mMessage;	//	persistent message for multi-frame messages
	std::string			mRequestHost;
};


class WebSocket::THandshakeResponseProtocol : public Http::TResponseProtocol
{
public:
	THandshakeResponseProtocol(const THandshakeMeta& Handshake);
};


//	a single message (which is decoded and encoded)
class WebSocket::TMessageProtocol : public Soy::TWriteProtocol
{
public:
	TMessageProtocol(THandshakeMeta& Handshake,const std::string& Message) :
		mHandshake		( Handshake ),
		mTextMessage	( Message ),
		mIsTextMessage	( true )
	{
	}
	TMessageProtocol(THandshakeMeta& Handshake,const ArrayBridge<uint8_t>& Message) :
		mHandshake		(Handshake),
		mBinaryMessage	(Message),
		mIsTextMessage	( false )
	{
	}

protected:
	virtual void		Encode(TStreamBuffer& Buffer) override;
	
public:
	THandshakeMeta&		mHandshake;

	//	todo: maybe need to allow zero binary message... and zero length string.
	bool				mIsTextMessage = false;
	std::string			mTextMessage;
	Array<uint8_t>		mBinaryMessage;
};


