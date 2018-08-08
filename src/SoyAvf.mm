#include "SoyAvf.h"
#include <SoyH264.h>
#include <SoyFileSystem.h>

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



std::ostream& operator<<(std::ostream& out,const AVAssetExportSessionStatus& in)
{
	switch ( in )
	{
		case AVAssetExportSessionStatusUnknown:		out << "AVAssetExportSessionStatusUnknown";	 return out;
		case AVAssetExportSessionStatusWaiting:		out << "AVAssetExportSessionStatusWaiting";	 return out;
		case AVAssetExportSessionStatusExporting:	out << "AVAssetExportSessionStatusExporting";	 return out;
		case AVAssetExportSessionStatusCompleted:	out << "AVAssetExportSessionStatusCompleted";	 return out;
		case AVAssetExportSessionStatusFailed:		out << "AVAssetExportSessionStatusFailed";	 return out;
		case AVAssetExportSessionStatusCancelled:	out << "AVAssetExportSessionStatusCancelled";	 return out;
		default:
		{
			out << "AVAssetExportSessionStatus<" << static_cast<int>( in ) << ">";
			return out;
		}
	}
	
}


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

	
	GetFormatDescriptionData( GetArrayBridge( Data ), Desc, ParamIndex, Format );
	return pPacket;
}


void Avf::GetFormatDescriptionData(ArrayBridge<uint8>&& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex,SoyMediaFormat::Type Format)
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
	
	Soy::Assert( ParamIndex < ParamCount, "SPS missing");
	
	const uint8_t* ParamsData = nullptr;;
	size_t ParamsSize = 0;
	Result = CMVideoFormatDescriptionGetH264ParameterSetAtIndex( FormatDesc, ParamIndex, &ParamsData, &ParamsSize, nullptr, nullptr);
	
	Avf::IsOkay( Result, "Failed to get H264 param X" );
	
	Data.PushBackArray( GetRemoteArray( ParamsData, ParamsSize ) );
}


std::shared_ptr<TMediaPacket> Avf::GetH264Packet(CMSampleBufferRef SampleBuffer,size_t StreamIndex)
{
	auto Desc = CMSampleBufferGetFormatDescription( SampleBuffer );
	
	//	need length-byte-size to get proper h264 format
	int nal_size_field_bytes = 0;
	auto Result = CMVideoFormatDescriptionGetH264ParameterSetAtIndex( Desc, 0, nullptr, nullptr, nullptr, &nal_size_field_bytes );
	Avf::IsOkay( Result, "Get H264 param NAL size");
	if ( nal_size_field_bytes < 0 )
		nal_size_field_bytes = 0;
	
	//	extract SPS/PPS packet
	auto Fourcc = CFSwapInt32HostToBig( CMFormatDescriptionGetMediaSubType(Desc) );
	auto Codec = SoyMediaFormat::FromFourcc( Fourcc, nal_size_field_bytes );
	std::shared_ptr<TMediaPacket> pPacket( new TMediaPacket() );
	auto& Packet = *pPacket;
	Packet.mMeta = GetStreamMeta( Desc );
	Packet.mMeta.mCodec = Codec;
	Packet.mMeta.mStreamIndex = StreamIndex;
	Packet.mFormat.reset( new Platform::TMediaFormat( Desc ) );
	
	CMTime PresentationTimestamp = CMSampleBufferGetPresentationTimeStamp(SampleBuffer);
	CMTime DecodeTimestamp = CMSampleBufferGetDecodeTimeStamp(SampleBuffer);
	CMTime SampleDuration = CMSampleBufferGetDuration(SampleBuffer);
	Packet.mTimecode = Soy::Platform::GetTime(PresentationTimestamp);
	Packet.mDecodeTimecode = Soy::Platform::GetTime(DecodeTimestamp);
	Packet.mDuration = Soy::Platform::GetTime(SampleDuration);
	//GetPixelBufferManager().mOnFrameFound.OnTriggered( Timestamp );
	
	
	//	get bytes, either blocks of data or a CVImageBuffer
	{
		CMBlockBufferRef BlockBuffer = CMSampleBufferGetDataBuffer( SampleBuffer );
		//CVImageBufferRef ImageBuffer = CMSampleBufferGetImageBuffer( SampleBuffer );
		
		if ( BlockBuffer )
		{
			//	read bytes from block
			auto DataSize = CMBlockBufferGetDataLength( BlockBuffer );
			Packet.mData.SetSize( DataSize );
			auto Result = CMBlockBufferCopyDataBytes( BlockBuffer, 0, Packet.mData.GetDataSize(), Packet.mData.GetArray() );
			Avf::IsOkay( Result, "CMBlockBufferCopyDataBytes" );
		}
		else
		{
			throw Soy::AssertException("Expecting block buffer");
		}
		
		//if ( BlockBuffer )
		//	CFRelease( BlockBuffer );
		
		//if ( ImageBuffer )
		//	CFRelease( ImageBuffer );
	}
	
	//	verify/convert h264 AVCC to ES/annexb
	H264::ConvertToFormat( Packet.mMeta.mCodec, SoyMediaFormat::H264_ES, GetArrayBridge(Packet.mData) );
	
	return pPacket;
}




//	lots of errors in macerrors.h with no string conversion :/
#define TESTENUMERROR(e,Enum)	if ( (e) == (Enum) )	return #Enum ;


//	http://stackoverflow.com/questions/2196869/how-do-you-convert-an-iphone-osstatus-code-to-something-useful
std::string Avf::GetString(OSStatus Status)
{
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
	
	//	as integer..
	std::stringstream Error;
	Error << "OSStatus = " << static_cast<sint32>(Status);
	return Error.str();
	
	//	could be fourcc?
	return Soy::FourCCToString( CFSwapInt32HostToBig(Status) );
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

bool Avf::IsOkay(OSStatus Error,const std::string& Context,bool Throw)
{
	//	kCVReturnSuccess
	if ( Error == noErr )
		return true;
	
	std::stringstream ErrorString;
	ErrorString << "OSStatus/CVReturn error in " << Context << ": " << GetString(Error);
	
	if ( Throw )
	{
		throw Soy::AssertException( ErrorString.str() );
	}
	else
	{
		std::Debug << ErrorString.str() << std::endl;
	}
	return false;
}



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


CMFormatDescriptionRef Avf::GetFormatDescription(const TStreamMeta& Stream)
{
	CFAllocatorRef Allocator = nil;
	CMFormatDescriptionRef FormatDesc = nullptr;
	
	
	if ( SoyMediaFormat::IsH264( Stream.mCodec ) )
	{
		auto& Sps = Stream.mSps;
		auto& Pps = Stream.mPps;
		Soy::Assert( !Sps.IsEmpty(), "H264 encoder requires SPS beforehand" );
		Soy::Assert( !Pps.IsEmpty(), "H264 encoder requires PPS beforehand" );
		
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
		/*
		 //	extensions to dictionary
		 if ( !Stream.mExtensions.IsEmpty() )
		 {
			auto Data = ArrayToNSData( GetArrayBridge(Stream.mExtensions) );
			NSError* Error = nullptr;
			NSPropertyListReadOptions Options;
			NSDictionary* ExtensionsDict = [NSPropertyListSerialization propertyListWithData:Data options:Options format:nullptr error:&Error];
		 
			Extensions = (__bridge CFDictionaryRef)ExtensionsDict;
		 }
		 */
		auto Result = CMFormatDescriptionCreate( Allocator, MediaType, MediaCodec, Extensions, &FormatDesc );
		Avf::IsOkay( Result, "CMFormatDescriptionCreate" );
	}
	
	
	//	setup some final bits
/*
	//	gr: these are missing
FormatName='avc1';
SpatialQuality=0;
Version=0;
RevisionLevel=0;
TemporalQuality=0;
Depth=24;
VerbatimISOSampleEntry=<00000098 61766331 00000000 00000001 00000000 00000000 00000000 00000000 08000800 00480000 00480000 00000000 00010000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000018 ffff0000 00426176 6343014d 4032ffe1 002b674d 40329652 00400080 dff80008 000a8400 00030004 00000300 f3920000 98960001 6e36fc63 83b42c5a 24010004 68eb7352>;

	
CVImageBufferChromaLocationBottomField=Left;
CVFieldCount=1;
SampleDescriptionExtensionAtoms={\n    avcC = <014d4032 ffe1002b 674d4032 96520040 0080dff8 0008000a 84000003 00040000 0300f392 00009896 00016e36 fc6383b4 2c5a2401 000468eb 7352>;\n};
CVPixelAspectRatio={\n    HorizontalSpacing = 1;\n    VerticalSpacing = 1;\n};
CVImageBufferChromaLocationTopField=Left;
FullRangeVideo=0;

CVImageBufferChromaLocationBottomField=Left;
CVFieldCount=1;
SampleDescriptionExtensionAtoms={\n    avcC = <014d4032 ffe1002b 674d4032 96520040 0080dff8 0008000a 84000003 00040000 0300f392 00009896 00016e36 fc6383b4 2c5a2401 000468eb 7352>;\n};
CVPixelAspectRatio={\n    HorizontalSpacing = 1;\n    VerticalSpacing = 1;\n};
CVImageBufferChromaLocationTopField=Left;
FullRangeVideo=0;
*/
	
	return FormatDesc;
}


SoyMediaFormat::Type Avf::SoyMediaFormat_FromFourcc(uint32 Fourcc,size_t H264LengthSize)
{
	auto AvfPixelFormat = Avf::GetPixelFormat( Fourcc );
	if ( AvfPixelFormat != SoyPixelsFormat::Invalid )
		return SoyMediaFormat::FromPixelFormat( AvfPixelFormat );
	
	//	double check for swapped endianness
	AvfPixelFormat = Avf::GetPixelFormat( Soy::SwapEndian(Fourcc) );
	if ( AvfPixelFormat != SoyPixelsFormat::Invalid )
	{
		std::Debug << "Warning, detected avf pixel format with reversed fourcc " << Soy::FourCCToString( Soy::SwapEndian(Fourcc) ) << std::endl;
		return SoyMediaFormat::FromPixelFormat( AvfPixelFormat );
	}
	
	return SoyMediaFormat::FromFourcc( Fourcc, H264LengthSize );
}

//	gr: speed this up! (or reduce usage) all the obj-c calls are expensive.
TStreamMeta Avf::GetStreamMeta(CMFormatDescriptionRef FormatDesc)
{
	TStreamMeta Meta;
	auto Fourcc = CMFormatDescriptionGetMediaSubType(FormatDesc);

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
	
	Meta.mCodec = Avf::SoyMediaFormat_FromFourcc( Fourcc, H264LengthSize );
	
	if ( SoyMediaFormat::IsH264( Meta.mCodec ) )
	{
		Avf::GetFormatDescriptionData( GetArrayBridge(Meta.mSps), FormatDesc, 0, SoyMediaFormat::H264_SPS_ES );
		Avf::GetFormatDescriptionData( GetArrayBridge(Meta.mPps), FormatDesc, 1, SoyMediaFormat::H264_PPS_ES );
	}
	
	Boolean usePixelAspectRatio = false;
	Boolean useCleanAperture = false;
	auto Dim = CMVideoFormatDescriptionGetPresentationDimensions( FormatDesc, usePixelAspectRatio, useCleanAperture );
	Meta.mPixelMeta.DumbSetWidth( Dim.width );
	Meta.mPixelMeta.DumbSetHeight( Dim.height );
	Meta.mPixelMeta.DumbSetFormat( SoyMediaFormat::GetPixelFormat( Meta.mCodec ) );
	
	//std::stringstream Debug;
	//Debug << "Extensions=" << Soy::Platform::GetExtensions( FormatDesc ) << "; ";
	
	/*
	 //	serialise extensions
	 {
		auto Extensions = (NSDictionary*)CMFormatDescriptionGetExtensions( FormatDesc );
		NSError *error = nullptr;
		NSPropertyListFormat Format = NSPropertyListBinaryFormat_v1_0;
		auto Data = [NSPropertyListSerialization dataWithPropertyList:Extensions format:Format options:0 error:&error];
	 
		NSDataToArray( Data, GetArrayBridge(Meta.mExtensions) );
	 }
		*/
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
			*/
			std::Debug << "Warning: generated meta from description, but not equal when converted back again. " << Meta << std::endl;
		}
	}
	
	
	return Meta;
}

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
	Soy::Assert( Asset, "Asset expected" );
	
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

NSURL* Avf::GetUrl(const std::string& Filename)
{
	return Platform::GetUrl( Filename );
}




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

void PixelReleaseCallback(void *releaseRefCon, const void *baseAddress)
{
	std::Debug << __func__ << std::endl;

	//	this page says we need to release
	//	http://codefromabove.com/2015/01/av-foundation-saving-a-sequence-of-raw-rgb-frames-to-a-movie/
	CFDataRef bufferData = (CFDataRef)releaseRefCon;
	CFRelease(bufferData);
}


CVPixelBufferRef Avf::PixelsToPixelBuffer(const SoyPixelsImpl& Image)
{
	CFAllocatorRef PixelBufferAllocator = nullptr;
	OSType PixelFormatType = GetPlatformPixelFormat( Image.GetFormat() );
	auto& PixelsArray = Image.GetPixelsArray();
	auto* Pixels = const_cast<uint8*>( PixelsArray.GetArray() );
	auto BytesPerRow = Image.GetMeta().GetRowDataSize();
	void* ReleaseContext = nullptr;
	CFDictionaryRef PixelBufferAttributes = nullptr;

#if defined(TARGET_OSX)
	//	gr: hack, cannot create RGBA pixel buffer on OSX. do a last-min conversion here, but ideally it's done beforehand
	//		REALLY ideally we can go from texture to CVPixelBuffer
	if ( Image.GetFormat() == SoyPixelsFormat::RGBA && PixelFormatType == kCVPixelFormatType_32RGBA )
	{
		std::Debug << "CVPixelBufferCreateWithBytes will fail with RGBA, forcing BGRA" << std::endl;
		PixelFormatType = kCVPixelFormatType_32BGRA;
	}
#endif
	
	CVPixelBufferRef PixelBuffer = nullptr;
	auto Result = CVPixelBufferCreateWithBytes( PixelBufferAllocator, Image.GetWidth(), Image.GetHeight(), PixelFormatType, Pixels, BytesPerRow, PixelReleaseCallback, ReleaseContext, PixelBufferAttributes, &PixelBuffer );
	
	std::stringstream Error;
	Error << "CVPixelBufferCreateWithBytes " << Image.GetMeta() << "(" << Soy::FourCCToString(PixelFormatType) << ")";
	Avf::IsOkay( Result, Error.str() );

	return PixelBuffer;
}




//	todo: abstract MediaFoundation templated version of this

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
		Soy::Assert( IsValid(), "Expected valid enum - or invalid enum is bad" );
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
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24RGB,	SoyPixelsFormat::RGB ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_24BGR,	SoyPixelsFormat::BGR ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32BGRA,	SoyPixelsFormat::BGRA ),
	
	//	gr: PopFace creating a pixel buffer from a unity "argb" texture, failed as RGBA is unsupported...
	//	gr: ARGB is accepted, but channels are wrong
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32RGBA,	SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_32ARGB,	SoyPixelsFormat::ARGB ),


	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarFullRange,	SoyPixelsFormat::Yuv_8_88_Full ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,	SoyPixelsFormat::Yuv_8_88_Ntsc ),

	/* gr: don't currently support these until we have pixel shaders for it
	//	gr: are these the same luma range?
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8,	SoyPixelsFormat::Yuv_844_Full ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8FullRange,	SoyPixelsFormat::Yuv_844_Full ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8_yuvs,	SoyPixelsFormat::Yuv_844_Ntsc ),
*/
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8FullRange,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr8_yuvs,	SoyPixelsFormat::Invalid ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8Planar,	SoyPixelsFormat::YYuv_8888_Ntsc ),
	CV_VIDEO_TYPE_META( kCVPixelFormatType_420YpCbCr8PlanarFullRange,	SoyPixelsFormat::YYuv_8888_Full ),
	
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
	
	
	CV_VIDEO_TYPE_META( kCVPixelFormatType_422YpCbCr_4A_8BiPlanar,	SoyPixelsFormat::Invalid ),
};



std::string Avf::GetPixelFormatString(OSType Format)
{
	auto Table = GetRemoteArray( Cv_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
	{
		std::stringstream Output;
		Output << "Unknown format " << Soy::FourCCToString( Format );
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
		std::Debug << "Unknown Avf CV pixel format " << Format << std::endl;
		return SoyPixelsFormat::Invalid;
	}
	return Meta->mSoyFormat;
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
