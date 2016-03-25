#include "SoyMedia.h"
#include "SortArray.h"
#include "SoyJson.h"

//gr: this is for the pass through encoder, maybe to avoid this dependancy I can move the pass throughs to their own files...
#include "SoyOpenGl.h"
#include "SoyOpenGlContext.h"

#if defined(TARGET_WINDOWS)
#include "SoyDirectx.h"
#endif


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

void TMediaExtractor::Seek(SoyTime Time,const std::function<void(SoyTime)>& FlushFrames)
{
	//	update the target seek time
	bool AllowReverse = CanSeekBackwards();
	if ( !AllowReverse && Time < mSeekTime )
	{
		std::stringstream Error;
		Error << "This decoder cannot seek backwards " << Time << " < " << mSeekTime;
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



void TMediaExtractor::OnPacketExtracted(std::shared_ptr<TMediaPacket>& Packet)
{
	Soy::Assert( Packet != nullptr, "Packet expected");

	OnPacketExtracted( Packet->mTimecode, Packet->mMeta.mStreamIndex );
	Wake();
}

void TMediaExtractor::OnPacketExtracted(SoyTime& Timecode,size_t StreamIndex)
{
	//	if this is the first timecode for the stream, set it
	if ( mStreamFirstFrameTime.find( StreamIndex ) == mStreamFirstFrameTime.end() )
	{
		mStreamFirstFrameTime[StreamIndex] = Timecode;
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
	//	todo: proper opengl -> CvPixelBuffer
	
	//	gr: Avf won't accept RGBA, but it will take RGB so we can force reading that format here
	if ( Image.GetMeta().GetFormat() != SoyPixelsFormat::RGBA )
	{
		throw Soy::AssertException("directx mode unsupported");
	}
	
	//	gr: cannot currently block for an opengl task, this is called from the main thread on mono, even though the render thread
	//		is different, it seems to block and our opengl callback isn't called... so, like the higher level code, no block
	auto ReadPixels = [this,Image,Timecode,&Context]
	{
		std::shared_ptr<SoyPixels> Pixels( new SoyPixels );
		auto& ImageMutable = const_cast<Directx::TTexture&>(Image);
		ImageMutable.Read( *Pixels, Context );
		Write( Pixels, Timecode );
	};

	//	gr: this needs to block because of context scope... should opengl one block?
	Soy::TSemaphore Semaphore;
	Context.PushJob( ReadPixels, Semaphore );
	Semaphore.Wait();
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
	Packet.mTimecode = Timecode;
	Packet.mMeta.mCodec = SoyMediaFormat::FromPixelFormat( Image.GetFormat() );
	Packet.mMeta.mPixelMeta = Image.GetMeta();
	Packet.mMeta.mStreamIndex = mStreamIndex;
	
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

