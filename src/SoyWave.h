#pragma once

#include "SoyTypes.h"



//	gr: not sure if this is a good namespace name?
namespace Wave
{
	class TMeta;
};

namespace SoyWaveEncoding
{
	enum Type
	{
		PCM
	};
}


class Wave::TMeta
{
public:
	TMeta();

	//	the header needs to know the size of all the data following the header
	void		WriteHeader(ArrayBridge<char>&& Stream,size_t DataSize);
	void		GetFormatSubChunkData(ArrayBridge<char>&& Data);

public:
	SoyWaveEncoding::Type	mEncoding;
	size_t					mChannelCount;
	size_t					mSampleRate;		//	8000	44100	hz?
	size_t					mBitsPerSample;		//	normal 8 or 16
};
