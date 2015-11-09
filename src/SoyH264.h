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
	bool		ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>& Data);
	inline bool	ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data)	{	return ResolveH264Format( Format, Data );	}
	void		ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);
	void		ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>& Data);
	inline void	ConvertToFormat(SoyMediaFormat::Type& DataFormat,SoyMediaFormat::Type NewFormat,ArrayBridge<uint8>&& Data)	{	ConvertToFormat( DataFormat, NewFormat, Data);	}

	size_t		GetNaluLengthSize(SoyMediaFormat::Type Format);
	void		RemoveHeader(SoyMediaFormat::Type Format,ArrayBridge<uint8>&& Data);
	ssize_t		FindNaluStartIndex(ArrayBridge<uint8>&& Data,size_t& NaluSize);

	bool		IsNalu4(ArrayBridge<uint8>& Data);
	bool		IsNalu3(ArrayBridge<uint8>& Data);
	inline bool	IsNalu4(ArrayBridge<uint8>&& Data)	{	return IsNalu4( Data );	}
	inline bool	IsNalu3(ArrayBridge<uint8>&& Data)	{	return IsNalu3( Data );	}
	
	uint8		EncodeNaluByte(H264NaluContent::Type Content,H264NaluPriority::Type Priority);
	void		DecodeNaluByte(uint8 Byte,H264NaluContent::Type& Content,H264NaluPriority::Type& Priority);	//	throws on error (eg. reservered-zero not zero)
}

