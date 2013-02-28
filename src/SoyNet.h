#pragma once

#include "SoyRef.h"
#include <ofxNetwork.h>


namespace SoyNet
{
	namespace TPeerState
	{
		enum Type
		{
			Disconnected,
			Connected
		};
	};

	class TManager;
	class TMatchRef;		
	class TMatchMeta;			//	match choices
	class TPeerRef;		
	class TPeer;				//	base
	class TPeerLocalLoop;		//	sends packets to itself
	
	//	gr: these will probably change into internal referencing types once 
	//		the socket management has been abstracted to cover not-just-tcp
	//		(ie. PacketEmulator, GameCenter etc
	class TClientRef;			//	wrapper for ofxTCP clientid 
	class TAddress;				//	unique socket-recieving address identifier
};



//	int for an id is bad. Lets make it typesafe.
class SoyNet::TClientRef
{
public:
	TClientRef() :
		mClientId	( -1 )
	{
	}
	explicit TClientRef(int ClientId) :
		mClientId	( ClientId )
	{
	}

	bool		IsValid() const								{	return mClientId >= 0;	} 

	inline bool	operator==(const TClientRef& That) const	{	return this->mClientId == That.mClientId;	}
	inline bool	operator!=(const TClientRef& That) const	{	return !( *this == That );	}

public:
	int			mClientId;
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyNet::TClientRef& Value)
{
	str << Value.mClientId;
	return str;
}


class SoyNet::TAddress
{
public:
	TAddress() :
		mPort		( 0 )
	{
		assert(!IsValid());
	}
	TAddress(const char* Address,uint16 Port,TClientRef ClientRef=TClientRef()) :
		mPort		( Port ),
		mAddress	( Address ),
		mClientRef	( ClientRef )
	{
		assert(IsValid());
	}

	bool				IsValid() const									{	return (mPort != 0) && !mAddress.IsEmpty();	}
	inline bool			operator==(const TAddress& That) const			{	return (mAddress == That.mAddress) && (this->mPort == That.mPort);	}
	inline bool			operator==(const TClientRef& ClientRef)const	{	return mClientRef == ClientRef;	}

public:
	BufferString<100>	mAddress;
	uint16				mPort;
	TClientRef			mClientRef;	//	clientid of ofxTCPServer's client. -1 if it's not a client (ie. ofxTCPClient's server or self)
};

template<class STRING>
inline STRING& operator<<(STRING& str,const SoyNet::TAddress& Value)
{
	const char* Address = Value.mAddress.IsEmpty() ? "null" : Value.mAddress;
	str << Address << ":" << Value.mPort;
	if ( Value.mClientRef.IsValid() )
		str << "[" << Value.mClientRef << "]";

	return str;
}






class SoyNet::TPeerRef
{
public:
	SoyRef	mRef;
};

class SoyNet::TPeer : public SoyNet::TPeerRef
{
public:
	TPeer();

	const TPeerRef&		GetRef() const		{	return *this;	}
};


class SoyNet::TMatchRef
{
public:
	SoyRef	mRef;
};

class SoyNet::TMatchMeta : public SoyNet::TMatchRef
{
public:

	const TMatchRef&		GetRef() const		{	return *this;	}
};


class SoyNet::TManager
{
public:
	void						FindMatches();								//	start match-finding

public:
	Array<TMatchMeta>			mMatches;
	Array<TPeer>				mPeerCache;
	ofEvent<Array<TMatchMeta>>	onMatchListChanged;
};


