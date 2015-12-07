#include "SoyPixels.h"





#define CV_VIDEO_TYPE_META(Enum,SoyFormat)	TCvVideoTypeMeta( Enum, #Enum, SoyFormat )
#define CV_VIDEO_INVALID_ENUM		0
class TCvVideoTypeMeta
{
public:
	TCvVideoTypeMeta(OSType Enum,const char* EnumName,SoyPixelsFormat::Type SoyFormat) :
	mEnum		( Enum ),
	mName		( EnumName ),
	mSoyFormat	( SoyFormat )
	{
		Soy::Assert( IsValid(), "Expected valid enum - or invalid enum is bad" );
	}
	TCvVideoTypeMeta() :
	mEnum		( CV_VIDEO_INVALID_ENUM ),
	mName		( "Invalid enum" ),
	mSoyFormat	( SoyPixelsFormat::Invalid )
	{
	}
	
	bool		IsValid() const		{	return mEnum != CV_VIDEO_INVALID_ENUM;	}
	
	bool		operator==(const OSType& Enum) const	{	return mEnum == Enum;	}
	bool		operator==(const SoyPixelsFormat::Type& Format) const	{	return mSoyFormat == Format;	}
	
public:
	OSType					mEnum;
	SoyPixelsFormat::Type	mSoyFormat;
	std::string				mName;
};

const TCvVideoTypeMeta VideoTypes[] =
{
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24RGB,	SoyPixelsFormat::RGB ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24BGR,	SoyPixelsFormat::BGR ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32BGRA,	SoyPixelsFormat::BGRA ),
	
	//	gr: popcast creating a pixel buffer from a unity "argb" texture, failed as RGBA is unsupported...
	//	gr: ARGB is accepted, but channels are wrong
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32RGBA,	SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ARGB,	SoyPixelsFormat::ARGB ),
/*
	CV_VIDEO_TYPE_META( kCVPixelFormatType_1Monochrome,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_2Indexed,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4Indexed,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_8Indexed,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_1IndexedGray_WhiteIsZero,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_2IndexedGray_WhiteIsZero,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4IndexedGray_WhiteIsZero,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_8IndexedGray_WhiteIsZero,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16BE555,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16LE555,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16LE5551,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16BE565,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16LE565,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ARGB,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ABGR,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_64ARGB,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_48RGB,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32AlphaGray,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16Gray,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4444YpCbCrA8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4444YpCbCrA8R,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_444YpCbCr8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr16,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr10,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_444YpCbCr10,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8Planar,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8PlanarFullRange,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr_4A_8BiPlanar,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarFullRange,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8_yuvs,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8FullRange,	SoyPixelsFormat::Invalid ),
 */
};



TCvVideoTypeMeta GetVideoMeta(int Enum)
{
	static const BufferArray<TCvVideoTypeMeta,100> Metas( VideoTypes );
	auto* Meta = Metas.Find( Enum );
	return Meta ? *Meta : TCvVideoTypeMeta();
}


TCvVideoTypeMeta GetVideoMeta(SoyPixelsFormat::Type Enum)
{
	static const BufferArray<TCvVideoTypeMeta,100> Metas( VideoTypes );
	auto* Meta = Metas.Find( Enum );
	return Meta ? *Meta : TCvVideoTypeMeta();
}

@implementation NSNumber (CVPixelFormatType)

- (NSString *)descriptivePixelFormat
{
	return @{
			 @(kCVPixelFormatType_1Monochrome): @"kCVPixelFormatType_1Monochrome",
			 @(kCVPixelFormatType_2Indexed): @"kCVPixelFormatType_2Indexed",
			 @(kCVPixelFormatType_4Indexed): @"kCVPixelFormatType_4Indexed",
			 @(kCVPixelFormatType_8Indexed): @"kCVPixelFormatType_8Indexed",
			 @(kCVPixelFormatType_1IndexedGray_WhiteIsZero): @"kCVPixelFormatType_1IndexedGray_WhiteIsZero",
			 @(kCVPixelFormatType_2IndexedGray_WhiteIsZero): @"kCVPixelFormatType_2IndexedGray_WhiteIsZero",
			 @(kCVPixelFormatType_4IndexedGray_WhiteIsZero): @"kCVPixelFormatType_4IndexedGray_WhiteIsZero",
			 @(kCVPixelFormatType_8IndexedGray_WhiteIsZero): @"kCVPixelFormatType_8IndexedGray_WhiteIsZero",
			 @(kCVPixelFormatType_16BE555): @"kCVPixelFormatType_16BE555",
			 @(kCVPixelFormatType_16LE555): @"kCVPixelFormatType_16LE555",
			 @(kCVPixelFormatType_16LE5551): @"kCVPixelFormatType_16LE5551",
			 @(kCVPixelFormatType_16BE565): @"kCVPixelFormatType_16BE565",
			 @(kCVPixelFormatType_16LE565): @"kCVPixelFormatType_16LE565",
			 @(kCVPixelFormatType_24RGB): @"kCVPixelFormatType_24RGB",
			 @(kCVPixelFormatType_24BGR): @"kCVPixelFormatType_24BGR",
			 @(kCVPixelFormatType_32ARGB): @"kCVPixelFormatType_32ARGB",
			 @(kCVPixelFormatType_32BGRA): @"kCVPixelFormatType_32BGRA",
			 @(kCVPixelFormatType_32ABGR): @"kCVPixelFormatType_32ABGR",
			 @(kCVPixelFormatType_32RGBA): @"kCVPixelFormatType_32RGBA",
			 @(kCVPixelFormatType_64ARGB): @"kCVPixelFormatType_64ARGB",
			 @(kCVPixelFormatType_48RGB): @"kCVPixelFormatType_48RGB",
			 @(kCVPixelFormatType_32AlphaGray): @"kCVPixelFormatType_32AlphaGray",
			 @(kCVPixelFormatType_16Gray): @"kCVPixelFormatType_16Gray",
			 @(kCVPixelFormatType_422YpCbCr8): @"kCVPixelFormatType_422YpCbCr8",
			 @(kCVPixelFormatType_4444YpCbCrA8): @"kCVPixelFormatType_4444YpCbCrA8",
			 @(kCVPixelFormatType_4444YpCbCrA8R): @"kCVPixelFormatType_4444YpCbCrA8R",
			 @(kCVPixelFormatType_444YpCbCr8): @"kCVPixelFormatType_444YpCbCr8",
			 @(kCVPixelFormatType_422YpCbCr16): @"kCVPixelFormatType_422YpCbCr16",
			 @(kCVPixelFormatType_422YpCbCr10): @"kCVPixelFormatType_422YpCbCr10",
			 @(kCVPixelFormatType_444YpCbCr10): @"kCVPixelFormatType_444YpCbCr10",
			 @(kCVPixelFormatType_420YpCbCr8Planar): @"kCVPixelFormatType_420YpCbCr8Planar",
			 @(kCVPixelFormatType_420YpCbCr8PlanarFullRange): @"kCVPixelFormatType_420YpCbCr8PlanarFullRange",
			 @(kCVPixelFormatType_422YpCbCr_4A_8BiPlanar): @"kCVPixelFormatType_422YpCbCr_4A_8BiPlanar",
			 @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange): @"kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange",
			 @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange): @"kCVPixelFormatType_420YpCbCr8BiPlanarFullRange",
			 @(kCVPixelFormatType_422YpCbCr8_yuvs): @"kCVPixelFormatType_422YpCbCr8_yuvs",
			 @(kCVPixelFormatType_422YpCbCr8FullRange): @"kCVPixelFormatType_422YpCbCr8FullRange"
			 }[self];
}

@end



std::string Soy::Platform::PixelFormatToString(NSNumber* FormatCv)
{
	auto StringNs = [FormatCv descriptivePixelFormat];
	return Soy::NSStringToString( StringNs );
}

std::string Soy::Platform::PixelFormatToString(id FormatCv)
{
	auto FormatInt = [FormatCv integerValue];
	NSNumber* FormatNumber = [[NSNumber alloc] initWithInteger:FormatInt];
	return PixelFormatToString( FormatNumber );
}

SoyPixelsFormat::Type Soy::Platform::GetFormat(NSNumber* FormatCv)
{
	auto Format = FormatCv.intValue;
	return GetFormat( Format );
	/*
	switch ( Format )
	{
		default:						return SoyPixelsFormat::Invalid;
		case kCVPixelFormatType_32BGRA:	return SoyPixelsFormat::BGRA;
		case kCVPixelFormatType_24RGB:	return SoyPixelsFormat::RGB;
			//		case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:	return SoyPixelsFormat::Yuv420_Biplanar_Full;
			//		case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:	return SoyPixelsFormat::Yuv420_Biplanar_Video;
	}
	 */
}


SoyPixelsFormat::Type Soy::Platform::GetFormat(OSType Format)
{
	auto Meta = GetVideoMeta( Format );
	return Meta.mSoyFormat;
}



OSType Soy::Platform::GetFormat(SoyPixelsFormat::Type Format)
{
	auto Meta = GetVideoMeta( Format );
	return Meta.mEnum;
}

