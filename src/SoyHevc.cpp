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

Hevc::NaluContent::Type Hevc::GetPacketType(std::span<uint8_t> Data,bool ExpectingNalu)
{
	auto HeaderSize = 2;
	auto NaluSize = ExpectingNalu ? H264::GetNaluLength(Data) : 0;
	
	if ( Data.size() <= NaluSize + HeaderSize )
		throw std::runtime_error("Not enough data provided for H264::GetPacketType()");

	auto HeaderStart = NaluSize;
	
	TBitReader Reader( Data.subspan(HeaderStart,2) );
	auto ForbiddenZero = Reader.ReadBit();
	auto ContentType = Reader.Read(6);
	auto Layer = Reader.Read(6);
	auto TemporalIdPlusOne = Reader.Read(3);

	if ( ForbiddenZero != 0 )
		throw std::runtime_error("Hevc Nalu header forbidden zero is not zero");
	//if ( Layer != 0 )
	//	throw std::runtime_error("Hevc Nalu header Layer is not zero");
	if ( TemporalIdPlusOne == 0 )
		throw std::runtime_error("TemporalIdPlusOne is zero");

	
	auto TypeEnum = static_cast<Hevc::NaluContent::Type>(ContentType);
	auto TidEnum = static_cast<Hevc::NaluTemporalId::Type>(TemporalIdPlusOne);
	
	//	catch bad packet data
	if ( std::find( NaluContent::BadTypes.begin(), NaluContent::BadTypes.end(), TypeEnum ) != NaluContent::BadTypes.end() )
	{
		std::stringstream Error;
		Error << "Decoded invalid Hevc content type ";// << H264NaluContent::ToString(TypeEnum);
		throw std::runtime_error(Error.str());
	}
	
	return TypeEnum;
}

bool Hevc::IsNaluHevc(std::span<uint8_t> Data)
{
	try 
	{
		auto NaluType = H264::GetNaluPrefix(Data);
		auto NaluLength = H264::GetNaluLength(NaluType);
		auto HevcData = Data.subspan( NaluLength );
		bool ExpectingNalu = false;
		auto HevcType = GetPacketType(HevcData,ExpectingNalu);
		return true;
	}
	catch(std::exception& e)
	{
		std::cerr << "Is not nalu+hevc data; " << e.what() << std::endl;
		return false;
	}
}
