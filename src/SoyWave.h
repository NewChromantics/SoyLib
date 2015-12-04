#pragma once

#include "SoyTypes.h"



namespace SoyWaveEncoding
{
	enum Type
	{
		PCM
	};
}

namespace SoyWaveBitsPerSample
{
	enum Type
	{
		Eight,
		Sixteen,
		Float,		//	32 bit -1 to 1
	};

	size_t	GetByteSize(Type Format);
};

//	gr: not sure if this is a good namespace name?
namespace Wave
{
	class TMeta;

	void	WriteSample(float Sample,SoyWaveBitsPerSample::Type Bits,ArrayBridge<uint8>&& Data);
	void	ConvertSample(const sint16 Input,float& Output);
};


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
	size_t					mSampleRate;		//	samples per second(freq/hz) 8000	44100	(440hz)
	SoyWaveBitsPerSample::Type	mBitsPerSample;		//	normal 8 or 16
};
