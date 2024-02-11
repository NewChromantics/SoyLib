#pragma once

#include "SoyMediaFormat.h"
#include <span>
#include "SoyH264.h"

//	h265, but to avoid programmer mistakes, we're using Hevc (vs h264 & h265)
namespace Hevc
{
	//	https://datatracker.ietf.org/doc/html/draft-schierl-payload-rtp-h265-01
	//	Nalu content type is the type in the 2 byte nalu-header
	//	0001|NaluContentType2|HevcData
	//	0|Typex5|000000|TIDx3
	namespace NaluContent
	{
		enum Type	//	6 bit
		{
			//	https://github.com/nokiatech/vpcc/blob/master/Sources/HEVC.h
			NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
			NAL_UNIT_CODED_SLICE_TRAIL_R = 1,

			NAL_UNIT_CODED_SLICE_TSA_N = 2,
			NAL_UNIT_CODED_SLICE_TSA_R = 3,

			NAL_UNIT_CODED_SLICE_STSA_N = 4,
			NAL_UNIT_CODED_SLICE_STSA_R = 5,

			NAL_UNIT_CODED_SLICE_RADL_N = 6,
			NAL_UNIT_CODED_SLICE_RADL_R = 7,

			NAL_UNIT_CODED_SLICE_RASL_N = 8,
			NAL_UNIT_CODED_SLICE_RASL_R = 9,

			NAL_UNIT_RESERVED_VCL_N10 = 10,
			NAL_UNIT_RESERVED_VCL_R11 = 11,
			NAL_UNIT_RESERVED_VCL_N12 = 12,
			NAL_UNIT_RESERVED_VCL_R13 = 13,
			NAL_UNIT_RESERVED_VCL_N14 = 14,
			NAL_UNIT_RESERVED_VCL_R15 = 15,

			NAL_UNIT_CODED_SLICE_BLA_W_LP = 16,
			NAL_UNIT_CODED_SLICE_BLA_W_RADL = 17,
			NAL_UNIT_CODED_SLICE_BLA_N_LP = 18,
			NAL_UNIT_CODED_SLICE_IDR_W_RADL = 19,
			NAL_UNIT_CODED_SLICE_IDR_N_LP = 20,
			NAL_UNIT_CODED_SLICE_CRA = 21,
			
			NAL_UNIT_RESERVED_IRAP_VCL22 = 22,
			NAL_UNIT_RESERVED_IRAP_VCL23 = 23,

			NAL_UNIT_RESERVED_VCL24 = 24,
			NAL_UNIT_RESERVED_VCL25 = 25,
			NAL_UNIT_RESERVED_VCL26 = 26,
			NAL_UNIT_RESERVED_VCL27 = 27,
			NAL_UNIT_RESERVED_VCL28 = 28,
			NAL_UNIT_RESERVED_VCL29 = 29,
			NAL_UNIT_RESERVED_VCL30 = 30,
			NAL_UNIT_RESERVED_VCL31 = 31,

			NAL_UNIT_VPS = 32,
			NAL_UNIT_SPS = 33,
			NAL_UNIT_PPS = 34,
			NAL_UNIT_ACCESS_UNIT_DELIMITER = 35,
			NAL_UNIT_EOS = 36,
			NAL_UNIT_EOB = 37,
			NAL_UNIT_FILLER_DATA = 38,
			NAL_UNIT_PREFIX_SEI = 39,
			NAL_UNIT_SUFFIX_SEI = 40,

			NAL_UNIT_RESERVED_NVCL41 = 41,
			NAL_UNIT_RESERVED_NVCL42 = 42,
			NAL_UNIT_RESERVED_NVCL43 = 43,
			NAL_UNIT_RESERVED_NVCL44 = 44,
			NAL_UNIT_RESERVED_NVCL45 = 45,
			NAL_UNIT_RESERVED_NVCL46 = 46,
			NAL_UNIT_RESERVED_NVCL47 = 47,

			NAL_UNIT_UNSPECIFIED_48 = 48,
			NAL_UNIT_UNSPECIFIED_49 = 49,
			NAL_UNIT_UNSPECIFIED_50 = 50,
			NAL_UNIT_UNSPECIFIED_51 = 51,
			NAL_UNIT_UNSPECIFIED_52 = 52,
			NAL_UNIT_UNSPECIFIED_53 = 53,
			NAL_UNIT_UNSPECIFIED_54 = 54,
			NAL_UNIT_UNSPECIFIED_55 = 55,
			NAL_UNIT_UNSPECIFIED_56 = 56,
			NAL_UNIT_UNSPECIFIED_57 = 57,
			NAL_UNIT_UNSPECIFIED_58 = 58,
			NAL_UNIT_UNSPECIFIED_59 = 59,
			NAL_UNIT_UNSPECIFIED_60 = 60,
			NAL_UNIT_UNSPECIFIED_61 = 61,
			NAL_UNIT_UNSPECIFIED_62 = 62,
			NAL_UNIT_UNSPECIFIED_63 = 63,

			NAL_UNIT_INVALID = 64,
			

			//	names to match h264
			Invalid	= NAL_UNIT_INVALID,
			EndOfStream = NAL_UNIT_EOS
		};

		DECLARE_SOYENUM(NaluContent);
	};
	namespace NaluTemporalId
	{
		enum Type
		{
			Invalid	= 0,
			One		= 1,
			Two		= 2,
			Three	= 3
		};
	};

	class PacketMeta_t
	{
	public:
		NaluContent::Type		mContentType = NaluContent::Invalid;
		int						mLayer = 0;
		NaluTemporalId::Type	mTemporalId = NaluTemporalId::Invalid;
	};
	PacketMeta_t				GetPacketMeta(std::span<uint8_t> Nalu,bool ExpectingNalu);
	inline NaluContent::Type	GetPacketType(std::span<uint8_t> Nalu,bool ExpectingNalu)	{	auto Meta = GetPacketMeta(Nalu,ExpectingNalu);	return Meta.mContentType;	}


	//	checks is nalu AND is valid h264 content
	bool				IsNaluHevc(std::span<uint8_t> Data);
}