#include "SoyH264.h"



void ReformatDeliminator(ArrayBridge<uint8>& Data,
						 std::function<size_t(ArrayBridge<uint8>& Data,size_t Position)> ExtractChunk,
						 std::function<void(size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)> InsertChunk)
{
	size_t Position = 0;
	while ( true )
	{
		auto ChunkLength = ExtractChunk( Data, Position );
		if ( ChunkLength == 0 )
			break;
		{
			std::stringstream Error;
			Error << "Extracted NALU length of " << ChunkLength << "/" << Data.GetDataSize();
			Soy::Assert( ChunkLength <= Data.GetDataSize(), Error.str() );
		}
		InsertChunk( ChunkLength, Data, Position );
		Position += ChunkLength;
	}
}



bool H264::IsNalu3(ArrayBridge<uint8>& Data)
{
	//	too small to be nalu3
	if ( Data.GetDataSize() < 4 )
		return false;

	//	check in case it's already NALU
	uint32 Magic;
	memcpy( &Magic, Data.GetArray(), sizeof(Magic) );

	if ( (Magic & 0xffffff) != 0x000001 )
		return false;
	
	//	next bit MUST be zero
	if ( (Magic & 0x00000080) != 0 )
		return false;
	
	//	next is nalu code etc
	return true;
}

bool H264::IsNalu4(ArrayBridge<uint8>& Data)
{
	//	too small to be nalu4
	if ( Data.GetDataSize() < 5 )
		return false;
	
	uint32 Magic;
	memcpy( &Magic, Data.GetArray(), sizeof(Magic) );
	if ( (Magic & 0xffffffff) != 0x00000001 )
		return false;
	
	uint8 NextBit = Data[4];
	//	next bit MUST be zero
	if ( (NextBit & 0x80) != 0 )
		return false;
	
	return true;
}


void H264::ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)
{
	if ( IsNalu3( Data ) || IsNalu4( Data ) )
	{
		Format = SoyMediaFormat::H264_ES;
		return;
	}
}


void H264::ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)
{
	//	verify header
	Soy::Assert( Data.GetDataSize() > 5, "Missing H264 packet header" );
	Soy::Assert( Format == SoyMediaFormat::H264_8 ||
				Format == SoyMediaFormat::H264_16 ||
				Format == SoyMediaFormat::H264_32 ||
				Format == SoyMediaFormat::H264_ES
				, "Expecting a kind of H264 format input" );
	
	if ( IsNalu3( Data ) || IsNalu4( Data ) )
	{
		Format = SoyMediaFormat::H264_ES;
		return;
	}

	//	packet can contain multiple NALU's
	//	gr: maybe split these... BUT... this packet is for a particular timecode so err... maintain multiple NALU's for a packet
	//	check for the leading size...
	int LengthSize = 0;
	if ( Format == SoyMediaFormat::H264_8 )
		LengthSize = 1;
	else if ( Format == SoyMediaFormat::H264_16 )
		LengthSize = 2;
	else if ( Format == SoyMediaFormat::H264_32 )
		LengthSize = 4;

	Soy::Assert( LengthSize != 0, "Unhandled H264 type");
	
	auto InsertChunkDelin = [](size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)
	{
		//	insert new delim
		//	gr: this is missing the importance flag!
		uint8 Delim[] =
		{
			0, 0, 0, 1,
			9,			// NAL type = Access Unit Delimiter;
			0xF0		// Slice types = ANY
		};
		Data.InsertArray( GetRemoteArray(Delim), Position );
		Position += sizeof( Delim );
	};
	
	auto ExtractChunkDelin = [LengthSize](ArrayBridge<uint8>& Data,size_t Position)
	{
		if (Position == Data.GetDataSize() )
			return (size_t)0;
		Soy::Assert( Position < Data.GetDataSize(), "H264 ExtractChunkDelin position gone out of bounds" );
		
		size_t ChunkLength = 0;
		
		if ( LengthSize == 1 )
		{
			ChunkLength |= Data.PopAt(Position);
		}
		else if ( LengthSize == 2 )
		{
			ChunkLength |= Data.PopAt(Position) << 8;
			ChunkLength |= Data.PopAt(Position) << 0;
		}
		else if ( LengthSize == 4 )
		{
			ChunkLength |= Data.PopAt(Position) << 24;
			ChunkLength |= Data.PopAt(Position) << 16;
			ChunkLength |= Data.PopAt(Position) << 8;
			ChunkLength |= Data.PopAt(Position) << 0;
		}
		
		return ChunkLength;
	};
	
	ReformatDeliminator( Data, ExtractChunkDelin, InsertChunkDelin );

	//	finally update the format
	Format = SoyMediaFormat::H264_ES;
}












