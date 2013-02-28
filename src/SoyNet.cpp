#include "ofxSoylent.h"
#include "SoyNet.h"


namespace SoyNet
{
	SoyRef AllocPeerRef()
	{
		static SoyRef LastRef( 0 );
		LastRef.mRef++;
		return LastRef;
	}
}


using namespace SoyNet;


TPeer::TPeer()
{
	mRef = AllocPeerRef();
}

