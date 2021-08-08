#include "SoyAvf.h"
#include "SoyH264.h"
#include "SoyFilesystem.h"
#include "SoyFourcc.h"

#include <CoreMedia/CMBase.h>
#include <VideoToolbox/VTBase.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreMedia/CMFormatDescription.h>
#include <CoreMedia/CMTime.h>
#include <VideoToolbox/VTSession.h>
#include <VideoToolbox/VTCompressionProperties.h>
#include <VideoToolbox/VTCompressionSession.h>
#include <VideoToolbox/VTDecompressionSession.h>
#include <VideoToolbox/VTErrors.h>

#include "magic_enum/include/magic_enum.hpp"
#include "AvfPixelBuffer.h"



#define CV_VIDEO_TYPE_META(Enum,SoyFormat)	TCvVideoTypeMeta( Enum, #Enum, SoyFormat )
#define CV_VIDEO_INVALID_ENUM		0
class TCvVideoTypeMeta
{
public:
	TCvVideoTypeMeta(OSType Enum,const char* EnumName,SoyPixelsFormat::Type SoyFormat) :
	mPlatformFormat		( Enum ),
	mName				( EnumName ),
	mSoyFormat			( SoyFormat )
	{
		if ( !IsValid() )
		throw Soy::AssertException("Expected valid enum - or invalid enum is bad" );
	}
	TCvVideoTypeMeta() :
	mPlatformFormat		( CV_VIDEO_INVALID_ENUM ),
	mName				( "Invalid enum" ),
	mSoyFormat			( SoyPixelsFormat::Invalid )
	{
	}
	
	bool		IsValid() const		{	return mPlatformFormat != CV_VIDEO_INVALID_ENUM;	}
	
	bool		operator==(const OSType& Enum) const					{	return mPlatformFormat == Enum;	}
	bool		operator==(const SoyPixelsFormat::Type& Format) const	{	return mSoyFormat == Format;	}
	
public:
	OSType					mPlatformFormat;
	SoyPixelsFormat::Type	mSoyFormat;
	std::string				mName;
};


std::ostream& operator<<(std::ostream& out,const AVAssetExportSessionStatus& in)
{
	out << magic_enum::enum_name(in);
	return out;
}


static TCvVideoTypeMeta Cv_PixelFormatMap[] =
{
	/*
	 //	from avfDecoder ResolveFormat(id)
	 //	gr: RGBA never seems to decode, but with no error[on osx]
	 case SoyPixelsFormat::RGBA:
	 //	BGRA is internal format on IOS so return that as default
	 case SoyPixelsFormat::BGRA:
	 default:
	 */
	
	CV_VIDEO_TYPE_META( kCVPixelFormatType_OneComponent8,	SoyPixelsFormat::Luma ),
	
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24RGB,	SoyPixelsFormat::RGB ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24BGR,	SoyPixelsFormat::BGR ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32BGRA,	SoyPixelsFormat::BGRA ),
	
	//	gr: PopFace creating a pixel buffer from a unity "argb" texture, failed as RGBA is unsupported...
	//	gr: ARGB is accepted, but channels are wrong
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32RGBA,	SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ARGB,	SoyPixelsFormat::ARGB ),
	
	//	gr: no colourspace distinction!
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,	SoyPixelsFormat::Yuv_8_88 ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarFullRange,	SoyPixelsFormat::Yuv_8_88 ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr_4A_8BiPlanar,	SoyPixelsFormat::Invalid ),	//	YuvAlpha_8888_8
	
	//	gr: this is CHROMA|YUV! not YUV, this is why the fourcc is 2vuy
	//		kCVPixelFormatType_422YpCbCr8     = '2vuy',     /* Component Y'CbCr 8-bit 4:2:2, ordered Cb Y'0 Cr Y'1 */
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8,	SoyPixelsFormat::Uvy_8_88 ),
	
	//	todo: check these
	//	gr: no colourspace distinction!
	//	no planar = interlaced
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8FullRange,	SoyPixelsFormat::YYuv_8888 ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8_yuvs,	SoyPixelsFormat::YYuv_8888 ),
	
	//	gr: planar = 3 planes
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8Planar,	SoyPixelsFormat::Yuv_8_8_8 ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8PlanarFullRange,	SoyPixelsFormat::Yuv_8_8_8 ),
	
	
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
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ABGR,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_64ARGB,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_48RGB,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32AlphaGray,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_16Gray,	SoyPixelsFormat::Invalid ),
	
	
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4444YpCbCrA8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_4444YpCbCrA8R,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_444YpCbCr8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr16,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr10,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_444YpCbCr10,	SoyPixelsFormat::Invalid ),
	
	//	the logitech C22 has this format, which apparently might be a kind of motion jpeg
	CV_VIDEO_TYPE_META( 'dmb1',	SoyPixelsFormat::Invalid ),

	//	hdis, fdis
	CV_VIDEO_TYPE_META( kCVPixelFormatType_DisparityFloat32,	SoyPixelsFormat::DepthDisparityFloat ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_DisparityFloat16,	SoyPixelsFormat::DepthDisparityHalf ),
	//	hdep, fdep
	CV_VIDEO_TYPE_META( kCVPixelFormatType_DepthFloat32,	SoyPixelsFormat::DepthFloatMetres ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_DepthFloat16,	SoyPixelsFormat::DepthHalfMetres ),
};



/*
std::shared_ptr<TMediaPacket> Avf::GetFormatDescriptionPacket(CMSampleBufferRef SampleBuffer,size_t ParamIndex,SoyMediaFormat::Type Format,size_t StreamIndex)
{
	auto Desc = CMSampleBufferGetFormatDescription( SampleBuffer );
	
	std::shared_ptr<TMediaPacket> pPacket( new TMediaPacket() );
	auto& Packet = *pPacket;
	Packet.mMeta = GetStreamMeta( Desc );
	Packet.mMeta.mStreamIndex = StreamIndex;
	Packet.mMeta.mCodec = Format;
	
	auto& Data = Packet.mData;
	Data.PushBack(0);
	Data.PushBack(0);
	Data.PushBack(0);
	Data.PushBack(1);
	//	http://stackoverflow.com/questions/24884827/possible-locations-for-sequence-picture-parameter-sets-for-h-264-stream
	if ( Format == SoyMediaFormat::H264_SPS_ES )
	{
		//	https://cardinalpeak.com/blog/the-h-264-sequence-parameter-set/
		//	https://tools.ietf.org/html/rfc6184#section-7.4.1
		auto Byte = H264::EncodeNaluByte( H264NaluContent::SequenceParameterSet,H264NaluPriority::Important );
		Data.PushBack( Byte );
	}
	
	if ( Format == SoyMediaFormat::H264_PPS_ES )
	{
		auto Byte = H264::EncodeNaluByte(H264NaluContent::PictureParameterSet, H264NaluPriority::Important );
		Data.PushBack( Byte );
	}

	
	GetFormatDescriptionData( GetArrayBridge( Data ), Desc, ParamIndex );
	return pPacket;
}
*/


SoyPixelsMeta Avf::GetPixelMeta(CVPixelBufferRef PixelBuffer)
{
	auto Height = CVPixelBufferGetHeight( PixelBuffer );
	auto Width = CVPixelBufferGetWidth( PixelBuffer );
	auto Format = CVPixelBufferGetPixelFormatType( PixelBuffer );
	auto SoyFormat = Avf::GetPixelFormat( Format );
	return SoyPixelsMeta( Width, Height, SoyFormat );
}

void Avf::GetFormatDescriptionData(ArrayBridge<uint8>&& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex)
{
	size_t ParamCount = 0;
	auto Result = CMVideoFormatDescriptionGetH264ParameterSetAtIndex( FormatDesc, 0, nullptr, nullptr, &ParamCount, nullptr );
	Avf::IsOkay( Result, "Get H264 param 0");
	
	/*
	 //	known bug on ios?
	 if (status ==
		CoreMediaGlue::kCMFormatDescriptionBridgeError_InvalidParameter) {
		DLOG(WARNING) << " assuming 2 parameter sets and 4 bytes NAL length header";
		pset_count = 2;
		nal_size_field_bytes = 4;
	 */
	
	if ( ParamIndex >= ParamCount )
		throw Soy::AssertException("SPS missing");
	
	const uint8_t* ParamsData = nullptr;;
	size_t ParamsSize = 0;
	Result = CMVideoFormatDescriptionGetH264ParameterSetAtIndex( FormatDesc, ParamIndex, &ParamsData, &ParamsSize, nullptr, nullptr);
	
	Avf::IsOkay( Result, "Failed to get H264 param X" );
	
	Data.PushBackArray( GetRemoteArray( ParamsData, ParamsSize ) );
}



//	lots of errors in macerrors.h with no string conversion :/
#define TESTENUMERROR(e,Enum)	if ( (e) == (Enum) )	return #Enum ;


//	http://stackoverflow.com/questions/2196869/how-do-you-convert-an-iphone-osstatus-code-to-something-useful
std::string Avf::GetString(OSStatus Status)
{
	//[NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil]);
	
	TESTENUMERROR(Status,kVTPropertyNotSupportedErr);
	TESTENUMERROR(Status,kVTPropertyReadOnlyErr);
	TESTENUMERROR(Status,kVTParameterErr);
	TESTENUMERROR(Status,kVTInvalidSessionErr);
	TESTENUMERROR(Status,kVTAllocationFailedErr);
	TESTENUMERROR(Status,kVTPixelTransferNotSupportedErr);
	TESTENUMERROR(Status,kVTCouldNotFindVideoDecoderErr);
	TESTENUMERROR(Status,kVTCouldNotCreateInstanceErr);
	TESTENUMERROR(Status,kVTCouldNotFindVideoEncoderErr);
	TESTENUMERROR(Status,kVTVideoDecoderBadDataErr);
	TESTENUMERROR(Status,kVTVideoDecoderUnsupportedDataFormatErr);
	TESTENUMERROR(Status,kVTVideoDecoderMalfunctionErr);
	TESTENUMERROR(Status,kVTVideoEncoderMalfunctionErr);
	TESTENUMERROR(Status,kVTVideoDecoderNotAvailableNowErr);
	TESTENUMERROR(Status,kVTImageRotationNotSupportedErr);
	TESTENUMERROR(Status,kVTVideoEncoderNotAvailableNowErr);
	TESTENUMERROR(Status,kVTFormatDescriptionChangeNotSupportedErr);
	TESTENUMERROR(Status,kVTInsufficientSourceColorDataErr);
	TESTENUMERROR(Status,kVTCouldNotCreateColorCorrectionDataErr);
	TESTENUMERROR(Status,kVTColorSyncTransformConvertFailedErr);
	TESTENUMERROR(Status,kVTVideoDecoderAuthorizationErr);
	TESTENUMERROR(Status,kVTVideoEncoderAuthorizationErr);
	TESTENUMERROR(Status,kVTColorCorrectionPixelTransferFailedErr);
	TESTENUMERROR(Status,kVTMultiPassStorageIdentifierMismatchErr);
	TESTENUMERROR(Status,kVTMultiPassStorageInvalidErr);
	TESTENUMERROR(Status,kVTFrameSiloInvalidTimeStampErr);
	TESTENUMERROR(Status,kVTFrameSiloInvalidTimeRangeErr);
	TESTENUMERROR(Status,kVTCouldNotFindTemporalFilterErr);
	TESTENUMERROR(Status,kVTPixelTransferNotPermittedErr);
	TESTENUMERROR(Status,kVTColorCorrectionImageRotationFailedErr);
	TESTENUMERROR(Status,kVTVideoDecoderRemovedErr);
	
	TESTENUMERROR(Status,kCVReturnInvalidArgument);
	
	TESTENUMERROR(Status,kCMBlockBufferStructureAllocationFailedErr);
	TESTENUMERROR(Status,kCMBlockBufferBlockAllocationFailedErr);
	TESTENUMERROR(Status,kCMBlockBufferBadCustomBlockSourceErr);
	TESTENUMERROR(Status,kCMBlockBufferBadOffsetParameterErr);
	TESTENUMERROR(Status,kCMBlockBufferBadLengthParameterErr);
	TESTENUMERROR(Status,kCMBlockBufferBadPointerParameterErr);
	TESTENUMERROR(Status,kCMBlockBufferEmptyBBufErr);
	TESTENUMERROR(Status,kCMBlockBufferUnallocatedBlockErr);
	TESTENUMERROR(Status,kCMBlockBufferInsufficientSpaceErr);
	
	TESTENUMERROR(Status,kCVReturnInvalidArgument);
	TESTENUMERROR(Status,kCVReturnAllocationFailed);
#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
	TESTENUMERROR(Status,kCVReturnUnsupported);
#endif
	TESTENUMERROR(Status,kCVReturnInvalidDisplay);
	TESTENUMERROR(Status,kCVReturnDisplayLinkAlreadyRunning);
	TESTENUMERROR(Status,kCVReturnDisplayLinkNotRunning);
	TESTENUMERROR(Status,kCVReturnDisplayLinkCallbacksNotSet);
	TESTENUMERROR(Status,kCVReturnInvalidPixelFormat);
	TESTENUMERROR(Status,kCVReturnInvalidSize);
	TESTENUMERROR(Status,kCVReturnInvalidPixelBufferAttributes);
	TESTENUMERROR(Status,kCVReturnPixelBufferNotOpenGLCompatible);
	TESTENUMERROR(Status,kCVReturnPixelBufferNotMetalCompatible);
	TESTENUMERROR(Status,kCVReturnWouldExceedAllocationThreshold);
	TESTENUMERROR(Status,kCVReturnPoolAllocationFailed);
	TESTENUMERROR(Status,kCVReturnInvalidPoolAttributes);
	
	//	decompression gives us this
	TESTENUMERROR(Status,MACH_RCV_TIMED_OUT);

	TESTENUMERROR(Status,kCVReturnInvalidPoolAttributes);

	//	corefoundation versions of VT errors
	switch ( static_cast<sint32>(Status) )
	{
		case -8961:	return "kVTPixelTransferNotSupportedErr -8961";
		case -8969:	return "kVTVideoDecoderBadDataErr -8969";
		case -8970:	return "kVTVideoDecoderUnsupportedDataFormatErr -8970";
		case -8960:	return "kVTVideoDecoderMalfunctionErr -8960";
		default:
			break;
	}
	
	//	gr: can't find any documentation on this value.
	if ( static_cast<sint32>(Status) == -12349 )
		return "Unknown VTDecodeFrame error -12349";
	
	//	gr: can't find any documentation on this value.
	if ( static_cast<sint32>(Status) == -12780 )
		return "Unknown VTCompressionSessionEncodeFrame error -12780. On IOS this seems fail with a YUV format but not BGRA32";

	
	//	as integer..
	std::stringstream Error;
	Error << "OSStatus = " << static_cast<sint32>(Status);
	return Error.str();
	
	//	could be fourcc?
	Soy::TFourcc Fourcc( CFSwapInt32HostToBig(Status) );
	return Fourcc.GetString();
	/*
	 //	example with specific bundles...
	 NSBundle *bundle = [NSBundle bundleWithIdentifier:@"com.apple.security"];
	 NSString *key = [NSString stringWithFormat:@"%d", (int)Status];
	 auto* StringNs = [bundle localizedStringForKey:key value:key table:@"SecErrorMessages"];
	 return Soy::NSStringToString( StringNs );
	 */
}
/*
 std::shared_ptr<AvfCompressor::TInstance> AvfCompressor::Allocate(const TCasterParams& Params)
 {
	return std::shared_ptr<AvfCompressor::TInstance>( new AvfCompressor::TInstance(Params) );
 }
 */


CFStringRef Avf::GetProfile(H264Profile::Type Profile)
{
	switch ( Profile )
	{
		case H264Profile::Baseline:	return kVTProfileLevel_H264_Baseline_AutoLevel;
			
		default:
			std::Debug << "Unhandled profile type " << Profile << " using baseline" << std::endl;
			return kVTProfileLevel_H264_Baseline_AutoLevel;
	}
}

/*
CMFormatDescriptionRef Avf::GetFormatDescription(const TStreamMeta& Stream)
{
	CFAllocatorRef Allocator = nil;
	CMFormatDescriptionRef FormatDesc = nullptr;
	
	
	if ( SoyMediaFormat::IsH264( Stream.mCodec ) )
	{
		auto& Sps = Stream.mSps;
		auto& Pps = Stream.mPps;
		if ( Sps.IsEmpty() )	throw Soy::AssertException("H264 encoder requires SPS beforehand" );
		if ( Pps.IsEmpty() )	throw Soy::AssertException("H264 encoder requires PPS beforehand" );
		
		BufferArray<const uint8_t*,2> Params;
		BufferArray<size_t,2> ParamSizes;
		Params.PushBack( Sps.GetArray() );
		ParamSizes.PushBack( Sps.GetDataSize() );
		Params.PushBack( Pps.GetArray() );
		ParamSizes.PushBack( Pps.GetDataSize() );
		
		auto NaluLengthSize = H264::GetNaluLengthSize( Stream.mCodec );
		
		//	don't think ios supports annexb, so we will have to convert, assume we can do this later...
		if ( NaluLengthSize == 0 )
		{
			//	gr: save this somewhere for when we convert frames from annexb
			NaluLengthSize = H264::GetNaluLengthSize( SoyMediaFormat::H264_32 );
			std::Debug << "Avf compressor doesn't support annexb/h264 elementry streams, forcing " << NaluLengthSize*8 << "bit NALU header size" << std::endl;
		}
		
		//	-12712 http://stackoverflow.com/questions/25078364/cmvideoformatdescriptioncreatefromh264parametersets-issues
		auto Result = CMVideoFormatDescriptionCreateFromH264ParameterSets( Allocator, Params.GetSize(), Params.GetArray(), ParamSizes.GetArray(), size_cast<int>(NaluLengthSize), &FormatDesc );
		Avf::IsOkay( Result, "CMVideoFormatDescriptionCreateFromH264ParameterSets" );
	}
	else
	{
		CFDictionaryRef Extensions = nullptr;
		CMMediaType MediaType;
		FourCharCode MediaCodec;
		GetMediaType( MediaType, MediaCodec, Stream.mCodec );
		/ *
		 //	extensions to dictionary
		 if ( !Stream.mExtensions.IsEmpty() )
		 {
			auto Data = ArrayToNSData( GetArrayBridge(Stream.mExtensions) );
			NSError* Error = nullptr;
			NSPropertyListReadOptions Options;
			NSDictionary* ExtensionsDict = [NSPropertyListSerialization propertyListWithData:Data options:Options format:nullptr error:&Error];
		 
			Extensions = (__bridge CFDictionaryRef)ExtensionsDict;
		 }
		 *//*
		auto Result = CMFormatDescriptionCreate( Allocator, MediaType, MediaCodec, Extensions, &FormatDesc );
		Avf::IsOkay( Result, "CMFormatDescriptionCreate" );
	}
		
	return FormatDesc;
}
			*/



//	gr: speed this up! (or reduce usage) all the obj-c calls are expensive.
TStreamMeta Avf::GetStreamMeta(CMFormatDescriptionRef FormatDesc)
{
	TStreamMeta Meta;
	auto FourccOrig = CMFormatDescriptionGetMediaSubType(FormatDesc);
	Soy::TFourcc Fourcc(FourccOrig);

	size_t H264LengthSize = 0;
	
	//if ( SoyMediaFormat::IsH264Fourcc(Fourcc) )
	{
		//	gr: this is okay if it goes wrong, probably just doesn't apply to this format
		//auto Result =
		int NalUnitLength = 0;
		CMVideoFormatDescriptionGetH264ParameterSetAtIndex( FormatDesc, 0, nullptr, nullptr, nullptr, &NalUnitLength );
		if ( NalUnitLength < 0 )
			NalUnitLength = 0;
		H264LengthSize = size_cast<size_t>( NalUnitLength );
		//AvfCompressor::IsOkay( Result, "Get H264 param NAL size", false );
	}
	
	//	PCM needs a number too
	{
		auto* pAudioFormat = CMAudioFormatDescriptionGetStreamBasicDescription( FormatDesc );
		if ( pAudioFormat )
		{
			auto& AudioFormat = *pAudioFormat;
			Meta.mAudioSampleRate = AudioFormat.mSampleRate;
			Meta.mAudioBytesPerPacket = AudioFormat.mBytesPerPacket;
			Meta.mAudioBytesPerFrame = AudioFormat.mBytesPerFrame;
			Meta.mAudioFramesPerPacket = AudioFormat.mFramesPerPacket;
			Meta.mChannelCount = AudioFormat.mChannelsPerFrame;

			//	change this to be like H264 different formats; AAC_8, AAC_16, AAC_float etc
			Meta.mAudioBitsPerChannel = AudioFormat.mBitsPerChannel;
			H264LengthSize = size_cast<size_t>(Meta.mAudioBitsPerChannel);
			
			//	gr: if auto bits per channel is zero, it's a compressed format
			if ( H264LengthSize == 0 )
				Meta.mCompressed = true;
		}
	}
	
	Meta.mCodecFourcc = Fourcc;
	
	if ( SoyMediaFormat::IsH264( Meta.mCodecFourcc ) )
	{
		Avf::GetFormatDescriptionData( GetArrayBridge(Meta.mSps), FormatDesc, 0 );
		Avf::GetFormatDescriptionData( GetArrayBridge(Meta.mPps), FormatDesc, 1 );
	}
	
	Boolean usePixelAspectRatio = false;
	Boolean useCleanAperture = false;
	auto Dim = CMVideoFormatDescriptionGetPresentationDimensions( FormatDesc, usePixelAspectRatio, useCleanAperture );
	Meta.mPixelMeta.DumbSetWidth( Dim.width );
	Meta.mPixelMeta.DumbSetHeight( Dim.height );
	Meta.mPixelMeta.DumbSetFormat( GetPixelFormat( Meta.mCodecFourcc ) );
	
	//std::stringstream Debug;
	//Debug << "Extensions=" << Soy::Platform::GetExtensions( FormatDesc ) << "; ";
	

	//	get specific bits
	//GetExtension( Desc, "FullRangeVideo", mMeta.mFullRangeYuv );
	
	//	pull out audio meta
	{
		auto* pAudioFormat = CMAudioFormatDescriptionGetStreamBasicDescription( FormatDesc );
		if ( pAudioFormat )
		{
			auto& AudioFormat = *pAudioFormat;
			Meta.mAudioSampleRate = AudioFormat.mSampleRate;
			Meta.mAudioBytesPerPacket = AudioFormat.mBytesPerPacket;
			Meta.mAudioBytesPerFrame = AudioFormat.mBytesPerFrame;
			Meta.mAudioFramesPerPacket = AudioFormat.mFramesPerPacket;
			Meta.mChannelCount = AudioFormat.mChannelsPerFrame;

			//	change this to be like H264 different formats; AAC_8, AAC_16, AAC_float etc
			Meta.mAudioBitsPerChannel = AudioFormat.mBitsPerChannel;
			
			//	flags per format
			static bool DebugFlags = false;
			if ( DebugFlags )
				std::Debug << Meta.mCodec << " sample flags: " << AudioFormat.mFormatFlags << std::endl;
		}
		else if ( SoyMediaFormat::IsAudio(Meta.mCodec) )
		{
			std::Debug << "Warning, format has audio data, but we've detected non-audio format (" << Meta.mCodec << ")" << std::endl;
		}
	}
	/*
	//	test validity of the conversion (slow!)
	static bool DoCompareFormatIntegrity = false;
	if ( DoCompareFormatIntegrity )
	{
		auto TestFormat = GetFormatDescription( Meta );
		if ( !CMFormatDescriptionEqual ( TestFormat, FormatDesc ) )
		{
			/*
			auto ExtensionsOld = CMFormatDescriptionGetExtensions( FormatDesc );
			auto MediaSubTypeOld = CMFormatDescriptionGetMediaSubType( FormatDesc );
			auto MediaTypeOld = CMFormatDescriptionGetMediaType( FormatDesc );
			auto PresentationDimOld = CMVideoFormatDescriptionGetPresentationDimensions( FormatDesc, true, true );
			auto DimOld = CMVideoFormatDescriptionGetDimensions( FormatDesc );
			auto ExtensionsStringOld = Soy::NSDictionaryToString( ExtensionsOld );
			
			auto ExtensionsNew = CMFormatDescriptionGetExtensions( TestFormat );
			auto MediaSubTypeNew = CMFormatDescriptionGetMediaSubType( TestFormat );
			auto MediaTypeNew = CMFormatDescriptionGetMediaType( TestFormat );
			auto PresentationDimNew = CMVideoFormatDescriptionGetPresentationDimensions( TestFormat, true, true );
			auto DimNew = CMVideoFormatDescriptionGetDimensions( TestFormat );
			auto ExtensionsStringNew = Soy::NSDictionaryToString( ExtensionsNew );
			*//*
			std::Debug << "Warning: generated meta from description, but not equal when converted back again. " << Meta << std::endl;
		}
	}
	*/
	
	return Meta;
}


/*
void Avf::GetMediaType(CMMediaType& MediaType,FourCharCode& MediaCodec,SoyMediaFormat::Type Format)
{
	if ( SoyMediaFormat::IsVideo(Format) )
	{
		MediaType = kCMMediaType_Video;
	}
	else if ( SoyMediaFormat::IsAudio(Format) )
	{
		MediaType = kCMMediaType_Audio;
	}
	else
	{
		//	throw?
		std::stringstream Error;
		Error << __func__ << " don't know what CMMediaType to use for " << Format;
		throw Soy::AssertException( Error.str() );
	}

	//	using cross platform code instead of AVF definitions like
	//		kCMVideoCodecType_H264
	MediaCodec = SoyMediaFormat::ToFourcc( Format );
}
*/

/*
Avf::TAsset::TAsset(const std::string& Filename)
{
	//	alloc asset
	auto Url = GetUrl( Filename );
	
	NSDictionary *Options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:AVURLAssetPreferPreciseDurationAndTimingKey];
	AVURLAsset* Asset = [[AVURLAsset alloc] initWithURL:Url options:Options];
	
	if ( !Asset )
		throw Soy::AssertException("Failed to create asset");
	
	if ( !Asset.readable )
		std::Debug << "Warning: Asset " << Filename << " reported as not-readable" << std::endl;
	
	mAsset.Retain( Asset );
	LoadTracks();
	
}


void Avf::TAsset::LoadTracks()
{
	AVAsset* Asset = mAsset.mObject;
	if ( !Asset )
		throw Soy::AssertException("Asset expected");
	
	Soy::TSemaphore LoadTracksSemaphore;
	__block Soy::TSemaphore& Semaphore = LoadTracksSemaphore;
	
	auto OnCompleted = ^
	{
		NSError* err = nil;
		auto Status = [Asset statusOfValueForKey:@"tracks" error:&err];
		if ( Status != AVKeyValueStatusLoaded)
		{
			std::stringstream Error;
			Error << "Error loading tracks: " << Soy::NSErrorToString(err);
			Semaphore.OnFailed( Error.str().c_str() );
			return;
		}
		
		Semaphore.OnCompleted();
	};
	
	//	load tracks
	NSArray* Keys = [NSArray arrayWithObjects:@"tracks",@"playable",@"hasProtectedContent",@"duration",@"preferredTransform",nil];
	[Asset loadValuesAsynchronouslyForKeys:Keys completionHandler:OnCompleted];
	
	LoadTracksSemaphore.Wait();
	
	//	grab duration
	//	gr: not valid here with AVPlayer!
	//	http://stackoverflow.com/a/7052147/355753
	auto Duration = Soy::Platform::GetTime( Asset.duration );
	
	//	get meta for each track
	NSArray* Tracks = [Asset tracks];
	for ( int t=0;	t<[Tracks count];	t++ )
	{
		//	get and retain track
		AVAssetTrack* Track = [Tracks objectAtIndex:t];
		if ( !Track )
			continue;
		
		TStreamMeta TrackMeta;
		TrackMeta.mStreamIndex = t;
		TrackMeta.mEncodingBitRate = Track.estimatedDataRate;
		
		//	extract meta from track
		NSArray* FormatDescriptions = [Track formatDescriptions];
		std::stringstream MetaDebug;
		if ( !FormatDescriptions )
		{
			MetaDebug << "Format descriptions missing";
		}
		else if ( FormatDescriptions.count == 0 )
		{
			MetaDebug << "Format description count=0";
		}
		else
		{
			//	use first description, warn if more
			if ( FormatDescriptions.count > 1 )
			{
				MetaDebug << "Found mulitple(" << FormatDescriptions.count << ") format descriptions. ";
			}
			
			id DescElement = FormatDescriptions[0];
			CMFormatDescriptionRef Desc = (__bridge CMFormatDescriptionRef)DescElement;
			
			
			TrackMeta = Avf::GetStreamMeta( Desc );
			
			//	save format
			mStreamFormats[t].reset( new Platform::TMediaFormat( Desc ) );
		}
		
		
		//	grab the transform from the video track
		TrackMeta.mPixelMeta.DumbSetWidth( Track.naturalSize.width );
		TrackMeta.mPixelMeta.DumbSetHeight( Track.naturalSize.height );
		TrackMeta.mTransform = Soy::MatrixToVector( Track.preferredTransform, vec2f(TrackMeta.mPixelMeta.GetWidth(),TrackMeta.mPixelMeta.GetHeight()) );
		
		TrackMeta.mDuration = Duration;
		
		mStreams.PushBack( TrackMeta );
	}
	
}
*/


CFStringRef Avf::GetProfile(H264Profile::Type Profile,Soy::TVersion Level)
{
	std::map<size_t,CFStringRef> BaselineLevels;
	BaselineLevels[0] = kVTProfileLevel_H264_Baseline_AutoLevel;
	BaselineLevels[103] = kVTProfileLevel_H264_Baseline_1_3;
	BaselineLevels[300] = kVTProfileLevel_H264_Baseline_3_0;
	BaselineLevels[301] = kVTProfileLevel_H264_Baseline_3_1;
	BaselineLevels[302] = kVTProfileLevel_H264_Baseline_3_2;
	BaselineLevels[400] = kVTProfileLevel_H264_Baseline_4_0;
	BaselineLevels[401] = kVTProfileLevel_H264_Baseline_4_1;
	BaselineLevels[402] = kVTProfileLevel_H264_Baseline_4_2;
	BaselineLevels[500] = kVTProfileLevel_H264_Baseline_5_0;
	BaselineLevels[501] = kVTProfileLevel_H264_Baseline_5_1;
	BaselineLevels[502] = kVTProfileLevel_H264_Baseline_5_2;
	
	std::map<size_t,CFStringRef> MainLevels;
	MainLevels[0] = kVTProfileLevel_H264_Main_AutoLevel;
	MainLevels[300] = kVTProfileLevel_H264_Main_3_0;
	MainLevels[301] = kVTProfileLevel_H264_Main_3_1;
	MainLevels[302] = kVTProfileLevel_H264_Main_3_2;
	MainLevels[400] = kVTProfileLevel_H264_Main_4_0;
	MainLevels[401] = kVTProfileLevel_H264_Main_4_1;
	MainLevels[402] = kVTProfileLevel_H264_Main_4_2;
	MainLevels[500] = kVTProfileLevel_H264_Main_5_0;
	MainLevels[501] = kVTProfileLevel_H264_Main_5_1;
	MainLevels[502] = kVTProfileLevel_H264_Main_5_2;
	
	std::map<size_t,CFStringRef> HighLevels;
	HighLevels[0] = kVTProfileLevel_H264_High_AutoLevel;
	HighLevels[300] = kVTProfileLevel_H264_High_3_0;
	HighLevels[301] = kVTProfileLevel_H264_High_3_1;
	HighLevels[302] = kVTProfileLevel_H264_High_3_2;
	HighLevels[400] = kVTProfileLevel_H264_High_4_0;
	HighLevels[401] = kVTProfileLevel_H264_High_4_1;
	HighLevels[402] = kVTProfileLevel_H264_High_4_2;
	HighLevels[500] = kVTProfileLevel_H264_High_5_0;
	HighLevels[501] = kVTProfileLevel_H264_High_5_1;
	HighLevels[502] = kVTProfileLevel_H264_High_5_2;
	
	std::map<size_t,CFStringRef> ExtendedLevels;
	ExtendedLevels[0] = kVTProfileLevel_H264_Extended_AutoLevel;
	ExtendedLevels[500] = kVTProfileLevel_H264_Extended_5_0;
	
	
	switch ( Profile )
	{
		case H264Profile::Baseline:	return BaselineLevels[Level.GetHundred()];
		case H264Profile::Main:		return MainLevels[Level.GetHundred()];
		case H264Profile::High:		return HighLevels[Level.GetHundred()];
		case H264Profile::Extended:	return ExtendedLevels[Level.GetHundred()];
		default:
			std::Debug << "Unhandled profile type " << Profile << " using baseline" << std::endl;
			return kVTProfileLevel_H264_Baseline_AutoLevel;
	}
}

/*
NSString* const Avf::GetFormatType(SoyMediaFormat::Type Format)
{
	if ( SoyMediaFormat::IsVideo( Format ) )
		return AVMediaTypeVideo;
	if ( SoyMediaFormat::IsAudio( Format ) )
		return AVMediaTypeAudio;
	
	std::stringstream Error;
	Error << "Cannot convert " << Format << " to AVMediaType";
	throw Soy::AssertException( Error.str() );
}
*/

const std::map<std::string,NSString *>& GetFileExtensionTypeConversion()
{
	static std::map<std::string,NSString *> FileExtensionToType;
	if ( FileExtensionToType.empty() )
	{
		FileExtensionToType["mov"] = AVFileTypeQuickTimeMovie;
		FileExtensionToType["qt"] = AVFileTypeQuickTimeMovie;
		FileExtensionToType["mp4"] = AVFileTypeMPEG4;
		FileExtensionToType["m4v"] = AVFileTypeAppleM4V;
		FileExtensionToType["m4a"] = AVFileTypeAppleM4A;
	#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
		FileExtensionToType["3gp"] = AVFileType3GPP;
		FileExtensionToType["3gpp"] = AVFileType3GPP;
		FileExtensionToType["sdv"] = AVFileType3GPP;
		FileExtensionToType["3g2"] = AVFileType3GPP2;
		FileExtensionToType["3gp2"] = AVFileType3GPP2;
	#endif
		FileExtensionToType["caf"] = AVFileTypeCoreAudioFormat;
		FileExtensionToType["wav"] = AVFileTypeWAVE;
		FileExtensionToType["wave"] = AVFileTypeWAVE;
		FileExtensionToType["aif"] = AVFileTypeAIFF;
		FileExtensionToType["aiff"] = AVFileTypeAIFF;
		FileExtensionToType["aifc"] = AVFileTypeAIFC;
		FileExtensionToType["cdda"] = AVFileTypeAIFC;
		FileExtensionToType["amr"] = AVFileTypeAMR;
		FileExtensionToType["mp3"] = AVFileTypeMPEGLayer3;
		FileExtensionToType["au"] = AVFileTypeSunAU;
		FileExtensionToType["ac3"] = AVFileTypeAC3;
	#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
		FileExtensionToType["eac3"] = AVFileTypeEnhancedAC3;
	#endif
	}
	return FileExtensionToType;
}

NSString* const Avf::GetFileExtensionType(const std::string& Extension)
{
	auto& FileExtensionToType = GetFileExtensionTypeConversion();
	
	auto FileTypeIt = FileExtensionToType.find( Extension );
	if ( FileTypeIt == FileExtensionToType.end() )
	{
		std::stringstream Error;
		Error << "Failed to match filename extension (" <<  Extension << ") to file type";
		throw Soy::AssertException( Error.str() );
	}
	
	return FileTypeIt->second;
}

void Avf::GetFileExtensions(ArrayBridge<std::string>&& Extensions)
{
	auto& FileExtensionToType = GetFileExtensionTypeConversion();
	for ( auto& it : FileExtensionToType )
	{
		Extensions.PushBack( it.first );
	}
}

bool Avf::IsFormatCompressed(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_8:
		case SoyMediaFormat::H264_16:
		case SoyMediaFormat::H264_32:
		case SoyMediaFormat::H264_ES:

		case SoyMediaFormat::Aac:
			return true;
			
		default:
			return false;
	}
}

bool Avf::IsKeyframe(CMSampleBufferRef SampleBuffer,bool DefaultValue)
{
	if ( !SampleBuffer )
		return DefaultValue;

	//	code based on chromium
	//	https://chromium.googlesource.com/chromium/src/media/+/cea1808de66191f7f1eb48b5579e602c0c781146/cast/sender/h264_vt_encoder.cc
	auto Attachments = CMSampleBufferGetSampleAttachmentsArray(SampleBuffer,false);
	if ( !Attachments )
		return DefaultValue;

	//	gr: why 0? more documentation please chromium
	if ( CFArrayGetCount(Attachments) < 1 )
		return DefaultValue;
	auto sample_attachments = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(Attachments, 0));
	if ( !sample_attachments )
		return DefaultValue;
	
	// If the NotSync key is not present, it implies Sync, which indicates a
	// keyframe (at least I think, VT documentation is, erm, sparse). Could
	// alternatively use kCMSampleAttachmentKey_DependsOnOthers == false.
	//bool IsDependent = CFDictionaryContainsKey(sample_attachments,kCMSampleAttachmentKey_DependsOnOthers);
	bool NotSync = CFDictionaryContainsKey(sample_attachments,kCMSampleAttachmentKey_NotSync);
	//bool PartialSync = CFDictionaryContainsKey(sample_attachments,kCMSampleAttachmentKey_PartialSync);
	//bool IsDependedOnByOthers = CFDictionaryContainsKey(sample_attachments,kCMSampleAttachmentKey_IsDependedOnByOthers);
	
	//	bool Keyframe = (!NotSync) || (!IsDependent);
	bool Keyframe = (!NotSync);
	return Keyframe;
}


std::string Avf::GetPixelFormatString(OSType Format)
{
	auto Table = GetRemoteArray( Cv_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
	{
		Soy::TFourcc Fourcc(Format);
		std::stringstream Output;
		Output << "<Unknown format " << Fourcc << ">";
		return Output.str();
	}

	return Meta->mName;
}

OSType Avf::GetPlatformPixelFormat(SoyPixelsFormat::Type Format)
{
	auto Table = GetRemoteArray( Cv_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return CV_VIDEO_INVALID_ENUM;
	
	return Meta->mPlatformFormat;
}

SoyPixelsFormat::Type Avf::GetPixelFormat(OSType Format)
{
	auto Table = GetRemoteArray( Cv_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
	{
		Soy::TFourcc Fourcc(Format);
		std::Debug << "Unknown Avf CV pixel format (" << Fourcc << " 0x" << std::hex << Format << ")" << std::dec << std::endl;
		
		return SoyPixelsFormat::Invalid;
	}
	return Meta->mSoyFormat;
}


SoyPixelsFormat::Type Avf::GetPixelFormat(const Soy::TFourcc& Fourcc)
{
	return GetPixelFormat(Fourcc.mFourcc32);
}

SoyPixelsFormat::Type Avf::GetPixelFormat(NSNumber* Format)
{
	auto FormatInt = [Format integerValue];
	return GetPixelFormat( static_cast<OSType>(FormatInt) );
}


std::string Avf::GetPixelFormatString(NSNumber* Format)
{
	auto FormatInt = [Format integerValue];
	return GetPixelFormatString( static_cast<OSType>(FormatInt) );
}

std::string Avf::GetPixelFormatString(id Format)
{
	auto FormatInt = [Format integerValue];
	return GetPixelFormatString( static_cast<OSType>(FormatInt) );
}



vec2x<uint32_t> Avf::GetSize(CVImageBufferRef Image)
{
	auto Size = CVImageBufferGetDisplaySize( Image );
	
	//	todo: check non integer sizes
	if ( Size.width < 0 )
		throw Soy::AssertException("Not expecting negative width for image");
	
	if ( Size.height < 0 )
		throw Soy::AssertException("Not expecting negative width for image");
	
	return vec2x<uint32_t>( Size.width, Size.height );
}

void Avf::IsOkay(OSStatus Error,const std::string& Context)
{
	IsOkay( Error, Context.c_str() );
}

void Avf::IsOkay(OSStatus Error,const char* Context)
{
	//	kCVReturnSuccess
	if ( Error == noErr )
		return;
	
	std::stringstream ErrorString;
	ErrorString << "OSStatus/CVReturn error in " << Context << ": " << GetString(Error);
	
	throw Soy::AssertException( ErrorString.str() );
}


void PixelReleaseCallback(void *releaseRefCon, const void *baseAddress)
{
	//std::Debug << __func__ << std::endl;
	
	//	this page says we need to release
	//	http://codefromabove.com/2015/01/av-foundation-saving-a-sequence-of-raw-rgb-frames-to-a-movie/
	if ( releaseRefCon != nullptr )
	{
		CFDataRef bufferData = (CFDataRef)releaseRefCon;
		CFRelease(bufferData);
	}
}

//	gr: these create buffers that hold their lifetime AFTER this function, so the Image and the PixelBuffer ref must be kept together
//		todo: pass in a lambda that gets called when image is released
CVPixelBufferRef Avf::PixelsToPixelBuffer(const SoyPixelsImpl& Image)
{
	CFAllocatorRef PixelBufferAllocator = nullptr;
	OSType PixelFormatType = GetPlatformPixelFormat( Image.GetFormat() );
	void* ReleaseContext = nullptr;
	
#if defined(TARGET_OSX)
	//	gr: hack, cannot create RGBA pixel buffer on OSX. do a last-min conversion here, but ideally it's done beforehand
	//		REALLY ideally we can go from texture to CVPixelBuffer
	//	gr: this is the same case on IOS... I'm starting to think we should ditch this and caller should fix at higher level, maybe.
	if ( Image.GetFormat() == SoyPixelsFormat::RGBA && PixelFormatType == kCVPixelFormatType_32RGBA )
	{
		//std::Debug << "CVPixelBufferCreateWithBytes will fail with RGBA, forcing BGRA" << std::endl;
		PixelFormatType = kCVPixelFormatType_32BGRA;
	}
#endif
	
	BufferArray<std::shared_ptr<SoyPixelsImpl>,3> Planes;
	Image.SplitPlanes( GetArrayBridge(Planes));
	
	CVPixelBufferRef PixelBuffer = nullptr;
	
	CFDictionaryRef PixelBufferAttributes = nullptr;
	
	//	on ios, if we create YUV formats with pixel references, we no longer get an error (see notes below)
	//	but we do get OnCompressionCallback = -12902 kvtparametererr error
	//	the fix is to create a new buffer and write to it, not remote buffers (which maybe why we get this in the console vtCompressionSessionRemote_EncodeFrameCommon )
	if ( Planes.GetSize() > 1 )	//	&& ios?
	{
		auto Width = Planes[0]->GetWidth();
		auto Height = Planes[0]->GetHeight();
		auto Result = CVPixelBufferCreate( PixelBufferAllocator, Width, Height, PixelFormatType, PixelBufferAttributes, &PixelBuffer );
		Avf::IsOkay( Result, std::string("CVPixelBufferCreate ") + Soy::TFourcc(PixelFormatType).GetString() );
	
		//	we already have a nice accessor!
		float3x3 Transform;
		std::shared_ptr<AvfDecoderRenderer> Decoder;
		static bool DoRetain = true;
		CVPixelBuffer PixelBufferAccessor(PixelBuffer,DoRetain,Decoder,Transform);
		BufferArray<SoyPixelsImpl*,3> OutputPlanes;
		PixelBufferAccessor.Lock( GetArrayBridge(OutputPlanes),Transform);
		for ( auto i=0;	i<OutputPlanes.GetSize();	i++ )
		{
			auto& DstPlane = *OutputPlanes[i];
			auto& SrcPlane = *Planes[i];
			DstPlane.Copy(SrcPlane);
		}
		PixelBufferAccessor.Unlock();
	}
	else if ( Planes.GetSize() > 1 )	//	handle multiplane
	{
		//auto& Plane0 = *Planes[0];
		size_t Widths[3] = {0};
		size_t Heights[3] = {0};
		size_t RowSizes[3] = {0};
		void* Datas[3] = {nullptr};
		for ( auto p=0;	p<Planes.GetSize();	p++ )
		{
			auto& Plane = *Planes[p];
			Widths[p] = Plane.GetWidth();
			Heights[p] = Plane.GetHeight();
			RowSizes[p] = Plane.GetRowPitchBytes();
			Datas[p] = Plane.GetPixelsArray().GetArray();
		}
		
		auto GetPlanarMeta = [&](int PlaneIndex)
		{
			CVPlanarComponentInfo Meta;
			Meta.offset = 0;
			Meta.rowBytes = size_cast<uint32_t>(RowSizes[PlaneIndex]);
			for ( int i=0;	i<PlaneIndex;	i++ )
				Meta.offset += RowSizes[i] * Heights[i];
			return Meta;
		};
		
		CVPixelBufferReleasePlanarBytesCallback Callback = [](void* CallbackReference,const void* DataPtr,size_t DataSize,size_t PlaneCount,const void* PlaneAddresses[])
		{
			//std::Debug << "Pixel Buffer released" << std::endl;
		};
		void* CallbackReference = nullptr;

		//	gr: on ios we get a -12780 error when using planar formats
		//	BUT BGRA32 works.
		//	kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange also works but ONLY when we provide the CVPlanarPixelBufferInfo_YCbCrBiPlanar
		//		struct.
		//	OSX doesn't require it
		CVPlanarPixelBufferInfo_YCbCrBiPlanar PlanarMeta_8_88;
		PlanarMeta_8_88.componentInfoY = GetPlanarMeta(0);
		PlanarMeta_8_88.componentInfoCbCr = GetPlanarMeta(1);
		CVPlanarPixelBufferInfo_YCbCrPlanar PlanarMeta_8_8_8;
		PlanarMeta_8_8_8.componentInfoY = GetPlanarMeta(0);
		PlanarMeta_8_8_8.componentInfoCb = GetPlanarMeta(1);
		PlanarMeta_8_8_8.componentInfoCr = GetPlanarMeta(2);
		void* PlanarMetaStruct = nullptr;
		size_t PlanarMetaSize = 0;

		if ( PixelFormatType == kCVPixelFormatType_420YpCbCr8Planar || PixelFormatType == kCVPixelFormatType_420YpCbCr8PlanarFullRange )
		{
			PlanarMetaStruct = &PlanarMeta_8_8_8;
			PlanarMetaSize = sizeof(PlanarMeta_8_8_8);
		}
		else if ( PixelFormatType == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange || PixelFormatType == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange )
		{
			PlanarMetaStruct = &PlanarMeta_8_88;
			PlanarMetaSize = sizeof(PlanarMeta_8_88);
		}

		
		auto Result = CVPixelBufferCreateWithPlanarBytes( PixelBufferAllocator, Widths[0], Heights[0], PixelFormatType,
														 PlanarMetaStruct, PlanarMetaSize,	//use these if contigiuous	//	gr: MUST use on ios
														 Planes.GetSize(),
														 Datas, Widths, Heights,
														 RowSizes,
														 Callback, CallbackReference,
														 PixelBufferAttributes,
														 &PixelBuffer );
		Avf::IsOkay( Result, "CVPixelBufferCreateWithPlanarBytes");
	}
	else
	{
		auto& PixelsArray = Image.GetPixelsArray();
		auto* Pixels = const_cast<uint8*>( PixelsArray.GetArray() );
		auto BytesPerRow = Image.GetMeta().GetRowDataSize();
		
		//	just using pixels directly
		auto Result = CVPixelBufferCreateWithBytes( PixelBufferAllocator, Image.GetWidth(), Image.GetHeight(), PixelFormatType, Pixels, BytesPerRow, PixelReleaseCallback, ReleaseContext, PixelBufferAttributes, &PixelBuffer );
		if ( Result != noErr )
		{
			auto PixelFormatString = GetPixelFormatString(PixelFormatType);
			std::stringstream Error;
			Error << "CVPixelBufferCreateWithBytes(" << Image.GetWidth() << "x" << Image.GetHeight() << " pixelformat=" << PixelFormatString << " ImageFormat=" << Image.GetFormat();
			Avf::IsOkay( Result, Error.str() );
		}
		Avf::IsOkay( Result, "CVPixelBufferCreateWithBytes");
	}
	
	return PixelBuffer;
}

CFPtr<CMFormatDescriptionRef> Avf::GetFormatDescriptionH264(const ArrayBridge<uint8_t>& Sps,const ArrayBridge<uint8_t>& Pps,H264::NaluPrefix::Type NaluPrefixType)
{
	CFAllocatorRef Allocator = nil;
	
	//	need to strip nalu prefix from these
	auto SpsPrefixLength = H264::GetNaluLength(Sps);
	auto PpsPrefixLength = H264::GetNaluLength(Pps);
	
	BufferArray<const uint8_t*,2> Params;
	BufferArray<size_t,2> ParamSizes;
	Params.PushBack( Sps.GetArray()+SpsPrefixLength );
	ParamSizes.PushBack( Sps.GetDataSize()-SpsPrefixLength );
	Params.PushBack( Pps.GetArray()+PpsPrefixLength );
	ParamSizes.PushBack( Pps.GetDataSize()-PpsPrefixLength );
	
	//	ios doesnt support annexb, so we will have to convert inputs
	//	lets use 32 bit nalu size prefix
	if ( NaluPrefixType == H264::NaluPrefix::AnnexB )
	NaluPrefixType = H264::NaluPrefix::ThirtyTwo;
	auto NaluLength = static_cast<int>(NaluPrefixType);
	
	CFPtr<CMFormatDescriptionRef> FormatDesc;
	//	-12712 http://stackoverflow.com/questions/25078364/cmvideoformatdescriptioncreatefromh264parametersets-issues
	auto Result = CMVideoFormatDescriptionCreateFromH264ParameterSets( Allocator, Params.GetSize(), Params.GetArray(), ParamSizes.GetArray(), NaluLength, &FormatDesc.mObject );
	Avf::IsOkay( Result, "CMVideoFormatDescriptionCreateFromH264ParameterSets" );
	
	return FormatDesc;
}
