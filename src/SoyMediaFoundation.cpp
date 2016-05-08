#include "SoyMediaFoundation.h"
#include <SoyDirectx.h>
#include <SoyMedia.h>

#include <Mferror.h>
#include <Codecapi.h>


#define DEFINE_GUID_CUSTOM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DEFINE_MEDIATYPE_GUID_CUSTOM(name, format) \
    DEFINE_GUID_CUSTOM(name,                       \
    format, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);


//	some fourcc's not declared in MediaFoundation
DEFINE_MEDIATYPE_GUID_CUSTOM( MFVideoFormat_VIDS,      FCC('vids') );
DEFINE_MEDIATYPE_GUID_CUSTOM( MFAudioFormat_AUDS,      FCC('auds') );


std::string GetFourCCString(uint32 MediaFormatFourCC)
{
	//	gr: these are in MFAPi.h, not sure why compiler isn't resolving them
#ifndef DIRECT3D_VERSION
#define D3DFMT_R8G8B8       20
#define D3DFMT_A8R8G8B8     21
#define D3DFMT_X8R8G8B8     22
#define D3DFMT_R5G6B5       23
#define D3DFMT_X1R5G5B5     24
#define D3DFMT_P8           41
#define LOCAL_D3DFMT_DEFINES 1
#endif	
	//	special cases
	switch ( MediaFormatFourCC )
	{
		case D3DFMT_R8G8B8:		return "D3DFMT_R8G8B8";
		case D3DFMT_A8R8G8B8:	return "D3DFMT_A8R8G8B8";
		case D3DFMT_X8R8G8B8:	return "D3DFMT_X8R8G8B8";
		case D3DFMT_R5G6B5:		return "D3DFMT_R5G6B5";
		case D3DFMT_X1R5G5B5:	return "D3DFMT_X1R5G5B5";
		case D3DFMT_P8:			return "D3DFMT_P8";
	}

	//	extract fourcc
	return Soy::FourCCToString( MediaFormatFourCC );
}	



const char* MediaGuidSpecialToString(REFGUID guid)
{
	if ( guid == MFVideoFormat_MPEG2 )		return "MFVideoFormat_MPEG2";
	if ( guid == MFVideoFormat_H264_ES )	return "MFVideoFormat_H264_ES";
	if ( guid == MFMediaType_Default )		return "Default";
	if ( guid == MFMediaType_Audio )		return "Audio";
	if ( guid == MFMediaType_Video )		return "Video";
	if ( guid == MFMediaType_Protected )	return "Protected";
	if ( guid == MFMediaType_SAMI )			return "SAMI";
	if ( guid == MFMediaType_Script )		return "Script";
	if ( guid == MFMediaType_Image )		return "Image";
	if ( guid == MFMediaType_HTML )			return "Html";
	if ( guid == MFMediaType_Binary )		return "Binary";
	if ( guid == MFMediaType_FileTransfer )	return "FileTransfer";
	if ( guid == MFImageFormat_JPEG )		return "MFImageFormat_JPEG";
	//	if ( guid == MFImageFormat_RGB32 )		return "MFImageFormat_RGB32";
	if ( guid == MFStreamFormat_MPEG2Transport )	return "MPEG2Transport";	//	.ts
	if ( guid == MFStreamFormat_MPEG2Program )		return "MPEG2Program";

	return nullptr;
}



bool GetMediaFormatGuidFourcc(GUID Guid,uint32& Part)
{
	//	data1 is the specific one. Pop it and if 0 matches Base then we know it's a MediaFormat guid
	Part = Guid.Data1;
	Guid.Data1 = 0;
	if ( Guid == MFVideoFormat_Base )
		return true;
	if ( Guid == MFAudioFormat_Base )
		return true;
	return false;
}



DWORD MediaFoundation::GetFourcc(SoyMediaFormat::Type Format)
{
	auto Guid = GetFormat( Format );
	uint32 Fourcc = 0;
	if ( !GetMediaFormatGuidFourcc( Guid, Fourcc ) )
		return 0;

	return Fourcc;
}


#define FORMAT_MAP(MajorFormat,MinorFormat,SoyFormat)	TPlatformFormatMap<GUID>( MajorFormat, MinorFormat, #MinorFormat, SoyFormat )
template<typename PLATFORMTYPE>
class TPlatformFormatMap
{
public:
	TPlatformFormatMap(PLATFORMTYPE MajorFormat,PLATFORMTYPE MinorFormat,const char* EnumName,SoyMediaFormat::Type SoyFormat) :
		mPlatformMajorFormat	( MajorFormat ),
		mPlatformMinorFormat	( MinorFormat ),
		mName					( EnumName ),
		mSoyFormat				( SoyFormat )
	{
		Soy::Assert( IsValid(), "Expected valid enum - or invalid enum is bad" );
	}
	TPlatformFormatMap() :
		mPlatformMajorFormat	( 0 ),
		mPlatformMinorFormat	( 0 ),
		mName					( "Invalid enum" ),
		mSoyFormat				( SoyPixelsFormat::Invalid )
	{
	}

	bool		IsValid() const		{	return mPlatformMinorFormat != PLATFORMTYPE();	}

	bool		operator==(const PLATFORMTYPE& Enum) const				{	return mPlatformMinorFormat == Enum;	}
	bool		operator==(const SoyPixelsFormat::Type& Format) const	{	return *this == SoyMediaFormat::FromPixelFormat(Format);	}
	bool		operator==(const SoyMediaFormat::Type& Format) const	{	return mSoyFormat == Format;	}

public:
	PLATFORMTYPE			mPlatformMajorFormat;
	PLATFORMTYPE			mPlatformMinorFormat;
	SoyMediaFormat::Type	mSoyFormat;
	std::string				mName;
};


//	gr: now fourcc's are in MediaFormat's, this should be irrelvent
TPlatformFormatMap<GUID> PlatformFormatMap[] =
{
	//	gr: these RGB[A] formats are BGRA for ENCODING... check decoder and remove RGBA options to force client to convert
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_RGB32,	SoyMediaFormat::RGBA ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_ARGB32,	SoyMediaFormat::ARGB ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_RGB32,	SoyMediaFormat::BGRA ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_RGB24,	SoyMediaFormat::RGB ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_RGB24,	SoyMediaFormat::BGR ),

	//	YUV format explanations
	//	https://msdn.microsoft.com/en-us/library/windows/desktop/aa370819(v=vs.85).aspx
	//	MFVideoFormat_NV11	NV11	4:1:1	Planar	8
	//	MFVideoFormat_NV12	NV12	4:2:0	Planar	8
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_NV12,	SoyMediaFormat::Yuv_8_88_Full ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_NV12,	SoyMediaFormat::Yuv_8_88_Ntsc ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_NV12,	SoyMediaFormat::Yuv_8_88_Smptec ),

	//	MFVideoFormat_YUY2	YUY2	4:2:2	Packed	8
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_YUY2,	SoyMediaFormat::YYuv_8888_Full ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_YUY2,	SoyMediaFormat::YYuv_8888_Ntsc ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_YUY2,	SoyMediaFormat::YYuv_8888_Smptec ),

	//	gr: not actually YYuv_8888?
	//	MFVideoFormat_UYVY	UYVY	4:2:2	Packed	8
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_IYUV,	SoyMediaFormat::YYuv_8888_Full ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_IYUV,	SoyMediaFormat::YYuv_8888_Ntsc ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_IYUV,	SoyMediaFormat::YYuv_8888_Smptec ),

	//	gr: not actually YYuv_8888
	//	MFVideoFormat_Y42T	Y42T	4:2:2	Packed	8
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_Y42T,	SoyMediaFormat::YYuv_8888_Full ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_Y42T,	SoyMediaFormat::YYuv_8888_Ntsc ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_Y42T,	SoyMediaFormat::YYuv_8888_Smptec ),

	//	from an apple sample movie
	//	http://www.fourcc.org/codecs.php
	//	YUV 4:2:2 CCIR 601 for V422 (no, I don't understand this either) 
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_VIDS,	SoyMediaFormat::Yuv_8_88_Full ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_VIDS,	SoyMediaFormat::Yuv_8_88_Ntsc ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_VIDS,	SoyMediaFormat::Yuv_8_88_Smptec ),


	/*
	//	alpha4 index4 ?
	//MFVideoFormat_AI44	AI44	4:4:4	Packed	Palettized
	//	alpha yuv a844?
	//MFVideoFormat_AYUV	AYUV	4:4:4	Packed	8
	//MFVideoFormat_I420	I420	4:2:0	Planar	8

	//	MFVideoFormat_IYUV	IYUV	4:2:0	Planar	8
	if ( Minor == MFVideoFormat_IYUV )
	return SoyMediaFormat::Yuv_8_44_Full;

	//	MFVideoFormat_Y41P	Y41P	4:1:1	Packed	8
	if ( Minor == MFVideoFormat_Y41P )
	return SoyMediaFormat::Yuv_822_Full;

	//	MFVideoFormat_Y41T	Y41T	4:1:1	Packed	8
	if ( Minor == MFVideoFormat_Y41P )
	return SoyMediaFormat::Yuv_822_Full;

	//	MFVideoFormat_YV12	YV12	4:2:0	Planar	8
	if ( Minor == MFVideoFormat_YV12 )
	return SoyMediaFormat::Yuv_8_44_Full;
	*/
	
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_MP4S,	SoyMediaFormat::Mpeg4 ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_H264,	SoyMediaFormat::H264_8 ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_H264,	SoyMediaFormat::H264_16 ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_H264,	SoyMediaFormat::H264_32 ),
	FORMAT_MAP( MFMediaType_Video,	MFVideoFormat_H264,	SoyMediaFormat::H264_ES ),

};



namespace MediaFoundation
{
	namespace Private
	{
		std::shared_ptr<TContext> Context;
	}
}



std::shared_ptr<MediaFoundation::TContext> MediaFoundation::GetContext()
{
	if ( !Private::Context )
	{
		Private::Context.reset(new TContext() );
	}
	return Private::Context;
}

void MediaFoundation::Shutdown()
{
	//	free last context
	if ( Private::Context.unique() )
		Private::Context.reset();
}

MediaFoundation::TContext::TContext()
{
	auto Result = MFStartup( MF_VERSION, MFSTARTUP_FULL );
	Directx::IsOkay(Result, "MFStartup");
}

MediaFoundation::TContext::~TContext()
{
	auto Result = MFShutdown();
	Directx::IsOkay(Result, "MFShutdown", false );
}


SoyMediaFormat::Type MediaFoundation::GetFormat(GUID Format)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return SoyMediaFormat::Invalid;

	return Meta->mSoyFormat;
}

SoyPixelsFormat::Type MediaFoundation::GetPixelFormat(GUID Format)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return SoyPixelsFormat::Invalid;

	return SoyMediaFormat::GetPixelFormat( Meta->mSoyFormat );
}

GUID MediaFoundation::GetFormat(SoyPixelsFormat::Type Format)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return GUID();

	return Meta->mPlatformMinorFormat;
}

GUID MediaFoundation::GetFormat(SoyMediaFormat::Type Format)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return GUID();

	return Meta->mPlatformMinorFormat;
}

SoyMediaFormat::Type MediaFoundation::GetFormat(GUID Major,GUID Minor,size_t H264NaluLengthSize)
{
	if (Major == MFMediaType_Audio)
	{
		uint32 Fourcc = 0;
		GetMediaFormatGuidFourcc( Minor, Fourcc );
		{
			auto Format = SoyMediaFormat::FromFourcc( Fourcc, H264NaluLengthSize );
			if ( Format != SoyMediaFormat::Invalid )
				return Format;
		}
	
		std::Debug << "Warning: audio media type not detected via fourcc (" << Minor << "/fourcc=" << Soy::FourCCToString(Fourcc) << ")" << std::endl;

		if ( Minor == MFAudioFormat_AUDS )
			return SoyMediaFormat::Audio_AUDS;

		if (Minor == MFAudioFormat_AAC)
			return SoyMediaFormat::Aac;

		if (Minor == MFAudioFormat_Float)
		{
			//throw Soy::AssertException("Gr: Verify MFAudioFormat_Float is PCM");
			return SoyMediaFormat::PcmLinear_float;
		}

		if (Minor == MFAudioFormat_PCM)
		{
			auto SampleByteSize = H264NaluLengthSize;
			switch (SampleByteSize)
			{
				case 1:		return SoyMediaFormat::PcmLinear_8;
				case 2:		return SoyMediaFormat::PcmLinear_16;
				default:	break;
			};
			std::stringstream Error;
			Error << "Cannot determine PCM format with sample size=" << SampleByteSize << std::endl;
			throw Soy::AssertException(Error.str());
		}
	}

	if ( Major == MFMediaType_Video )
	{
		//	gr: do fourcc method by default so we can remove all this code
		//	gr: on win7 with bjork, we get a fourcc of avc1, but the rest doesn't match MediaFormat Base, so this func fails, but secretly has what we want...s
		uint32 Fourcc = 0;
		GetMediaFormatGuidFourcc( Minor, Fourcc );
		{
			auto Format = SoyMediaFormat::FromFourcc( Fourcc, H264NaluLengthSize );
			if ( Format != SoyMediaFormat::Invalid )
				return Format;
		}

		std::Debug << "Warning: video media type not detected via fourcc (" << Minor << "/fourcc=" << Soy::FourCCToString(Fourcc) << ")" << std::endl;

		if ( Minor == MFVideoFormat_H264 )
		{
			switch( H264NaluLengthSize )
			{
				case 1:		return SoyMediaFormat::H264_8;
				case 2:		return SoyMediaFormat::H264_16;
				case 4:		return SoyMediaFormat::H264_32;
				default:	break;
			};
			std::stringstream Error;
			Error << "Cannot determine h264 format with nalulength=" << H264NaluLengthSize << std::endl;
			throw Soy::AssertException( Error.str() );
		}

		if ( Minor == MFVideoFormat_MPEG2 )
			return SoyMediaFormat::Mpeg2;

		if (  Minor == MFVideoFormat_H264_ES )
			return SoyMediaFormat::H264_ES;

		if ( Minor == MFStreamFormat_MPEG2Transport )
			return SoyMediaFormat::Mpeg2TS;

		//	apple sample 3gp movie comes out as this
		if ( Minor == MFVideoFormat_M4S2 )
			return SoyMediaFormat::Mpeg4;

		//	look for pixel formats
		auto PixelFormat = MediaFoundation::GetPixelFormat( Minor );
		if ( PixelFormat != SoyPixelsFormat::Invalid )
		{
			return SoyMediaFormat::FromPixelFormat( PixelFormat );
		}



	}

	std::string GuidString;
	{
		std::stringstream ss;
		ss << Major << "/" << Minor;
		GuidString = ss.str();
	}

	std::stringstream Error;
	Error << "Cannot determine media format from guids " << GuidString;
	throw Soy::AssertException( Error.str() );
}


std::ostream& operator<<(std::ostream& os,MFVideoTransferMatrix Mode)
{
	switch ( Mode )
	{
		case MFVideoTransferMatrix_BT709:		os << "MFVideoTransferMatrix_BT709";	break;
		case MFVideoTransferMatrix_BT601:		os << "MFVideoTransferMatrix_BT601";	break;
		case MFVideoTransferMatrix_SMPTE240M:	os << "MFVideoTransferMatrix_SMPTE240M";	break;
		case MFVideoTransferMatrix_Unknown:		os << "MFVideoTransferMatrix_Unknown";	break;
		default:
			os << "MFVideoTransferMatrix_<unknown:" << static_cast<int>(Mode) << ">";
			break;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, REFGUID guid)
{
	//	some other special guids
	if ( guid == MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID )
	{
		os << "MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID";
		return os;
	}

	if ( guid == MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID )
	{
		os << "MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID";
		return os;
	}


	//	media format guids are explicit
	{
		uint32 MediaFormatFourCC = 0;
		if ( GetMediaFormatGuidFourcc( guid, MediaFormatFourCC ) )
		{
			os << GetFourCCString( MediaFormatFourCC );
			return os;
		}
	}

	{
		auto* MediaTypeName = MediaGuidSpecialToString( guid );
		if ( MediaTypeName )
		{
			os << MediaTypeName;
			return os;
		}
	}

	OLECHAR Buffer[100] = {0};
	StringFromGUID2( guid, Buffer, sizeofarray(Buffer) );
	os << Buffer;
	/*
	os << std::uppercase;
	os.width(8);
	os << std::hex << guid.Data1 << '-';

	os.width(4);
	os << std::hex << guid.Data2 << '-';

	os.width(4);
	os << std::hex << guid.Data3 << '-';

	os.width(2);
	os << std::hex
	<< static_cast<short>(guid.Data4[0])
	<< static_cast<short>(guid.Data4[1])
	<< '-'
	<< static_cast<short>(guid.Data4[2])
	<< static_cast<short>(guid.Data4[3])
	<< static_cast<short>(guid.Data4[4])
	<< static_cast<short>(guid.Data4[5])
	<< static_cast<short>(guid.Data4[6])
	<< static_cast<short>(guid.Data4[7]);
	os << std::nouppercase;
	*/
	return os;
}



AutoReleasePtr<IMFMediaType> MediaFoundation::GetPlatformFormat(GUID MajorFormat,GUID MinorFormat)
{
	AutoReleasePtr<IMFMediaType> Type;
	auto Result = MFCreateMediaType( &Type.mObject );
	Directx::IsOkay( Result, "MFCreateMediaType" );

	Result = Type->SetGUID( MF_MT_MAJOR_TYPE, MajorFormat );
	Directx::IsOkay( Result, "MFCreateMediaType->SetGuid(MF_MT_MAJOR_TYPE)" );

	Result = Type->SetGUID( MF_MT_SUBTYPE, MinorFormat );
	Directx::IsOkay( Result, "MFCreateMediaType->SetGuid(MF_MT_SUBTYPE)" );

	return Type;
}


AutoReleasePtr<IMFMediaType> MediaFoundation::GetPlatformFormat(SoyMediaFormat::Type Format)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
	{
		std::stringstream Error;
		Error << __func__ << " failed to get meta for " << Format;
		throw Soy::AssertException( Error.str() );
	}

	return GetPlatformFormat( Meta->mPlatformMajorFormat, Meta->mPlatformMinorFormat );
}


AutoReleasePtr<IMFMediaType> MediaFoundation::GetPlatformFormat(SoyMediaFormat::Type Format,size_t Width,size_t Height)
{
	auto MediaFormat = GetPlatformFormat( Format );
	
	auto Result = MFSetAttributeSize( MediaFormat.mObject, MF_MT_FRAME_SIZE, Width, Height );   
	MediaFoundation::IsOkay( Result, "set MF_MT_FRAME_SIZE" );

	size_t PixelAspectRatio = 1;

	Result = MFSetAttributeRatio( MediaFormat.mObject, MF_MT_PIXEL_ASPECT_RATIO, PixelAspectRatio, 1 );   
	MediaFoundation::IsOkay( Result, "set MF_MT_PIXEL_ASPECT_RATIO" );

	return MediaFormat;
}

AutoReleasePtr<IMFMediaType> MediaFoundation::GetPlatformFormat(SoyPixelsFormat::Type Format)
{
	auto MediaFormat = SoyMediaFormat::FromPixelFormat( Format );
	return GetPlatformFormat( MediaFormat );
}


AutoReleasePtr<IMFSample> MediaFoundation::CreatePixelBuffer(TMediaPacket& Packet)
{
//#define WIN_7_MODE

#if defined(WIN_7_MODE)
	throw Soy::AssertException("Not supported in win7 build");
#else
	AutoReleasePtr<IMFMediaBuffer> pBuffer;

	//	make this true, and flip read in shader
	static bool BottomUp = true;
	auto Fourcc = MediaFoundation::GetFourcc( Packet.mMeta.mCodec );
	auto Result = MFCreate2DMediaBuffer( Packet.mMeta.mPixelMeta.GetWidth(), Packet.mMeta.mPixelMeta.GetHeight(), Fourcc, BottomUp, &pBuffer.mObject );
	MediaFoundation::IsOkay( Result, "MFCreate2DMediaBuffer" );
	pBuffer.Retain();

	auto WritePixelsToBuffer = [&pBuffer](const SoyPixelsImpl& Pixels)
	{
		auto& Buffer = *pBuffer.mObject;
		DWORD BufferSize = 0;
		DWORD BufferCurrentLength = 0;
		BYTE* BufferData = nullptr;
		Buffer.Lock( &BufferData, &BufferSize, &BufferCurrentLength );
		if ( BufferData == nullptr )
			throw Soy::AssertException("Failed to lock pixels for buffer");

		//	gr: buffer is often "pre-allocated"... not sure if I need to set current length before writing if I want to write more than current length?
		BufferCurrentLength = 0;

		auto WriteLength = BufferSize - BufferCurrentLength;
		auto& PixelsArray = Pixels.GetPixelsArray();
		auto* PixelsData = PixelsArray.GetArray();
		WriteLength = std::min<size_t>( WriteLength, PixelsArray.GetDataSize() );
		memcpy( &BufferData[BufferCurrentLength], PixelsData, WriteLength );
		auto SetResult = Buffer.SetCurrentLength( BufferCurrentLength + WriteLength );
		auto Result = Buffer.Unlock();
		IsOkay( Result, "Updating buffer current length");
		IsOkay( Result, "Unlocking buffer");
	};

	//	fill it
	if ( Packet.mPixelBuffer )
	{
		auto& PixelBuffer = *Packet.mPixelBuffer;
		float3x3 Transform;
		BufferArray<SoyPixelsImpl*,4> Pixels;
		PixelBuffer.Lock( GetArrayBridge(Pixels), Transform );
		if ( Pixels.IsEmpty() )
			throw Soy::AssertException("Failed to get pixels from buffer");
		try
		{
			WritePixelsToBuffer( *Pixels[0] );
			PixelBuffer.Unlock();
		}
		catch(...)
		{
			PixelBuffer.Unlock();
			throw;
		}
	}
	else if ( !Packet.mData.IsEmpty() )
	{
		auto& Data = Packet.mData;
		auto Meta = Packet.mMeta.mPixelMeta;
		SoyPixelsRemote Pixels( GetArrayBridge(Data), Meta );
		WritePixelsToBuffer( Pixels );
	}

	AutoReleasePtr<IMFSample> pSample;
	Result = MFCreateSample( &pSample.mObject );
	MediaFoundation::IsOkay( Result, "MFCreateSample" );
	pSample.Retain();

	Result = pSample.mObject->AddBuffer( pBuffer.mObject );
	MediaFoundation::IsOkay( Result, "AddBuffer" );

	return pSample;
#endif
}


AutoReleasePtr<IMFMediaType> MediaFoundation::GetPlatformFormat(TMediaEncoderParams Params,size_t Width,size_t Height)
{
	auto MfFormat = MediaFoundation::GetPlatformFormat( Params.mCodec, Width, Height );
	
	//	setup params
	auto& MediaFormat = *MfFormat.mObject;


	if ( Params.mAverageBitRate != 0 )
	{
		auto Result = MediaFormat.SetUINT32(MF_MT_AVG_BITRATE, Params.mAverageBitRate );   
		MediaFoundation::IsOkay( Result, "set MF_MT_AVG_BITRATE" );
	}
	
	{
		auto Result = MediaFormat.SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);   
		MediaFoundation::IsOkay( Result, "set MF_MT_INTERLACE_MODE" );
	}

	if ( Params.mFrameRate != 0 )
	{
		auto Result = MFSetAttributeRatio( MfFormat.mObject, MF_MT_FRAME_RATE, Params.mFrameRate, 1 );   
		MediaFoundation::IsOkay( Result, "set MF_MT_FRAME_RATE" );
	}

	static bool SetIndependent = false;
	if ( SetIndependent )
	{
		auto Result = MediaFormat.SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1 );   
		MediaFoundation::IsOkay( Result, "set MF_MT_ALL_SAMPLES_INDEPENDENT" );
	}

	//	check format and only apply if h264?
	if ( Params.mH264Profile != H264Profile::Invalid )
	{
		auto ProfileValue = static_cast<uint32>( Params.mH264Profile );
		auto Result = MediaFormat.SetUINT32(MF_MT_MPEG2_PROFILE, ProfileValue );   
		MediaFoundation::IsOkay( Result, "set MF_MT_MPEG2_PROFILE" );
	}

	return MfFormat;
}

