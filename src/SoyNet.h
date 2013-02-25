#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"

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
};


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


