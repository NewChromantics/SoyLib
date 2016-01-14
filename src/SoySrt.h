#pragma once

#include "SoyProtocol.h"
#include "SoyTime.h"


namespace Srt
{
	class TFrame;
	static const char*	FileExtensions[] = {".srt"};
}



class Srt::TFrame : public Soy::TReadProtocol, public Soy::TWriteProtocol
{
public:
	TFrame();
	
	virtual TProtocolState::Type	Decode(TStreamBuffer& Buffer) override;
	virtual void					Encode(TStreamBuffer& Buffer) override;

public:
	std::string	mString;
	size_t		mIndex;	//	framecounter?
	SoyTime		mStart;
	SoyTime		mEnd;
};


