#include "ofxSoylent.h"
#include "SoyNet.h"
#include <regex>


namespace SoyNet
{
	SoyRef AllocPeerRef()
	{
		static SoyRef LastRef( "Peer" );
		LastRef.mRef++;
		return LastRef;
	}
}


using namespace SoyNet;



TAddress::TAddress(BufferString<200> AddressAndPort,TClientRef ClientRef) :
	mPort		( 0 ),
	mClientRef	( ClientRef )
{
	//	split address:port
	std::regex Pattern("([a-zA-Z0-9\\-\\.]+)(:[0-9]+)?" );
	std::smatch Match;
	std::string AddressPortStr( AddressAndPort.c_str() );
	if ( std::regex_match( AddressPortStr, Match, Pattern ) )
	{
		mAddress = Match[1].str();
		if ( Match.size() > 2 )
		{
			auto& PortStr = Match[2].str();
			assert( PortStr[0] == ':' );
			TString PortString( PortStr.c_str()+1 );
			int32 Port;
			if ( PortString.GetInteger( Port ) )
				mPort = Port;
			else
			{
				//	regex should have only matched an integer
				assert(false);
			}
		}
	}
	else
	{
		//	gr: check non-matching addresses to make sure pattern is good
		assert(false);
	}

	assert(IsValid());
}


TPeer::TPeer()
{
	mRef = AllocPeerRef();
}


