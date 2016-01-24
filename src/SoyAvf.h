#pragma once


#include <SoyMedia.h>
#include <SoyH264.h>

namespace Avf
{
	class TAsset;
	SoyMediaFormat::Type			SoyMediaFormat_FromFourcc(uint32 Fourcc,int H264LengthSize);
	SoyPixelsFormat::Type			SoyPixelFormat_FromFourcc(uint32 Fourcc);

#if defined(__OBJC__)
	std::shared_ptr<TMediaPacket>	GetH264Packet(CMSampleBufferRef SampleBuffer,size_t StreamIndex);
	std::shared_ptr<TMediaPacket>	GetFormatDescriptionPacket(CMSampleBufferRef SampleBuffer,size_t ParamIndex,SoyMediaFormat::Type Format,size_t StreamIndex);
	void							GetFormatDescriptionData(ArrayBridge<uint8>&& Data,CMFormatDescriptionRef FormatDesc,size_t ParamIndex,SoyMediaFormat::Type Format);
	TStreamMeta						GetStreamMeta(CMFormatDescriptionRef FormatDesc);
	CMFormatDescriptionRef			GetFormatDescription(const TStreamMeta& Stream);
	void							GetMediaType(CMMediaType& MediaType,FourCharCode& MediaCodec,SoyMediaFormat::Type Format);
	CFStringRef						GetProfile(H264Profile::Type Profile,Soy::TVersion Level);
	NSString* const					GetFormatType(SoyMediaFormat::Type Format);
	NSString* const					GetFileExtensionType(const std::string& Extension);
	NSURL*							GetUrl(const std::string& Filename);
	bool							IsKeyframe(CMSampleBufferRef SampleBuffer,bool DefaultValue);
	bool							IsFormatCompressed(SoyMediaFormat::Type Format);
	CVPixelBufferRef				PixelsToPixelBuffer(const SoyPixelsImpl& Pixels);

	//	OSStatus == CVReturn
	bool							IsOkay(OSStatus Error,const std::string& Context,bool Throw=true);
	std::string						GetString(OSStatus Status);
	CFStringRef						GetProfile(H264Profile::Type Profile);
#endif
}


#if defined(__OBJC__)
class Platform::TMediaFormat
{
public:
	TMediaFormat(CMFormatDescriptionRef Desc) :
	mDesc	( Desc )
	{
		CFRetain( mDesc );
	}
	~TMediaFormat()
	{
		CFRelease( mDesc );
	}
	
	
	CMFormatDescriptionRef	mDesc;
};
#endif



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

