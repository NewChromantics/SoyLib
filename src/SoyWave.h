#pragma once

#include "SoyMedia.h"


namespace SoyWaveBitsPerSample
{
	size_t	GetByteSize(SoyMediaFormat::Type Format);
};

//	gr: not sure if this is a good namespace name?
namespace Wave
{
	class TMeta;

	void	ConvertSample(const sint16 Input,float& Output);
	void	ConvertSample(const sint8 Input,float& Output);
	void	ConvertSample(const float Input,sint8& Output);
	void	ConvertSample(const float Input,uint8& Output);
	void	ConvertSample(const float Input,sint16& Output);
	void	ConvertSample(const float Input,float& Output);

	
	template<typename OLDTYPE,typename NEWTYPE>
	inline void	ConvertSamples(const ArrayBridge<OLDTYPE>& Input,ArrayBridge<NEWTYPE>& Output)
	{
		for ( int i=0;	i<Input.GetSize();	i++ )
		{
			auto& SampleIn = Input[i];
			NEWTYPE SampleOut;
			ConvertSample( SampleIn, SampleOut );
			Output.PushBack( SampleOut );
		}
	}
	template<typename OLDTYPE,typename NEWTYPE>
	inline void	ConvertSamples(const ArrayBridge<OLDTYPE>&& Input,ArrayBridge<NEWTYPE>&& Output)
	{
		ConvertSamples( Input, Output );
	}
	template<typename OLDTYPE,typename NEWTYPE>
	inline void	ConvertSamples(const ArrayBridge<OLDTYPE>& Input,ArrayBridge<NEWTYPE>&& Output)
	{
		ConvertSamples( Input, Output );
	}
	template<typename OLDTYPE,typename NEWTYPE>
	inline void	ConvertSamples(const ArrayBridge<OLDTYPE>&& Input,ArrayBridge<NEWTYPE>& Output)
	{
		ConvertSamples( Input, Output );
	}

};


//	this is superceeded by TStreamMeta
class Wave::TMeta
{
public:
	TMeta();

	//	the header needs to know the size of all the data following the header
	void		WriteHeader(ArrayBridge<char>&& Stream,size_t DataSize);
	void		GetFormatSubChunkData(ArrayBridge<char>&& Data);

public:
	SoyMediaFormat::Type	mFormat;
	size_t					mChannelCount;
	size_t					mSampleRate;		//	samples per second(freq/hz) 8000	44100	(440hz)
};
