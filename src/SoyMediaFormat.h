#pragma once


#include <SoyPixels.h>


//	merge this + pixel format at some point
namespace SoyMediaFormat
{
	enum Type
	{
		Invalid = SoyPixelsFormat::Invalid,
		
		Greyscale = SoyPixelsFormat::Greyscale,
		GreyscaleAlpha = SoyPixelsFormat::GreyscaleAlpha,
		RGB = SoyPixelsFormat::RGB,
		RGBA = SoyPixelsFormat::RGBA,
		BGRA = SoyPixelsFormat::BGRA,
		BGR = SoyPixelsFormat::BGR,
		ARGB = SoyPixelsFormat::ARGB,
		KinectDepth = SoyPixelsFormat::KinectDepth,
		FreenectDepth10bit = SoyPixelsFormat::FreenectDepth10bit,
		FreenectDepth11bit = SoyPixelsFormat::FreenectDepth11bit,
		FreenectDepthmm = SoyPixelsFormat::FreenectDepthmm,
		Luma_Full = SoyPixelsFormat::Luma_Full,
		Luma_Ntsc = SoyPixelsFormat::Luma_Ntsc,
		Luma_Smptec = SoyPixelsFormat::Luma_Smptec,
		Yuv_8_88_Full = SoyPixelsFormat::Yuv_8_88_Full,
		Yuv_8_88_Ntsc = SoyPixelsFormat::Yuv_8_88_Ntsc,
		Yuv_8_88_Smptec = SoyPixelsFormat::Yuv_8_88_Smptec,
		Yuv_8_8_8_Full = SoyPixelsFormat::Yuv_8_8_8_Full,
		Yuv_8_8_8_Ntsc = SoyPixelsFormat::Yuv_8_8_8_Ntsc,
		Yuv_8_8_8_Smptec = SoyPixelsFormat::Yuv_8_8_8_Smptec,
		YYuv_8888_Full = SoyPixelsFormat::YYuv_8888_Full,
		YYuv_8888_Ntsc = SoyPixelsFormat::YYuv_8888_Ntsc,
		YYuv_8888_Smptec = SoyPixelsFormat::YYuv_8888_Smptec,
		ChromaUV_8_8 = SoyPixelsFormat::ChromaUV_8_8,
		ChromaUV_88 = SoyPixelsFormat::ChromaUV_88,
		Palettised_RGB_8 = SoyPixelsFormat::Palettised_RGB_8,
		Palettised_RGBA_8 = SoyPixelsFormat::Palettised_RGBA_8,
		
		NotPixels = SoyPixelsFormat::Count,
		
		//	file:///Users/grahamr/Downloads/513_direct_access_to_media_encoding_and_decoding.pdf
		//	gr: too specific? extended/sub modes?... prefer this really...
		H264_8,			//	AVCC format (length8+payload)
		H264_16,		//	AVCC format (length16+payload)
		H264_32,		//	AVCC format (length32+payload)
		H264_ES,		//	ES format (0001+payload)	elementry/transport/annexb/Nal stream (H264 ES in MF)
		H264_SPS_ES,	//	SPS data, nalu
		H264_PPS_ES,	//	PPS data, nalu
		
		Mpeg2TS,		//	general TS data
		Mpeg2TS_PSI,	//	PSI table from TS
		
		Mpeg2,
		Mpeg4,
		Mpeg4_v3,		//	windows mpeg4 variant MP43 (msmpeg4v3)
		VC1,			//	in TS files, not sure what this is yet
		
		//	encoded images
		Png,
		Jpeg,
		Gif,
		Tga,
		Bmp,
		Psd,
		//MovingJpeg,		//	mjpeg
		
		//	audio
		Audio_AUDS,		//	fourcc from mediafoundation. means "audio stream"
		Wave,
		Aac,
		Ac3,
		Mpeg2Audio,			//	in TS files, not sure what format this is yet
		Mp3,
		Dts,
		PcmAndroidRaw,		//	temp until I work out what this actually is
		PcmLinear_8,
		PcmLinear_16,		//	signed, see SoyWave
		PcmLinear_20,
		PcmLinear_24,
		PcmLinear_float,	//	-1..1 see SoyWave
		
        QuicktimeTimecode,  //  explicitly listing this until I've established what the format is
		
		Html,
		Text,
		Json,				//	has seperate mime type
		Subtitle,
		ClosedCaption,
		Timecode,
		MetaData,
		Muxed,
	};
	
	SoyPixelsFormat::Type	GetPixelFormat(Type MediaFormat);
	Type					FromPixelFormat(SoyPixelsFormat::Type PixelFormat);
	
	bool		IsVideo(Type Format);	//	or pixels
	inline bool	IsPixels(Type Format)	{	return GetPixelFormat( Format ) != SoyPixelsFormat::Invalid;	}
	bool		IsAudio(Type Format);
	bool		IsText(Type Format);
	bool		IsH264(Type Format);
	bool		IsImage(Type Format);	//	encoded image
	Type		FromFourcc(uint32 Fourcc,int H264LengthSize=-1,bool TryReversed=true);
	uint32		ToFourcc(Type Format);
	bool		IsH264Fourcc(uint32 Fourcc);
	std::string	ToMime(Type Format);
	Type		FromMime(const std::string& Mime);
	
	DECLARE_SOYENUM(SoyMediaFormat);
}


