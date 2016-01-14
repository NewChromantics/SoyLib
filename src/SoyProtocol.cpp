#include "SoyProtocol.h"



std::map<TProtocolState::Type,std::string> TProtocolState::EnumMap =
{
	{	TProtocolState::Invalid,	"Invalid"	},
	{	TProtocolState::Waiting,	"Waiting"	},
	{	TProtocolState::Finished,	"Finished"	},
	{	TProtocolState::Ignore,		"Ignore"	},
	{	TProtocolState::Disconnect,	"Disconnect"	},
	{	TProtocolState::Abort,		"Abort"	},
};

