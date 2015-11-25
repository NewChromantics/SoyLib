#include "SoyH264.h"



std::map<H264NaluContent::Type,std::string> H264NaluContent::EnumMap =
{
#if defined(TARGET_WINDOWS)
#define ENUM_CASE(e)	{	H264NaluContent::e,	#e	}
#else
#define ENUM_CASE(e)	{	e,	#e	}
#endif
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
	ENUM_CASE( STAP_A ),
	ENUM_CASE( STAP_B ),
	ENUM_CASE( MTAP_16 ),
	ENUM_CASE( MTAP_24 ),
	ENUM_CASE( FU_A ),
	ENUM_CASE( FU_B ),
	ENUM_CASE( Unspecified30 ),
	ENUM_CASE( Unspecified31 ),
#undef ENUM_CASE
};

std::map<H264NaluPriority::Type,std::string> H264NaluPriority::EnumMap =
{
#if defined(TARGET_WINDOWS)
#define ENUM_CASE(e)	{	H264NaluPriority::e,	#e	}
#else
#define ENUM_CASE(e)	{	e,	#e	}
#endif
	ENUM_CASE( Invalid ),
	ENUM_CASE( Important ),
	ENUM_CASE( Two ),
	ENUM_CASE( One ),
	ENUM_CASE( Zero ),
#undef ENUM_CASE
};


std::map<H264Profile::Type, std::string> H264Profile::EnumMap =
{
#if defined(TARGET_WINDOWS)
#define ENUM_CASE(e)	{	H264NaluPriority::e,	#e	}
#else
#define ENUM_CASE(e)	{	e,	#e	}
#endif
	ENUM_CASE( Invalid ),
	ENUM_CASE( Baseline ),
	ENUM_CASE( Main ),
	ENUM_CASE( Extended ),
	ENUM_CASE( High ),
	
	ENUM_CASE( High10Intra ),
	ENUM_CASE( High422Intra ),
	ENUM_CASE( High4 ),
	ENUM_CASE( High5 ),
	ENUM_CASE( High6 ),
	ENUM_CASE( High7 ),
	ENUM_CASE( High8 ),
	ENUM_CASE( High9 ),
	ENUM_CASE( HighMultiviewDepth ),
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



bool H264::IsNalu(const ArrayBridge<uint8>& Data,size_t& NaluSize,size_t& HeaderSize)
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
	try
	{
		DecodeNaluByte( NaluByte, Content, Priority );
	}
	catch(std::exception&e)
	{
		std::Debug << __func__ << " exception: " << e.what() << std::endl;
		throw;
	}
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
			return static_cast<size_t>(0);
		
		size_t RealHeaderSize = 0;
		auto Start = H264::FindNaluStartIndex( GetArrayBridge(StartData), NaluSize, RealHeaderSize );
		Soy::Assert( Start >= 0, "Failed to find NALU header in annex b packet");
		Soy::Assert( NaluSize != 0, "Failed to find NALU header size in annex b packet");
		
		//	recalc data start
		static bool EatNaluByte = true;
		auto HeaderSize = EatNaluByte ? RealHeaderSize : NaluSize;
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

void H264::DecodeNaluByte(SoyMediaFormat::Type Format,const ArrayBridge<uint8>&& Data,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority)
{
	size_t NaluSize;
	size_t HeaderSize;
	if ( !IsNalu( Data, NaluSize, HeaderSize ) )
	{
		Content = H264NaluContent::Invalid;
		return;
	}
	
	DecodeNaluByte( Data[HeaderSize], Content, Priority );
}

class TBitReader
{
public:
	TBitReader(const ArrayBridge<uint8>& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	TBitReader(const ArrayBridge<uint8>&& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	
	void			Read(uint32& Data,size_t BitCount);
	void			Read(uint64& Data,size_t BitCount);
	void			Read(uint8& Data,size_t BitCount);
	size_t			BitPosition() const					{	return mBitPos;	}
	
	template<int BYTECOUNT,typename STORAGE>
	void			ReadBytes(STORAGE& Data,size_t BitCount);

	void			ReadExponentialGolombCode(uint32& Data);
	void			ReadExponentialGolombCodeSigned(sint32& Data);
	
private:
	const ArrayBridge<uint8>&	mData;
	size_t				mBitPos;	//	current bit-to-read/write-pos (the tail)
};

/*
unsigned int ReadBit()
{
	ATLASSERT(m_nCurrentBit <= m_nLength * 8);
	int nIndex = m_nCurrentBit / 8;
	int nOffset = m_nCurrentBit % 8 + 1;
	
	m_nCurrentBit ++;
	return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
}
*/

void TBitReader::ReadExponentialGolombCode(uint32& Data)
{
	int i = 0;
	
	while( true )
	{
		uint8 Bit;
		Read( Bit, 1 );
		
		if ( (Bit == 0) && (i < 32) )
			i++;
		else
			break;
	}
	Read( Data, i );
	Data += (1 << i) - 1;
}

void TBitReader::ReadExponentialGolombCodeSigned(sint32& Data)
{
	uint32 r;
	ReadExponentialGolombCode(r);
	if (r & 0x01)
	{
		Data = (r+1)/2;
	}
	else
	{
		Data = -size_cast<sint32>(r/2);
	}
}


template<int BYTECOUNT,typename STORAGE>
void TBitReader::ReadBytes(STORAGE& Data,size_t BitCount)
{
	//	gr: definitly correct
	Data = 0;
	
	BufferArray<uint8,BYTECOUNT> Bytes;
	int ComponentBitCount = size_cast<int>(BitCount);
	while ( ComponentBitCount > 0 )
	{
		Read( Bytes.PushBack(), std::min<size_t>(8,ComponentBitCount) );
		ComponentBitCount -= 8;
	}
	//	gr: should we check for mis-aligned bitcount?
	
	Data = 0;
	for ( int i=0;	i<Bytes.GetSize();	i++ )
	{
		int Shift = (i * 8);
		Data |= static_cast<STORAGE>(Bytes[i]) << Shift;
	}
	
	STORAGE DataBackwardTest = 0;
	for ( int i=0;	i<Bytes.GetSize();	i++ )
	{
		auto Shift = (Bytes.GetSize()-1-i) * 8;
		DataBackwardTest |= static_cast<STORAGE>(Bytes[i]) << Shift;
	}
	
	//	turns out THIS is the right way
	Data = DataBackwardTest;
}

void TBitReader::Read(uint32& Data,size_t BitCount)
{
	//	break up data
	if ( BitCount <= 8 )
	{
		uint8 Data8;
		Read( Data8, BitCount );
		Data = Data8;
		return;
	}
	
	if ( BitCount <= 8 )
	{
		ReadBytes<1>( Data, BitCount );
		return;
	}
	
	if ( BitCount <= 16 )
	{
		ReadBytes<2>( Data, BitCount );
		return;
	}
	
	if ( BitCount <= 32 )
	{
		ReadBytes<4>( Data, BitCount );
		return;
	}
	
	std::stringstream Error;
	Error << __func__ << " not handling bit count > 32; " << BitCount;
	throw Soy::AssertException( Error.str() );
}


void TBitReader::Read(uint64& Data,size_t BitCount)
{
	if ( BitCount <= 8 )
	{
		ReadBytes<1>( Data, BitCount );
		return;
	}
	
	if ( BitCount <= 16 )
	{
		ReadBytes<2>( Data, BitCount );
		return;
	}
	
	if ( BitCount <= 32 )
	{
		ReadBytes<4>( Data, BitCount );
		return;
	}
	
	if ( BitCount <= 64 )
	{
		ReadBytes<8>( Data, BitCount );
		return;
	}
	
	std::stringstream Error;
	Error << __func__ << " not handling bit count > 32; " << BitCount;
	throw Soy::AssertException( Error.str() );
}

void TBitReader::Read(uint8& Data,size_t BitCount)
{
	if ( BitCount <= 0 )
		return;
	Soy::Assert( BitCount <= 8, "trying to read>8 bits to 8bit value");
	
	//	current byte
	auto CurrentByte = mBitPos / 8;
	auto CurrentBit = mBitPos % 8;
	
	//	out of range
	Soy::Assert( CurrentByte < mData.GetSize(), "Reading byte out of range");
	
	//	move along
	mBitPos += BitCount;
	
	//	get byte
	Data = mData[CurrentByte];
	
	//	pick out certain bits
	//	gr: reverse endianess to what I thought...
	//Data >>= CurrentBit;
	Data >>= 8-CurrentBit-BitCount;
	Data &= (1<<BitCount)-1;
}
/*

const unsigned char * m_pStart;
unsigned short m_nLength;
int m_nCurrentBit;

unsigned int ReadBit()
{
	assert(m_nCurrentBit <= m_nLength * 8);
	int nIndex = m_nCurrentBit / 8;
	int nOffset = m_nCurrentBit % 8 + 1;
	
	m_nCurrentBit ++;
	return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
}

unsigned int ReadBits(int n)
{
	int r = 0;
	int i;
	for (i = 0; i < n; i++)
	{
		r |= ( ReadBit() << ( n - i - 1 ) );
	}
	return r;
}

unsigned int ReadExponentialGolombCode()
{
	int r = 0;
	int i = 0;
	
	while( (ReadBit() == 0) && (i < 32) )
	{
		i++;
	}
	
	r = ReadBits(i);
	r += (1 << i) - 1;
	return r;
}

unsigned int ReadSE()
{
	int r = ReadExponentialGolombCode();
	if (r & 0x01)
	{
		r = (r+1)/2;
	}
	else
	{
		r = -(r/2);
	}
	return r;
}

void Parse(const unsigned char * pStart, unsigned short nLen)
{
	m_pStart = pStart;
	m_nLength = nLen;
	m_nCurrentBit = 0;
	
	int frame_crop_left_offset=0;
	int frame_crop_right_offset=0;
	int frame_crop_top_offset=0;
	int frame_crop_bottom_offset=0;
	
	int profile_idc = ReadBits(8);
	int constraint_set0_flag = ReadBit();
	int constraint_set1_flag = ReadBit();
	int constraint_set2_flag = ReadBit();
	int constraint_set3_flag = ReadBit();
	int constraint_set4_flag = ReadBit();
	int constraint_set5_flag = ReadBit();
	int reserved_zero_2bits  = ReadBits(2);
	int level_idc = ReadBits(8);
	int seq_parameter_set_id = ReadExponentialGolombCode();
	
	
	if( profile_idc == 100 || profile_idc == 110 ||
	   profile_idc == 122 || profile_idc == 244 ||
	   profile_idc == 44 || profile_idc == 83 ||
	   profile_idc == 86 || profile_idc == 118 )
	{
		int chroma_format_idc = ReadExponentialGolombCode();
		
		if( chroma_format_idc == 3 )
		{
			int residual_colour_transform_flag = ReadBit();
		}
		int bit_depth_luma_minus8 = ReadExponentialGolombCode();
		int bit_depth_chroma_minus8 = ReadExponentialGolombCode();
		int qpprime_y_zero_transform_bypass_flag = ReadBit();
		int seq_scaling_matrix_present_flag = ReadBit();
		
		if (seq_scaling_matrix_present_flag)
		{
			int i=0;
			for ( i = 0; i < 8; i++)
			{
				int seq_scaling_list_present_flag = ReadBit();
				if (seq_scaling_list_present_flag)
				{
					int sizeOfScalingList = (i < 6) ? 16 : 64;
					int lastScale = 8;
					int nextScale = 8;
					int j=0;
					for ( j = 0; j < sizeOfScalingList; j++)
					{
						if (nextScale != 0)
						{
							int delta_scale = ReadSE();
							nextScale = (lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					}
				}
			}
		}
	}
	
	int log2_max_frame_num_minus4 = ReadExponentialGolombCode();
	int pic_order_cnt_type = ReadExponentialGolombCode();
	if( pic_order_cnt_type == 0 )
	{
		int log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
	}
	else if( pic_order_cnt_type == 1 )
	{
		int delta_pic_order_always_zero_flag = ReadBit();
		int offset_for_non_ref_pic = ReadSE();
		int offset_for_top_to_bottom_field = ReadSE();
		int num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
		int i;
		for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
		{
			ReadSE();
			//sps->offset_for_ref_frame[ i ] = ReadSE();
		}
	}
	int max_num_ref_frames = ReadExponentialGolombCode();
	int gaps_in_frame_num_value_allowed_flag = ReadBit();
	int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
	int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
	int frame_mbs_only_flag = ReadBit();
	if( !frame_mbs_only_flag )
	{
		int mb_adaptive_frame_field_flag = ReadBit();
	}
	int direct_8x8_inference_flag = ReadBit();
	int frame_cropping_flag = ReadBit();
	if( frame_cropping_flag )
	{
		frame_crop_left_offset = ReadExponentialGolombCode();
		frame_crop_right_offset = ReadExponentialGolombCode();
		frame_crop_top_offset = ReadExponentialGolombCode();
		frame_crop_bottom_offset = ReadExponentialGolombCode();
	}
	int vui_parameters_present_flag = ReadBit();
	pStart++;
	
	int Width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
	int Height = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);
	
	printf("\n\nWxH = %dx%d\n\n",Width,Height);
	
}

*/
H264::TSpsParams H264::ParseSps(const ArrayBridge<uint8>&& Data)
{
	return ParseSps( Data );
}


H264::TSpsParams H264::ParseSps(const ArrayBridge<uint8>& Data)
{
	//	test against the working version from stackoverflow
	//Parse( Data.GetArray(), Data.GetDataSize() );
	
	TSpsParams Params;
	
	auto _DataSkipNaluByte = GetRemoteArray( Data.GetArray()+1, Data.GetDataSize()-1 );
	auto DataSkipNaluByte = GetArrayBridge( _DataSkipNaluByte );
	bool SkipNaluByte = false;
	try
	{
		H264NaluContent::Type Content;
		H264NaluPriority::Type Priority;
		DecodeNaluByte( Data[0], Content, Priority );
		SkipNaluByte = true;
	}
	catch (...)
	{
	}
	TBitReader Reader( SkipNaluByte ? DataSkipNaluByte : Data );
	
//	http://stackoverflow.com/questions/12018535/get-the-width-height-of-the-video-from-h-264-nalu
	uint8 Param8;
	Reader.Read( Param8, 8 );
	Params.mProfile = H264Profile::Validate( Param8 );
	
	Reader.Read( Params.mConstraintFlag[0], 1 );
	Reader.Read( Params.mConstraintFlag[1], 1 );
	Reader.Read( Params.mConstraintFlag[2], 1 );
	Reader.Read( Params.mConstraintFlag[3], 1 );
	Reader.Read( Params.mConstraintFlag[4], 1 );
	Reader.Read( Params.mConstraintFlag[5], 1 );
	
	Reader.Read( Params.mReservedZero, 2 );

	uint8 Level;
	Reader.Read( Level, 8 );
	Params.mLevel = H264::DecodeLevel( Level );
	
	Reader.ReadExponentialGolombCode( Params.seq_parameter_set_id );
	
	
	if( Params.mProfile == H264Profile::High ||
	   Params.mProfile == H264Profile::High10Intra ||
	   Params.mProfile == H264Profile::High422Intra ||
	   Params.mProfile == H264Profile::High4 ||
	   Params.mProfile == H264Profile::High5 ||
	   Params.mProfile == H264Profile::High6 ||
	   Params.mProfile == H264Profile::High7 ||
	   Params.mProfile == H264Profile::High8 ||
	   Params.mProfile == H264Profile::High9 ||
	   Params.mProfile == H264Profile::HighMultiviewDepth )
	{
		Reader.ReadExponentialGolombCode( Params.chroma_format_idc );
		
		if( Params.chroma_format_idc == 3 )
		{
			Reader.Read( Params.residual_colour_transform_flag, 1 );
		}
		
		Reader.ReadExponentialGolombCode(Params.bit_depth_luma_minus8 );
		Reader.ReadExponentialGolombCode(Params.bit_depth_chroma_minus8 );
		Reader.ReadExponentialGolombCode(Params.qpprime_y_zero_transform_bypass_flag );
		Reader.ReadExponentialGolombCode(Params.seq_scaling_matrix_present_flag );
		
		if (Params.seq_scaling_matrix_present_flag)
		{
			int i=0;
			for ( i = 0; i < 8; i++)
			{
				Reader.Read(Params.seq_scaling_list_present_flag,1);
				if (Params.seq_scaling_list_present_flag)
				{
					int sizeOfScalingList = (i < 6) ? 16 : 64;
					int lastScale = 8;
					int nextScale = 8;
					int j=0;
					for ( j = 0; j < sizeOfScalingList; j++)
					{
						if (nextScale != 0)
						{
							Soy_AssertTodo();
							//int delta_scale = ReadSE();
							//nextScale = (lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					}
				}
			}
		}
	}
	
	
	
	Reader.ReadExponentialGolombCode( Params.log2_max_frame_num_minus4 );
	Reader.ReadExponentialGolombCode( Params.pic_order_cnt_type );
	if ( Params.pic_order_cnt_type == 0 )
	{
		Reader.ReadExponentialGolombCode( Params.log2_max_pic_order_cnt_lsb_minus4 );
	}
	else if ( Params.pic_order_cnt_type == 1 )
	{
		Reader.Read( Params.delta_pic_order_always_zero_flag, 1 );
		
		Reader.ReadExponentialGolombCodeSigned( Params.offset_for_non_ref_pic );
		Reader.ReadExponentialGolombCodeSigned( Params.offset_for_top_to_bottom_field );
		Reader.ReadExponentialGolombCode( Params.num_ref_frames_in_pic_order_cnt_cycle );
		for( int i = 0; i <Params.num_ref_frames_in_pic_order_cnt_cycle; i++ )
		{
			sint32 Dummy;
			Reader.ReadExponentialGolombCodeSigned( Dummy );
			//sps->offset_for_ref_frame[ i ] = ReadSE();
		}
	}
	
	Reader.ReadExponentialGolombCode( Params.num_ref_frames );
	Reader.Read( Params.gaps_in_frame_num_value_allowed_flag, 1 );
	Reader.ReadExponentialGolombCode( Params.pic_width_in_mbs_minus_1 );
	Reader.ReadExponentialGolombCode( Params.pic_height_in_map_units_minus_1 );
	Reader.Read( Params.frame_mbs_only_flag, 1 );
	if( !Params.frame_mbs_only_flag )
	{
		Reader.Read( Params.mb_adaptive_frame_field_flag, 1 );
	}
	Reader.Read( Params.direct_8x8_inference_flag, 1 );
	Reader.Read( Params.frame_cropping_flag, 1 );
	if( Params.frame_cropping_flag )
	{
		Reader.ReadExponentialGolombCode( Params.frame_crop_left_offset );
		Reader.ReadExponentialGolombCode( Params.frame_crop_right_offset );
		Reader.ReadExponentialGolombCode( Params.frame_crop_top_offset );
		Reader.ReadExponentialGolombCode( Params.frame_crop_bottom_offset );
	}
	Reader.Read( Params.vui_prameters_present_flag, 1 );
	Reader.Read( Params.rbsp_stop_one_bit, 1 );
	
	Params.mWidth = ((Params.pic_width_in_mbs_minus_1 +1)*16) - Params.frame_crop_bottom_offset*2 - Params.frame_crop_top_offset*2;
	Params.mHeight = ((2 - Params.frame_mbs_only_flag)* (Params.pic_height_in_map_units_minus_1 +1) * 16) - (Params.frame_crop_right_offset * 2) - (Params.frame_crop_left_offset * 2);

	
	return Params;
}


void H264::SetSpsProfile(ArrayBridge<uint8>&& Data,H264Profile::Type Profile)
{
	Soy::Assert( Data.GetSize() > 0, "Not enough SPS data");
	auto& ProfileByte = Data[0];

	//	simple byte
	ProfileByte = size_cast<uint8>( Profile );
}

Soy::TVersion H264::DecodeLevel(uint8 Level8)
{
	//	show level
	//	https://github.com/ford-prefect/gst-plugins-bad/blob/master/sys/applemedia/vtdec.c
	//	http://stackoverflow.com/questions/21120717/h-264-video-wont-play-on-ios
	//	value is decimal * 10. Even if the data is bad, this should still look okay
	int Minor = Level8 % 10;
	int Major = (Level8-Minor) / 10;

	return Soy::TVersion( Major, Minor );
}

bool H264::IsKeyframe(H264NaluContent::Type Content)
{
	//	gr: maybe we should take priority into effect too?
	switch ( Content )
	{
		case H264NaluContent::Slice_CodedIDRPicture:
			return true;
			
		default:
			return false;
	}
}

bool H264::IsKeyframe(SoyMediaFormat::Type Format,const ArrayBridge<uint8>&& Data)
{
	try
	{
		H264NaluContent::Type Content;
		H264NaluPriority::Type Priority;
		DecodeNaluByte( Format, GetArrayBridge(Data), Content, Priority );
		return IsKeyframe( Content );
	}
	catch(...)
	{
		return false;
	}
}

