#include "SoyMedia.h"
#include "SortArray.h"
#include "SoyJson.h"
#include "SoyWave.h"

//gr: this is for the pass through encoder, maybe to avoid this dependancy I can move the pass throughs to their own files...
#include "SoyOpenGl.h"
#include "SoyOpenGlContext.h"

#if defined(TARGET_WINDOWS)
#include "SoyDirectx.h"
#endif

#include "SoyPool.h"


prmem::Heap SoyMedia::DefaultHeap(true, true, "SoyMedia::DefaultHeap" );




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
	
	//	pre-push check for flush fence
	if ( !PrePushBuffer( Packet->GetSortingTimecode() ) )
	{
		return;
	}
	
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



void TMediaPacketBuffer::FlushFrames(SoyTime FlushTime)
{
	ReleaseFramesAfter( FlushTime );
	
	//	gr: if the flush fence is zero, it doesn't get used, so correct it
	mFlushFenceTime = FlushTime;
	if ( !mFlushFenceTime.IsValid() )
		mFlushFenceTime.mTime = 1;
}



void TMediaPacketBuffer::ReleaseFramesAfter(SoyTime FlushTime)
{
	std::lock_guard<std::mutex> Lock( mPacketsLock  );
	
	for ( ssize_t i=mPackets.GetSize()-1;	i>=0;	i-- )
	{
		auto& Packet = *mPackets[i];
		if ( Packet.GetStartTime() <= FlushTime )
			continue;
		
		mPackets.RemoveBlock( i, 1 );
	}
}


bool TMediaPacketBuffer::PrePushBuffer(SoyTime Timestamp)
{
	if ( !mFlushFenceTime.IsValid() )
		return true;
	
	//	if we do =, then when the fence is 1, it rejects 1.
	if ( Timestamp > mFlushFenceTime )
	{
		std::Debug << "TMediaBufferManager::PrePush dropped (fenced " << Timestamp << " >= " << mFlushFenceTime << ")" << std::endl;
		return false;
	}
	
	//	new lower timestamp, release fence
	std::Debug << "TMediaBufferManager::FlushFence(" << mFlushFenceTime << ") dropped" << std::endl;
	mFlushFenceTime = SoyTime();
	return true;
}



TMediaExtractor::TMediaExtractor(const TMediaExtractorParams& Params) :
	SoyWorkerThread			( Params.mThreadName, SoyWorkerWaitMode::Wake ),
	mExtractAheadMs			( Params.mReadAheadMs ),
	mOnPacketExtracted		( Params.mOnFrameExtracted ),
	mParams					( Params )
{
	//	gr: need some kind of heirachy for the initial time, to disallow TVideoDecoder from going past 0 if the extractor doesn't support it
	mSeekTime = Params.mInitialTime;
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

void TMediaExtractor::Seek(SoyTime Time,const std::function<void(SoyTime)>& FlushFrames)
{
	//	update the target seek time
	bool AllowReverse = CanSeekBackwards();
	if ( !AllowReverse && Time < mSeekTime )
	{
		std::stringstream Error;
		Error << "This decoder (" << Soy::GetTypeName(*this) << ") cannot seek backwards " << Time << " < " << mSeekTime;
		throw Soy::AssertException( Error.str() );
	}
	
	mSeekTime = Time;
	SoyTime FlushFramesAfter = Time;

	//	let extractors throw, but catch it as a warning for now. Maybe later allow this to throw back an error to user/unity
	try
	{
		if ( OnSeek() )
		{
			FlushFrames( FlushFramesAfter );
		}
	}
	catch(std::exception& e)
	{
		std::Debug << "Exception during Seek(" << Time << ") " << e.what() << std::endl;
		//	wake up the thread regardless even if we're reporting an error back
		//Wake();
		//throw;
	}

	//	wake up thread to try and read more frames again
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
		ReadPacketsUntil( GetExtractTime(), Loop );
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
					//	gr: wording is wrong here, but regardless, we block when buffer X is full, we shouldn't need them all flushed!
					//	gr: use only for testing for now!
					static bool DiscardDoesntBlock = false;
					if ( DiscardDoesntBlock && this->mParams.mDiscardOldFrames )
						return false;

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

void TMediaExtractor::GetMeta(TJsonWriter& Json)
{
	Json.Push("CanSeekBackwards", CanSeekBackwards() );
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
	//	todo: do this as a func controlled by the video decoder

	//	skip non-keyframes
	if ( !IsKeyframe && mParams.mOnlyExtractKeyframes )
		return false;
	
	if ( Time >= mSeekTime )
		return true;
	
	//	in the past, if it's not a keyframe, lets skip it
	if ( !IsKeyframe )
	{
		OnSkippedExtractedPacket( Time );
		return false;
	}
	
	return true;
}

void TMediaExtractor::OnSkippedExtractedPacket(const SoyTime& Timecode)
{
	std::Debug << "Extractor skipped frame " << Timecode << " (vs " << mSeekTime << ") in the past (non-keyframe)" << std::endl;
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



void TMediaExtractor::OnPacketExtracted(std::shared_ptr<TMediaPacket>& Packet)
{
	Soy::Assert( Packet != nullptr, "Packet expected");
	OnPacketExtracted( *Packet );
}

void TMediaExtractor::OnPacketExtracted(TMediaPacket& Packet)
{
	OnPacketExtracted( Packet.mTimecode, Packet.mMeta.mStreamIndex );
}

void TMediaExtractor::OnPacketExtracted(SoyTime& Timecode,size_t StreamIndex)
{
	//	if this is the first timecode for the stream, set it
	if ( mStreamFirstFrameTime.find( StreamIndex ) == mStreamFirstFrameTime.end() )
	{
		if ( mParams.mResetInternalTimestamp )
			mStreamFirstFrameTime[StreamIndex] = Timecode;
		else
			mStreamFirstFrameTime[StreamIndex] = SoyTime();
	}

	//	correct timecode
	SoyTime FirstTimecode = mStreamFirstFrameTime[StreamIndex];
	if ( Timecode < FirstTimecode )
	{
		//	bad first timecode!
		std::Debug << "Bad first timecode (" << FirstTimecode << " vs " << Timecode << ") for stream " << StreamIndex << std::endl;
		Timecode.mTime = 0;
	}
	else
	{
		Timecode.mTime -= FirstTimecode.mTime;
	}

	//	output timecode never zero
	if ( Timecode.mTime == 0 )
		Timecode.mTime = 1;

	//	callback
	if ( mOnPacketExtracted )
		mOnPacketExtracted( Timecode, StreamIndex );

	Wake();
}

SoyTime TMediaExtractor::GetExtractorRealTimecode(SoyTime PacketTimecode,ssize_t StreamIndex)
{
	SoyTime FirstTimecode;

	if ( mStreamFirstFrameTime.find( StreamIndex ) != mStreamFirstFrameTime.end() )
	{
		FirstTimecode = mStreamFirstFrameTime[StreamIndex];
	}
	else if ( !mStreamFirstFrameTime.empty() )
	{
		auto it = mStreamFirstFrameTime.begin();
		FirstTimecode = it->second;
	}

	//	correct never-zero timecode
	if ( PacketTimecode.mTime == 1 )
		PacketTimecode.mTime = 0;

	PacketTimecode += FirstTimecode;
	return PacketTimecode;
}

/*
SoyTime TMediaExtractor::GetRealTime(ssize_t StreamIndex,const SoyTime& FrameTime)
{
	SoyTime FirstTime = GetStreamFirstTime( StreamIndex );

	//	frametime of 1 is actually 0
	if ( FrameTime.mTime == 1 )
		return FirstTime;

	FirstTime += FrameTime;
	return FirstTime;
}

SoyTime TMediaExtractor::GetPacketTime(ssize_t StreamIndex,const SoyTime& RealTime)
{
	SoyTime FirstTime = GetStreamFirstTime( StreamIndex );

	if ( RealTime < FirstTime )
	{
		//	first time is wrong if RealTime is an actual timestamp
		return SoyTime();
	}

	SoyTime FrameTime = RealTime;
	FrameTime -= FirstTime;

	//	valid frame time is never zero
	if ( FrameTime.mTime == 0 )
		FrameTime.mTime = 1; 

	return FrameTime;
}
*/

void TMediaExtractor::FlushFrames(SoyTime FlushTime)
{
	for ( auto it=mStreamBuffers.begin();	it!=mStreamBuffers.end();	it++ )
	{
		auto Buffer = it->second;
		if ( !Buffer )
			continue;
		
		Buffer->FlushFrames( FlushTime );
	}
}




void TMediaEncoder::PushFrame(std::shared_ptr<TMediaPacket>& Packet,std::function<bool(void)> Block)
{
	//	gr: check/assign stream index here?
	Soy::Assert( mOutput!=nullptr, "TMediaEncoder::PushFrame output expected" );
	mOutput->PushPacket( Packet, Block );
}

void TMediaEncoder::GetMeta(TJsonWriter& Json)
{
	if ( mOutput )
		Json.Push("EncoderPendingOutputFrames", mOutput->GetPacketCount() );
}


TMediaMuxer::TMediaMuxer(std::shared_ptr<TStreamWriter> Output,std::shared_ptr<TMediaPacketBuffer>& Input,const std::string& ThreadName) :
	SoyWorkerThread		( ThreadName, SoyWorkerWaitMode::Wake ),
	mOutput				( Output ),
	mInput				( Input ),
	mDefferedPackets	( SoyMedia::DefaultHeap )
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

void TMediaMuxer::GetMeta(TJsonWriter& Json)
{
	auto InputQueueSize = mInput ? mInput->GetPacketCount() : -1;
	auto DefferedQueueSize = mDefferedPackets.GetSize();

	Json.Push("MuxerInputQueueCount", InputQueueSize);
	Json.Push("MuxerDefferedQueueCount", DefferedQueueSize);
}



void TMediaBufferManager::GetMeta(const std::string& Prefix,TJsonWriter& Json)
{
	Json.Push( Prefix + "RealFirstTimestamp", mFirstTimestamp.mTime );
	Json.Push( Prefix + "HadEndOfFilePacket", mHasEof );
}


void TMediaBufferManager::CorrectRequestedFrameTimestamp(SoyTime& Timestamp)
{
	//	first request from the client is most likely to be 0, but all our frames start at one
	if ( Timestamp.mTime == 0 )
	{
		Timestamp.mTime = 1;
	}
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
		//	gr: if using pre-seek, lets ignore setting it for now
		if ( mParams.mPreSeek.IsValid() )
		{
			std::Debug << "Using preseek, so skipping mFirstTimestamp " << std::endl;
			mFirstTimestamp = SoyTime( 1ull );
		}
		/*
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
		 */
	}
	
	/*
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
	*/
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

void TMediaBufferManager::FlushFrames(SoyTime FlushTime)
{
	ReleaseFramesAfter( FlushTime );
	
	//	gr: if the flush fence is zero, it doesn't get used, so correct it
	mFlushFenceTime = FlushTime;
	if ( !mFlushFenceTime.IsValid() )
		mFlushFenceTime.mTime = 1;
}

bool TMediaBufferManager::PrePushBuffer(SoyTime Timestamp)
{
	if ( !mFlushFenceTime.IsValid() )
		return true;

	//	if we do =, then when the fence is 1, it rejects 1.
	if ( Timestamp > mFlushFenceTime )
	{
		std::Debug << "TMediaBufferManager::PrePush dropped (fenced " << Timestamp << " >= " << mFlushFenceTime << ")" << std::endl;
		return false;
	}

	//	new lower timestamp, release fence
	std::Debug << "TMediaBufferManager::FlushFence(" << mFlushFenceTime << ") dropped" << std::endl;
	mFlushFenceTime = SoyTime();
	return true;
}

SoyTime TAudioBufferBlock::GetEndTime() const
{
	auto LastSampleIndex = mData.GetSize();
	return GetSampleTime( LastSampleIndex );
}


SoyTime TAudioBufferBlock::GetLastDataTime() const
{
	auto LastSampleIndex = mData.GetSize();
	LastSampleIndex -= mChannels;
	return GetSampleTime( LastSampleIndex );
}


SoyTime TAudioBufferBlock::GetSampleTime(size_t SampleIndex) const
{
	if ( !IsValid() )
		return SoyTime();

	//	frequency is samples per sec
	float SampleDuration = 1.f / (float)(mFrequency * mChannels);
	float SampleTime = SampleIndex * SampleDuration;
	float SampleTimeMsf = SampleTime * 1000.f;
	
	//	determine if this doesn't align to a MS, and move FORWARD so we don't repeat data
	float fractpart, intpart;
	fractpart = modff( SampleTimeMsf, &intpart );
	if ( fractpart > 0 )
	{
		intpart++;
	}
	
	auto SampleTimeMs = size_cast<uint64>( intpart );
	return mStartTime + SoyTime(SampleTimeMs);
}

ssize_t TAudioBufferBlock::GetTimeSampleIndex(SoyTime Time) const
{
	if ( mChannels == 0 )
		return -1;

	//	how many ms is one sample?
	float SamplesPerSec = mFrequency * mChannels;
	float SamplesPerMs = SamplesPerSec / 1000.f;
	//float SampleDuration = 1.f / (float)(mFrequency * mChannels);

	if ( Time == mStartTime )
		return 0;
	
	if ( Time > mStartTime )
	{
		auto AheadMs = Time.GetTime() - mStartTime.GetTime();
		auto SampleIndexf = AheadMs * SamplesPerMs;
		auto SampleIndex = static_cast<size_t>( SampleIndexf );
		//	always return first channel of sample!
		SampleIndex -= SampleIndex % mChannels;

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

	//	set new start time
	mStartTime = Time;
	
	return RemoveCount;
}


void TAudioBufferBlock::Append(const TAudioBufferBlock& NewData)
{
	static bool Verbose = false;

	if ( !IsValid() )
	{
		*this = NewData;
		return;
	}
	Soy::Assert(mChannels == NewData.mChannels, "TAudioBufferBlock::Append channel size mismatch");
	Soy::Assert(mFrequency == NewData.mFrequency, "TAudioBufferBlock::Append frequency mismatch");

	//	deal with gaps in time with padding
	auto CurrentEnd = GetEndTime();
	auto NewDataEnd = NewData.GetEndTime();

	static int ToleranceForPaddingMs = 2;

	BufferArray<float, 4> PadData;
	auto LastSampleIndex = mData.GetSize() - 1 - mChannels;
	for ( int c = 0; c < mChannels; c++ )
	{
		static bool PadWithZero = true;
		if ( PadWithZero )
			PadData.PushBack( 0 );
		else
			PadData.PushBack(mData[LastSampleIndex + c]);
	}
	
	auto RequiredPadding = NewData.GetStartTime().GetDiff( CurrentEnd );


	TAudioBufferBlock NewDataMutable;
	auto* pUseNewData = &NewData;

	//	check for overlap
	if ( RequiredPadding < 0 )
	{
		if ( Verbose )
			std::Debug << "Warning: overlap at append... (" << CurrentEnd << " < " << NewData.GetStartTime() << ")" << std::endl;
		NewDataMutable = NewData;
		NewDataMutable.Clip( CurrentEnd, NewDataMutable.GetEndTime() );
		pUseNewData = &NewDataMutable;
		NewDataEnd = NewDataMutable.GetEndTime();
	}
	else if ( RequiredPadding > ToleranceForPaddingMs )
	{
		//	gr: rare
		//if ( Verbose )
			std::Debug << "Padding at append... (" << CurrentEnd << " < " << NewData.GetStartTime() << ")" << std::endl;
		
		while ( CurrentEnd < NewData.GetStartTime() )
		{
			//std::Debug << "Padding at append... (" << CurrentEnd << " < " << NewData.GetStartTime() << ")" << std::endl;
			mData.PushBackArray(PadData);
			CurrentEnd = GetEndTime();
		}
	}

	auto& UseNewData = *pUseNewData;
	mData.PushBackArray(UseNewData.mData);

	//	print warning if new end time didn't align....
	auto FinalNewEndTime = GetEndTime();
	if ( NewDataEnd != FinalNewEndTime )
	{
		if ( Verbose )
			std::Debug << "Appended buffer, end is now " << FinalNewEndTime << " (appended end was " << NewDataEnd << ")" << std::endl;
	}

}

void TAudioBufferBlock::Clip(SoyTime Start, SoyTime End)
{
	auto EndIndex = GetTimeSampleIndex(End);
	if ( EndIndex >= 0 && EndIndex < mData.GetSize() )
		mData.SetSize(EndIndex + 1);

	RemoveDataUntil(Start);
}

void TAudioBufferBlock::SetChannels(size_t Channels)
{
	if ( mChannels == Channels )
		return;

	//	re-sample channels
	if ( Channels < mChannels )
	{
		auto RemoveChannelCount = mChannels - Channels;
		for ( int i = mData.GetSize() - mChannels; i >= 0;	i-=mChannels )
		{
			//	remove the LAST X samples, so we assume 0&1 are left & right, and all the others (eg, surround) gets discarded
			mData.RemoveBlock(i + Channels, RemoveChannelCount);
		}
		mChannels = Channels;
		return;
	}
	
	if ( Channels > mChannels )
	{
		auto InsertChannelCount = Channels - mChannels;
		
		//	reserve data
		mData.Reserve( InsertChannelCount* mData.GetSize() );
		
		for ( int i = mData.GetSize() - mChannels; i >= 0;	i-=mChannels )
		{
			//	insert X samples
			auto Sample = mData[i];
			mData.InsertBlock(i + 1, InsertChannelCount);
			for ( int c = 0; c < InsertChannelCount; c++ )
				mData[i + 1 + c] = Sample;
		}
		mChannels = Channels;
		return;
	}
}

void TAudioBufferBlock::SetFrequencey(size_t Frequency)
{
	if ( mFrequency == Frequency )
		return;

	std::Debug << "todo: resample audio frequency from " << mFrequency << " to " << Frequency << std::endl;
}

void TAudioBufferManager::PushAudioBuffer(TAudioBufferBlock& AudioData)
{
	{
		//	convert to the right format
		Soy::Assert( AudioData.mFrequency != 0, "Audio data should not have zero frequency");
		Soy::Assert( AudioData.mChannels != 0, "Audio data should not have zero channels");

		if ( mFormat.mChannels == 0 )
			mFormat.mChannels = AudioData.mChannels;
		if ( mFormat.mFrequency == 0 )
			mFormat.mFrequency = AudioData.mFrequency;

		//	gr;  maybe consider changing this to allow rejection of data
		mOnAudioBlockPushed.OnTriggered( AudioData );

		AudioData.SetChannels( mFormat.mChannels );
		AudioData.SetFrequencey( mFormat.mFrequency );

		auto Start = AudioData.GetStartTime();
		auto End = AudioData.GetEndTime();
		auto EndIndex = AudioData.GetTimeSampleIndex(End);


		std::lock_guard<std::mutex> Lock( mBlocksLock );
		mBlocks.PushBack( AudioData );
	}
	mOnFramePushed.OnTriggered( AudioData.mStartTime );
}


void TAudioBufferManager::PopNextAudioData(ArrayBridge<float>&& Data,bool PadData)
{
	//	just read data straight from the blocks
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	
	size_t OrigDataSize = Data.GetSize();
	Data.Clear();
	
	while ( Data.GetSize() < OrigDataSize )
	{
		//	pop from first block
		if ( mBlocks.IsEmpty() )
			break;
		
		size_t Required = OrigDataSize - Data.GetSize();
		auto& Block = mBlocks[0];
		
		//	this corrupts block's start time
		auto PopAmount = std::min( Required, Block.mData.GetSize() );
		if ( PopAmount > 0 )
		{
			auto PopBlockData = GetArrayBridge(Block.mData).GetSubArray( 0, PopAmount );
			Data.PushBackArray( PopBlockData );
			Block.mData.RemoveBlock( 0, PopAmount );
		}
		
		//	cull data
		if ( Block.mData.IsEmpty() )
		{
			mBlocks.RemoveBlock( 0, 1 );
		}
	}
	
	//	didn't get enough data
	if ( Data.GetSize() < OrigDataSize )
	{
		if ( PadData )
		{
			while ( Data.GetSize() < OrigDataSize )
				Data.PushBack(0);
		}
	}
}


bool TAudioBufferManager::GetAudioBuffer(TAudioBufferBlock& FinalOutputBlock,bool VerboseDebug,bool PadOutputTail,bool ClipOutputFront,bool ClipOutputBack,bool CullBuffer)
{
	Soy::Assert(FinalOutputBlock.IsValid(), "Need target block for PopAudioBuffer");

	auto StartTime = FinalOutputBlock.GetStartTime();
	auto EndTime = FinalOutputBlock.GetEndTime();
	auto SampleRate = FinalOutputBlock.mFrequency;
	auto Channels = FinalOutputBlock.mChannels;

	if ( VerboseDebug )
		std::Debug << "[" << StartTime << "] Pop audio at " << StartTime << " for " << EndTime.GetDiff(StartTime) << "ms" << std::endl;

	
	//	pop out ALL the data for this time block
	TAudioBufferBlock OutputBlock;
	BufferArray<size_t,10> CopiedBlockIndexes;
	SoyTime BlocksTailTime;

	{
		if ( !mBlocksLock.try_lock() )
		{
			std::Debug << "GetAudioBuffer lock contention" << std::endl;
			return false;
		}
		//std::lock_guard<std::mutex> Lock(mBlocksLock);
		
		if ( !mBlocks.IsEmpty() )
			BlocksTailTime = mBlocks.GetBack().GetEndTime();

		//	build data from blocks that cover our timespan
		for ( int b=0;	b<mBlocks.GetSize();	b++ )
		{
			auto& Block = mBlocks[b];
			auto BlockStart = Block.GetStartTime();
			auto BlockEnd = Block.GetEndTime();

			//	in the future
			if ( BlockStart >= EndTime )
				break;

			//	in the past
			if ( BlockEnd < StartTime )
				continue;

			if ( VerboseDebug )
				std::Debug << "[" << StartTime << "] appending block " << b << " " << BlockStart << "..." << BlockEnd << std::endl;
			OutputBlock.Append(Block);
			CopiedBlockIndexes.PushBack( b );
		}
		
		mBlocksLock.unlock();
	}
	
	//	no data for time, so return without modifying
	if ( !OutputBlock.IsValid() )
	{
		if ( VerboseDebug )
			std::Debug << "No audio data for " << StartTime << "..." << EndTime << std::endl;
		return false;
	}

	//	reformat
	{
		OutputBlock.SetChannels(Channels);

		//	gr: this code needs to re-sample
		if ( OutputBlock.mFrequency != SampleRate )
			std::Debug << "Warning: data sample rate (" << OutputBlock.mFrequency << ") doesn't match desired rate (" << SampleRate << ")" << std::endl;
	}

	//	clip output block
	{
		SoyTime ClipStartTime = ClipOutputFront ? StartTime : OutputBlock.GetStartTime();
		SoyTime ClipEndTime = ClipOutputBack ? EndTime : OutputBlock.GetEndTime();
		OutputBlock.Clip(ClipStartTime, ClipEndTime);
		if ( VerboseDebug )
			std::Debug << "[" << StartTime << "] Clipped output to " << OutputBlock.GetStartTime() << "..." << OutputBlock.GetEndTime() << std::endl;
	}
	
	//	cull old data
	if ( CullBuffer )
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
	
		//	delete all blocks up to last, should cover it all linearly
		auto LastDeleteBlockIndex = CopiedBlockIndexes.GetBack();
		mBlocks.RemoveBlock( 0, LastDeleteBlockIndex );
	}

	//	should now be in same format
	auto& Data = FinalOutputBlock.mData;

	if ( OutputBlock.mData.GetSize() > Data.GetSize() )
	{
		//	clip buffer
		if ( ClipOutputBack )
			OutputBlock.mData.SetSize(Data.GetSize());
	}
	else if ( OutputBlock.mData.GetSize() < Data.GetSize() && PadOutputTail )
	{
		//	pad
		auto PadAmount = Data.GetSize() - OutputBlock.mData.GetSize();
		
		//	gr: should be rare now so always output
		std::Debug << "[" << StartTime << "] Padding audio by " << PadAmount << " samples... All-data-tail=" << BlocksTailTime << std::endl;
		while ( OutputBlock.mData.GetSize() < Data.GetSize() )
		{
			OutputBlock.mData.PushBack(0);
		}
	}
	
	static bool DebugAudioOutput = false;
	if ( DebugAudioOutput )
	{
		auto OutputStart = OutputBlock.GetStartTime();
		auto OutputEnd = OutputBlock.GetEndTime();

		std::stringstream Output100;
		for ( int i = 0; i <std::min<size_t>(10,OutputBlock.mData.GetSize()); i++ )
			Output100 << OutputBlock.mData[i] << " ";
		std::Debug << "[" << StartTime << "] Outputting " << OutputBlock.mData.GetSize() << " samples between " << OutputStart << " and " << OutputEnd << " ... " << Output100.str() << std::endl;
	}

	//	copy final output
	Data.Copy( OutputBlock.mData );
	FinalOutputBlock.mStartTime = OutputBlock.mStartTime;

	return true;
}


void TAudioBufferManager::ReleaseFrames()
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	mBlocks.Clear();
}

void TAudioBufferManager::SetPlayerTime(const SoyTime& Time)
{
	TMediaBufferManager::SetPlayerTime(Time);
	
	static bool CullData = true;
	static bool ClipCulledData = true;
	if ( CullData )
	{
		ReleaseFramesBefore( Time, ClipCulledData );
	}
}

void TAudioBufferManager::ReleaseFramesAfter(SoyTime FlushTime)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );

	//	find last index to keep
	for ( ssize_t i=mBlocks.GetSize()-1;	i>=0;	i-- )
	{
		auto& Block = mBlocks[i];
		if ( Block.GetStartTime() > FlushTime )
		{
			mBlocks.RemoveBlock( i, 1 );
			continue;
		}

		break;
	}
	
	
}


void TAudioBufferManager::ReleaseFramesBefore(SoyTime FlushTime,bool ClipOldData)
{
	static bool VerboseDebug = false;
	std::lock_guard<std::mutex> Lock( mBlocksLock );

	//	find last index to keep
	for ( ssize_t i=mBlocks.GetSize()-1;	i>=0;	i-- )
	{
		auto& Block = mBlocks[i];
		auto BlockEndTime = Block.GetEndTime();
		auto BlockStartTime = Block.GetStartTime();

		//	wholly in the past
		if ( BlockEndTime <= FlushTime )
		{
			if ( VerboseDebug )
				std::Debug << "Culled block " << i << " " << BlockStartTime << "..." << BlockEndTime << std::endl;
			mBlocks.RemoveBlock(i, 1);
			continue;
		}

		if ( ClipOldData )
		{
			if ( BlockStartTime < FlushTime )
			{
				Block.Clip(FlushTime, BlockEndTime);
				if ( VerboseDebug )
					std::Debug << "Cull-clipped block " << i << " to " << Block.GetStartTime() << "..." << Block.GetEndTime() << " (should be " << FlushTime << "..." << BlockEndTime << ")" << std::endl;
			}
		}

	}


}


void TAudioBufferManager::GetMeta(const std::string& Prefix,TJsonWriter& Json)
{
	Json.Push( Prefix + "Type", "Audio" );
	TMediaBufferManager::GetMeta( Prefix, Json );
	
	BufferArray<uint64,10> NextTimecodes;
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		for ( int i=0;	i<mBlocks.GetSize() && !NextTimecodes.IsFull();	i++ )
		{
			auto& Block = mBlocks[i];
			NextTimecodes.PushBack( Block.GetStartTime().mTime );
		}
	}
	
	Json.Push( (Prefix + "NextFrameTime").c_str(), GetArrayBridge(NextTimecodes) );
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


void TTextBufferManager::GetBuffer(std::stringstream& Output,SoyTime& StartTime,SoyTime& EndTime)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	SoyTime TargetTime = StartTime;
	
	//	invalidate output
	StartTime = SoyTime();

	for ( int b = 0;	b < mBlocks.GetSize();	b++ )
	{
		auto& pBlock = mBlocks[b];
		auto& Block = *pBlock;

		if ( !Block.ContainsTime(TargetTime) )
			continue;

		//	insert line breaks if we have previous entries
		if ( StartTime.IsValid() )
			Output << '\n';

		Soy::ArrayToString( GetArrayBridge(Block.mData), Output );
		if ( !StartTime.IsValid() )
			StartTime = pBlock->GetStartTime();
		if ( !EndTime.IsValid() )
			EndTime = pBlock->GetEndTime();

		StartTime = std::min( StartTime, pBlock->GetStartTime() );
		EndTime = std::max( EndTime, pBlock->GetEndTime() );

		Soy::Assert( StartTime.IsValid(), "Expected output time to be valid");
	}
}

void TTextBufferManager::ReleaseFrames()
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );
	mBlocks.Clear();
}


void TTextBufferManager::ReleaseFramesAfter(SoyTime FlushTime)
{
	std::lock_guard<std::mutex> Lock( mBlocksLock );

	//	find last index to keep
	for ( ssize_t i=mBlocks.GetSize()-1;	i>=0;	i-- )
	{
		auto& Block = mBlocks[i];
		if ( Block->GetStartTime() > FlushTime )
		{
			mBlocks.RemoveBlock( i, 1 );
			continue;
		}

		break;
	}
}

void TTextBufferManager::GetMeta(const std::string& Prefix,TJsonWriter& Json)
{
	Json.Push( Prefix + "Type", "Text" );
	TMediaBufferManager::GetMeta( Prefix, Json );
	
	BufferArray<uint64,10> NextTimecodes;
	{
		std::lock_guard<std::mutex> Lock( mBlocksLock );
		for ( int i=0;	i<mBlocks.GetSize() && !NextTimecodes.IsFull();	i++ )
		{
			auto& Block = mBlocks[i];
			if ( !Block )
				continue;
			NextTimecodes.PushBack( Block->GetStartTime().mTime );
		}
	}
	
	Json.Push( (Prefix + "NextFrameTime").c_str(), GetArrayBridge(NextTimecodes) );
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
	
	switch ( Format )
	{
		case SoyMediaFormat::PcmAndroidRaw:
		case SoyMediaFormat::PcmLinear_8:
		case SoyMediaFormat::PcmLinear_16:
		case SoyMediaFormat::PcmLinear_20:
		case SoyMediaFormat::PcmLinear_24:
		case SoyMediaFormat::PcmLinear_float:
			return true;
			
		default:
			break;
	}
	
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

		if ( !Output.PrePushBuffer( Frame.mTimestamp ) )
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
		
		if ( !Output.PrePushBuffer( Frame.mTimestamp ) )
			return true;

		SoyPixelsRemote Pixels( GetArrayBridge(Packet.mData), Packet.mMeta.mPixelMeta );

		Frame.mPixels.reset( new TDumbPixelBuffer( Pixels, Packet.mMeta.GetTransform() ) );
		Output.PushPixelBuffer( Frame, Block );
		return true;
	}

	return false;
}

//	gr: copied from AndroidMovieDecoder.cpp
//	note: this currently casts away const-ness (because of GetRemoteArray)
template<typename NEWTYPE,typename OLDTYPE>
inline FixedRemoteArray<NEWTYPE> CastArray(const ArrayBridge<OLDTYPE>&& Array)
{
	auto OldDataSize = Array.GetDataSize();
	auto OldElementSize = Array.GetElementSize();
	auto NewElementSize = sizeof(NEWTYPE);
	
	auto NewElementCount = OldDataSize / NewElementSize;
	auto* NewData = reinterpret_cast<const NEWTYPE*>( Array.GetArray() );
	return GetRemoteArray( NewData, NewElementCount );
}



bool TMediaPassThroughDecoder::ProcessAudioPacket(const TMediaPacket& Packet)
{
	auto& Meta = Packet.mMeta;
	auto Timestamp = Packet.mTimecode;

	auto& Output = GetAudioBufferManager();
	Output.CorrectDecodedFrameTimestamp( Timestamp );
	Output.mOnFrameDecoded.OnTriggered( Timestamp );

	TAudioBufferBlock AudioBlock;
	AudioBlock.mChannels = Meta.mChannelCount;
	AudioBlock.mFrequency = Meta.mAudioSampleRate;
	AudioBlock.mStartTime = Timestamp;

	auto& BufferArray = Packet.mData;

	auto Format = Meta.mCodec;
	
	//	convert to float audio
	if ( Format == SoyMediaFormat::PcmLinear_16  )
	{
		auto Data16 = CastArray<sint16>( GetArrayBridge(BufferArray) );
		Wave::ConvertSamples( GetArrayBridge(Data16), GetArrayBridge(AudioBlock.mData) );
		Output.mOnFrameDecoded.OnTriggered( Timestamp );
		Output.PushAudioBuffer( AudioBlock );
		return true;
	}
	else if ( Format == SoyMediaFormat::PcmLinear_8 )
	{
		auto Data8 = CastArray<sint8>( GetArrayBridge(BufferArray) );
		Wave::ConvertSamples( GetArrayBridge(Data8), GetArrayBridge(AudioBlock.mData) );
		Output.mOnFrameDecoded.OnTriggered( Timestamp );
		Output.PushAudioBuffer( AudioBlock );
		return true;
	}
	else if ( Format == SoyMediaFormat::PcmLinear_float )
	{
		auto Dataf = CastArray<float>( GetArrayBridge(BufferArray) );
		Wave::ConvertSamples( GetArrayBridge(Dataf), GetArrayBridge(AudioBlock.mData) );
		Output.mOnFrameDecoded.OnTriggered( Timestamp );
		Output.PushAudioBuffer( AudioBlock );
		return true;
	}
	else
	{
		Output.mOnFrameDecodeFailed.OnTriggered( Timestamp );
		std::stringstream Error;
		Error << __func__ << " cannot handle " << Meta.mCodec;
		throw Soy::AssertException( Error.str() );
	}
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

TDumbPixelBuffer::TDumbPixelBuffer(const SoyPixelsImpl& Pixels,const float3x3& Transform) :
	mTransform	( Transform )
{
	mPixels.Copy( Pixels );
}


void TPixelBufferManager::GetMeta(const std::string& Prefix,TJsonWriter& Json)
{
	Json.Push( Prefix + "Type", "Pixel" );
	TMediaBufferManager::GetMeta( Prefix, Json );
	
	
	BufferArray<uint64,10> NextTimecodes;
	{
		std::lock_guard<std::mutex> Lock( mFrameLock );
		for ( int i=0;	i<mFrames.size() && !NextTimecodes.IsFull();	i++ )
		{
			auto& Frame = mFrames[i];
			NextTimecodes.PushBack( Frame.mTimestamp.mTime );
		}
	}
	
	Json.Push( (Prefix + "NextFrameTime").c_str(), GetArrayBridge(NextTimecodes) );
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
	CorrectRequestedFrameTimestamp(RequestedTimestamp);

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
		
		if ( mParams.mPopNearestFrame && LastPixelBuffer )
		{
			//	if this frame is further away than the currently found one, abort
			auto OldDiff = Timestamp.GetDiff( RequestedTimestamp );
			auto NewDiff = Frame.mTimestamp.GetDiff( RequestedTimestamp );
			if ( labs(NewDiff) > labs(OldDiff) )
				break;
		}
		else
		{
			//	first frame in the buffer is ahead. don't pop it otherwise we'll just eat through the buffer				
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

bool TPixelBufferManager::PrePushBuffer(SoyTime Timestamp)
{
	if ( !TMediaBufferManager::PrePushBuffer( Timestamp ) )
		return false;

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


void TPixelBufferManager::ReleaseFramesAfter(SoyTime FlushTime)
{
	//	free up frames
	mFrameLock.lock();

	for ( auto it=mFrames.begin();	it!=mFrames.end();	)
	{
		auto& Frame = *it;

		/*
		if ( Frame.mTimestamp < FlushTime )	//	gr: should we ditch super-old frames too?
		{
			it++;
			continue;
		}
		*/
		Frame.mPixels.reset();
		it = mFrames.erase(it);
	}
	mFrameLock.unlock();
}




TMediaPassThroughEncoder::TMediaPassThroughEncoder(std::shared_ptr<TMediaPacketBuffer>& OutputBuffer,size_t StreamIndex) :
	TMediaEncoder	( OutputBuffer ),
	mOutputMeta		( 256, 256, SoyPixelsFormat::RGBA ),
	mStreamIndex	( StreamIndex )
{
}


void TMediaPassThroughEncoder::Write(const Opengl::TTexture& Image,SoyTime Timecode,Opengl::TContext& Context)
{
	//	todo: proper opengl -> CvPixelBuffer
	
	//	gr: Avf won't accept RGBA, but it will take RGB so we can force reading that format here
	if ( Image.GetFormat() != SoyPixelsFormat::RGBA )
	{
		throw Soy::AssertException("opengl mode unsupported");
	}
	
	//	gr: cannot currently block for an opengl task, this is called from the main thread on mono, even though the render thread
	//		is different, it seems to block and our opengl callback isn't called... so, like the higher level code, no block
	auto ReadPixels = [this,Image,Timecode]
	{
		std::shared_ptr<SoyPixels> Pixels( new SoyPixels );
		Image.Read( *Pixels, SoyPixelsFormat::RGB );
		Write( Pixels, Timecode );
	};
	Context.PushJob( ReadPixels );
}


void TMediaPassThroughEncoder::Write(const Directx::TTexture& Image,SoyTime Timecode,Directx::TContext& Context)
{
#if defined(TARGET_WINDOWS)
	
	if ( Image.GetMode() != Directx::TTextureMode::ReadOnly )
	{
		throw Soy::AssertException("TMediaPassThroughEncoder::Write cannot read texture to cpu");
	}

	//	gr: cannot currently block for an opengl task, this is called from the main thread on mono, even though the render thread
	//		is different, it seems to block and our opengl callback isn't called... so, like the higher level code, no block
	auto* ContextCopy = &Context;
	auto ReadPixels = [this,Image,Timecode,ContextCopy]
	{
		std::shared_ptr<SoyPixels> Pixels( new SoyPixels );
		auto& ImageMutable = const_cast<Directx::TTexture&>(Image);
		ImageMutable.Read( *Pixels, *ContextCopy );
		Write( Pixels, Timecode );
	};

	Context.PushJob( ReadPixels );
#else
	throw Soy::AssertException( std::string(__func__) + " not supported");
#endif
}


void TMediaPassThroughEncoder::Write(std::shared_ptr<SoyPixelsImpl> pImage,SoyTime Timecode)
{
	Soy::Assert( pImage!=nullptr, "Image expected");
	auto& Image = *pImage;
	
	std::shared_ptr<TMediaPacket> pPacket( new TMediaPacket() );
	auto& Packet = *pPacket;
	
	auto& PixelsArray = Image.GetPixelsArray();
	Packet.mData.Copy( PixelsArray );
	Packet.mMeta.mCodec = SoyMediaFormat::FromPixelFormat( Image.GetFormat() );
	Packet.mMeta.mPixelMeta = Image.GetMeta();
	Packet.mMeta.mStreamIndex = mStreamIndex;
	
	//	stop timecode correction by setting decode timecode (must be valid)
	if ( !Timecode.IsValid() )
		Timecode = SoyTime(1ull);
	Packet.mTimecode = Timecode;
	Packet.mDecodeTimecode = Timecode;
	
	auto Block = []()
	{
		return false;
	};

	PushFrame( pPacket, Block );
}


void TMediaPassThroughEncoder::OnError(const std::string& Error)
{
	mFatalError << Error;
}



TTextureBuffer::TTextureBuffer(std::shared_ptr<Opengl::TContext> Context) :
	mOpenglContext	( Context )
{	
}

TTextureBuffer::TTextureBuffer(std::shared_ptr<Directx::TContext> Context) :
	mDirectxContext	( Context )
{
}

TTextureBuffer::TTextureBuffer(std::shared_ptr<SoyPixelsImpl> Pixels) :
	mPixels	( Pixels )
{
}

TTextureBuffer::TTextureBuffer(std::shared_ptr<Directx::TTexture> Texture,std::shared_ptr<TPool<Directx::TTexture>> TexturePool) :
	mDirectxTexture		( Texture ),
	mDirectxTexturePool	( TexturePool )
{
}


TTextureBuffer::~TTextureBuffer()
{
	if ( mOpenglTexture && mOpenglTexturePool )
	{
		try
		{
			mOpenglTexturePool->Release( mOpenglTexture );
		}
		catch(...)
		{
		}
	}

	if ( mOpenglContext )
	{
		Opengl::DeferDelete( mOpenglContext, mOpenglTexture );
		mOpenglContext.reset();
	}

#if defined(TARGET_WINDOWS)
	if ( mDirectxTexture && mDirectxTexturePool )
	{
		try
		{
			mDirectxTexturePool->Release( mDirectxTexture );
		}
		catch(...)
		{
		}
	}

	if ( mDirectxContext )
	{
		Opengl::DeferDelete( mDirectxContext, mDirectxTexture );
		mDirectxContext.reset();
	}
#endif
}



TAudioSplitChannelDecoder::TAudioSplitChannelDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TAudioBufferManager> OutputBuffer,std::shared_ptr<TAudioBufferManager>& RealOutput,std::shared_ptr<TMediaDecoder>& RealDecoder,size_t RealChannel) :
	TMediaPassThroughDecoder	( ThreadName, InputBuffer, OutputBuffer ),
	mRealOutputAudioBuffer		( RealOutput )
{
	//	setup a listener for the real output, modify and push to our "input"
	Soy::Assert( mRealOutputAudioBuffer != nullptr, "Real output expected");
	Soy::Assert( mAudioOutput != nullptr, "audio output buffer expected");
	

	auto OnRealBlock = [this](TAudioBufferBlock& Block)
	{
		//	extract the channel we want
		TAudioBufferBlock ChannelBlock;
		ChannelBlock.Append( Block );

	#pragma message("Need to set which channel to clip here")
		ChannelBlock.SetChannels(1);
		mAudioOutput->PushAudioBuffer( ChannelBlock );
	};

	mRealOutputAudioBuffer->mOnAudioBlockPushed.AddListener( OnRealBlock );
}

