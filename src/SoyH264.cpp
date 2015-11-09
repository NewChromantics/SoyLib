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



bool H264::IsNalu(ArrayBridge<uint8>& Data,size_t& NaluSize,size_t& HeaderSize)
{
	//	too small to be nalu3
	if ( Data.GetDataSize() < 4 )
		return false;

	//	read magic
	uint32 Magic;
	memcpy( &Magic, Data.GetArray(), sizeof(Magic) );

	//	test for nalu sizes
	//	gr: big endian!
	if ( (Magic & 0xffffffff) == 0x01000000 )
	{
		NaluSize = 4;
	}
	else if ( (Magic & 0xffffff) == 0x010000 )
	{
		NaluSize = 3;
	}
	else
	{
		return false;
	}
	
	//	missing next byte
	if ( Data.GetDataSize() < NaluSize+1 )
		return false;

	//	eat nalu byte
	H264NaluContent::Type Content;
	H264NaluPriority::Type Priority;
	uint8 NaluByte = Data[NaluSize];
	try
	{
		H264::DecodeNaluByte( NaluByte, Content, Priority );
	}
	catch(...)
	{
		return false;
	}
	
	//	+nalubyte
	HeaderSize = NaluSize+1;
	
	//	eat another spare byte...
	//	todo: verify this is the proper way of doing things...
	if ( Content == H264NaluContent::AccessUnitDelimiter )
	{
		//	eat the AUD type value
		HeaderSize += 1;
	}

	//	real data should follow now
	return true;
}


void H264::RemoveHeader(SoyMediaFormat::Type Format,ArrayBridge<uint8>&& Data,bool KeepNaluByte)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_ES:
		case SoyMediaFormat::H264_SPS_ES:
		case SoyMediaFormat::H264_PPS_ES:
		{
			//	gr: CMVideoFormatDescriptionCreateFromH264ParameterSets requires the byte!
			size_t NaluSize,HeaderSize;
			if ( IsNalu( Data, NaluSize, HeaderSize ) )
			{
				if ( KeepNaluByte )
					Data.RemoveBlock( 0, NaluSize );
				else
					Data.RemoveBlock( 0, HeaderSize );
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
	size_t NaluSize = 0;
	size_t HeaderSize = 0;
	
	//	todo: could try and decode length size from Data size...
	if ( !IsNalu( Data, NaluSize, HeaderSize ) )
		return false;
	
	uint8 NaluByte = Data[NaluSize];
	
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
	ConvertToFormat( Format, Format, Data );
}


ssize_t H264::FindNaluStartIndex(ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize)
{
	//	look for start of nalu
	for ( ssize_t i=0;	i<Data.GetSize();	i++ )
	{
		auto ChunkPart = GetRemoteArray( &Data[i], Data.GetSize()-i );

		if ( !H264::IsNalu( GetArrayBridge(ChunkPart), NaluSize, HeaderSize ) )
			continue;
		
		return i;
	}
	return -1;
}


void H264::ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>& Data)
{
	//	verify header
	Soy::Assert( Data.GetDataSize() > 5, "Missing H264 packet header" );
	Soy::Assert( SoyMediaFormat::IsH264(DataFormat), "Expecting a kind of H264 format input" );
	
	//	find real format of data
	ResolveH264Format( DataFormat, Data );
	
	//	check if already in desired format
	if ( NewFormat == SoyMediaFormat::H264_ES )
	{
		if ( DataFormat == SoyMediaFormat::H264_ES ||
			DataFormat == SoyMediaFormat::H264_SPS_ES ||
			DataFormat == SoyMediaFormat::H264_PPS_ES )
		{
			return;
		}
	}
	if ( NewFormat == DataFormat )
		return;
	
	//	packet can contain multiple NALU's
	//	gr: maybe split these... BUT... this packet is for a particular timecode so err... maintain multiple NALU's for a packet
	
	auto InsertChunk_AnnexB = [](size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)
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
		Position += Delim.GetDataSize();
	};
	
	auto InsertChunk_Avcc = [NewFormat](size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)
	{
		BufferArray<uint8,4> Delin;
		
		//	find all the proper prefix stuff
		if ( NewFormat == SoyMediaFormat::H264_8 )
		{
			auto Size = size_cast<uint8>( ChunkLength );
			GetArrayBridge(Delin).PushBackReinterpretReverse( Size );
		}
		if ( NewFormat == SoyMediaFormat::H264_16 )
		{
			auto Size = size_cast<uint16>( ChunkLength );
			GetArrayBridge(Delin).PushBackReinterpretReverse( Size );
		}
		if ( NewFormat == SoyMediaFormat::H264_32 )
		{
			auto Size = size_cast<uint32>( ChunkLength );
			GetArrayBridge(Delin).PushBackReinterpretReverse( Size );
		}
		
		Data.InsertArray( GetArrayBridge(Delin), Position );
		Position += Delin.GetDataSize();
	};
	
	auto ExtractChunk_Avcc = [DataFormat](ArrayBridge<uint8>& Data,size_t Position)
	{
		auto LengthSize = GetNaluLengthSize( DataFormat );
		Soy::Assert( LengthSize != 0, "Unhandled H264 type");
		
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
		
		if ( ChunkLength > Data.GetDataSize()-Position )
			std::Debug << "Extracted bad chunklength=" << ChunkLength << std::endl;
		
		return ChunkLength;
	};
	
	auto ExtractChunk_AnnexB = [DataFormat](ArrayBridge<uint8>& Data,size_t Position)
	{
		//	re-search for next nalu (offset from the start or we'll just match it again)
		//	gr: be careful, SPS is only 6/7 bytes including the nalu header
		//	gr: some packets are 1 byte long. so no offset.
		static int _SearchOffset = 0;
		
		//	extract header
		size_t NaluSize = 0;
		auto StartOffset = Position;
		auto StartData = GetRemoteArray( Data.GetArray()+StartOffset, Data.GetDataSize()-StartOffset );
		if ( GetArrayBridge(StartData).IsEmpty() )
			return 0ul;
		
		size_t HeaderSize = 0;
		auto Start = H264::FindNaluStartIndex( GetArrayBridge(StartData), NaluSize, HeaderSize );
		Soy::Assert( Start >= 0, "Failed to find NALU header in annex b packet");
		Soy::Assert( NaluSize != 0, "Failed to find NALU header size in annex b packet");
		
		//	recalc data start
		Start = StartOffset + HeaderSize;
		
		size_t EndNaluSize = 0;
		size_t EndHeaderSize = 0;
		auto EndOffset = Start + _SearchOffset;
		Soy::Assert( EndOffset <= Data.GetDataSize(), "search nalu end is too far forward" );
		auto EndData = GetRemoteArray( Data.GetArray()+EndOffset, Data.GetDataSize()-EndOffset );
		auto End = FindNaluStartIndex( GetArrayBridge(EndData), EndNaluSize, EndHeaderSize );
		if ( End < 0 )
			End = Data.GetDataSize();
		else
			End += EndOffset;

		//	calc length
		size_t ChunkLength = End - Start;
		
		//	remove the header
		Data.RemoveBlock( Position, HeaderSize );
		
		if ( ChunkLength > Data.GetDataSize()-Position )
			std::Debug << "Extracted bad chunklength=" << ChunkLength << std::endl;

		return ChunkLength;
	};
	
	
	std::function<size_t(ArrayBridge<uint8>& Data,size_t Position)> Extracter;
	std::function<void(size_t ChunkLength,ArrayBridge<uint8>& Data,size_t& Position)> Inserter;

	if ( DataFormat == SoyMediaFormat::H264_8 ||
		DataFormat == SoyMediaFormat::H264_16 ||
		DataFormat == SoyMediaFormat::H264_32
		)
	{
		Extracter = ExtractChunk_Avcc;
	}
	if ( DataFormat == SoyMediaFormat::H264_ES ||
		DataFormat == SoyMediaFormat::H264_PPS_ES ||
		DataFormat == SoyMediaFormat::H264_SPS_ES )
	{
		Extracter = ExtractChunk_AnnexB;
	}
	
	if ( NewFormat == SoyMediaFormat::H264_8 ||
		NewFormat == SoyMediaFormat::H264_16 ||
		NewFormat == SoyMediaFormat::H264_32
		)
	{
		Inserter = InsertChunk_Avcc;
	}
	if ( NewFormat == SoyMediaFormat::H264_ES ||
		NewFormat == SoyMediaFormat::H264_PPS_ES ||
		NewFormat == SoyMediaFormat::H264_SPS_ES )
	{
		Inserter = InsertChunk_AnnexB;
	}
	
	//	reformat & save
	ReformatDeliminator( Data, Extracter, Inserter );
	DataFormat = NewFormat;
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
	Soy::Assert( Zero==0, "Nalu zero bit non-zero");
	//	catch bad cases. look out for genuine cases, but if this is zero, NALU delin might have been read wrong
	Soy::Assert( Content8!=0, "Nalu content type is invalid (zero)");
	Priority = H264NaluPriority::Validate( Idc );
	Content = H264NaluContent::Validate( Content8 );
}









