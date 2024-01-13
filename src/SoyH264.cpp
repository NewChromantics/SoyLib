#include "SoyH264.h"


H264Profile::Type H264Profile::Validate(uint8_t Value)
{
	switch(Value)
	{
		case Baseline:
		case Main:
		case High:
		case High10Intra:
		case High422Intra:
		case High4:
		case High5:
		case High6:
		case High7:
		case High8:
		case High9:
		case HighMultiviewDepth:
		case Profile_444:
			return static_cast<Type>(Value);
			
			//	unknown
		default:break;
	}
	
	std::stringstream Error;
	Error << "Unknown/Invalid H264Profile value 0x" << std::hex << Value;
	throw std::runtime_error(Error.str());
}

template <typename T>
T SwapEndian(T Value)
{
	//	MSVCC
	//	unsigned long _byteswap_ulong(unsigned long value);
	//	GCC
	//uint32_t __builtin_bswap32 (uint32_t x)
	static_assert(std::is_trivially_copyable<T>::value, "SwapEndian only for POD types");
	auto* Start = reinterpret_cast<uint8_t*>(&Value);
	auto* End = Start + sizeof(T);
	std::reverse(Start, End);
	return Value;
}


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
	//Error << __func__ << " unhandled format " << Format;
	Error << __func__ << " unhandled format ";
	throw Soy::AssertException( Error.str() );
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
	Error << __func__ << " trying to trim header from non h264 format ";// << Format;
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


size_t H264::FindNaluStartIndex(ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize)
{
	//	look for start of nalu
	for ( ssize_t i=0;	i<Data.GetSize();	i++ )
	{
		auto ChunkPart = GetRemoteArray( &Data[i], Data.GetSize()-i );

		if ( !H264::IsNalu( GetArrayBridge(ChunkPart), NaluSize, HeaderSize ) )
			continue;
		
		return i;
	}
	throw Soy::AssertException("Failed to find NALU start");
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
	if ( Zero!=0 )
		throw std::runtime_error("Nalu zero bit non-zero");
	//	catch bad cases. look out for genuine cases, but if this is zero, NALU delin might have been read wrong
	if ( Content8==0 )
		throw std::runtime_error("Nalu content type is invalid (zero)");

	//	swich this for magic_enum
	//Priority = H264NaluPriority::Validate( Idc );
	//Content = H264NaluContent::Validate( Content8 );
	Priority = static_cast<H264NaluPriority::Type>( Idc );
	Content = static_cast<H264NaluContent::Type>( Content8 );
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
		mData	( const_cast<uint8_t*>(Data.GetArray()), Data.GetDataSize() )
	{
	}
	TBitReader(const ArrayBridge<uint8>&& Data) :
		mData	( const_cast<uint8_t*>(Data.GetArray()), Data.GetDataSize() )
	{
	}
	TBitReader(std::span<uint8> Data) :
		mData	( Data )
	{
	}

	bool			ReadBit();
	void			Read(uint32& Data,size_t BitCount);
	void			Read(uint64& Data,size_t BitCount);
	void			Read(uint8& Data,size_t BitCount);
	size_t			BitPosition() const					{	return mBitPos;	}
	
	template<int BYTECOUNT,typename STORAGE>
	void			ReadBytes(STORAGE& Data,size_t BitCount);

	void			ReadExponentialGolombCode(uint32& Data);
	void			ReadExponentialGolombCodeSigned(sint32& Data);
	
private:
	std::span<uint8_t>	mData;
	
	//	current bit-to-read/write-pos (the tail).
	//	This is absolute, so we get the current byte from this value
	//	it also means this class is limited to (32/64bit max / 8) byte-sized data
	size_t				mBitPos = 0;
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

bool TBitReader::ReadBit()
{
	uint8_t Byte = 0;
	Read( Byte, 1 );
	return Byte == 1;
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
	if ( BitCount > 8 )
		throw std::runtime_error("trying to read>8 bits to 8bit value");
	
	
	//	if we're on bit [7] and ask for 2 bits, we're going to overflow the byte
	//	and the code below doesn't handle that.
	//	quick hack, just read bit by bit
	//	todo: fix properly
	{
		auto CurrentBit = (mBitPos) % 8;
		auto EndBit = (mBitPos+BitCount-1) % 8;
		if ( EndBit < CurrentBit )
		{
			Data = 0;
			for ( auto b=0;	b<BitCount;	b++ )
			{
				uint8_t BitValue = ReadBit() ? (1<<b) : 0;
				Data |= BitValue;
			}
			return;
		}
	}
	
	
	//	current byte
	auto CurrentByte = mBitPos / 8;
	auto CurrentBit = mBitPos % 8;
	
	//	out of range
	if ( CurrentByte >= mData.size() )
		throw std::runtime_error("Reading byte out of range");
	
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


void read_scaling_list(TBitReader& b, int* scalingList, int sizeOfScalingList, bool& useDefaultScalingMatrixFlag )
{
	// NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
	int lastScale = 8;
	int nextScale = 8;
	int delta_scale;
	for( int j = 0; j < sizeOfScalingList; j++ )
	{
		if( nextScale != 0 )
		{
			if( false )
			{
				nextScale = scalingList[ j ];
				if (useDefaultScalingMatrixFlag)
				{
					nextScale = 0;
				}
				delta_scale = (nextScale - lastScale) % 256 ;
			}

			//delta_scale = bs_read_se(b);
			b.ReadExponentialGolombCodeSigned(delta_scale);

			if ( true )
			{
				nextScale = ( lastScale + delta_scale + 256 ) % 256;
				useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
			}
		}
		if( true )
		{
			scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
		}
		lastScale = scalingList[ j ];
	}
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



//	gr: I feel like this code is from https://github.com/aizvorski/h264bitstream
//	todo: re-use that lib instead of this copy+pasta
H264::TSpsParams H264::ParseSps(std::span<uint8> Data)
{
	//	test against the working version from stackoverflow
	//Parse( Data.GetArray(), Data.GetDataSize() );
	
	TSpsParams Params;
	
	//	gr: cleaning this code may have broken something somewhere, but an error
	//		should make updating code much easier
	//	read out content type, if it's not sps, we're probably providing data with a Nalu prefix,
	//	which should be stripped
	H264NaluContent::Type Content = H264NaluContent::Invalid;
	H264NaluPriority::Type Priority = H264NaluPriority::Invalid;
	DecodeNaluByte( Data[0], Content, Priority );
	if ( Content != H264NaluContent::SequenceParameterSet )
	{
		std::stringstream Error;
		Error << "ParseSps(x" << Data.size() << ") is not SPS (is " << Content << "). Maybe data is prefixed with nalu";
		throw std::runtime_error(Error.str());
	}

	//	SPS data comes after the content/priority
	//	https://www.cardinalpeak.com/blog/the-h-264-sequence-parameter-set
	TBitReader Reader( Data.subspan(1) );
	
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
			/*
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
							//throw std::runtime_error("todo: parse SPS scaling matrix");
							sint32 delta_scale = 0;
							Reader.ReadExponentialGolombCodeSigned(delta_scale);
							nextScale = (lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					}
				}
			}
			 */
			//	https://github.com/aizvorski/h264bitstream/blob/ae72f7395f328876199a7e928d3b4a6dc6a7ce14/h264_stream.c#L396C9-L415C10
			if( Params.seq_scaling_matrix_present_flag )
			{
				auto MatrixSize = ((Params.chroma_format_idc != 3) ? 8 : 12);
				for( int i = 0; i < MatrixSize; i++ )
				{
					auto seq_scaling_list_present_flag_i = Reader.ReadBit();
					if( seq_scaling_list_present_flag_i )
					{
						//	gr: warning, not initialised in bitstream repos
						int ScalingList4x4[6][16];
						int ScalingList8x8[6][64];
						if( i < 6 )
						{
							
							//	https://github.com/search?q=repo%3Aaizvorski%2Fh264bitstream%20UseDefaultScalingMatrix4x4Flag&type=code
							//	gr: this seems to never be initialised in reference code
							//&( sps->UseDefaultScalingMatrix4x4Flag[ i ] ) );
							bool UseDefaultScalingFlag = false;
							read_scaling_list( Reader, ScalingList4x4[ i ], 16, UseDefaultScalingFlag );
						}
						else
						{
							//	gr: this seems to never be initialised in reference code
							//&sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ]
							bool UseDefaultScalingFlag = false;
							
							read_scaling_list( Reader, ScalingList8x8[ i - 6 ], 64, UseDefaultScalingFlag );
						}
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
	
	Params.mWidth = ((Params.pic_width_in_mbs_minus_1 +1)*16);
	Params.mHeight = ((2 - Params.frame_mbs_only_flag)* (Params.pic_height_in_map_units_minus_1 +1) * 16);

	//	gr: check this...
	//	- why is it two (always aligned?)
	//	- is bottom correct given textures start at 0,0=topleft or 0,0=bottomleft
	Params.mCroppedRect.Left = Params.frame_crop_left_offset * 2;
	Params.mCroppedRect.Top = Params.frame_crop_top_offset * 2;
	Params.mCroppedRect.Right = Params.frame_crop_right_offset * 2;
	Params.mCroppedRect.Bottom = Params.frame_crop_bottom_offset * 2;

	
	return Params;
}


void H264::SetSpsProfile(ArrayBridge<uint8>&& Data,H264Profile::Type Profile)
{
	if ( Data.GetSize() == 0 )
		throw std::runtime_error("Not enough SPS data");
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

bool H264::IsKeyframe(H264NaluContent::Type Content) __noexcept
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

bool H264::IsKeyframe(SoyMediaFormat::Type Format,const ArrayBridge<uint8>&& Data) __noexcept
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




void H264::ConvertNaluPrefix(std::vector<uint8_t>& PacketData,H264::NaluPrefix::Type TargetType)
{
	//	detect current prefix
	auto CurrentType = GetNaluPrefix(PacketData);
	
	//	already correct
	if ( CurrentType == TargetType )
		return;

	//	gr: if there are multiple packets, we're going to get a mix of prefixes
	{
		auto SubPackets = SplitNalu( PacketData );
		if ( SubPackets.size() > 1 )
			throw std::runtime_error("todo: handle multiple nalu packets when changing prefix");
	}
	
	auto CurrentPrefixLength = GetNaluLength(CurrentType);
	{
		//	todo: read length from the prefix of 8/16/32 bit length and check this matches the sub-data
		//	scoped as this span quickly goes out of date when we resize the data
		auto Data = std::span(PacketData).subspan( CurrentPrefixLength );
	}
	
	auto OverwriteLength32 =[&]()
	{
		//	gr: the length needs to be big endian in avcc, so swap.
		//		the length should also not include the prefix.
		auto DataSize = PacketData.size() - 4;
		auto DataSizeBigEndian = SwapEndian<uint32_t>(DataSize);
		
		auto* PacketDataSize = reinterpret_cast<uint32_t*>( std::span(PacketData).data() );
		*PacketDataSize = DataSizeBigEndian;
	};
	
	//	just for simplicity, just do each conversion
	if ( CurrentType == NaluPrefix::AnnexB001 && TargetType == NaluPrefix::ThirtyTwo )
	{
		//	change prefix from 3 to 4 byte
		PacketData.insert( PacketData.begin(), 0 );
		OverwriteLength32();
	}
	else if ( CurrentType == NaluPrefix::AnnexB0001 && TargetType == NaluPrefix::ThirtyTwo )
	{
		OverwriteLength32();
	}
	else
	{
		throw std::runtime_error("Unhandled nalu conversion combination");
	}
}

size_t H264::GetNextNaluOffset(std::span<uint8_t> Data, size_t StartFrom)
{
	if ( Data.size() < 4 )
		return 0;
	
	//	detect 001
	auto* DataPtr = Data.data();
	
	for ( int i=size_cast<int>(StartFrom);	i< Data.size()-3;	i++ )
	{
		if (DataPtr[i + 0] != 0)	continue;
		if (DataPtr[i + 1] != 0)	continue;
		if (DataPtr[i + 2] != 1)	continue;
		
		//	check i-1 for 0 in case it's 0001 rather than 001
		if (DataPtr[i - 1] == 0)
			return i - 1;
		
		return i;
	}
	
	return 0;
}


//	returns true if any data changed
bool H264::StripEmulationPrevention(std::vector<uint8_t>& Data)
{
	bool Changed = false;
	
	//	https://stackoverflow.com/a/24890903/355753
	//	this is when some perfectly valid data contains 0 0 0
	//	so to prevent this being detected as 001 or 0001 emulation
	//	prevention is inserted to turn it into 003 or 0030
	for ( auto i=0;	i<Data.size()-2;	i++ )
	{
		auto& a = Data[i+0];
		auto& b = Data[i+1];
		auto& c = Data[i+2];
		
		//	looking for 003
		if ( a == 0 && b == 0 && c == 3 )
		{
			//	change to 000
			c = 0;
			Changed = true;
		}
	}
	
	return Changed;
}

std::vector<std::span<uint8_t>> H264::SplitNalu(std::span<uint8_t> Data)
{
	std::vector<std::span<uint8_t>> Packets;
	
	auto OnFoundPacket = [&](std::span<uint8_t> Packet)
	{
		Packets.push_back(Packet);
	};
	
	SplitNalu( Data, OnFoundPacket );
	return Packets;
}


void H264::SplitNalu(std::span<uint8_t> Data,std::function<void(std::span<uint8_t>)> OnNalu)
{
	//	gr: this was happening in android test app, GetSubArray() will throw, so catch it
	if ( Data.empty() )
	{
		std::Debug << "Unexpected " << __PRETTY_FUNCTION__ << " Data.Size=" << Data.size() << std::endl;
		return;
	}
	
	//	split up packet if there are multiple nalus
	size_t PrevNalu = 0;
	while (true)
	{
		auto PacketData = Data.subspan(PrevNalu);
		auto NextNalu = H264::GetNextNaluOffset( PacketData );
		if (NextNalu == 0)
		{
			//	everything left
			OnNalu( PacketData );
			break;
		}
		else
		{
			auto SubArray = PacketData.subspan(0, NextNalu);
			OnNalu( SubArray );
			PrevNalu += NextNalu;
		}
	}
}



H264::NaluPrefix::Type H264::GetNaluPrefix(std::span<uint8_t> Data)
{
	//	todo: test for u8/u16/u32 size prefix
	if ( Data.size() < 4 )
		throw std::runtime_error("Data too small to determine packet size");
	
	auto p0 = Data[0];
	auto p1 = Data[1];
	auto p2 = Data[2];
	auto p3 = Data[3];
	
	if ( p0 == 0 && p1 == 0 && p2 == 1 )
		return H264::NaluPrefix::AnnexB001;
	
	if ( p0 == 0 && p1 == 0 && p2 == 0 && p3 == 1)
		return H264::NaluPrefix::AnnexB0001;
	
	throw std::runtime_error("todo: detect nalu prefix that's not 001 or 0001");
}


size_t H264::GetNaluLength(NaluPrefix::Type Prefix)
{
	switch( Prefix )
	{
		case NaluPrefix::AnnexB0001:
			return 4;
			
		case NaluPrefix::AnnexB001:
		case NaluPrefix::Eight:
		case NaluPrefix::Sixteen:
		case NaluPrefix::ThirtyTwo:
			return static_cast<size_t>( Prefix );
			
		default:
			throw std::runtime_error("Unknown nalu prefix type");
	}
}

size_t H264::GetNaluLength(std::span<uint8_t> Data)
{
	auto Type = GetNaluPrefix(Data);
	return GetNaluLength( Type );
}


H264NaluContent::Type H264::GetPacketType(std::span<uint8_t> Data)
{
	auto HeaderLength = GetNaluLength(Data);
	if ( Data.size() <= HeaderLength )
		throw std::runtime_error("Not enough data provided for H264::GetPacketType()");

	auto TypeAndPriority = Data[HeaderLength];
	auto Type = TypeAndPriority & 0x1f;
	//auto Priority = TypeAndPriority >> 5;
	
	auto TypeEnum = static_cast<H264NaluContent::Type>(Type);
	return TypeEnum;
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
			if ( ChunkLength > Data.GetDataSize() )
				throw std::runtime_error(Error.str());
		}
		
		InsertChunk( ChunkLength, Data, Position );
		Position += ChunkLength;
	}
}


void H264::UnitTest()
{
	//	magic_enum wasn't parsing this correctly
	{
		const uint8_t ProfileInput = 0xf4;	//	f4 = high
		auto Profile = H264Profile::Validate(ProfileInput);
		if ( Profile != H264Profile::High4 )
			throw std::runtime_error("Failed to parse 0xf4/High profile value");
	}
		
}
