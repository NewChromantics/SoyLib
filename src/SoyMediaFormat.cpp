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


namespace SoyMediaMetaFlags
{
	//	copy & paste this to c#
	enum Type
	{
		None						= 0,
		IsVideo						= 1<<0,
		IsAudio						= 1<<1,
		IsH264						= 1<<2,
		IsText						= 1<<3,
		IsImage						= 1<<4,
	};
}

class SoyMediaFormatMeta
{
public:
	SoyMediaFormatMeta() :
		mFormat			( SoyMediaFormat::Invalid ),
		mFlags			( 0 ),
		mSubtypeSize	( 0 )
	{
	}	
	SoyMediaFormatMeta(SoyMediaFormat::Type Format,const std::initializer_list<std::string>& Mimes,const std::initializer_list<uint32_t>& Fourccs,int Flags,size_t SubtypeSize) :
		mFormat			( Format ),
		mFlags			( Flags ),
		mSubtypeSize	( SubtypeSize )
	{
		for ( auto Fourcc : Fourccs )
			mFourccs.PushBack( Fourcc );
		for ( auto Mime : Mimes )
			mMimes.PushBack( Mime );
	}
	SoyMediaFormatMeta(SoyMediaFormat::Type Format,const std::initializer_list<std::string>& Mimes,const uint32_t& Fourcc,int Flags,size_t SubtypeSize) :
		mFormat			( Format ),
		mFlags			( Flags ),
		mSubtypeSize	( SubtypeSize )
	{
		mFourccs.PushBack( Fourcc );
		for ( auto Mime : Mimes )
			mMimes.PushBack( Mime );
	}
	SoyMediaFormatMeta(SoyMediaFormat::Type Format,const std::string& Mime,const std::initializer_list<uint32_t>& Fourccs,int Flags,size_t SubtypeSize) :
		mFormat			( Format ),
		mFlags			( Flags ),
		mSubtypeSize	( SubtypeSize )
	{
		for ( auto Fourcc : Fourccs )
			mFourccs.PushBack( Fourcc );
		mMimes.PushBack( Mime );
	}
	SoyMediaFormatMeta(SoyMediaFormat::Type Format,const std::string& Mime,const uint32_t& Fourcc,int Flags,size_t SubtypeSize) :
		mFormat			( Format ),
		mFlags			( Flags ),
		mSubtypeSize	( SubtypeSize )
	{
		mFourccs.PushBack( Fourcc );
		mMimes.PushBack( Mime );
	}
	
	SoyMediaFormat::Type		mFormat;
	BufferArray<uint32_t,5>		mFourccs;
	BufferArray<std::string,5>	mMimes;
	uint32_t					mFlags;
	size_t						mSubtypeSize;	//	zero is non-specific
	
	bool					Is(SoyMediaMetaFlags::Type Flag) const		{	return bool_cast( mFlags & Flag );	}
	
	bool					operator==(const SoyMediaFormat::Type Format) const	{	return mFormat == Format;	}
};


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
	{ SoyMediaFormat::Divx,				"Divx" },
	{ SoyMediaFormat::MotionJpeg,		"MotionJpeg" },
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
	{ SoyMediaFormat::PcmLinear_float,	"PcmLinear_float" },
	{ SoyMediaFormat::Audio_AUDS,		"Audio_AUDS" },
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
	{ SoyMediaFormat::YYuv_8888_Full,	"YYuv_8888_Full" },
	{ SoyMediaFormat::YYuv_8888_Ntsc,	"YYuv_8888_Ntsc" },
	{ SoyMediaFormat::YYuv_8888_Smptec,	"YYuv_8888_Smptec" },
	{ SoyMediaFormat::ChromaUV_8_8,		"ChromaUV_8_8" },
	{ SoyMediaFormat::ChromaUV_88,		"ChromaUV_88" },
	{ SoyMediaFormat::Palettised_RGB_8,	"Palettised_RGB_8" },
	{ SoyMediaFormat::Palettised_RGBA_8,	"Palettised_RGBA_8" },
};


namespace SoyMediaFormat
{
	const Array<SoyMediaFormatMeta>&	GetFormatMap();
	void								GetFormatMetas(ArrayBridge<const SoyMediaFormatMeta*>&& MetaMatches,uint32_t Fourcc,size_t Size);
	const SoyMediaFormatMeta&			GetFormatMeta(SoyMediaFormat::Type Format);
	const SoyMediaFormatMeta&			GetFormatMetaFromMime(const std::string& Mime);
}


const Array<SoyMediaFormatMeta>& SoyMediaFormat::GetFormatMap()
{
	static SoyMediaFormatMeta _FormatMap[] =
	{
		SoyMediaFormatMeta(),
		
		SoyMediaFormatMeta( SoyMediaFormat::H264_8,			"video/avc",	'avc1', SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 1 ),
		SoyMediaFormatMeta( SoyMediaFormat::H264_16,		"video/avc",	'avc1', SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 2 ),

		//	win7 mediafoundation gives H264 with unknown size, so we assume 32
		//	gr^^ this should change to ES and re-resolve if not annex-b
		SoyMediaFormatMeta( SoyMediaFormat::H264_32,		"video/avc",	{'avc1','H264'}, SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 4 ),
		SoyMediaFormatMeta( SoyMediaFormat::H264_ES,		"video/avc",	'avc1', SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::H264_PPS_ES,	"video/avc",	'avc1', SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::H264_SPS_ES,	"video/avc",	'avc1', SoyMediaMetaFlags::IsVideo|SoyMediaMetaFlags::IsH264, 0 ),

		SoyMediaFormatMeta( SoyMediaFormat::Mpeg2TS,		"video/ts",		'xxxx', SoyMediaMetaFlags::IsVideo, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Mpeg2TS_PSI,	"video/ts",		'xxxx', SoyMediaMetaFlags::IsVideo, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Mpeg2,			"video/mpeg2",	'xxxx', SoyMediaMetaFlags::IsVideo, 0 ),
		
		//	windows media foundation has this fourcc in caps (all?)
		SoyMediaFormatMeta( SoyMediaFormat::Mpeg4,			"video/mp4",		{'mp4v','MP4V'}, SoyMediaMetaFlags::IsVideo, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Mpeg4_v3,		"video/mp43",		'MP43', SoyMediaMetaFlags::IsVideo, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::VC1,			"video/xxx",		'xxxx', SoyMediaMetaFlags::IsVideo, 0 ),
		
		//	verify mime
		SoyMediaFormatMeta( SoyMediaFormat::Divx,			"video/divx",		'divx', SoyMediaMetaFlags::IsVideo, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::MotionJpeg,		"video/mjpg",		'MJPG', SoyMediaMetaFlags::IsVideo, 0 ),

		
		SoyMediaFormatMeta( SoyMediaFormat::Wave,			"audio/wave",		'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Audio_AUDS,		"audio/Audio_AUDS",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),
		
		//	verify mime
		SoyMediaFormatMeta( SoyMediaFormat::Ac3,			"audio/ac3",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Mpeg2Audio,		"audio/mpeg",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Dts,			"audio/dts",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),

		//	gr: change this to handle multiple mime types per format
		SoyMediaFormatMeta( SoyMediaFormat::Aac,			{ Mime::Aac_Default, Mime::Aac_Android, Mime::Aac_x, Mime::Aac_Other},	'aac ', SoyMediaMetaFlags::IsAudio, 0 ),

		//	https://en.wikipedia.org/wiki/Pulse-code_modulation
		SoyMediaFormatMeta( SoyMediaFormat::PcmLinear_8,	"audio/L8",		'lpcm', SoyMediaMetaFlags::IsAudio, 8  ),
		SoyMediaFormatMeta( SoyMediaFormat::PcmLinear_16,	"audio/L16",	'lpcm', SoyMediaMetaFlags::IsAudio, 16  ),
		SoyMediaFormatMeta( SoyMediaFormat::PcmLinear_20,	"audio/L20",	'lpcm', SoyMediaMetaFlags::IsAudio, 20  ),
		SoyMediaFormatMeta( SoyMediaFormat::PcmLinear_24,	"audio/L24",	'lpcm', SoyMediaMetaFlags::IsAudio, 24  ),
		SoyMediaFormatMeta( SoyMediaFormat::PcmAndroidRaw,	MIMETYPE_AUDIO_RAW,	'lpcm', SoyMediaMetaFlags::IsAudio, 0 ),

		//	find mime
		SoyMediaFormatMeta( SoyMediaFormat::PcmLinear_float,	"audio/L32",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),

		//	audio/mpeg is what android reports when I try and open mp3
		SoyMediaFormatMeta( SoyMediaFormat::Mp3,			"audio/mpeg",	'xxxx', SoyMediaMetaFlags::IsAudio, 0 ),
		
		//	verify these mimes
		SoyMediaFormatMeta( SoyMediaFormat::Png,			"image/png",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Jpeg,			"image/jpeg",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Bmp,			"image/bmp",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Tga,			"image/tga",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Psd,			"image/Psd",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Gif,			"image/gif",	'xxxx', SoyMediaMetaFlags::IsImage, 0 ),

		
		SoyMediaFormatMeta( SoyMediaFormat::Text,			"text/plain",	'xxxx', SoyMediaMetaFlags::IsText, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Json,			"application/javascript",	'xxxx', SoyMediaMetaFlags::IsText, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Html,			"text/html",	'xxxx', SoyMediaMetaFlags::IsText, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::ClosedCaption,	"text/plain",	'xxxx', SoyMediaMetaFlags::IsText, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Subtitle,		"text/plain",	'xxxx', SoyMediaMetaFlags::IsText, 0 ),
		
		SoyMediaFormatMeta( SoyMediaFormat::QuicktimeTimecode,	"application/quicktimetimecode",	'tmcd', SoyMediaMetaFlags::None, 0 ),

		//	pixel formats
		//	gr: these fourcc's are from mediafoundation
		SoyMediaFormatMeta( SoyMediaFormat::Greyscale,			"application/Greyscale",		'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::GreyscaleAlpha,		"application/GreyscaleAlpha",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::RGB,				"application/RGB",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::RGBA,				"application/RGBA",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::BGRA,				"application/BGRA",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::BGR,				"application/BGR",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::ARGB,				"application/ARGB",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::KinectDepth,		"application/KinectDepth",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::FreenectDepth10bit,	"application/FreenectDepth10bit",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::FreenectDepth11bit,	"application/FreenectDepth11bit",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::FreenectDepthmm,	"application/FreenectDepthmm",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Luma_Full,			"application/Luma_Full",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Luma_Ntsc,			"application/Luma_Ntsc",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Luma_Smptec,		"application/Luma_Smptec",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_88_Full,		"application/Yuv_8_88_Full",	{'NV12','VIDS'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_88_Ntsc,		"application/Yuv_8_88_Ntsc",	{'NV12','VIDS'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_88_Smptec,	"application/Yuv_8_88_Smptec",	{'NV12','VIDS'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_8_8_Full,		"application/Yuv_8_8_8_Full",	'I420', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_8_8_Ntsc,		"application/Yuv_8_8_8_Ntsc",	'I420', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Yuv_8_8_8_Smptec,	"application/Yuv_8_8_8_Smptec",	'I420', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::YYuv_8888_Full,		"application/YYuv_8888_Full",	{'YUY2','IYUV','Y42T'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::YYuv_8888_Ntsc,		"application/YYuv_8888_Ntsc",		{'YUY2','IYUV','Y42T'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::YYuv_8888_Smptec,	"application/YYuv_8888_Smptec",	{'YUY2','IYUV','Y42T'}, SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::ChromaUV_8_8,		"application/ChromaUV_8_8",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::ChromaUV_88,		"application/ChromaUV_88",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Palettised_RGB_8,	"application/Palettised_RGB_8",	'xxxx', SoyMediaMetaFlags::None, 0 ),
		SoyMediaFormatMeta( SoyMediaFormat::Palettised_RGBA_8,	"application/Palettised_RGBA_8",	'xxxx', SoyMediaMetaFlags::None, 0 ),
	};

	static Array<SoyMediaFormatMeta> FormatMap( _FormatMap );
	return FormatMap;
}

const SoyMediaFormatMeta& SoyMediaFormat::GetFormatMeta(SoyMediaFormat::Type Format)
{
	auto& Metas = GetFormatMap();
	auto* Meta = Metas.Find( Format );
	if ( !Meta )
	{
		std::stringstream Error;
		Error << "Missing meta for " << Format;
		throw Soy::AssertException( Error.str() );
	}
	return *Meta;
}


const SoyMediaFormatMeta& SoyMediaFormat::GetFormatMetaFromMime(const std::string& Mime)
{
	auto& Metas = GetFormatMap();
	for ( int m=0;	m<Metas.GetSize();	m++ )
	{
		auto& Meta = Metas[m];
		
		if ( !Meta.mMimes.Find( Mime ) )
			continue;
		
		return Meta;
	}
	
	std::stringstream Error;
	Error << "No formats found matching mime " << Mime;
	throw Soy::AssertException( Error.str() );
}

void SoyMediaFormat::GetFormatMetas(ArrayBridge<const SoyMediaFormatMeta*>&& MetaMatches,uint32_t Fourcc,size_t Size)
{
	auto& Metas = GetFormatMap();
	
	auto FourccSwapped = Soy::SwapEndian( Fourcc );
	
	for ( int m=0;	m<Metas.GetSize();	m++ )
	{
		auto& Meta = Metas[m];
		if ( !Meta.mFourccs.Find( Fourcc ) )
		{
			if ( !Meta.mFourccs.Find( FourccSwapped ) )
				continue;
			else
				std::Debug << "Warning: Detected reversed fourcc.(" << Soy::FourCCToString(FourccSwapped) << ") todo: Fix endianness at source." << std::endl;
			
		}
	
		//	match size
		if ( Meta.mSubtypeSize != Size )
			continue;

		MetaMatches.PushBack( &Meta );
	}
}


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
	
	auto& Meta = GetFormatMeta( Format );
	return Meta.Is( SoyMediaMetaFlags::IsVideo );
}

bool SoyMediaFormat::IsImage(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.Is( SoyMediaMetaFlags::IsImage );
}

bool SoyMediaFormat::IsH264(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.Is( SoyMediaMetaFlags::IsH264 );
}

bool SoyMediaFormat::IsAudio(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.Is( SoyMediaMetaFlags::IsAudio );
}


bool SoyMediaFormat::IsText(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.Is( SoyMediaMetaFlags::IsText );

}

std::string SoyMediaFormat::ToMime(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.mMimes[0];
	
}

SoyMediaFormat::Type SoyMediaFormat::FromMime(const std::string& Mime)
{
	auto& Meta = GetFormatMetaFromMime( Mime );
	return Meta.mFormat;
}

uint32 SoyMediaFormat::ToFourcc(SoyMediaFormat::Type Format)
{
	auto& Meta = GetFormatMeta( Format );
	return Meta.mFourccs[0];
}

	

SoyMediaFormat::Type SoyMediaFormat::FromFourcc(uint32 Fourcc,int H264LengthSize)
{
	BufferArray<const SoyMediaFormatMeta*,10> Metas;
	GetFormatMetas( GetArrayBridge(Metas), Fourcc, H264LengthSize );
	
	if ( Metas.IsEmpty() )
	{
		std::Debug << "Unknown fourcc type: " << Soy::FourCCToString(Fourcc) << " (" << H264LengthSize << ")" << std::endl;
		return SoyMediaFormat::Invalid;
	}

	//	multiple found
	if ( Metas.GetSize() > 1 )
		std::Debug << "Warning found multiple metas for fourcc " << Soy::FourCCToString(Fourcc) << " returning first" << std::endl;

	return Metas[0]->mFormat;

}
