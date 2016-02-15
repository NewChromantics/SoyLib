#include "SoyMedia.h"
#include "SortArray.h"


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
	{ SoyMediaFormat::LumaFull,			"LumaFull" },
	{ SoyMediaFormat::LumaVideo,		"LumaVideo" },
	{ SoyMediaFormat::Yuv_8_88_Full,	"Yuv_8_88_Full" },
	{ SoyMediaFormat::Yuv_8_88_Video,	"Yuv_8_88_Video" },
	{ SoyMediaFormat::Yuv_8_8_8_Full,	"Yuv_8_8_8_Full" },
	{ SoyMediaFormat::Yuv_8_8_8_Video,	"Yuv_8_8_8_Video" },
	{ SoyMediaFormat::Yuv_844_Full,		"Yuv_844_Full" },
	{ SoyMediaFormat::ChromaUV_8_8,		"ChromaUV_8_8" },
	{ SoyMediaFormat::ChromaUV_88,		"ChromaUV_88" },
	{ SoyMediaFormat::ChromaUV_44,		"ChromaUV_44" },
	{ SoyMediaFormat::Palettised_8_8,	"Palettised_8_8" },
};



std::ostream& operator<<(std::ostream& out,const TStreamMeta& in)
{
	out << "mCodec=" << in.mCodec << ", ";
	out << "Mime=" << in.GetMime() << ", ";
	out << "mDescription=" << in.mDescription << ", ";
	out << "mCompressed=" << in.mCompressed << ", ";
	out << "mFramesPerSecond=" << in.mFramesPerSecond << ", ";
	out << "mDuration=" << in.mDuration << ", ";
	out << "mEncodingBitRate=" << in.mEncodingBitRate << ", ";
	out << "mStreamIndex=" << in.mStreamIndex << ", ";
	out << "mMediaTypeIndex=" << in.mMediaTypeIndex << ", ";
	out << "mPixelMeta=" << in.mPixelMeta << ", ";
	out << "mInterlaced=" << in.mInterlaced << ", ";
	out << "mVideoClockWiseRotationDegrees=" << in.mVideoClockWiseRotationDegrees << ", ";
	out << "m3DVideo=" << in.m3DVideo << ", ";
	//out << "mYuvMatrix=" << in.mYuvMatrix << ", ";
	out << "mDrmProtected=" << in.mDrmProtected << ", ";
	out << "mMaxKeyframeSpacing=" << in.mMaxKeyframeSpacing << ", ";
	out << "mAverageBitRate=" << in.mAverageBitRate << ", ";
	out << "mChannelCount=" << in.mChannelCount << ", ";
	
	return out;
}


std::ostream& operator<<(std::ostream& out,const TMediaPacket& in)
{
	out << "MediaPacket: ";
	out << " Codec=" << in.mMeta.mCodec;
	out << " mTimecode=" << in.mTimecode;
	out << " mIsKeyFrame=" << in.mIsKeyFrame;
	out << " mEncrypted=" << in.mEncrypted;
	out << " Size=" << in.mData.GetDataSize();
	return out;
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
			
		case SoyMediaFormat::Text:		return "text/plain";
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

void TStreamMeta::SetPixelMeta(const SoyPixelsMeta& Meta)
{
	mPixelMeta = Meta;
	
	//	auto set codec if not set
	if ( mCodec == SoyMediaFormat::Invalid )
		mCodec = SoyMediaFormat::FromPixelFormat( Meta.GetFormat() );
}



TMediaDecoder::TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TPixelBufferManager> OutputBuffer) :
	SoyWorkerThread	( ThreadName, SoyWorkerWaitMode::Wake ),
	mInput			( InputBuffer ),
	mPixelOutput	( OutputBuffer )
{
	Soy::Assert( mInput!=nullptr, "Expected input");
	Soy::Assert( mPixelOutput!=nullptr, "Expected output target");
	//	wake when new packets arrive
	mOnNewPacketListener = WakeOnEvent( mInput->mOnNewPacket );
}


TMediaDecoder::TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TAudioBufferManager> OutputBuffer) :
	SoyWorkerThread	( ThreadName, SoyWorkerWaitMode::Wake ),
	mInput			( InputBuffer ),
	mAudioOutput	( OutputBuffer )
{
	Soy::Assert( mInput!=nullptr, "Expected input");
	Soy::Assert( mAudioOutput!=nullptr, "Expected output target");
	//	wake when new packets arrive
	mOnNewPacketListener = WakeOnEvent( mInput->mOnNewPacket );
}


TMediaDecoder::TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TTextBufferManager> OutputBuffer) :
	SoyWorkerThread	( ThreadName, SoyWorkerWaitMode::Wake ),
	mInput			( InputBuffer ),
	mTextOutput		( OutputBuffer )
{
	Soy::Assert( mInput!=nullptr, "Expected input");
	Soy::Assert( mTextOutput!=nullptr, "Expected output target");
	//	wake when new packets arrive
	auto OnNewPacket = [this](std::shared_ptr<TMediaPacket>& Packet)
	{
		this->Wake();
	};
	mOnNewPacketListener = mInput->mOnNewPacket.AddListener(OnNewPacket);
}

TMediaDecoder::~TMediaDecoder()
{
	if ( mInput )
	{
		mInput->mOnNewPacket.RemoveListener( mOnNewPacketListener );
	}
	WaitToFinish();
}

bool TMediaDecoder::CanSleep()
{
	//	don't sleep when there's work to do!
	if ( mInput )
	{
		if ( mInput->HasPackets() )
			return false;
	}
	
	return true;
}

bool TMediaDecoder::Iteration()
{
	bool FatalCleared = false;
	
	try
	{
		Soy::Assert( mInput!=nullptr, "Input missing");
		
		//	pop next packet
		auto Packet = mInput->PopPacket();
		if ( Packet )
		{
			//std::Debug << "Encoder Processing packet " << Packet->mTimecode << "(pts) " << Packet->mDecodeTimecode << "(dts)" << std::endl;
			if ( !ProcessPacket( Packet ) )
			{
				std::Debug << "Returned decoder input packet (" << *Packet << ") back to buffer" << std::endl;
				//	gr: only do this for important frames?
				mInput->UnPopPacket( Packet );
			}
		}
		Soy::StringStreamClear(mFatalError);
	}
	catch (std::exception& e)
	{
		//	gr: unpop packet?
		
		std::Debug << "Error processing input packet " << e.what() << std::endl;
		if ( !FatalCleared )
			Soy::StringStreamClear(mFatalError);
		FatalCleared = true;
		mFatalError << "Error processing input packet " << e.what();
	}
	
	
	//	read outputs
	try
	{
		if ( mPixelOutput )
		{
			ProcessOutputPacket( *mPixelOutput );
		}
		if ( mAudioOutput )
		{
			ProcessOutputPacket( *mAudioOutput );
		}
		if ( mTextOutput )
		{
			ProcessOutputPacket( *mTextOutput );
		}
		Soy::StringStreamClear(mFatalError);
	}
	catch (std::exception& e)
	{
		std::Debug << "Error processing output packet " << e.what() << std::endl;
		if ( !FatalCleared )
			Soy::StringStreamClear(mFatalError);
		FatalCleared = true;
		mFatalError << "Error processing output packet " << e.what();
	}
	
	
	return true;
}

bool TMediaDecoder::ProcessPacket(std::shared_ptr<TMediaPacket>& Packet)
{
	return ProcessPacket( *Packet );
}

TPixelBufferManager& TMediaDecoder::GetPixelBufferManager()
{
	Soy::Assert( mPixelOutput != nullptr, "MediaEncoder missing pixel buffer" );
	return *mPixelOutput;
}


TAudioBufferManager& TMediaDecoder::GetAudioBufferManager()
{
	Soy::Assert( mAudioOutput != nullptr, "MediaEncoder missing audio buffer" );
	return *mAudioOutput;
}

void TMediaDecoder::OnDecodeFrameSubmitted(const SoyTime& Time)
{
	if ( mPixelOutput )
	{
		mPixelOutput->mOnFrameDecodeSubmission.OnTriggered( Time );
	}

	if ( mAudioOutput )
	{
		mAudioOutput->mOnFrameDecodeSubmission.OnTriggered( Time );
	}
}



TMediaPacketBuffer::~TMediaPacketBuffer()
{
	//	make sure everyone has finished accessing
	if ( !mPacketsLock.try_lock() )
	{
		std::Debug << "Something still accessing media packet buffer on destruction! clear up accessor first!" << std::endl;
	}
	else
	{
		mPacketsLock.unlock();
	}
	
	std::lock_guard<std::mutex> Lock( mPacketsLock );
	mPackets.Clear();
}

std::shared_ptr<TMediaPacket> TMediaPacketBuffer::PopPacket()
{
	if ( mPackets.IsEmpty() )
		return nullptr;
	
	std::lock_guard<std::mutex> Lock( mPacketsLock );
	//	todo: options here so we can get the next packet we need
	//		where we skip over all frames until the prev-to-Time keyframe
	if ( mPackets.IsEmpty() )
		return nullptr;
	
	//if ( mPackets[0]->mTimecode > Time )
	//	return nullptr;
	
	return mPackets.PopAt(0);
}


void TMediaPacketBuffer::UnPopPacket(std::shared_ptr<TMediaPacket> Packet)
{
	std::lock_guard<std::mutex> Lock( mPacketsLock );
	
	//	gr: no sorted insert, assuming this was the last packet popped (assuming we onlyhave one thing popping packets from this buffer)
	//		so go straight back to the start
	GetArrayBridge(mPackets).InsertAt( 0, Packet );
}

void TMediaPacketBuffer::PushPacket(std::shared_ptr<TMediaPacket> Packet,std::function<bool()> Block)
{
	Soy::Assert( Packet != nullptr, "Packet expected");
	
	//	gr: maybe needs to be atomic?
	while ( mPackets.GetSize() >= mMaxBufferSize )
	{
		if ( !Block() )
		{
			std::Debug << "MediaPacketBuffer buffer full " << mPackets.GetSize() << "/" << mMaxBufferSize << ", dropped packet: " << *Packet << std::endl;
			return;
		}
	}
	
	//	gr: fix incoming timestamps
	CorrectIncomingPacketTimecode( *Packet );

	{
		std::lock_guard<std::mutex> Lock( mPacketsLock );
		
		//	sort by decode order
		auto SortPackets = [](const std::shared_ptr<TMediaPacket>& a,const std::shared_ptr<TMediaPacket>& b)
		{
			auto Timea = a->mDecodeTimecode;
			auto Timeb = b->mDecodeTimecode;
				
			if ( Timea < Timeb )
				return -1;
			if ( Timea > Timeb )
				return 1;
				
			return 0;
		};

		//	it is going to be rare for DTS to be out of order, and on android(note4 especially) it really slows down up the GPU, so issue a warning
		if ( !mPackets.IsEmpty() )
		{
			auto TailTimecode = mPackets.GetBack()->mDecodeTimecode;
			if ( Packet->mDecodeTimecode < TailTimecode )
			{
				std::Debug << "Warning; adding DTS " << Packet->mDecodeTimecode << " out of order (tail=" << TailTimecode << ")" << std::endl;
			}
		}
		
		SortArrayLambda<std::shared_ptr<TMediaPacket>> SortedPackets( GetArrayBridge(mPackets), SortPackets );
		SortedPackets.Push( Packet );
		
		//std::Debug << "Pushed intput packet to buffer " << *Packet << std::endl;
	}
	mOnNewPacket.OnTriggered( Packet );
}

void TMediaPacketBuffer::CorrectIncomingPacketTimecode(TMediaPacket& Packet)
{
	//	apparently (not seen it yet) in some formats (eg. ts) some players (eg. vlc) can't cope if DTS is same as PTS
	static SoyTime DecodeToPresentationOffset( 1ull );
	static bool DebugCorrection = false;

	if ( !mLastPacketTimestamp.IsValid() )
		mLastPacketTimestamp = SoyTime( 1ull ) + DecodeToPresentationOffset;
	
	//	gr: if there is no decode timestamp, assume it needs to be in the order it comes in
	if ( !Packet.mDecodeTimecode.IsValid() )
	{
		Packet.mDecodeTimecode = mLastPacketTimestamp + mAutoTimestampDuration;
		if ( DebugCorrection )
			std::Debug << "Corrected incoming packet DECODE timecode; now " << Packet.mTimecode << "(PTS) and " << Packet.mDecodeTimecode << "(DTS)" << std::endl;
	}

	//	don't have any timecodes
	if ( !Packet.mTimecode.IsValid() )
	{
		Packet.mTimecode = Packet.mDecodeTimecode + DecodeToPresentationOffset;
		if ( DebugCorrection )
			std::Debug << "Corrected incoming packet PRESENTATION timecode; now " << Packet.mTimecode << "(PTS) and " << Packet.mDecodeTimecode << "(DTS)" << std::endl;
	}

	//	gr: store last DECODE timestamp so this basically forces packets to be ordered by decode timestamps
	mLastPacketTimestamp = Packet.mDecodeTimecode;
}



TMediaExtractor::TMediaExtractor(const TMediaExtractorParams& Params) :
	SoyWorkerThread		( Params.mThreadName, SoyWorkerWaitMode::Wake ),
	mExtractAheadMs		( Params.mReadAheadMs ),
	mOnPacketExtracted	( Params.mOnFrameExtracted )
{
}

TMediaExtractor::~TMediaExtractor()
{
	WaitToFinish();
}

std::shared_ptr<TMediaPacketBuffer> TMediaExtractor::AllocStreamBuffer(size_t StreamIndex)
{
	auto& Buffer = mStreamBuffers[StreamIndex];
	
	if ( !Buffer )
	{
		Buffer.reset( new TMediaPacketBuffer );
		
		auto OnPacketExtracted = [StreamIndex,this](std::shared_ptr<TMediaPacket>& Packet)
		{
			if ( !mOnPacketExtracted )
				return;
			
			mOnPacketExtracted( Packet->mTimecode, Packet->mMeta.mStreamIndex );
		};
		
		Buffer->mOnNewPacket.AddListener( OnPacketExtracted );
	}
	
	return Buffer;
}

std::shared_ptr<TMediaPacketBuffer> TMediaExtractor::GetStreamBuffer(size_t StreamIndex)
{
	if ( mStreamBuffers.find( StreamIndex ) == mStreamBuffers.end() )
		return nullptr;
	
	return mStreamBuffers[StreamIndex];
}

void TMediaExtractor::Seek(SoyTime Time)
{
	//	update the target seek time
	if ( Time >= mSeekTime )
	{
		std::stringstream Error;
		Error << "Can't currently handle seeking backwards " << Time << " < " << mSeekTime;
		Soy::Assert( Time >= mSeekTime, Error.str() );
	}
	
	//	update the target time and wake up thread in case we need to read frames
	mSeekTime = Time + mExtractAheadMs;
	Wake();
}


void TMediaExtractor::OnError(const std::string& Error)
{
	//	set fatal error, stop thread?
	std::Debug << "Media extractor OnError(" << Error << ")" << std::endl;
	mFatalError = Error;
}

void TMediaExtractor::OnClearError()
{
	mFatalError = std::string();
}

bool TMediaExtractor::Iteration()
{
	//	tell extractor to read more frames
	//	stall thread if buffer is full, drop frames etc
	try
	{
		auto Loop = [this]
		{
			return IsWorking();
		};
		ReadPacketsUntil( mSeekTime, Loop );
	}
	catch (std::exception& e)
	{
		OnError( e.what() );
	}
	
	return true;
}


void TMediaExtractor::ReadPacketsUntil(SoyTime Time,std::function<bool()> While)
{
	while ( While() )
	{
		try
		{
			auto NextPacket = ReadNextPacket();
			
			//	no packet, error? try again
			if ( !NextPacket )
				continue;
			
			//	if we successfully read a packet, clear the last error
			OnClearError();

			//	packet can have content AND EOF, or just EOF
			bool EndOfStream = NextPacket->mEof;
			
			//	can have a packet with no data (eg. eof) skip this
			if ( NextPacket->HasData() )
			{
				
				//	block thread unless it's stopped
				auto Block = [this,&NextPacket]()
				{
					//	gr: this is happening a LOT, probably because the extractor is very fast... maybe throttle the thread...
					if ( IsWorking() )
					{
						//std::Debug << "MediaExtractor blocking in push packet; " << *NextPacket << std::endl;
						std::this_thread::sleep_for( std::chrono::milliseconds(100) );
					}
					return IsWorking();
				};
				
				auto Buffer = GetStreamBuffer( NextPacket->mMeta.mStreamIndex );
				//	if the buffer doesn't exist, we drop the packet. todo; make it easy to skip in ReadNextPacket!
				if ( Buffer )
				{
					Buffer->PushPacket( NextPacket, Block );
				}
			}
			else if ( !EndOfStream )
			{
				std::Debug << "Warning, empty packet AND NOT eof; " << *NextPacket << std::endl;
			}

			//	todo proper handling stream EOF
			if ( EndOfStream )
			{
				//	slow down the now-idle-ish thread
				static int SleepEofMs = 400;
				std::this_thread::sleep_for( std::chrono::milliseconds(SleepEofMs) );
				return;
			}
			
			//	passed the time we were reading until, abort current loop
			if ( NextPacket->mTimecode >= Time )
				return;
		}
		catch ( std::exception& e)
		{
			std::stringstream Error;
			Error << "Error extracting next packet " << e.what();
			OnError( Error.str() );
			return;
		}
	}
}


TStreamMeta TMediaExtractor::GetStream(size_t Index)
{
	Array<TStreamMeta> Streams;
	GetStreams( GetArrayBridge(Streams) );
	auto TotalStreamCount = Streams.GetSize();
	
	if ( Index >= Streams.GetSize() )
	{
		std::stringstream Error;
		Error << "Stream " << Index << "/" << TotalStreamCount << " doesn't exist";
		throw Soy::AssertException( Error.str() );
	}
	
	return Streams[Index];
}


TStreamMeta TMediaExtractor::GetVideoStream(size_t Index)
{
	Array<TStreamMeta> Streams;
	GetStreams( GetArrayBridge(Streams) );
	auto TotalStreamCount = Streams.GetSize();
	
	//	remove non-video streams
	for ( auto i=size_cast<ssize_t>(Streams.GetSize())-1;	i>=0;	i-- )
	{
		if ( !SoyMediaFormat::IsVideo( Streams[i].mCodec ) )
			Streams.RemoveBlock( i, 1 );
	}
	
	if ( Index >= Streams.GetSize() )
	{
		std::stringstream Error;
		Error << "Video stream " << Index << "/" << TotalStreamCount << " doesn't exist";
		throw Soy::AssertException( Error.str() );
	}
	
	return Streams[Index];
}

bool TMediaExtractor::CanPushPacket(SoyTime Time,size_t StreamIndex,bool IsKeyframe)
{
	//	todo; pass a func from the decoder which accesses the buffers/current time
	return true;
}

void TMediaExtractor::OnStreamsChanged(const ArrayBridge<TStreamMeta>&& Streams)
{
	mOnStreamsChanged.OnTriggered( Streams );
}

void TMediaExtractor::OnStreamsChanged()
{
	Array<TStreamMeta> Streams;
	GetStreams( GetArrayBridge(Streams) );
	OnStreamsChanged( GetArrayBridge(Streams) );
}

//	gr: maybe we need to correct timecodes in the extractor, not the decoder, as
//	+a) we need to sync all streams really
//	+b) we calc duration below
//	+c) dictate decode order correction here
//	-a) decode timecodes may be special...
//	gr: this is AT LEAST needed for correct stats (evident when we have 1 frame movies...)
void TMediaExtractor::CorrectExtractedPacketTimecode(TMediaPacket& Packet)
{
	if ( Packet.mTimecode.mTime == 0 )
		Packet.mTimecode.mTime = 1;
}

void TMediaExtractor::OnPacketExtracted(SoyTime& Timecode,size_t StreamIndex)
{
	if ( mOnPacketExtracted )
		mOnPacketExtracted( Timecode, StreamIndex );
}





void TMediaEncoder::PushFrame(std::shared_ptr<TMediaPacket>& Packet,std::function<bool(void)> Block)
{
	//	gr: check/assign stream index here?
	Soy::Assert( mOutput!=nullptr, "TMediaEncoder::PushFrame output expected" );
	mOutput->PushPacket( Packet, Block );
}



TMediaMuxer::TMediaMuxer(std::shared_ptr<TStreamWriter> Output,std::shared_ptr<TMediaPacketBuffer>& Input,const std::string& ThreadName) :
	SoyWorkerThread	( ThreadName, SoyWorkerWaitMode::Wake ),
	mOutput			( Output ),
	mInput			( Input )
{
	//Soy::Assert( mOutput!=nullptr, "TMpeg2TsMuxer output missing");
	Soy::Assert( mInput!=nullptr, "TMpeg2TsMuxer input missing");
	mOnPacketListener = WakeOnEvent( mInput->mOnNewPacket );
	
	Start();
}

TMediaMuxer::~TMediaMuxer()
{
	WaitToFinish();
}



bool TMediaMuxer::CanSleep()
{
	if ( !mInput )
		return true;
	if ( !mInput->HasPackets() )
		return true;
	
	return false;
}


bool TMediaMuxer::Iteration()
{
	//	pop next packet
	if ( !mInput )
		return true;
	
	auto Packet = mInput->PopPacket();
	if ( !Packet )
		return true;
	
	try
	{
		if ( !IsStreamsReady( Packet ) )
			return true;

		ProcessPacket( Packet, *mOutput );
	}
	catch (std::exception& e)
	{
		std::Debug << __func__ << " error; " << e.what() << std::endl;
	}
	
	return true;
}

bool TMediaMuxer::IsStreamsReady(std::shared_ptr<TMediaPacket> Packet)
{
	//	if streams already setup, check this packet can be accepted
	if ( !mStreams.IsEmpty() )
	{
		bool StreamExists = false;
		for ( int i=0;	i<mStreams.GetSize();	i++ )
			StreamExists |= (mStreams[i].mStreamIndex == Packet->mMeta.mStreamIndex);
		
		if ( !StreamExists )
		{
			std::stringstream Error;
			Error << "Trying to add packet (" << *Packet << ") for missing stream " << Packet->mMeta.mStreamIndex << " but streams already set";
			throw Soy::AssertException( Error.str() );
		}
	}
	
	//	shall we setup the streams?
	if ( mStreams.IsEmpty() )
	{
		//	wait for a minimum number of packets for a stream before starting.
		//	assume that normally each stream will get at least 1 packet before we go onto the second[timecode]
		static int MinPacketsPerStream = 3;
		bool AllStreamsHaveMinPackets = (!mDefferedPackets.IsEmpty());
		std::map<size_t,int> PacketsPerStream;
		std::map<size_t,TStreamMeta> StreamMetas;
		
		for ( int i=0;	i<mDefferedPackets.GetSize();	i++ )
		{
			auto& DefferedPacket = *mDefferedPackets[i];
			PacketsPerStream[ DefferedPacket.mMeta.mStreamIndex ]++;
			StreamMetas[ DefferedPacket.mMeta.mStreamIndex ] = DefferedPacket.mMeta;
		}
		for ( auto& PacketsInStream : PacketsPerStream )
		{
			bool HasMin = (PacketsInStream.second >= MinPacketsPerStream);
			AllStreamsHaveMinPackets &= HasMin;
		}
		
		if ( !AllStreamsHaveMinPackets )
		{
			std::Debug << "Deffering creation of streams (x" << PacketsPerStream.size() << ") until all have at least " << MinPacketsPerStream << " packets" << std::endl;
			mDefferedPackets.PushBack( Packet );
			return false;
		}

		//	streams need setting up
		Array<TStreamMeta> Streams;
		for ( auto& StreamMeta : StreamMetas )
			Streams.PushBack( StreamMeta.second );
		SetStreams( GetArrayBridge(Streams) );
	}

	//	flush deffered packets
	while ( !mDefferedPackets.IsEmpty() )
	{
		auto DefferedPacket = mDefferedPackets.PopAt(0);
		ProcessPacket( DefferedPacket, *mOutput );
	}
	
	return true;
}


void TMediaMuxer::SetStreams(const ArrayBridge<TStreamMeta>&& Streams)
{
	Soy::Assert( mStreams.IsEmpty(), "Streams already configured");
	
	mStreams.Copy( Streams );
	SetupStreams( GetArrayBridge(mStreams) );
	
}



void TMediaBufferManager::CorrectDecodedFrameTimestamp(SoyTime& Timestamp)
{
	//	gr: assuming we should never get here with an invalid timestamp, we force the first timestamp to 1 instead of 0 (so it doesn't clash with "invalid")
	//	if we add something to handle when we get no timestamps from the decoder, maybe revise this a little (but maybe that should be handled by the decoder)
	if ( Timestamp.mTime == 0 )
	{
		std::Debug << "CorrectDecodedFrameTimestamp() got timestamp of 0, should now be corrected with CorrectExtractedTimecode" << std::endl;
		Timestamp.mTime = 1;
	}
	
	if ( !mFirstTimestamp.IsValid() )
	{
		//	disregard pre seek
		if ( mParams.mPreSeek > Timestamp )
		{
			std::Debug << "Pre-seek " << mParams.mPreSeek << " is more than first timestamp " << Timestamp << "... pre-seek didn't work? Ignoring for Internal timestamp reset" << std::endl;
			mFirstTimestamp = Timestamp;
		}
		else
		{
			SoyTime PreSeekTime = mParams.mPreSeek;
			mFirstTimestamp = SoyTime( Timestamp.GetTime() - PreSeekTime.GetTime() );
			std::Debug << "First timestamp after pre-seek correction (" << PreSeekTime << ") is " << mFirstTimestamp << std::endl;
		}
	}
	
	//	correct timestamp
	if ( mParams.mResetInternalTimestamp )
	{
		//	timestamp is out of order, need to re-adjust the base
		if ( Timestamp < mFirstTimestamp )
		{
			std::Debug << "error correcting timestamp " << Timestamp << " against first timestamp: " << mFirstTimestamp << ". resetting base timestamp" << std::endl;

			//	"now" should be last + 1 for correction, as we can't know the frame-step here(maybe we can get this from some stream meta)
			mFirstTimestamp = Timestamp;	//	this==0
			mAdjustmentTimestamp.mTime = mLastTimestamp.mTime+1;
		}

		Timestamp.mTime += mAdjustmentTimestamp.mTime;

		//	special case, if our frames start at 0(corrected to 1), mFirstTimestamp will be 1, which gives us 0
		//	do not output 0.
		if ( mFirstTimestamp.mTime > 1 )
			Timestamp.mTime -= mFirstTimestamp.mTime;
	}
	
	mLastTimestamp = Timestamp;
}


void TMediaBufferManager::SetPlayerTime(const SoyTime &Time)
{
	mPlayerTime = Time;
}


size_t TMediaBufferManager::GetMinBufferSize() const
{
	//	if we're at the end of the file we don't wait because there won't be any more
	if ( HasAllFrames() )
		return 0;
	
	//	gr: check in case == is bad too
	if ( mParams.mMaxBufferSize < mParams.mMinBufferSize )
	{
		std::Debug << "Warning pixel buffer MaxSize(" << mParams.mMaxBufferSize << ") < MinSize(" << mParams.mMinBufferSize << ")" << std::endl;
		return (mParams.mMaxBufferSize > 1) ? (mParams.mMaxBufferSize-1) : 0;
	}
	
	return mParams.mMinBufferSize;
}

void TMediaBufferManager::OnPushEof()
{
	mHasEof = true;
}


SoyTime TAudioBufferBlock::GetSampleTime(size_t SampleIndex) const
{
	//	frequency is samples per sec
	float SampleDuration = 1.f / (float)(mFrequency * mChannels);
	float SampleTime = SampleIndex * SampleDuration;
	auto SampleTimeMs = size_cast<uint64>( SampleTime * 1000.f );
	return mStartTime + SoyTime(SampleTimeMs);
}

ssize_t TAudioBufferBlock::GetTimeSampleIndex(SoyTime Time) const
{
	float SampleDuration = 1.f / (float)(mFrequency * mChannels);
	
	if ( Time == mStartTime )
		return 0;
	
	if ( Time > mStartTime )
	{
		SampleDuration *= 1000.f;
		auto AheadMs = Time.GetTime() - mStartTime.GetTime();
		auto SampleIndex = AheadMs * SampleDuration;
		return SampleIndex;
	}
	else
	{
		return -1;
	}
}


size_t TAudioBufferBlock::RemoveDataUntil(SoyTime Time)
{
	if ( mData.IsEmpty() )
		return 0;
	
	auto StartTime = GetSampleTime( 0 );
	
	if ( Time <= StartTime )
		return 0;
	
	size_t RemoveCount = GetTimeSampleIndex( Time );
	RemoveCount = std::min( RemoveCount, mData.GetSize() );

	mData.RemoveBlock( 0, RemoveCount );
	
	return RemoveCount;
}


void TAudioBufferManager::PushAudioBuffer(const TAudioBufferBlock& AudioData)
{
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		
		Soy::Assert( AudioData.mFrequency != 0, "Audio data should not have zero frequency");
		
		mBlocks.PushBack( AudioData );
	}
	mOnFramePushed.OnTriggered( AudioData.mStartTime );
}

void TAudioBufferManager::PopAudioBuffer(ArrayBridge<float>&& Data,size_t Channels,size_t SampleRate,SoyTime StartTime,SoyTime EndTime)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );

	//	gr: this code needs to re-sample
	if ( !mBlocks.IsEmpty() )
	{
		auto& Block0 = mBlocks[0];
		if ( Block0.mFrequency != SampleRate )
		{
			std::Debug << "Warning: data sample rate (" << Block0.mFrequency << ") doesn't match desired rate (" << SampleRate << ")" << std::endl;
		}
	}
	
	//	work out where audio starts and pop chunks out of each block
	int DataIndex = 0;
	
	while ( DataIndex < Data.GetSize() )
	{
		//	eat from head block until its empty
		if ( mBlocks.IsEmpty() )
			break;
		
		auto& Block = mBlocks[0];
		
		Block.RemoveDataUntil( StartTime );
		
		if ( Block.mData.IsEmpty() )
		{
			mBlocks.RemoveBlock( 0, 1 );
			continue;
		}
		Data[DataIndex] = Block.mData.PopAt(0);
		DataIndex++;
	}
	
	//	clear data we haven't written
	for ( int i=DataIndex;	i<Data.GetSize();	i++ )
	{
		Data[i] = 0.f;
	}
}

void TAudioBufferManager::PeekAudioBuffer(ArrayBridge<float>&& Data,size_t MaxSamples,SoyTime& SampleStart,SoyTime& SampleEnd)
{
	size_t BlockIndex = 0;
	while ( Data.GetSize() < MaxSamples )
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		if ( BlockIndex >= mBlocks.GetSize() )
			break;
		
		auto& Block = mBlocks[BlockIndex];
		
		//	copy some data out
		auto CopySize = std::min( Block.mData.GetSize(), MaxSamples - Data.GetSize() );

		//	if we get this... empty block? bail, don't get stuck
		if ( CopySize == 0 )
			break;

		{
			auto* Dst = Data.PushBlock( CopySize );
			auto* Src = Block.mData.GetArray();
			memcpy( Dst, Src, CopySize * Data.GetElementSize() );
		}
		
		//	update times of the data we've copied
		if ( !SampleStart.IsValid() )
			SampleStart = Block.GetSampleTime(0);
		SampleEnd = Block.GetSampleTime(CopySize-1);
		
		BlockIndex++;
	}
}


void TAudioBufferManager::ReleaseFrames()
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	mBlocks.Clear();
}



void TTextBufferManager::PushBuffer(std::shared_ptr<TMediaPacket> Buffer)
{
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		
		//	gr: fix this in data generation, not here!
		static bool CorrectPreviousDuration = true;
		static bool Debug_Correction = false;
		auto Prev = !mBlocks.IsEmpty() ? mBlocks.GetBack() : nullptr;
		if ( CorrectPreviousDuration && Prev )
		{
			if ( !Prev->mDuration.IsValid() )
			{
				Prev->mDuration = Buffer->mTimecode - Prev->mTimecode;
				if ( Debug_Correction )
					std::Debug << "Corrected prev text data duration from 0 to " << Prev->mDuration << std::endl;
			}
		}
		
		{
			std::stringstream SampleStream;
			Soy::ArrayToString( GetArrayBridge(Buffer->mData), SampleStream );
			auto Sample = SampleStream.str();
			
			static bool Debug_TextPush = false;
			if ( Debug_TextPush )
				std::Debug << "New text push; " << Buffer->mTimecode << "; " << Sample << std::endl;
		}
		
		mBlocks.PushBack( Buffer );
	}
	mOnFramePushed.OnTriggered( Buffer->mTimecode );
}

SoyTime TTextBufferManager::PopBuffer(std::stringstream& Output,SoyTime Time,bool SkipOldText)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	SoyTime OutputTime;
	
	while ( !mBlocks.IsEmpty() )
	{
		auto pBlock = mBlocks[0];
		if ( pBlock->mTimecode > Time )
			break;
		
		//	in range
		pBlock = mBlocks.PopAt(0);
		auto& Block = *pBlock;
		
		//	if old, skip
		if ( SkipOldText )
		{
			auto EndTime = Block.GetEndTime();
			if ( EndTime < Time )
				continue;
		}
		
		//	insert line breaks if we have previous entries
		if ( OutputTime.IsValid() )
			Output << '\n';
		
		Soy::ArrayToString( GetArrayBridge(Block.mData), Output );
		
		//	return the end-time
		OutputTime = Block.GetEndTime();
		Soy::Assert( OutputTime.IsValid(), "Expected output time to be valid");
	}
	
	return OutputTime;
}

void TTextBufferManager::ReleaseFrames()
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	mBlocks.Clear();
}



TMediaPassThroughDecoder::TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TPixelBufferManager> OutputBuffer) :
	TMediaDecoder	( ThreadName, InputBuffer, OutputBuffer )
{
	Start();
}

TMediaPassThroughDecoder::TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TAudioBufferManager> OutputBuffer) :
	TMediaDecoder	( ThreadName, InputBuffer, OutputBuffer )
{
	Start();
}

TMediaPassThroughDecoder::TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TTextBufferManager> OutputBuffer) :
	TMediaDecoder	( ThreadName, InputBuffer, OutputBuffer )
{
	Start();
}


bool TMediaPassThroughDecoder::HandlesCodec(SoyMediaFormat::Type Format)
{
	//	gr: automate this to be some codec->function map and search it
	
	if ( SoyMediaFormat::IsPixels( Format ) )
		return true;
	
	if ( SoyMediaFormat::IsText( Format ) )
		return true;
	
	return false;
}


bool TMediaPassThroughDecoder::ProcessPacket(std::shared_ptr<TMediaPacket>& Packet)
{
	//	note: before correction!
	OnDecodeFrameSubmitted( Packet->mTimecode );

	if ( mTextOutput )
	{
		if ( ProcessTextPacket( Packet ) )
			return true;
	}
	
	return ProcessPacket( *Packet );
}

bool TMediaPassThroughDecoder::ProcessPacket(const TMediaPacket& Packet)
{
	try
	{
		if ( mPixelOutput )
		{
			if ( ProcessPixelPacket( Packet ) )
				return true;
		}
		
		if ( mAudioOutput )
		{
			if ( ProcessAudioPacket( Packet ) )
				return true;
		}

		std::stringstream Error;
		Error << "TMediaPassThroughDecoder doesn't know how to handle " << Packet;
		throw Soy::AssertException( Error.str() );
	}
	catch(std::exception& e)
	{
		std::Debug << "TMediaPassThroughDecoder error; " << e.what() << std::endl;
	}
	
	return true;
}

bool TMediaPassThroughDecoder::ProcessPixelPacket(const TMediaPacket& Packet)
{
	auto Block = [this]
	{
		if ( !IsWorking() )
			return false;
		return true;
	};
	
	//	pass through pixel buffers
	if ( Packet.mPixelBuffer )
	{
		TPixelBufferFrame Frame;
		Frame.mTimestamp = Packet.mTimecode;

		auto& Output = GetPixelBufferManager();
		Output.CorrectDecodedFrameTimestamp( Frame.mTimestamp );
		Output.mOnFrameDecoded.OnTriggered( Frame.mTimestamp );

		if ( !Output.PrePushPixelBuffer( Frame.mTimestamp ) )
			return true;
	
		Frame.mPixels = Packet.mPixelBuffer;
		Output.PushPixelBuffer( Frame, Block );
		return true;
	}

	//	handle generic pixels
	if ( Packet.mMeta.mPixelMeta.IsValid() )
	{
		TPixelBufferFrame Frame;
		Frame.mTimestamp = Packet.mTimecode;
		
		auto& Output = GetPixelBufferManager();
		Output.CorrectDecodedFrameTimestamp( Frame.mTimestamp );
		Output.mOnFrameDecoded.OnTriggered( Frame.mTimestamp );
		
		if ( !Output.PrePushPixelBuffer( Frame.mTimestamp ) )
			return true;

		SoyPixelsRemote Pixels( GetArrayBridge(Packet.mData), Packet.mMeta.mPixelMeta );

		Frame.mPixels.reset( new TDumbPixelBuffer( Pixels ) );
		Output.PushPixelBuffer( Frame, Block );
		return true;
	}

	return false;
}

bool TMediaPassThroughDecoder::ProcessAudioPacket(const TMediaPacket& Packet)
{
	//	todo
	return false;
}

bool TMediaPassThroughDecoder::ProcessTextPacket(std::shared_ptr<TMediaPacket>& Packet)
{
	auto& Output = *mTextOutput;
	Output.CorrectDecodedFrameTimestamp( Packet->mTimecode );
	Output.mOnFrameDecoded.OnTriggered( Packet->mTimecode );
	
	Output.PushBuffer( Packet );
	return true;
}



TDumbPixelBuffer::TDumbPixelBuffer(SoyPixelsMeta Meta)
{
	if (!Meta.IsValid())
	{
		std::stringstream Error;
		Error << "Pixel buffer meta invalid " << Meta;
		throw Soy::AssertException(Error.str());
	}
	
	mPixels.Init( Meta );
}

TDumbPixelBuffer::TDumbPixelBuffer(const SoyPixelsImpl& Pixels)
{
	mPixels.Copy( Pixels );
}



SoyTime TPixelBufferManager::GetNextPixelBufferTime(bool Safe)
{
	if ( mFrames.empty() )
		return SoyTime();
	
	if ( Safe )
	{
		std::lock_guard<std::mutex> Lock( mFrameLock );
		return mFrames[0].mTimestamp;
	}
	else
	{
		return mFrames[0].mTimestamp;
	}
}

bool TPixelBufferManager::IsPixelBufferFull() const
{
	//	gr: avoid a lock please! just have to assume number won't be corrupt
	auto FrameCount = mFrames.size();
	
	//	just in case number is corrupted due to [lack of] threadsafety
	std::clamp<size_t>( FrameCount, 0, 100 );
	
	return FrameCount >= mParams.mMaxBufferSize;
}

bool TPixelBufferManager::PeekPixelBuffer(SoyTime Timestamp)
{
	if ( mFrames.empty() )
		return false;
	
	//	if the first is in the past, or now, then we have one, otherwise it's in the future and not ready to be popped
	//	gr: be safe, lock
	static bool ContentionPeekResult = true;
	if ( !mFrameLock.try_lock() )
		return ContentionPeekResult;
	
	auto& NextFrameTime = mFrames[0].mTimestamp;
	bool Result = NextFrameTime <= Timestamp;
	
	mFrameLock.unlock();
	
	return Result;
}


std::shared_ptr<TPixelBuffer> TPixelBufferManager::PopPixelBuffer(SoyTime& Timestamp)
{
	SoyTime RequestedTimestamp = Timestamp;
	static bool AvoidLockContention = true;
	
	if ( AvoidLockContention )
	{
		//	pre-empty jump out to avoid lock contention
		if ( mFrames.empty() )
			return nullptr;
	}
	
	//	frames are sorted so need to pick the frame closest to us in the past
	if ( AvoidLockContention )
	{
		if ( !mFrameLock.try_lock() )
			return nullptr;
	}
	else
	{
		mFrameLock.lock();
	}
	
	Soy::TScopeCall AutoUnlock( nullptr, [&]{	mFrameLock.unlock();	} );

	//	require buffering of N frames
	//	gr: this may need to be more intelligent to skip over frames in the past still....
	auto MinBufferSize = GetMinBufferSize();
	if ( MinBufferSize > 0 )
	{
		if ( mFrames.size() < MinBufferSize )
		{
			std::Debug << "Waiting for " << (MinBufferSize-mFrames.size()) << " more frames to buffer..." << std::endl;
			return nullptr;
		}
	}

	//	not synchronised, just grab next frame
	if ( !mParams.mPopFrameSync )
	{
		if ( mFrames.empty() )
		{
			return nullptr;
		}

		auto& Frame = *mFrames.begin();
		Timestamp = Frame.mTimestamp;
		std::shared_ptr<TPixelBuffer> LastPixelBuffer = Frame.mPixels;
		mFrames.erase( mFrames.begin() );
		return LastPixelBuffer;
	}
	
	BufferArray<SoyTime,100> FramesSkipped;
	std::shared_ptr<TPixelBuffer> LastPixelBuffer;
	for ( auto it=mFrames.begin();	it!=mFrames.end();	)
	{
		auto& Frame = *it;
		
		if ( mParams.mPopNearestFrame )
		{
			//	if this frame is further away than the currently found one, abort
			if ( LastPixelBuffer )
			{
				auto OldDiff = Timestamp.GetDiff( RequestedTimestamp );
				auto NewDiff = Frame.mTimestamp.GetDiff( RequestedTimestamp );
				if ( labs(NewDiff) > labs(OldDiff) )
					break;
			}
			else
			{
				//	don't pop if the only frame in the buffer is in the future.
				//	this means we'll look ahead... IF there is one ahead that's better.
				if ( Frame.mTimestamp > RequestedTimestamp )
					break;
			}
		}
		else
		{
			//	future frame, stop looking
			if ( Frame.mTimestamp > RequestedTimestamp )
				break;
		}
		
		//	in the past, pop this
		LastPixelBuffer = Frame.mPixels;
		Timestamp = Frame.mTimestamp;
		FramesSkipped.PushBack( Frame.mTimestamp );
		it = mFrames.erase( it );
	}

	//	gr: last frame, wasn't skipped, it was returned!
	if ( !FramesSkipped.IsEmpty() )
		FramesSkipped.PopBack();

	//	gr: make sure this doesn't cause a deadlock as the mFrameLock is now released AFTER this
	//	must have skipped a frame, report it
	if ( !FramesSkipped.IsEmpty() )
		mOnFramePopSkipped.OnTriggered( GetArrayBridge(FramesSkipped) );
	
	return LastPixelBuffer;
}

bool ComparePixelBuffers(const TPixelBufferFrame& a,const TPixelBufferFrame& b)
{
	return a.mTimestamp < b.mTimestamp;
}

bool TPixelBufferManager::PrePushPixelBuffer(SoyTime Timestamp)
{
	//	always let frame through
	if ( !mParams.mAllowPushRejection )
		return true;
	
	if ( mPlayerTime <= Timestamp )
		return true;
	
	//	frame already in the past, skip
	mOnFramePushSkipped.OnTriggered( Timestamp );
	return false;
}

bool TPixelBufferManager::PushPixelBuffer(TPixelBufferFrame& PixelBuffer,std::function<bool()> Block)
{
	if ( !PixelBuffer.mTimestamp.IsValid() )
	{
		std::Debug << "TPixelBufferManager::PushPixelBuffer packet with invalid timestamp. should ALWAYS be corrected to at least 1" << std::endl;
		PixelBuffer.mTimestamp.mTime = 1;
	}
	
	do
	{
		//	wait for frames to be popped
		if ( mFrames.size() >= std::max<size_t>(mParams.mMaxBufferSize,1) )
		{
			if ( mParams.mDebugFrameSkipping )
				std::Debug << "Frame buffer full... (" << mFrames.size() << ") " << std::endl;
			
			if ( !Block )
				break;
			
			auto PauseMs = mParams.mPushBlockSleepMs * mFrames.size();
			if ( mParams.mDebugFrameSkipping )
				std::Debug << __func__ << " pausing for " << PauseMs << "ms" << std::endl;
			std::this_thread::sleep_for( std::chrono::milliseconds(PauseMs) );
			continue;
		}
		
		mFrameLock.lock();
		mFrames.push_back( PixelBuffer );
		std::sort( mFrames.begin(), mFrames.end(), ComparePixelBuffers );
		mFrameLock.unlock();
		
		mOnFramePushed.OnTriggered( PixelBuffer.mTimestamp );
		return true;
	}
	while ( Block() );
	
	mOnFramePushFailed.OnTriggered( PixelBuffer.mTimestamp );
	
	return false;
}



void TPixelBufferManager::ReleaseFrames()
{
	//	free up frames
	mFrameLock.lock();
	//	erroenous loop, just here for debugging
	for ( auto it=mFrames.begin();	it!=mFrames.end();	)
	{
		//	gr: any frames in here shouldn't be locked/in use as they should have been popped before use
		auto& Frame = *it;
		Frame.mPixels.reset();
		it = mFrames.erase(it);
	}
	mFrames.clear();
	mFrameLock.unlock();
}



