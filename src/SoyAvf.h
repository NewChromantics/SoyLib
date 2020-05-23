#pragma once

#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include "SoyMedia.h"
#include "SoyH264.h"
#include "Array.hpp"
#include "SoyPixels.h"

class SoyPixelsImpl;

//	rather than ifdef out functions, fake forward declarations
#if !defined(__OBJC__)
class NSString;
class NSNumber;
class id;
#endif

namespace Avf
{
	class TAsset;
	
	SoyMediaFormat::Type			SoyMediaFormat_FromFourcc(uint32 Fourcc,size_t H264LengthSize);
	void							GetFileExtensions(ArrayBridge<std::string>&& Extensions);

	//	OSStatus == CVReturn
	void							IsOkay(OSStatus Error,const std::string& Context);
	void							IsOkay(OSStatus Error,const char* Context);
	std::string						GetCVReturnString(CVReturn Error);
	std::string						GetCodec(CMFormatDescriptionRef FormatDescription);
	std::string						GetExtensions(CMFormatDescriptionRef FormatDescription);
	

	std::shared_ptr<TMediaPacket>	GetH264Packet(CMSampleBufferRef SampleBuffer,size_t StreamIndex);
	std::shared_ptr<TMediaPacket>	GetFormatDescriptionPacket(CMSampleBufferRef SampleBuffer,size_t ParamIndex,SoyMediaFormat::Type Format,size_t StreamIndex);
	TStreamMeta						GetStreamMeta(CMFormatDescriptionRef FormatDesc);
	void							GetFormatDescriptionData(ArrayBridge<uint8>&& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex);
	CFPtr<CMFormatDescriptionRef>	GetFormatDescriptionH264(const ArrayBridge<uint8_t>& Sps,const ArrayBridge<uint8_t>& Pps,H264::NaluPrefix::Type NaluPrefixType);

	void							GetFormatDescriptionData(ArrayBridge<uint8>&& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex);
	CMFormatDescriptionRef			GetFormatDescription(const TStreamMeta& Stream);
	void							GetMediaType(CMMediaType& MediaType,FourCharCode& MediaCodec,SoyMediaFormat::Type Format);
	CFStringRef						GetProfile(H264Profile::Type Profile,Soy::TVersion Level);
	NSString* const					GetFormatType(SoyMediaFormat::Type Format);
	NSString* const					GetFileExtensionType(const std::string& Extension);
	bool							IsKeyframe(CMSampleBufferRef SampleBuffer,bool DefaultValue);
	bool							IsFormatCompressed(SoyMediaFormat::Type Format);
	CVPixelBufferRef				PixelsToPixelBuffer(const SoyPixelsImpl& Pixels);

	//	OSStatus == CVReturn
	std::string						GetString(OSStatus Status);
	CFStringRef						GetProfile(H264Profile::Type Profile);
	std::string						GetString(OSStatus Status);
	SoyPixelsFormat::Type			GetPixelFormat(OSType Format);
	SoyPixelsFormat::Type			GetPixelFormat(NSNumber* Format);
	OSType							GetPlatformPixelFormat(SoyPixelsFormat::Type Format);
	std::string						GetPixelFormatString(OSType Format);
	std::string						GetPixelFormatString(id Format);
	std::string						GetPixelFormatString(NSNumber* Format);
	OSType							GetPlatformPixelFormat(SoyPixelsFormat::Type Format);
	SoyPixelsFormat::Type			GetPixelFormat(OSType Format);
	SoyPixelsFormat::Type			GetPixelFormat(NSNumber* Format);

	vec2x<uint32_t>					GetSize(CVImageBufferRef Image);
}



/*
#if defined(__OBJC__)
class Avf::TAsset
{
public:
	TAsset(const std::string& Filename);
	
	void				LoadTracks();	//	blocking & throwing load of desired track[s]
	
	std::shared_ptr<Platform::TMediaFormat>		GetStreamFormat(size_t StreamIndex)	{	return mStreamFormats[StreamIndex];	}
	
public:
	Array<TStreamMeta>	mStreams;
	ObjcPtr<AVAsset>	mAsset;
	std::map<size_t,std::shared_ptr<Platform::TMediaFormat>>	mStreamFormats;
};
#endif
*/

#if defined(__OBJC__)
std::ostream& operator<<(std::ostream& out,const AVAssetExportSessionStatus& in);
#endif
