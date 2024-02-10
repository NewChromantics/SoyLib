#include "SoyHevc.hpp"


namespace Hevc
{
namespace NaluContent
{
std::array BadTypes =
{
	NAL_UNIT_RESERVED_VCL_N10,
	NAL_UNIT_RESERVED_VCL_R11,
	NAL_UNIT_RESERVED_VCL_N12,
	NAL_UNIT_RESERVED_VCL_R13,
	NAL_UNIT_RESERVED_VCL_N14,
	NAL_UNIT_RESERVED_VCL_R15,
	NAL_UNIT_RESERVED_IRAP_VCL22,
	NAL_UNIT_RESERVED_IRAP_VCL23,
	NAL_UNIT_RESERVED_VCL24,
	NAL_UNIT_RESERVED_VCL25,
	NAL_UNIT_RESERVED_VCL26,
	NAL_UNIT_RESERVED_VCL27,
	NAL_UNIT_RESERVED_VCL28,
	NAL_UNIT_RESERVED_VCL29,
	NAL_UNIT_RESERVED_VCL30,
	NAL_UNIT_RESERVED_VCL31,
	NAL_UNIT_RESERVED_NVCL41,
	NAL_UNIT_RESERVED_NVCL42,
	NAL_UNIT_RESERVED_NVCL43,
	NAL_UNIT_RESERVED_NVCL44,
	NAL_UNIT_RESERVED_NVCL45,
	NAL_UNIT_RESERVED_NVCL46,
	NAL_UNIT_RESERVED_NVCL47,
	NAL_UNIT_UNSPECIFIED_48,
	NAL_UNIT_UNSPECIFIED_49,
	NAL_UNIT_UNSPECIFIED_50,
	NAL_UNIT_UNSPECIFIED_51,
	NAL_UNIT_UNSPECIFIED_52,
	NAL_UNIT_UNSPECIFIED_53,
	NAL_UNIT_UNSPECIFIED_54,
	NAL_UNIT_UNSPECIFIED_55,
	NAL_UNIT_UNSPECIFIED_56,
	NAL_UNIT_UNSPECIFIED_57,
	NAL_UNIT_UNSPECIFIED_58,
	NAL_UNIT_UNSPECIFIED_59,
	NAL_UNIT_UNSPECIFIED_60,
	NAL_UNIT_UNSPECIFIED_61,
	NAL_UNIT_UNSPECIFIED_62,
	NAL_UNIT_UNSPECIFIED_63,
};
}
}

Hevc::NaluContent::Type Hevc::GetPacketType(std::span<uint8_t> Data)
{
	auto HevcHeaderSize = 2;
	
	auto HeaderLength = H264::GetNaluLength(Data);
	if ( Data.size() <= HeaderLength + HevcHeaderSize )
		throw std::runtime_error("Not enough data provided for H264::GetPacketType()");

	//	Nalu content type is the type in the 2 byte nalu-header
	//	0001|NaluContentType2|HevcData
	//	0|Typex5|000000|TIDx3
	uint8_t HeaderA = Data[HeaderLength+0];
	uint8_t HeaderB = Data[HeaderLength+1];

	//	first bit, and last 2 are 0
	auto ForbiddenZeroA = HeaderA & (0x1);
	auto Type5 = (HeaderA >> 1) & 0x1f;	//	& 5 bits
	auto ReservedZeroA = HeaderA & (0xc0);

	//	first 4 are 0
	auto ReservedZeroB = HeaderB & (0xf);	//	4 bits
	auto Tid3 = (HeaderB >> 4) & 0x7;	//	& 3 bits

	if ( ForbiddenZeroA != 0 )
		throw std::runtime_error("Hevc Nalu header forbidden zero is not zero");
	if ( ReservedZeroA != 0 )
		throw std::runtime_error("Hevc Nalu header reserved A is not zero");
	if ( ReservedZeroB != 0 )
		throw std::runtime_error("Hevc Nalu header reserved B is not zero");

	
	auto TypeEnum = static_cast<Hevc::NaluContent::Type>(Type5);
	auto TidEnum = static_cast<Hevc::NaluTemporalId::Type>(Tid3);
	
	//	catch bad packet data
	if ( std::find( NaluContent::BadTypes.begin(), NaluContent::BadTypes.end(), TypeEnum ) != NaluContent::BadTypes.end() )
	{
		std::stringstream Error;
		Error << "Decoded invalid Hevc content type ";// << H264NaluContent::ToString(TypeEnum);
		throw std::runtime_error(Error.str());
	}
	
	return TypeEnum;
}
