#include "SoyMedia.h"
#include "SortArray.h"


//	android define
#define MIMETYPE_AUDIO_RAW	"audio/raw"


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
	{ SoyMediaFormat::Mpeg2,			"Mpeg2" },
	{ SoyMediaFormat::Mpeg4,			"Mpeg4" },
	{ SoyMediaFormat::Wave,				"wave" },
	{ SoyMediaFormat::Aac,				"aac" },
	{ SoyMediaFormat::PcmAndroidRaw,	"PcmAndroidRaw" },
	{ SoyMediaFormat::PcmLinear_8,		"PcmLinear_8" },
	{ SoyMediaFormat::PcmLinear_16,		"PcmLinear_16" },
	{ SoyMediaFormat::PcmLinear_20,		"PcmLinear_20" },
	{ SoyMediaFormat::PcmLinear_24,		"PcmLinear_24" },
	{ SoyMediaFormat::Text,				"text" },
	{ SoyMediaFormat::Subtitle,			"subtitle" },
	{ SoyMediaFormat::ClosedCaption,	"closedcaption" },
	{ SoyMediaFormat::Timecode,			"timecode" },
	{ SoyMediaFormat::MetaData,			"metadata" },
	{ SoyMediaFormat::Muxed,			"muxed" },
	
	
	{ SoyMediaFormat::Greyscale,		"Greyscale" },
	{ SoyMediaFormat::GreyscaleAlpha,	"GreyscaleAlpha" },
	{ SoyMediaFormat::RGB,				"RGB" },
	{ SoyMediaFormat::RGBA,				"RGBA" },
	{ SoyMediaFormat::BGRA,				"BGRA" },
	{ SoyMediaFormat::BGR,				"BGR" },
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
	{ SoyMediaFormat::ChromaUV_8_8,		"ChromaUV_8_8" },
	{ SoyMediaFormat::ChromaUV_88,		"ChromaUV_88" },
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
	
	switch ( Format )
	{
		case SoyMediaFormat::Mpeg2TS:		return true;
		case SoyMediaFormat::Mpeg2:			return true;
		case SoyMediaFormat::Mpeg4:			return true;
		default:							return false;
	}
}

bool SoyMediaFormat::IsH264(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::H264_8:		return true;
		case SoyMediaFormat::H264_16:		return true;
		case SoyMediaFormat::H264_32:		return true;
		case SoyMediaFormat::H264_ES:		return true;
		case SoyMediaFormat::H264_SPS_ES:	return true;
		case SoyMediaFormat::H264_PPS_ES:	return true;
		default:							return false;
	}
}

bool SoyMediaFormat::IsAudio(SoyMediaFormat::Type Format)
{
	switch ( Format )
	{
		case SoyMediaFormat::Aac:
		case SoyMediaFormat::PcmLinear_8:
		case SoyMediaFormat::PcmLinear_16:
		case SoyMediaFormat::PcmLinear_20:
		case SoyMediaFormat::PcmLinear_24:
		case SoyMediaFormat::Wave:
		case SoyMediaFormat::PcmAndroidRaw:
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
		case SoyMediaFormat::Mpeg4:		return "video/mp4";	//	find the proper version of this
	
		case SoyMediaFormat::Wave:		return "audio/wave";
			
		//	gr: change this to handle multiple mime types per format
#if defined(TARGET_ANDROID)
		case SoyMediaFormat::Aac:		return "audio/mp4a-latm";
#else
		case SoyMediaFormat::Aac:		return "audio/x-aac";
#endif
		//	https://en.wikipedia.org/wiki/Pulse-code_modulation
		case SoyMediaFormat::PcmLinear_8:	return "audio/L8";
		case SoyMediaFormat::PcmLinear_16:	return "audio/L16";
		case SoyMediaFormat::PcmLinear_20:	return "audio/L20";
		case SoyMediaFormat::PcmLinear_24:	return "audio/L24";
			
		case SoyMediaFormat::PcmAndroidRaw:	return MIMETYPE_AUDIO_RAW;
			
		default:						return "invalid/invalid";
	}
	
}

SoyMediaFormat::Type SoyMediaFormat::FromMime(const std::string& Mime)
{
	if ( Mime == ToMime( SoyMediaFormat::H264_8 ) )			return SoyMediaFormat::H264_8;
	if ( Mime == ToMime( SoyMediaFormat::H264_16 ) )		return SoyMediaFormat::H264_16;
	if ( Mime == ToMime( SoyMediaFormat::H264_32 ) )		return SoyMediaFormat::H264_32;
	if ( Mime == ToMime( SoyMediaFormat::H264_ES ) )		return SoyMediaFormat::H264_ES;
	if ( Mime == ToMime( SoyMediaFormat::H264_PPS_ES ) )	return SoyMediaFormat::H264_PPS_ES;
	if ( Mime == ToMime( SoyMediaFormat::H264_SPS_ES ) )	return SoyMediaFormat::H264_SPS_ES;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg2TS ) )		return SoyMediaFormat::Mpeg2TS;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg2 ) )			return SoyMediaFormat::Mpeg2;
	if ( Mime == ToMime( SoyMediaFormat::Mpeg4 ) )			return SoyMediaFormat::Mpeg4;
	if ( Mime == ToMime( SoyMediaFormat::Wave ) )			return SoyMediaFormat::Wave;
	if ( Mime == ToMime( SoyMediaFormat::Aac ) )			return SoyMediaFormat::Aac;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_8 ) )	return SoyMediaFormat::PcmLinear_8;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_16 ) )	return SoyMediaFormat::PcmLinear_16;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_20 ) )	return SoyMediaFormat::PcmLinear_20;
	if ( Mime == ToMime( SoyMediaFormat::PcmLinear_24 ) )	return SoyMediaFormat::PcmLinear_24;
	if ( Mime == ToMime( SoyMediaFormat::PcmAndroidRaw ) )	return SoyMediaFormat::PcmAndroidRaw;
	
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
		case SoyMediaFormat::Aac:	return 'aac ';
		case SoyMediaFormat::Mpeg4:	return 'mp4v';

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

	

SoyMediaFormat::Type SoyMediaFormat::FromFourcc(uint32 Fourcc,int H264LengthSize)
{
	//	todo: handle reverse here automatically and use ToFourcc()
	switch ( Fourcc )
	{
		case 'avc1':
		case '1cva':
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
		case '462H':
			return SoyMediaFormat::H264_32;
			break;
			

		case 'aac ':
		case ' caa':
			return SoyMediaFormat::Aac;

		//	windows/MediaFoundation have fourcc's in caps
		case 'MP4V':
		case 'V4PM':
		case 'mp4v':
		case 'v4pm':	
			return SoyMediaFormat::Mpeg4;

		case 'lpcm':
		case 'mcpl':
			if ( H264LengthSize == 8 )
				return SoyMediaFormat::PcmLinear_8;
			if ( H264LengthSize == 16 )
				return SoyMediaFormat::PcmLinear_16;
			if ( H264LengthSize == 20 )
				return SoyMediaFormat::PcmLinear_20;
			if ( H264LengthSize == 24 )
				return SoyMediaFormat::PcmLinear_24;
			break;
	}
	
	std::Debug << "Unknown fourcc type: " << Soy::FourCCToString(Fourcc) << std::endl;
	return SoyMediaFormat::Invalid;
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
			if ( !ProcessPacket( *Packet ) )
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

	if ( !mLastPacketTimestamp.IsValid() )
		mLastPacketTimestamp = SoyTime( 1ull ) + DecodeToPresentationOffset;
	
	//	gr: if there is no decode timestamp, assume it needs to be in the order it comes in
	if ( !Packet.mDecodeTimecode.IsValid() )
	{
		Packet.mDecodeTimecode = mLastPacketTimestamp + mAutoTimestampDuration;
		std::Debug << "Corrected incoming packet DECODE timecode; now " << Packet.mTimecode << "(PTS) and " << Packet.mDecodeTimecode << "(DTS)" << std::endl;
	}

	//	don't have any timecodes
	if ( !Packet.mTimecode.IsValid() )
	{
		Packet.mTimecode = Packet.mDecodeTimecode + DecodeToPresentationOffset;
		std::Debug << "Corrected incoming packet PRESENTATION timecode; now " << Packet.mTimecode << "(PTS) and " << Packet.mDecodeTimecode << "(DTS)" << std::endl;
	}

	//	gr: store last DECODE timestamp so this basically forces packets to be ordered by decode timestamps
	mLastPacketTimestamp = Packet.mDecodeTimecode;
}


TMediaExtractor::TMediaExtractor(const std::string& ThreadName,SoyTime ExtractAheadMs,std::function<void(const SoyTime,size_t)> OnPacketExtracted) :
	SoyWorkerThread		( ThreadName, SoyWorkerWaitMode::Wake ),
	mExtractAheadMs		( ExtractAheadMs ),
	mOnPacketExtracted	( OnPacketExtracted )
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

void TMediaExtractor::OnEof()
{
	//	stop the thread?
	//std::Debug << "Media extractor EOF" << std::endl;
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
			
			//	temp
			if ( NextPacket->mEof )
				return;
			
			//	block thread unless it's stopped
			auto Block = [this,&NextPacket]()
			{
				//	gr: this is happening a LOT, probably because the extractor is very fast... maybe throttle the thread...
				if ( IsWorking() )
				{
					std::Debug << "MediaExtractor blocking in push packet; " << *NextPacket << std::endl;
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

			//	end/break even for packets we don't save
			if ( NextPacket->mEof )
				OnEof();
			
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

void TMediaEncoder::PushFrame(std::shared_ptr<TMediaPacket>& Packet,std::function<bool(void)> Block)
{
	//	gr: check/assign stream index here?
	Soy::Assert( mOutput!=nullptr, "TMediaEncoder::PushFrame output expected" );
	mOutput->PushPacket( Packet, Block );
}



TMediaMuxer::TMediaMuxer(std::shared_ptr<TStreamWriter>& Output,std::shared_ptr<TMediaPacketBuffer>& Input,const std::string& ThreadName) :
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
		Timestamp.mTime = 1;
	
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
		Timestamp.mTime -= mFirstTimestamp.mTime;
	}
	
	mLastTimestamp = Timestamp;
}


void TMediaBufferManager::SetPlayerTime(const SoyTime &Time)
{
	mPlayerTime = Time;
}




void TAudioBufferManager::PushAudioBuffer(const TAudioBufferBlock& AudioData)
{
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		mBlocks.PushBack( AudioData );
	}
	mOnFramePushed.OnTriggered( AudioData.mStartTime );
}

void TAudioBufferManager::PopAudioBuffer(ArrayBridge<float>&& Data,size_t Channels,SoyTime StartTime,SoyTime EndTime)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );

	//	init output
	for ( int i=0;	i<Data.GetSize();	i++ )
		Data[i] = 0.5f;
	
	//	work out where audio starts and pop chunks out of each block
	int DataIndex = 0;
	
	while ( DataIndex < Data.GetSize() )
	{
		//	eat from head block until its empty
		if ( mBlocks.IsEmpty() )
			break;
		
		auto& Block = mBlocks[0];
		if ( Block.mData.IsEmpty() )
		{
			mBlocks.RemoveBlock( 0, 1 );
			continue;
		}
		Data[DataIndex] = Block.mData.PopAt(0);
		DataIndex++;
	}
}

void TAudioBufferManager::ReleaseFrames()
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	mBlocks.Clear();
}

