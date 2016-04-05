#include "SoyMediaFormat.h"


//	android sdk define
#define MIMETYPE_AUDIO_RAW	"audio/raw"


namespace Mime
{
	const char*	Aac_Android = "audio/mp4a-latm";
	const char*	Aac_Other = "audio/aac";
	const char*	Aac_x = "audio/x-aac";
#if defined(TARGET_ANDROID)
	const char*	Aac_Default = Aac_Android;
#else
	const char*	Aac_Default = Aac_Other;
#endif
}



std::map<SoyMediaFormat::Type,std::string> SoyMediaFormat::EnumMap =
{
	{ SoyMediaFormat::Invalid,			"Invalid" },
	{ SoyMediaFormat::H264_8,			"H264_8" },
	{ SoyMediaFormat::H264_16,			"H264_16" },
	{ SoyMediaFormat::H264_32,			"H264_32" },
	{ SoyMediaFormat::H264_ES,			"H264_ES" },
	{ SoyMediaFormat::H264_SPS_ES,		"H264_SPS_ES" },
	{ SoyMediaFormat::H264_PPS_ES,		"H264_PPS_ES" },
	{ SoyMediaFormat::Mpeg2TS,			"Mpeg2TS" },
	{ SoyMediaFormat::Mpeg2TS_PSI,		"Mpeg2TS_PSI" },
	{ SoyMediaFormat::Mpeg2,			"Mpeg2" },
	{ SoyMediaFormat::Mpeg4,			"Mpeg4" },
	{ SoyMediaFormat::Mpeg4_v3,			"Mpeg4_v3" },
	{ SoyMediaFormat::VC1,				"VC1" },
	{ SoyMediaFormat::Audio_AUDS,		"Audio_AUDS" },
	{ SoyMediaFormat::Wave,				"wave" },
	{ SoyMediaFormat::Aac,				"aac" },
	{ SoyMediaFormat::Ac3,				"Ac3" },
	{ SoyMediaFormat::Mpeg2Audio,		"Mpeg2Audio" },
	{ SoyMediaFormat::Mp3,				"Mp3" },
	{ SoyMediaFormat::Dts,				"Dts" },
	{ SoyMediaFormat::PcmAndroidRaw,	"PcmAndroidRaw" },
	{ SoyMediaFormat::PcmLinear_8,		"PcmLinear_8" },
	{ SoyMediaFormat::PcmLinear_16,		"PcmLinear_16" },
	{ SoyMediaFormat::PcmLinear_20,		"PcmLinear_20" },
	{ SoyMediaFormat::PcmLinear_24,		"PcmLinear_24" },
	{ SoyMediaFormat::Text,				"text" },
	{ SoyMediaFormat::Json,				"Json" },
	{ SoyMediaFormat::Html,				"Html" },
	{ SoyMediaFormat::Subtitle,			"subtitle" },
	{ SoyMediaFormat::ClosedCaption,	"closedcaption" },
	{ SoyMediaFormat::Timecode,			"timecode" },
	{ SoyMediaFormat::QuicktimeTimecode,	"QuicktimeTimecode" },
	{ SoyMediaFormat::MetaData,			"metadata" },
	{ SoyMediaFormat::Muxed,			"muxed" },

	{ SoyMediaFormat::Png,				"Png" },
	{ SoyMediaFormat::Jpeg,				"Jpeg" },
	{ SoyMediaFormat::Gif,				"Gif" },
	{ SoyMediaFormat::Tga,				"Tga" },
	{ SoyMediaFormat::Bmp,				"Bmp" },
	{ SoyMediaFormat::Psd,				"Psd" },
	
	{ SoyMediaFormat::Greyscale,		"Greyscale" },
	{ SoyMediaFormat::GreyscaleAlpha,	"GreyscaleAlpha" },
	{ SoyMediaFormat::RGB,				"RGB" },
	{ SoyMediaFormat::RGBA,				"RGBA" },
	{ SoyMediaFormat::BGRA,				"BGRA" },
	{ SoyMediaFormat::BGR,				"BGR" },
	{ SoyMediaFormat::ARGB,				"ARGB" },
	{ SoyMediaFormat::KinectDepth,		"KinectDepth" },
	{ SoyMediaFormat::FreenectDepth10bit,	"FreenectDepth10bit" },
	{ SoyMediaFormat::FreenectDepth11bit,	"FreenectDepth11bit" },
	{ SoyMediaFormat::FreenectDepthmm,		"FreenectDepthmm" },
	{ SoyMediaFormat::Luma_Full,			"Luma_Full" },
	{ SoyMediaFormat::Luma_Ntsc,		"Luma_Ntsc" },
	{ SoyMediaFormat::Luma_Smptec,		"Luma_Smptec" },
	{ SoyMediaFormat::Yuv_8_88_Full,	"Yuv_8_88_Full" },
	{ SoyMediaFormat::Yuv_8_88_Ntsc,	"Yuv_8_88_Ntsc" },
	{ SoyMediaFormat::Yuv_8_88_Smptec,	"Yuv_8_88_Video" },
	{ SoyMediaFormat::Yuv_8_8_8_Full,	"Yuv_8_8_8_Full" },
	{ SoyMediaFormat::Yuv_8_8_8_Ntsc,	"Yuv_8_8_8_Ntsc" },
	{ SoyMediaFormat::Yuv_8_8_8_Smptec,	"Yuv_8_8_8_Smptec" },
	{ SoyMediaFormat::Yuv_844_Full,		"Yuv_844_Full" },
	{ SoyMediaFormat::ChromaUV_8_8,		"ChromaUV_8_8" },
	{ SoyMediaFormat::ChromaUV_88,		"ChromaUV_88" },
	{ SoyMediaFormat::ChromaUV_44,		"ChromaUV_44" },
	{ SoyMediaFormat::Palettised_RGB_8,	"Palettised_RGB_8" },
	{ SoyMediaFormat::Palettised_RGBA_8,	"Palettised_RGBA_8" },
};



SoyPixelsFormat::Type SoyMediaFormat::GetPixelFormat(SoyMediaFormat::Type MediaFormat)
{
	if ( MediaFormat == SoyMediaFormat::Invalid )
		return SoyPixelsFormat::Invalid;
	
	if ( MediaFormat >= SoyMediaFormat::NotPixels )
		return SoyPixelsFormat::Invalid;
	
	return static_cast<SoyPixelsFormat::Type>( MediaFormat );
}


SoyMediaFormat::Type SoyMediaFormat::FromPixelFormat(SoyPixelsFormat::Type PixelFormat)
{
	return static_cast<SoyMediaFormat::Type>( PixelFormat );
}



bool SoyMediaFormat::IsVideo(SoyMediaFormat::Type Format)
{
	if ( IsPixels(Format) )
		return true;
	if ( IsH264(Format) )
		return true;
	if ( IsImage(Format) )
		return true;
	
	switch ( Format )
	{
		case SoyMediaFormat::Mpeg2TS:
		case SoyMediaFormat::Mpeg2TS_PSI:
		case SoyMediaFormat::Mpeg2:
		case SoyMediaFormat::Mpeg4:
		case SoyMediaFormat::Mpeg4_v3:
		case SoyMediaFormat::VC1:
			return true;
			
		default:
			return false;
	}
}

bool SoyMediaFormat::IsImage(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::Png:
		case SoyMediaFormat::Jpeg:
		case SoyMediaFormat::Gif:
		case SoyMediaFormat::Tga:
		case SoyMediaFormat::Bmp:
		case SoyMediaFormat::Psd:
			return true;
			
		default:
			return false;
	}
}
bool SoyMediaFormat::IsH264(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_8:
		case SoyMediaFormat::H264_16:
		case SoyMediaFormat::H264_32:
		case SoyMediaFormat::H264_ES:
		case SoyMediaFormat::H264_SPS_ES:
		case SoyMediaFormat::H264_PPS_ES:
			return true;
			
		default:
			return false;
	}
}

bool SoyMediaFormat::IsAudio(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::Aac:
		case SoyMediaFormat::Ac3:
		case SoyMediaFormat::Mpeg2Audio:
		case SoyMediaFormat::Mp3:
		case SoyMediaFormat::Dts:
		case SoyMediaFormat::PcmLinear_8:
		case SoyMediaFormat::PcmLinear_16:
		case SoyMediaFormat::PcmLinear_20:
		case SoyMediaFormat::PcmLinear_24:
		case SoyMediaFormat::Wave:
		case SoyMediaFormat::Audio_AUDS:
		case SoyMediaFormat::PcmAndroidRaw:
			return true;
			
		default:
			return false;
	}
}


bool SoyMediaFormat::IsText(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::Text:
		case SoyMediaFormat::Html:
		case SoyMediaFormat::Json:
		case SoyMediaFormat::ClosedCaption:
		case SoyMediaFormat::Subtitle:
			return true;
		
		default:
			return false;
	}
}

std::string SoyMediaFormat::ToMime(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_8:		return "video/avc";
		case SoyMediaFormat::H264_16:		return "video/avc";
		case SoyMediaFormat::H264_32:		return "video/avc";
		case SoyMediaFormat::H264_ES:		return "video/avc";		//	find the proper version of this
		case SoyMediaFormat::H264_PPS_ES:	return "video/avc";		//	find the proper version of this
		case SoyMediaFormat::H264_SPS_ES:	return "video/avc";		//	find the proper version of this
			
		case SoyMediaFormat::Mpeg2TS:	return "video/ts";		//	find the proper version of this
		case SoyMediaFormat::Mpeg2:		return "video/mpeg2";	//	find the proper version of this
		case SoyMediaFormat::Mpeg4:		return "video/mp4";		//	find the proper version of this
		case SoyMediaFormat::Mpeg4_v3:	return "video/mp43";	//	find the proper version of this
	
		case SoyMediaFormat::Wave:		return "audio/wave";
		case SoyMediaFormat::Audio_AUDS:	return "audio/Audio_AUDS";
			
		//	gr: change this to handle multiple mime types per format
		case SoyMediaFormat::Aac:		return Mime::Aac_Default;

		//	https://en.wikipedia.org/wiki/Pulse-code_modulation
		case SoyMediaFormat::PcmLinear_8:	return "audio/L8";
		case SoyMediaFormat::PcmLinear_16:	return "audio/L16";
		case SoyMediaFormat::PcmLinear_20:	return "audio/L20";
		case SoyMediaFormat::PcmLinear_24:	return "audio/L24";
			
		case SoyMediaFormat::PcmAndroidRaw:	return MIMETYPE_AUDIO_RAW;
		case SoyMediaFormat::Mp3:		return "audio/mpeg";	//	audio/mpeg is what android reports when I try and open mp3

		//	verify these
		case SoyMediaFormat::Png:		return "image/png";
		case SoyMediaFormat::Jpeg:		return "image/jpeg";
		case SoyMediaFormat::Bmp:		return "image/bmp";
		case SoyMediaFormat::Tga:		return "image/tga";
		case SoyMediaFormat::Psd:		return "image/Psd";
		case SoyMediaFormat::Gif:		return "image/gif";
			
		case SoyMediaFormat::Text:		return "text/plain";
		case SoyMediaFormat::Json:		return "application/javascript";
		case SoyMediaFormat::Html:		return "text/html";
			
		default:						return "invalid/invalid";
	}
	
}

SoyMediaFormat::Type SoyMediaFormat::FromMime(const std::string& Mime)
{
	//	special multiple-mime case
	if ( Mime == Mime::Aac_Android )	return SoyMediaFormat::Aac;
	if ( Mime == Mime::Aac_x )			return SoyMediaFormat::Aac;
	if ( Mime == Mime::Aac_Other )		return SoyMediaFormat::Aac;
	
	if ( Mime == ToMime( SoyMediaFormat::H264_8 ) )			return SoyMediaFormat::H264_8;
	if ( Mime == ToMime( SoyMediaFormat::H264_16 ) )		return SoyMediaFormat::H264_16;
	if ( Mime == ToMime( SoyMediaFormat::H264_32 ) )		return SoyMediaFormat::H264_32;
	if ( Mime == ToMime( SoyMediaFormat::H264_ES ) )		return SoyMediaFormat::H264_ES;
	if ( Mime == ToMime( SoyMediaFormat::H264_PPS_ES ) )	return SoyMediaFormat::H264_PPS_ES;
	if ( Mime == ToMime( SoyMediaFormat::H264_SPS_ES ) )	return SoyMediaFormat::H264_SPS_ES;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg2TS ) )		return SoyMediaFormat::Mpeg2TS;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg2 ) )			return SoyMediaFormat::Mpeg2;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg4 ) )			return SoyMediaFormat::Mpeg4;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg4_v3 ) )		return SoyMediaFormat::Mpeg4_v3;
	if ( Mime == ToMime( SoyMediaFormat::Wave ) )			return SoyMediaFormat::Wave;
	if ( Mime == ToMime( SoyMediaFormat::Audio_AUDS ) )		return SoyMediaFormat::Audio_AUDS;
	if ( Mime == ToMime( SoyMediaFormat::Aac ) )			return SoyMediaFormat::Aac;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_8 ) )	return SoyMediaFormat::PcmLinear_8;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_16 ) )	return SoyMediaFormat::PcmLinear_16;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_20 ) )	return SoyMediaFormat::PcmLinear_20;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_24 ) )	return SoyMediaFormat::PcmLinear_24;
	if ( Mime == ToMime( SoyMediaFormat::PcmAndroidRaw ) )	return SoyMediaFormat::PcmAndroidRaw;
	if ( Mime == ToMime( SoyMediaFormat::Mp3 ) )			return SoyMediaFormat::Mp3;
	
	if ( Mime == ToMime( SoyMediaFormat::Png ) )			return SoyMediaFormat::Png;
	if ( Mime == ToMime( SoyMediaFormat::Jpeg ) )			return SoyMediaFormat::Jpeg;
	if ( Mime == ToMime( SoyMediaFormat::Bmp ) )			return SoyMediaFormat::Bmp;
	if ( Mime == ToMime( SoyMediaFormat::Tga ) )			return SoyMediaFormat::Tga;
	if ( Mime == ToMime( SoyMediaFormat::Psd ) )			return SoyMediaFormat::Psd;

	std::Debug << "Unknown mime type: " << Mime << std::endl;
	return SoyMediaFormat::Invalid;
}

bool SoyMediaFormat::IsH264Fourcc(uint32 Fourcc)
{
	switch ( Fourcc )
	{
		case 'avc1':
		case '1cva':
			return true;
		default:
			return false;
	};
}

uint32 SoyMediaFormat::ToFourcc(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::Aac:		return 'aac ';
		case SoyMediaFormat::Mpeg4:		return 'mp4v';
		case SoyMediaFormat::Mpeg4_v3:	return 'MP43';

		case SoyMediaFormat::PcmLinear_8:
		case SoyMediaFormat::PcmLinear_16:
		case SoyMediaFormat::PcmLinear_20:
		case SoyMediaFormat::PcmLinear_24:
			return 'lpcm';
		
		case SoyMediaFormat::H264_ES:
		case SoyMediaFormat::H264_SPS_ES:
		case SoyMediaFormat::H264_PPS_ES:
		case SoyMediaFormat::H264_8:
		case SoyMediaFormat::H264_16:
		case SoyMediaFormat::H264_32:
			return 'avc1';
			
		default:
			break;
	}

	std::stringstream Error;
	Error << __func__ << " unhandled format -> fourcc " << Format;
	throw Soy::AssertException( Error.str() );
}

	

SoyMediaFormat::Type SoyMediaFormat::FromFourcc(uint32 Fourcc,int H264LengthSize,bool TryReversed)
{
	switch ( Fourcc )
	{
		case 'avc1':
			if ( H264LengthSize == 0 )
				return SoyMediaFormat::H264_ES;
			if ( H264LengthSize == 1 )
				return SoyMediaFormat::H264_8;
			if ( H264LengthSize == 2 )
				return SoyMediaFormat::H264_16;
			if ( H264LengthSize == 4 )
				return SoyMediaFormat::H264_32;
			break;

		//	win7 MF - don't know how to get size atm so assuming 32bit (if it matters)
		case 'H264':
			return SoyMediaFormat::H264_32;
			break;
			

		case 'aac ':
			return SoyMediaFormat::Aac;

		//	windows/MediaFoundation have fourcc's in caps
		case 'MP4V':
		case 'mp4v':
			return SoyMediaFormat::Mpeg4;
			
		case 'MP43':
			return SoyMediaFormat::Mpeg4_v3;

		case 'lpcm':
			if ( H264LengthSize == 8 )
				return SoyMediaFormat::PcmLinear_8;
			if ( H264LengthSize == 16 )
				return SoyMediaFormat::PcmLinear_16;
			if ( H264LengthSize == 20 )
				return SoyMediaFormat::PcmLinear_20;
			if ( H264LengthSize == 24 )
				return SoyMediaFormat::PcmLinear_24;
			break;
			
		//	found in quicktime mov's
		case 'tmcd':
            return SoyMediaFormat::QuicktimeTimecode;
	}
	
	//	detect reversed fourcc's and encourage converting at the source
	if ( TryReversed )
	{
		auto FourccSwapped = Soy::SwapEndian( Fourcc );
		auto Type = FromFourcc( FourccSwapped, H264LengthSize, false );
		if ( Type != SoyMediaFormat::Invalid )
		{
			std::Debug << "Warning: Detected reversed fourcc.(" << Soy::FourCCToString(FourccSwapped) << ") todo: Fix endianness at source." << std::endl;
			return Type;
		}
	}

	
	std::Debug << "Unknown fourcc type: " << Soy::FourCCToString(Fourcc) << std::endl;
	return SoyMediaFormat::Invalid;
}
