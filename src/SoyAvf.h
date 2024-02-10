#pragma once

#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include "SoyMedia.h"
#include "SoyH264.h"
#include "SoyHevc.hpp"
#include "Array.hpp"
#include "SoyPixels.h"
#include "SoyFourcc.h"

class SoyPixelsImpl;

//	rather than ifdef out functions, fake forward declarations
#if !defined(__OBJC__)
class NSString;
class NSNumber;
class id;
#endif



namespace Hevc
{
	class Headers_t
	{
	public:
		std::span<uint8_t>	mVps;
		std::span<uint8_t>	mSps;
		std::span<uint8_t>	mPps;
		std::span<uint8_t>	mPrefixSei;
		std::span<uint8_t>	mSuffixSei;
		//	32 (video parameter set),
		//	33 (sequence parameter set),
		//	34 (picture parameter set),
		//	39 (prefix SEI) and
		//	40 (suffix SEI). At least one of each parameter set must be provided.
		
		bool	IsComplete();
		void 	StripNaluPrefixes();
		void	StripEmulationPrevention();
		std::array<const uint8_t*,5>	GetArrays();
		std::array<size_t,5>			GetSizes();
	};
}


namespace Avf
{
	class TAsset;
	
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
	void							GetFormatDescriptionData(std::vector<uint8_t>& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex);
	CFPtr<CMFormatDescriptionRef>	GetFormatDescriptionH264(std::span<uint8_t> Sps,std::span<uint8_t> Pps,H264::NaluPrefix::Type NaluPrefixType,bool StripEmulationPrevention);
	CFPtr<CMFormatDescriptionRef>	GetFormatDescriptionHevc(Hevc::Headers_t Headers,H264::NaluPrefix::Type NaluPrefixType,bool StripEmulationPrevention);

	CMFormatDescriptionRef			GetFormatDescription(const TStreamMeta& Stream);
	H264::NaluPrefix::Type			GetFormatInputH264NaluPrefix(CMFormatDescriptionRef Format);
	H264::NaluPrefix::Type			GetFormatInputHevcNaluPrefix(CMFormatDescriptionRef Format);
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
	SoyPixelsFormat::Type			GetPixelFormat(const Soy::TFourcc& Fourcc);
	SoyPixelsFormat::Type			GetPixelFormat(OSType Format);
	SoyPixelsFormat::Type			GetPixelFormat(NSNumber* Format);
	OSType							GetPlatformPixelFormat(SoyPixelsFormat::Type Format);
	std::string						GetPixelFormatString(OSType Format);
	std::string						GetPixelFormatString(id Format);
	std::string						GetPixelFormatString(NSNumber* Format);
	OSType							GetPlatformPixelFormat(SoyPixelsFormat::Type Format);
	SoyPixelsFormat::Type			GetPixelFormat(OSType Format);
	SoyPixelsFormat::Type			GetPixelFormat(NSNumber* Format);
	SoyPixelsMeta					GetPixelMeta(CVPixelBufferRef PixelBuffer);

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
