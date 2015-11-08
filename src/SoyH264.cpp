#include "SoyH264.h"



std::map<H264NaluContent::Type,std::string> H264NaluContent::EnumMap =
{
#define ENUM_CASE(e)	{	e,	#e	}
	ENUM_CASE( Invalid ),
	ENUM_CASE( Unspecified ),
	ENUM_CASE( Slice_NonIDRPicture ),
	ENUM_CASE( Slice_CodedPartitionA ),
	ENUM_CASE( Slice_CodedPartitionB ),
	ENUM_CASE( Slice_CodedPartitionC ),
	ENUM_CASE( Slice_CodedIDRPicture ),
	ENUM_CASE( SupplimentalEnhancementInformation ),
	ENUM_CASE( SequenceParameterSet ),
	ENUM_CASE( PictureParameterSet ),
	ENUM_CASE( AccessUnitDelimiter ),
	ENUM_CASE( EndOfSequence ),
	ENUM_CASE( EndOfStream ),
	ENUM_CASE( FillerData ),
	ENUM_CASE( SequenceParameterSetExtension ),
	ENUM_CASE( Reserved14 ),
	ENUM_CASE( Reserved15 ),
	ENUM_CASE( Reserved16 ),
	ENUM_CASE( Reserved17 ),
	ENUM_CASE( Reserved18 ),
	ENUM_CASE( Slice_AuxCodedUnpartitioned ),
	ENUM_CASE( Reserved20 ),
	ENUM_CASE( Reserved21 ),
	ENUM_CASE( Reserved22 ),
	ENUM_CASE( Reserved23 ),
	ENUM_CASE( Unspecified24 ),
	ENUM_CASE( Unspecified25 ),
	ENUM_CASE( Unspecified26 ),
	ENUM_CASE( Unspecified27 ),
	ENUM_CASE( Unspecified28 ),
	ENUM_CASE( Unspecified29 ),
	ENUM_CASE( Unspecified30 ),
	ENUM_CASE( Unspecified31 ),
#undef ENUM_CASE
};

std::map<H264NaluPriority::Type,std::string> H264NaluPriority::EnumMap =
{
#define ENUM_CASE(e)	{	e,	#e	}
	ENUM_CASE( Invalid ),
	ENUM_CASE( Important ),
	ENUM_CASE( Two ),
	ENUM_CASE( One ),
	ENUM_CASE( Zero ),
#undef ENUM_CASE
};




size_t H264::GetNaluLengthSize(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_8:	return 1;
		case SoyMediaFormat::H264_16:	return 2;
		case SoyMediaFormat::H264_32:	return 4;
	
		case SoyMediaFormat::H264_ES:
		case SoyMediaFormat::H264_SPS_ES:
		case SoyMediaFormat::H264_PPS_ES:
			return 0;
			
		default:
			break;
	}
	
	std::stringstream Error;
	Error << __func__ << " unhandled format " << Format;
	throw Soy::AssertException( Error.str() );
}



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

	//	gr: big endian
	if ( (Magic & 0xffffff) != 0x010000 )
	//if ( (Magic & 0xffffff) != 0x000001 )
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
	
	//	gr: big endian
	//if ( (Magic & 0xffffffff) != 0x00000001 )
	if ( (Magic & 0xffffffff) != 0x01000000 )
		return false;
	
	uint8 NextBit = Data[4];
	//	next bit MUST be zero
	if ( (NextBit & 0x80) != 0 )
		return false;
	
	return true;
}

void H264::RemoveHeader(SoyMediaFormat::Type Format,ArrayBridge<uint8>&& Data)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_ES:
		case SoyMediaFormat::H264_SPS_ES:
		case SoyMediaFormat::H264_PPS_ES:
		{
			//	gr: CMVideoFormatDescriptionCreateFromH264ParameterSets requires the byte!
			static bool RemoveNalByte = false;
			if ( IsNalu3( Data ) )
			{
				Data.RemoveBlock( 0, 3+RemoveNalByte );
				return;
			}
			else if ( IsNalu4( Data ) )
			{
				Data.RemoveBlock( 0, 4+RemoveNalByte );
				return;
			}
			throw Soy::AssertException("Tried to trim header from h264 ES data but couldn't find nalu header");
		}
			
			//	gr: is there more to remove than the length?
		case SoyMediaFormat::H264_8:
			Data.RemoveBlock( 0, 1 );
			return;
			
		case SoyMediaFormat::H264_16:
			Data.RemoveBlock( 0, 2 );
			return;
			
		case SoyMediaFormat::H264_32:
			Data.RemoveBlock( 0, 4 );
			return;
	
		default:
			break;
	}
	
	std::stringstream Error;
	Error << __func__ << " trying to trim header from non h264 format " << Format;
	throw Soy::AssertException( Error.str() );
}

bool H264::ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>& Data)
{
	uint8 NaluByte = 0xff;
	if ( IsNalu3( Data ) )
	{
		NaluByte = Data[3];
	}
	else if ( IsNalu4( Data ) )
	{
		NaluByte = Data[4];
	}
	else
	{
		//	todo: could try and decode length size from Data size...
		return false;
	}
	
	H264NaluContent::Type Content;
	H264NaluPriority::Type Priority;
	DecodeNaluByte( NaluByte, Content, Priority );
		
	if ( Content == H264NaluContent::SequenceParameterSet )
		Format = SoyMediaFormat::H264_SPS_ES;
	else if ( Content == H264NaluContent::PictureParameterSet )
		Format = SoyMediaFormat::H264_PPS_ES;
	else
		Format = SoyMediaFormat::H264_ES;
	
	return true;
}


void H264::ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)
{
	//	verify header
	Soy::Assert( Data.GetDataSize() > 5, "Missing H264 packet header" );
	Soy::Assert( SoyMediaFormat::IsH264(Format), "Expecting a kind of H264 format input" );
	
	if ( ResolveH264Format( Format, Data ) )
	{
		return;
	}

	//	packet can contain multiple NALU's
	//	gr: maybe split these... BUT... this packet is for a particular timecode so err... maintain multiple NALU's for a packet
	//	check for the leading size...
	auto LengthSize = GetNaluLengthSize( Format );
	Soy::Assert( LengthSize != 0, "Unhandled H264 type");
	
	auto InsertChunkDelin = [](size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)
	{
		//	insert new delim
		//	gr: this is missing the importance flag!
		BufferArray<uint8,10> Delim;
		Delim.PushBack(0);
		Delim.PushBack(0);
		Delim.PushBack(0);
		Delim.PushBack(1);
		Delim.PushBack( EncodeNaluByte( H264NaluContent::AccessUnitDelimiter, H264NaluPriority::Zero ) );
		Delim.PushBack(0xF0);// Slice types = ANY
		Data.InsertArray( GetArrayBridge(Delim), Position );
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



uint8 H264::EncodeNaluByte(H264NaluContent::Type Content,H264NaluPriority::Type Priority)
{
//	uint8 Idc_Important = 0x3 << 5;	//	0x60
	//	uint8 Idc = Idc_Important;	//	011 XXXXX
	uint8 Idc = Priority;
	Idc <<= 5;
	uint8 Type = Content;
	
	uint8 Byte = Idc|Type;
	return Byte;
}

void H264::DecodeNaluByte(uint8 Byte,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority)
{
	uint8 Zero = (Byte >> 7) & 0x1;
	uint8 Idc = (Byte >> 5) & 0x3;
	uint8 Content8 = (Byte >> 0) & (0x1f);
	Soy::Assert( Zero ==0, "Nalu zero bit non-zero");
	Priority = H264NaluPriority::Validate( Idc );
	Content = H264NaluContent::Validate( Content8 );
}









