#include "SoyWave.h"
#include "array.hpp"
#include "heaparray.hpp"



size_t SoyWaveBitsPerSample::GetByteSize(SoyWaveBitsPerSample::Type Format)
{
	switch ( Format )
	{
		case SoyWaveBitsPerSample::Eight:
			return 1;
		case SoyWaveBitsPerSample::Sixteen:
			return 2;
		case SoyWaveBitsPerSample::Float:
			return 4;
	}
}

Wave::TMeta::TMeta() :
	mEncoding		( SoyWaveEncoding::PCM ),
	mChannelCount	( 2 ),
	mSampleRate		( 44100 ),
	mBitsPerSample	( SoyWaveBitsPerSample::Float )
{
}
	

void Wave::TMeta::GetFormatSubChunkData(ArrayBridge<char>&& Data)
{
	auto BytesPerSample = SoyWaveBitsPerSample::GetByteSize( mBitsPerSample );

	//	http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
#define	WAVE_FORMAT_PCM			0x0001
#define	WAVE_FORMAT_IEEE_FLOAT	0x0003
#define	WAVE_FORMAT_ALAW	0x0006	//	8-bit ITU-T G.711 A-law
#define	WAVE_FORMAT_MULAW	0x0007	//	8-bit ITU-T G.711 µ-law
#define	WAVE_FORMAT_EXTENSIBLE	0xFFFE	//	Determined by SubFormat
	
	//	The "WAVE" format consists of two subchunks: "fmt " and "data":
	//	The "fmt " subchunk describes the sound data's format:
	uint16 AudioFormat = (mBitsPerSample == SoyWaveBitsPerSample::Float) ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	
	uint32 ByteRate = mSampleRate * mChannelCount * (BytesPerSample);
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
	Soy::Assert( mEncoding == SoyWaveEncoding::PCM, "Expected PCM encoding");
	

	//	generate subchunk 1 data
	Array<char> SubChunk1;
	GetFormatSubChunkData( GetArrayBridge(SubChunk1) );
	Soy::Assert( SubChunk1.GetDataSize() == 16, "Expected format subchunk to be 16 bytes long");

	//	== NumSamples * NumChannels * BitsPerSample/8
	//	This is the number of bytes in the data. You can also think of this as the size of the read of the subchunk following this number.
	uint32 SubChunk2Size = size_cast<uint32>( DataSize );

	Data.PushBackReinterpret("RIFF",4);		//	0x52494646
	uint32 SubChunk1Size = SubChunk1.GetDataSize();	//	PCM
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


void Wave::WriteSample(float Samplef,SoyWaveBitsPerSample::Type Bits,ArrayBridge<uint8>&& Data)
{
	Soy::Assert( Samplef >= -1.f && Samplef <= 1.f, "Float sample out of range");

	switch ( Bits )
	{
		case SoyWaveBitsPerSample::Eight:
		{
			uint8 Sample = size_cast<uint8>( (Samplef + 1.f) * 127.f );
			Data.PushBack( Sample );
			return;
		}
		
		case SoyWaveBitsPerSample::Sixteen:
		{
			//	16bit samples are signed
			auto Sample = Soy::Lerp<sint16>( -32768, 32767, Samplef );
			Data.PushBackReinterpret( Sample );
			return;
		}

		case SoyWaveBitsPerSample::Float:
			Data.PushBackReinterpret( Samplef );
			return;
	}

}

