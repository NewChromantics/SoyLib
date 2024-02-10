#pragma once

#include "SoyMediaFormat.h"
#include <span>


namespace H264NaluContent
{
	enum Type
	{
		Invalid						= -1,
		Unspecified					= 0,
		Slice_NonIDRPicture			= 1,
		Slice_CodedPartitionA		= 2,
		Slice_CodedPartitionB		= 3,
		Slice_CodedPartitionC		= 4,
		Slice_CodedIDRPicture		= 5,
		SupplimentalEnhancementInformation	= 6,	//	Supplemental enhancement information
		SequenceParameterSet		= 7,
		PictureParameterSet			= 8,
		AccessUnitDelimiter			= 9,
		EndOfSequence				= 10,
		EndOfStream					= 11,
		FillerData					= 12,
		SequenceParameterSetExtension	= 13,
		Reserved14					= 14,
		Reserved15					= 15,
		Reserved16					= 16,
		Reserved17					= 17,
		Reserved18					= 18,
		Slice_AuxCodedUnpartitioned	= 19,
		Reserved20					= 20,
		Reserved21					= 21,
		Reserved22					= 22,
		Reserved23					= 23,
		
		//	gr: are these specific to RTP ?
		//	names from libav rtp_h264.c
		STAP_A						= 24,	//	one packet, multiple nals). is this just for RTP?
		STAP_B						= 25,
		MTAP_16						= 26,
		MTAP_24						= 27,
		FU_A						= 28,	// fragmented nal. Is this just for RTP?
		FU_B						= 29,
		Unspecified30				= 30,
		Unspecified31				= 31,
	};

	bool		IsValidContent(Type t);	//	return false for reserved, invalid etc
	
	DECLARE_SOYENUM(H264NaluContent);
}

namespace H264NaluPriority
{
	enum Type
	{
		Invalid		= -1,
		//	gr: find the proper names :)
		Important	= 3,
		Two			= 2,
		One			= 1,
		Zero		= 0,
	};
	DECLARE_SOYENUM(H264NaluPriority);
}


namespace H264Profile
{
	//	http://stackoverflow.com/questions/21120717/h-264-video-wont-play-on-ios
	//	file:///Users/grahamr/Downloads/T-REC-H.264-201304-S!!PDF-E%20(2).pdf	page 71
	enum Type
	{
		Invalid		= 0,
		Baseline	= 0x42,	//	66
		Main		= 0x4D,	//	77
		Extended	= 0x58,	//	88
		High		= 0x64,	//	100
		
		//	these were used in my SPS parsing, but I can't find what they correspond to...
		//	http://blog.mediacoderhq.com/h264-profiles-and-levels/
		High10Intra		= 0x6E,	//	110	10 bit intra... IF constraint3?
		High422Intra	= 0x7A,	//	122 4:2:2 intra... IF constraint3?
		High4		= 0xF4,	//	244
		High5		= 0x2C,	//	44 requires constraint_set3_flag
		High6		= 0x53,	//	83
		High7		= 0x56,	//	86
		
		High8		= 0x76,	//	118
		High9		= 0x80,	//	128
		
		HighMultiviewDepth	= 0x8A,	//	138


		//	more from/for MediaFoundation for MF_MT_MPEG2_PROFILE
		Profile_444        = 144,
	};

	//	gr: too many problems getting magic_enum to work with 0..255 (default is -128...128)
	//	https://github.com/Neargye/magic_enum/blob/master/doc/limitations.md
	//	phasing out magic_enum
	//DECLARE_SOYENUM(H264Profile);
	//	gr: this will throw if Value==H264Profile::Invalid
	Type		Validate(uint8_t Value);
}


namespace H264
{
	class TSpsParams;

	void UnitTest();		//	throws if any errors

	//	nalu prefix is same in h264 & Hevc
	namespace NaluPrefix
	{
		enum Type
		{
			AnnexB001	= 3,	//	byte length
			AnnexB0001	= 0,	//	magic number
			
			Eight		= 1,	//	keep 1,2,4 as the byte-length (for avf input)
			Sixteen		= 2,
			ThirtyTwo	= 4
		};
	}
	NaluPrefix::Type		GetNaluPrefix(std::span<uint8_t> Data);
	size_t					GetNaluLength(NaluPrefix::Type Prefix);
	size_t					GetNaluLength(std::span<uint8_t> Data);
	H264NaluContent::Type	GetPacketType(std::span<uint8_t> Data);
	void					ConvertNaluPrefix(std::vector<uint8_t>& Nalu,H264::NaluPrefix::Type NaluSize);
	size_t					GetNextNaluOffset(std::span<uint8_t> Data, size_t StartFrom = 3);	//	returns 0 if there is no next

	
	bool		ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>& Data);
	inline bool	ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)	{	return ResolveH264Format( Format, Data );	}
	void		ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);
	void		ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>& Data);
	inline void	ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>&& Data)	{	ConvertToFormat( DataFormat, NewFormat, Data);	}

	size_t		GetNaluLengthSize(SoyMediaFormat::Type Format);
	void		RemoveHeader(SoyMediaFormat::Type Format,ArrayBridge<uint8>&& Data,bool KeepNaluByte);
	size_t		FindNaluStartIndex(ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize);

	bool		IsNalu(const ArrayBridge<uint8>& Data,size_t& NaluSize,size_t& HeaderSize);
	inline bool	IsNalu(const ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize)	{	return IsNalu( Data, NaluSize, HeaderSize );	}
	
	uint8		EncodeNaluByte(H264NaluContent::Type Content,H264NaluPriority::Type Priority);
	void		DecodeNaluByte(uint8 Byte,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority);	//	throws on error (eg. reservered-zero not zero)
	void		DecodeNaluByte(SoyMediaFormat::Type Format,const ArrayBridge<uint8>&& Data,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority);	//	throws on error (eg. reservered-zero not zero)

	bool		IsKeyframe(H264NaluContent::Type Content) __noexcept;
	bool		IsKeyframe(SoyMediaFormat::Type Format,const ArrayBridge<uint8>&& Data) __noexcept;
	
	TSpsParams	ParseSps(std::span<uint8> SpsData);
	
	//	modify sps!
	Soy::TVersion	DecodeLevel(uint8 Level8);
	void			SetSpsProfile(ArrayBridge<uint8>&& Data,H264Profile::Type Profile);
	void			SetSpsLevel(ArrayBridge<uint8>&& Data,Soy::TVersion Level);
	
	void			SplitNalu(std::span<uint8_t> Data,std::function<void(std::span<uint8_t>)> OnNalu);
	std::vector<std::span<uint8_t>>	SplitNalu(std::span<uint8_t> Data);

	bool			StripEmulationPrevention(std::span<uint8_t> Data);	//	returns true if any data changed
}


class H264::TSpsParams
{
public:
	//	gr: I think there's a more complete class somewhere
	class Rect_t
	{
	public:
		size_t	Left = 0;
		size_t	Top = 0;
		size_t	Right = 0;
		size_t	Bottom = 0;
	};
	
public:
	size_t				mWidth = 0;
	size_t				mHeight= 0;
	Rect_t				mCroppedRect;
	H264Profile::Type	mProfile = H264Profile::Invalid;
	Soy::TVersion		mLevel;
	
	size_t				GetCroppedWidth() const	{	return mWidth - mCroppedRect.Right - mCroppedRect.Left;	}
	size_t				GetCroppedHeight() const	{	return mHeight - mCroppedRect.Bottom - mCroppedRect.Top;	}
	
	
	uint8		mConstraintFlag[6] = {0};
	uint8		mReservedZero = 0;
	uint32		seq_parameter_set_id = 0;
	uint32		chroma_format_idc = 0;
	uint8		residual_colour_transform_flag = 0;
	uint32		bit_depth_luma_minus8 = 0;
	uint32		bit_depth_chroma_minus8 = 0;
	uint32		qpprime_y_zero_transform_bypass_flag = 0;
	uint32		seq_scaling_matrix_present_flag = 0;
	uint32		seq_scaling_list_present_flag = 0;
	uint32		log2_max_frame_num_minus4 = 0;
	uint32		pic_order_cnt_type = 0;
	uint32		log2_max_pic_order_cnt_lsb_minus4 = 0;
	uint32		num_ref_frames = 0;
	uint32		gaps_in_frame_num_value_allowed_flag = 0;
	uint32		pic_width_in_mbs_minus_1 = 0;
	uint32		pic_height_in_map_units_minus_1 = 0;
	uint32		frame_mbs_only_flag = 0;
	uint32		direct_8x8_inference_flag = 0;
	uint32		frame_cropping_flag = 0;
	uint32		vui_prameters_present_flag = 0;
	uint32		rbsp_stop_one_bit = 0;
	uint8		mb_adaptive_frame_field_flag = 0;
	uint8		delta_pic_order_always_zero_flag = 0;
	uint32		frame_crop_left_offset = 0;
	uint32		frame_crop_right_offset = 0;
	uint32		frame_crop_top_offset = 0;
	uint32		frame_crop_bottom_offset = 0;
	
	sint32		offset_for_non_ref_pic = 0;
	sint32		offset_for_top_to_bottom_field = 0;
	uint32		num_ref_frames_in_pic_order_cnt_cycle = 0;
};

