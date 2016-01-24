#include "SoyWave.h"
#include "array.hpp"
#include "heaparray.hpp"



size_t SoyWaveBitsPerSample::GetByteSize(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::PcmLinear_8:		return 1;
		case SoyMediaFormat::PcmLinear_16:		return 2;
		case SoyMediaFormat::PcmLinear_24:		return 3;
		case SoyMediaFormat::PcmLinear_float:	return 4;
	
		default:
			break;
	}
	
	std::stringstream Error;
	Error << "Don't know how many wave bytes " << Format << " takes";
	throw Soy::AssertException( Error.str() );
}

Wave::TMeta::TMeta() :
	mFormat			( SoyMediaFormat::PcmLinear_float ),
	mChannelCount	( 2 ),
	mSampleRate		( 44100 )
{
}
	

void Wave::TMeta::GetFormatSubChunkData(ArrayBridge<char>&& Data)
{
	auto BytesPerSample = SoyWaveBitsPerSample::GetByteSize( mFormat );

	//	gr: WAVE_FORMAT_PCM already defined in windows 8.1 headers, but check it's the same value
	//	http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
#if defined(WAVE_FORMAT_PCM)
	static_assert(WAVE_FORMAT_PCM == 0x0001,"WAVE_FORMAT_PCM value mis match");
#else
#define	WAVE_FORMAT_PCM			0x0001
#endif

#define	WAVE_FORMAT_IEEE_FLOAT	0x0003
#define	WAVE_FORMAT_ALAW	0x0006	//	8-bit ITU-T G.711 A-law
#define	WAVE_FORMAT_MULAW	0x0007	//	8-bit ITU-T G.711 µ-law
#define	WAVE_FORMAT_EXTENSIBLE	0xFFFE	//	Determined by SubFormat
	
	//	The "WAVE" format consists of two subchunks: "fmt " and "data":
	//	The "fmt " subchunk describes the sound data's format:
	uint16 AudioFormat = (mFormat == SoyMediaFormat::PcmLinear_float) ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	
	uint32 ByteRate = size_cast<uint32>( mSampleRate * mChannelCount * BytesPerSample );
	uint16 BlockAlignment = mChannelCount * (BytesPerSample);	//	block size
	Data.PushBackReinterpret( AudioFormat );
	Data.PushBackReinterpret( size_cast<uint16>( mChannelCount ) );
	Data.PushBackReinterpret( size_cast<uint32>( mSampleRate ) );	//	blocks per second
	Data.PushBackReinterpret( ByteRate );
	Data.PushBackReinterpret( BlockAlignment );
	Data.PushBackReinterpret( size_cast<uint16>( BytesPerSample*8 ) );
	
	
	//	2   ExtraParamSize   if PCM, then doesn't exist
    //	X   ExtraParams      space for extra parameters

}

void Wave::TMeta::WriteHeader(ArrayBridge<char>&& Data,size_t DataSize)
{
	//	http://soundfile.sapp.org/doc/WaveFormat/
	//Soy::Assert( mEncoding == SoyWaveEncoding::PCM, "Expected PCM encoding");
	

	//	generate subchunk 1 data
	Array<char> SubChunk1;
	GetFormatSubChunkData( GetArrayBridge(SubChunk1) );
	Soy::Assert( SubChunk1.GetDataSize() == 16, "Expected format subchunk to be 16 bytes long");

	//	== NumSamples * NumChannels * BitsPerSample/8
	//	This is the number of bytes in the data. You can also think of this as the size of the read of the subchunk following this number.
	uint32 SubChunk2Size = size_cast<uint32>( DataSize );

	Data.PushBackReinterpret("RIFF",4);		//	0x52494646
	uint32 SubChunk1Size = size_cast<uint32>( SubChunk1.GetDataSize() );	//	PCM
	uint32 ChunkSize = 4 + 8 + SubChunk1Size + 8 + SubChunk2Size;
	Data.PushBackReinterpret( ChunkSize );
	Data.PushBackReinterpret("WAVE",4);		//	0x57415645
	Data.PushBackReinterpret("fmt ",4);
	Data.PushBackReinterpret( SubChunk1Size );
	Data.PushBackArray( SubChunk1 );
	Data.PushBackReinterpret("data",4);		//	0x64617461
	Data.PushBackReinterpret(SubChunk2Size);
	
	//	data follows
};


void Wave::ConvertSample(const sint16 Input,float& Output)
{
	//	0..1
	Output = Soy::Range<sint16>( Input, -32768, 32767 );
	Output *= 2.f;
	Output -= 1.f;
}


void Wave::ConvertSample(const sint8 Input,float& Output)
{
	//	0..1
	Output = Soy::Range<sint8>( Input, -127, 127 );
	Output *= 2.f;
	Output -= 1.f;
}


void Wave::ConvertSample(const float Input,sint8& Output)
{
	Output = size_cast<sint8>( (Input + 1.f) * 127.f );
}

void Wave::ConvertSample(const float Input,uint8& Output)
{
	Output = size_cast<uint8>( (Input + 1.f) * 127.f );
}

void Wave::ConvertSample(const float Input,sint16& Output)
{
	Output = Soy::Lerp<sint16>( -32768, 32767, Input );
}

void Wave::ConvertSample(const float Input,float& Output)
{
	Output = Input;
}

