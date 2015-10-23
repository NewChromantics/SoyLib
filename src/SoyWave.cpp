#include "SoyWave.h"
#include "array.hpp"
#include "heaparray.hpp"


Wave::TMeta::TMeta() :
	mEncoding		( SoyWaveEncoding::PCM ),
	mChannelCount	( 2 ),
	mSampleRate		( 44100 ),
	mBitsPerSample	( 16 )
{
}
	

void Wave::TMeta::GetFormatSubChunkData(ArrayBridge<char>&& Data)
{
	//	The "WAVE" format consists of two subchunks: "fmt " and "data":
	//	The "fmt " subchunk describes the sound data's format:
	uint16 AudioFormat = 1;	//	PCM
	uint32 ByteRate = mSampleRate * mChannelCount * (mBitsPerSample/8);
	uint16 BlockAlignment = mChannelCount * (mBitsPerSample/8);
	Data.PushBackReinterpret( AudioFormat );
	Data.PushBackReinterpret( size_cast<uint16>( mChannelCount ) );
	Data.PushBackReinterpret( size_cast<uint32>( mSampleRate ) );
	Data.PushBackReinterpret( ByteRate );
	Data.PushBackReinterpret( BlockAlignment );
	Data.PushBackReinterpret( size_cast<uint16>( mBitsPerSample ) );
	
	
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
