#pragma once

#include <SoyMedia.h>


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
		Unspecified24				= 24,
		Unspecified25				= 25,
		Unspecified26				= 26,
		Unspecified27				= 27,
		Unspecified28				= 28,
		Unspecified29				= 29,
		Unspecified30				= 30,
		Unspecified31				= 31,
	};
	
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



namespace H264
{
	class TSpsParams;
	
	bool		ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>& Data);
	inline bool	ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)	{	return ResolveH264Format( Format, Data );	}
	void		ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);
	void		ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>& Data);
	inline void	ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>&& Data)	{	ConvertToFormat( DataFormat, NewFormat, Data);	}

	size_t		GetNaluLengthSize(SoyMediaFormat::Type Format);
	void		RemoveHeader(SoyMediaFormat::Type Format,ArrayBridge<uint8>&& Data,bool KeepNaluByte);
	ssize_t		FindNaluStartIndex(ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize);

	bool		IsNalu(ArrayBridge<uint8>& Data,size_t& NaluSize,size_t& HeaderSize);
	inline bool	IsNalu(ArrayBridge<uint8>&& Data,size_t& NaluSize,size_t& HeaderSize)	{	return IsNalu( Data, NaluSize, HeaderSize );	}
	
	uint8		EncodeNaluByte(H264NaluContent::Type Content,H264NaluPriority::Type Priority);
	void		DecodeNaluByte(uint8 Byte,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority);	//	throws on error (eg. reservered-zero not zero)
	
	TSpsParams	ParseSps(const ArrayBridge<uint8>&& Data);
}


class H264::TSpsParams
{
public:
	TSpsParams()
	{
		memset( this, 0, sizeof(*this) );
	}
	size_t		mWidth;
	size_t		mHeight;
	uint8		mProfile;
	uint8		mConstraintFlag[6];
	uint8		mReservedZero;
	uint8		mLevel;
	uint32		seq_parameter_set_id;
	uint32		chroma_format_idc;
	uint8		residual_colour_transform_flag;
	uint32		bit_depth_luma_minus8;
	uint32		bit_depth_chroma_minus8;
	uint32		qpprime_y_zero_transform_bypass_flag;
	uint32		seq_scaling_matrix_present_flag;
	uint32		seq_scaling_list_present_flag;
	uint32		log2_max_frame_num_minus4;
	uint32		pic_order_cnt_type;
	uint32		log2_max_pic_order_cnt_lsb_minus4;
	uint32		num_ref_frames;
	uint32		gaps_in_frame_num_value_allowed_flag;
	uint32		pic_width_in_mbs_minus_1;
	uint32		pic_height_in_map_units_minus_1;
	uint32		frame_mbs_only_flag;
	uint32		direct_8x8_inference_flag;
	uint32		frame_cropping_flag;
	uint32		vui_prameters_present_flag;
	uint32		rbsp_stop_one_bit;
	uint8		mb_adaptive_frame_field_flag;
	uint8		delta_pic_order_always_zero_flag;
	uint32		frame_crop_left_offset;
	uint32		frame_crop_right_offset;
	uint32		frame_crop_top_offset;
	uint32		frame_crop_bottom_offset;
	
	int32		offset_for_non_ref_pic;
	int32		offset_for_top_to_bottom_field;
	uint32		num_ref_frames_in_pic_order_cnt_cycle;
};
