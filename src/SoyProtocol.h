#pragma once

#include "SoyEnum.h"

class TStreamBuffer;

namespace Soy
{
	class TReadProtocol;
	class TWriteProtocol;
};

namespace TProtocolState
{
	enum Type
	{
		Invalid,
		Waiting,			//	waiting for more data
		Finished,			//	this one is done
		Ignore,				//	finished, but do nothing with result
		Disconnect,			//	finished, process and disconnect
	};
	DECLARE_SOYENUM(TProtocolState);
};




//	this is instanced, more like a protocol-packet decoder
class Soy::TReadProtocol
{
public:
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer)=0;
};

class Soy::TWriteProtocol
{
public:
	virtual void					Encode(TStreamBuffer& Buffer)=0;
};

