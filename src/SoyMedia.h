#pragma once

#include "SoyPixels.h"
#include "SoyThread.h"
#include "SoyMediaFormat.h"

class TStreamWriter;
class TStreamBuffer;
class TMediaPacket;
class TJsonWriter;

namespace Soy
{
	class TYuvParams;
}

//	implement this per-platform
//	currently all this is used in PopMovieTexture/PopCast, but is forward declared
namespace Platform
{
	class TMediaFormat;
}

namespace Opengl
{
	class TContext;
	class TTexture;
}

namespace Directx
{
	class TContext;
	class TTexture;
}

namespace SoyMedia
{
	extern prmem::Heap	DefaultHeap;
}


class Soy::TYuvParams
{
private:
	TYuvParams(float LumaMin,float LumaMax,float ChromaVRed,float ChromaUGreen,float ChromaVGreen,float ChromaUBlue) :
	mLumaMin		( LumaMin ),
	mLumaMax		( LumaMax ),
	mChromaVRed		( ChromaVRed ),
	mChromaUGreen	( ChromaUGreen ),
	mChromaVGreen	( ChromaVGreen ),
	mChromaUBlue	( ChromaUBlue )
	{
	}
public:
	static TYuvParams	Video()
	{
		float LumaMin = 16.0/255.0;
		float LumaMax = 253.0/255.0;
		float ChromaVRed = 1.5958;
		float ChromaUGreen = -0.39173;
		float ChromaVGreen = -0.81290;
		float ChromaUBlue = 2.017;
		return TYuvParams( LumaMin, LumaMax, ChromaVRed, ChromaUGreen, ChromaVGreen, ChromaUBlue );
	}
	static TYuvParams	Full()
	{
		float LumaMin = 0;
		float LumaMax = 1;
		float ChromaVRed = 1.4;
		float ChromaUGreen = -0.343;
		float ChromaVGreen = -0.711;
		float ChromaUBlue = 1.765;
		return TYuvParams( LumaMin, LumaMax, ChromaVRed, ChromaUGreen, ChromaVGreen, ChromaUBlue );
	}
	
	float	mLumaMin;
	float	mLumaMax;
	float	mChromaVRed;
	float	mChromaUGreen;
	float	mChromaVGreen;
	float	mChromaUBlue;
};



//	now this is in soy, give it a better name!
class TStreamMeta
{
public:
	TStreamMeta() :
	mCodec				( SoyMediaFormat::Invalid ),
	mMediaTypeIndex		( 0 ),
	mPixelRowSize		( 0 ),
	mStreamIndex		( 0 ),
	mCompressed			( false ),
	mFramesPerSecond	( 0 ),
	mChannelCount		( 0 ),
	mInterlaced			( false ),
	mVideoClockWiseRotationDegrees	( 0 ),
	m3DVideo			( false ),
	mDrmProtected		( false ),
	mMaxKeyframeSpacing	( 0 ),
	mAverageBitRate		( 0 ),
	mEncodingBitRate	( 0 ),
	mAudioSampleRate	( 0 ),
	mAudioBytesPerPacket	( 0 ),
	mAudioBytesPerFrame		( 0 ),
	mAudioFramesPerPacket	( 0 ),
	mAudioSampleCount		( 0 ),
	mDecodesOutOfOrder		( false ),
	mAudioBitsPerChannel	( 0 ),
	mAudioSamplesIndependent	( true )
	{
	};
	
	void				SetMime(const std::string& Mime)	{	mCodec = SoyMediaFormat::FromMime( Mime );	}
	std::string			GetMime() const						{	return SoyMediaFormat::ToMime( mCodec );	}
	void				SetPixelMeta(const SoyPixelsMeta& Meta);
	
	float3x3			GetTransform() const
	{
		return mTransform;
	}
	
public:
	SoyMediaFormat::Type	mCodec;
	
	//	specific to h264... make this more generic
	BufferArray<uint8,200>	mSps;			//	no size/nalu header!
	BufferArray<uint8,200>	mPps;			//	no size/nalu header!
	
	std::string			mDescription;		//	other meta that doesnt fit here (eg. unsupported type)
	bool				mCompressed;
	float				mFramesPerSecond;	//	0 when not known. in audio this is samples per second (hz)
	SoyTime				mDuration;
	size_t				mEncodingBitRate;
	bool				mDecodesOutOfOrder;
	
	//	windows media foundation
	size_t				mStreamIndex;		//	windows MediaFoundation can have multiple metas for a single stream (MediaType index splits this), otherwise this would be EXTERNAL from the meta
	size_t				mMediaTypeIndex;	//	applies to Windows MediaFoundation streams

	//	we can't get the stride from an IMFMediaBuffer (the non-specialised type, not 2D) but we can pre-empty it when we get the stream info, so caching it here for now. 
	//	detectable for webcams. note; at meta time we don't always know the format, but we know the pitch/row stride, so can't store padding value and have to work it out later
	//	0=unknown
	size_t				mPixelRowSize;	

	//	video
	SoyPixelsMeta		mPixelMeta;			//	could be invalid format (if unknown, so just w/h) or because its audio etc
	bool				mInterlaced;
	float				mVideoClockWiseRotationDegrees;	//	todo: change for a 3x3 matrix
	bool				m3DVideo;
	bool				mDrmProtected;
	size_t				mMaxKeyframeSpacing;	//	gr: not sure of entropy yet
	size_t				mAverageBitRate;		//	gr: not sure of entropy yet
	float3x3			mTransform;
	
	//	audio
	size_t				mChannelCount;			//	for audio. Maybe expand to planes? but mPixelMeta tell us this
	size_t				mAudioSampleRate;		//	todo: standardise this to khz?
	size_t				mAudioBytesPerPacket;
	size_t				mAudioBytesPerFrame;
	size_t				mAudioFramesPerPacket;
	size_t				mAudioBitsPerChannel;	//	change this to be like H264 different formats; AAC_8, AAC_16, AAC_float etc
	bool				mAudioSamplesIndependent;

	//	this is more meta for the data... not the stream... should it be here? should it be split?
	size_t				mAudioSampleCount;
};
std::ostream& operator<<(std::ostream& out,const TStreamMeta& in);



//	merge this with TMediaPacket so the content can be abstract like TPixelBuffer
//	but cover more than just pixels, like TMediaPacket
class TPixelBuffer
{
public:
	//	different paths return arrays now - shader/fbo blit is pretty generic now so move it out of pixel buffer
	//	generic array, handle that internally (each implementation tends to have it's own lock info anyway)
	//	for future devices (metal, dx), expand these
	//virtual void		Lock(ArrayBridge<Metal::TTexture>&& Textures)=0;
	//virtual void		Lock(ArrayBridge<Cuda::TTexture>&& Textures)=0;
	//virtual void		Lock(ArrayBridge<Opencl::TTexture>&& Textures)=0;
	virtual void		Lock(ArrayBridge<Opengl::TTexture>&& Textures,Opengl::TContext& Context,float3x3& Transform)=0;
	virtual void		Lock(ArrayBridge<Directx::TTexture>&& Textures,Directx::TContext& Context,float3x3& Transform)=0;
	virtual void		Lock(ArrayBridge<SoyPixelsImpl*>&& Textures,float3x3& Transform)=0;
	virtual void		Unlock()=0;
};


class TPixelBufferFrame
{
public:
	std::shared_ptr<TPixelBuffer>	mPixels;
	SoyTime							mTimestamp;
};



class TPixelBufferParams
{
public:
	TPixelBufferParams() :
		mPopFrameSync			( true ),
		mAllowPushRejection		( true ),
		mDebugFrameSkipping		( false ),
		mPushBlockSleepMs		( 3 ),
		mMaxBufferSize			( 10 ),
		mMinBufferSize			( 5 ),			//	specific per-codec for OOO packets, not applicable to a lot of other things (audio may want it, text etc)
		mPopNearestFrame		( false )
	{
	}
	
	size_t		mPushBlockSleepMs;
	bool		mDebugFrameSkipping;
	size_t		mMinBufferSize;				//	require X frames to be buffered before letting any be popped, this is to cope with OOO decoding. This may need to go up with different codecs (KBBBBI vs KBI)
	size_t		mMaxBufferSize;				//	restrict mem/platform buffer usage (platform buffers should probably be managed explicitly if there are limits)
	SoyTime		mPreSeek;					//	gr: put this somewhere else!
	bool		mPopFrameSync;				//	false to pop ASAP (out of sync)
	bool		mPopNearestFrame;			//	instead of skipping to the latest frame (<= now), we find the nearest frame (98 will pop frame 99) and allow looking ahead
	bool		mAllowPushRejection;		//	early frame rejection
};



class TDumbPixelBuffer : public TPixelBuffer
{
public:
	TDumbPixelBuffer(SoyPixelsMeta Meta);
	TDumbPixelBuffer(const SoyPixelsImpl& Pixels,const float3x3& Transform);
	
	virtual void		Lock(ArrayBridge<Opengl::TTexture>&& Textures,Opengl::TContext& Context,float3x3& Transform) override	{}
	virtual void		Lock(ArrayBridge<Directx::TTexture>&& Textures,Directx::TContext& Context,float3x3& Transform) override	{}
	virtual void		Lock(ArrayBridge<SoyPixelsImpl*>&& Textures,float3x3& Transform) override
	{
		Transform = mTransform;
		Textures.PushBack( &mPixels );
	}

	virtual void	Unlock() override
	{
	}

public:
	SoyPixels		mPixels;
	float3x3		mTransform;
};



//	gr: I want to merge these pixel & audio types into the TPacketMediaBuffer type
class TMediaBufferManager
{
public:
	TMediaBufferManager(const TPixelBufferParams& Params) :
		mParams	( Params ),
		mHasEof	( false )
	{
		
	}
	
	virtual void				GetMeta(const std::string& Prefix,TJsonWriter& Json);
	virtual void				SetPlayerTime(const SoyTime& Time);			//	maybe turn this into a func to PULL the time rather than maintain it in here...
	virtual void				CorrectDecodedFrameTimestamp(SoyTime& Timestamp);	//	adjust timestamp if neccessary
	virtual void				CorrectRequestedFrameTimestamp(SoyTime& Timestamp);
	virtual void				ReleaseFrames()=0;
	void						FlushFrames(SoyTime FlushTime);
	virtual bool				PrePushBuffer(SoyTime Timestamp);

	void						SetMinBufferSize(size_t MinBufferSize)		{	mParams.mMinBufferSize = std::max( mParams.mMinBufferSize, MinBufferSize );	}
	
protected:
	virtual void				ReleaseFramesAfter(SoyTime FlushTime)=0;
	void						OnPushEof();
	bool						HasAllFrames() const	{	return mHasEof;	}
	size_t						GetMinBufferSize() const;

public:
	SoyEvent<const SoyTime>				mOnFramePushed;	//	decoded and pushed into buffer
	SoyEvent<const SoyTime>				mOnFramePushSkipped;
	SoyEvent<const SoyTime>				mOnFramePushFailed;		//	triggered if the buffer is full and we didn't block when pushing
	SoyEvent<const ArrayBridge<SoyTime>>	mOnFramePopSkipped;
	SoyEvent<const SoyTime>				mOnFrameDecodeSubmission;
	SoyEvent<const SoyTime>				mOnFrameDecoded;
	SoyEvent<const SoyTime>				mOnFrameExtracted;			//	extracted, but not decoded yet
	SoyEvent<const SoyTime>				mOnFrameDecodeFailed;

protected:
	TPixelBufferParams				mParams;
	SoyTime							mPlayerTime;
	SoyTime							mFlushFenceTime;		//	if valid, don't allow frames over this, post-seek. Resets when we get a packet under

	SoyTime							mFirstTimestamp;
	SoyTime							mAdjustmentTimestamp;
	SoyTime							mLastTimestamp;		//	to cope with errors with incoming timestamps, we record the last output timestamp to re-adjust against

private:
	bool							mHasEof;
};


//	gr: abstracted so we can share or redirect pixel buffers
class TPixelBufferManager : public TMediaBufferManager
{
public:
	TPixelBufferManager(const TPixelBufferParams& Params) :
		TMediaBufferManager	( Params )
	{
	}
	
	virtual void		GetMeta(const std::string& Prefix,TJsonWriter& Json) override;

	//	gr: most of these will move to base
	SoyTime				GetNextPixelBufferTime(bool Safe=true);
	std::shared_ptr<TPixelBuffer>	PopPixelBuffer(SoyTime& Timestamp);
	bool				PushPixelBuffer(TPixelBufferFrame& PixelBuffer,std::function<bool()> Block);
	virtual bool		PrePushBuffer(SoyTime Timestamp) override;
	bool				PeekPixelBuffer(SoyTime Timestamp);	//	is there a new pixel buffer?
	bool				IsPixelBufferFull() const;

	virtual void		ReleaseFrames() override;
	virtual void		ReleaseFramesAfter(SoyTime FlushTime) override;
	
	
private:
	std::mutex						mFrameLock;
	std::vector<TPixelBufferFrame>	mFrames;
};


//	gr: hacky for now but work it back to a media packet buffer
class TAudioBufferBlock
{
public:
	TAudioBufferBlock() :
		mData		( SoyMedia::DefaultHeap ),
		mChannels	( 0 ),
		mFrequency	( 0 )
	{
	}
	
	bool				IsValid() const		{	return !mData.IsEmpty();	}
	SoyTime				GetStartTime() const			{	return mStartTime;	}
	SoyTime				GetEndTime() const;
	SoyTime				GetSampleTime(size_t SampleIndex) const;
	ssize_t				GetTimeSampleIndex(SoyTime Time) const;		//	can be out of range of the data
	size_t				RemoveDataUntil(SoyTime Time);				//	returns number of samples removed
	
	void				Append(const TAudioBufferBlock& NewData);
	void				Clip(SoyTime Start, SoyTime End);

	//	reformatting
	void				SetChannels(size_t Channels);

public:
	//	consider using stream meta here
	size_t				mChannels;
	size_t				mFrequency;		//	samples per sec
	
	//	gr: maybe change this into some format where we can access mData[Time]
	SoyTime				mStartTime;
	Array<float>		mData;
};

class TAudioBufferManager : public TMediaBufferManager
{
public:
	TAudioBufferManager(const TPixelBufferParams& Params) :
		mBlocks				( SoyMedia::DefaultHeap ),
		TMediaBufferManager	( Params ),
		mChannelCache		( 0 ),
		mFrequencyCache		( 0 ),
		mBlockFlushStart	( 0 )
	{
	}
	
	virtual void	GetMeta(const std::string& Prefix,TJsonWriter& Json) override;

	void			PushAudioBuffer(const TAudioBufferBlock& AudioData);
	bool			GetAudioBuffer(TAudioBufferBlock& OutputBlock,bool HighPrecisionExtraction,bool VerboseDebug);	//	returns false if NO data, pads with zeros if not all there

	virtual void	SetPlayerTime(const SoyTime& Time) override;	//	clear out old data

	virtual void	ReleaseFrames() override;
	virtual void	ReleaseFramesAfter(SoyTime FlushTime) override;
	void			ReleaseFramesBefore(SoyTime FlushTime,bool ClipOldData);

	//	meta of first block. returns zero if none exist
	size_t			GetChannels() const			{	return mChannelCache;	}
	size_t			GetFrequency() const		{	return mFrequencyCache;	}

private:
	//	gr: cached these to avoid locks & for when we've flushed all the data
	size_t						mFrequencyCache;
	size_t						mChannelCache;

	std::mutex					mBlocksLock;
	Array<TAudioBufferBlock>	mBlocks;
	std::atomic<size_t>			mBlockFlushStart;
};


class TTextBufferManager : public TMediaBufferManager
{
public:
	TTextBufferManager(const TPixelBufferParams& Params) :
		mBlocks				( SoyMedia::DefaultHeap ),
		TMediaBufferManager	( Params )
	{
	}
	
	virtual void	GetMeta(const std::string& Prefix,TJsonWriter& Json) override;

	void			GetBuffer(std::stringstream& Output,SoyTime& StartTime,SoyTime& EndTime);

	void			PushBuffer(std::shared_ptr<TMediaPacket> Buffer);
	SoyTime			PopBuffer(std::stringstream& Output,SoyTime Time,bool SkipOldText);	//	returns end-time of the data extracted (invalid if none popped)
	virtual void	ReleaseFrames() override;
	virtual void	ReleaseFramesAfter(SoyTime FlushTime) override;
	
private:
	std::mutex			mBlocksLock;
	Array<std::shared_ptr<TMediaPacket>>	mBlocks;
};


//	don't overload this? or replace TPixelBuffer (hardware abstraction) with this
class TMediaPacket
{
public:
	TMediaPacket() :
		mData			( SoyMedia::DefaultHeap ),
		mIsKeyFrame		( false ),
		mEncrypted		( false ),
		mEof			( false )
	{
	}

	//	gr: re-instating this, we should enforce decode timecodes in the extractor.
	SoyTime					GetSortingTimecode() const	{	return mDecodeTimecode.IsValid() ? mDecodeTimecode : mTimecode;	}
	SoyTime					GetStartTime() const		{	return mTimecode;	}
	SoyTime					GetEndTime() const			{	return mTimecode + mDuration;	}
	bool					ContainsTime(const SoyTime& Time) const	{	return Time >= GetStartTime() && Time <= GetEndTime();	}
	bool					HasData() const
	{
		if ( mPixelBuffer )
			return true;
		if ( !mData.IsEmpty() )
			return true;
		return false;
	}
	
public:
	bool					mEof;		//	if EOF, data is optional (see HasData())
	SoyTime					mTimecode;	//	presentation time
	SoyTime					mDuration;
	SoyTime					mDecodeTimecode;
	bool					mIsKeyFrame;
	bool					mEncrypted;
	
	std::shared_ptr<Platform::TMediaFormat>	mFormat;
	TStreamMeta				mMeta;			//	format info
	
	Array<uint8>					mData;
	std::shared_ptr<TPixelBuffer>	mPixelBuffer;	//	some extractors go straight to hardware, TPixelBuffer is that encapsulation, so this is in place of data
};
std::ostream& operator<<(std::ostream& out,const TMediaPacket& in);



//	abstracted so later we can handle multiple streams at once as seperate buffers
class TMediaPacketBuffer
{
public:
	TMediaPacketBuffer(size_t MaxBufferSize=10) :
		mPackets				( SoyMedia::DefaultHeap ),
		mMaxBufferSize			( MaxBufferSize ),
		mAutoTimestampDuration	( 33ull )
	{
	}
	~TMediaPacketBuffer();
	
	//	todo: options here so we can get the next packet we need
	//		where we skip over all frames until the prev-to-Time keyframe
	//std::shared_ptr<TMediaPacket>	PopPacket(SoyTime Time);
	std::shared_ptr<TMediaPacket>	PopPacket();
	void							UnPopPacket(std::shared_ptr<TMediaPacket> Packet);		//	gr: force a packet back into the buffer at the start; todo; enforce a decoder timestamp and just use push(but with a forced re-insertion)
	void							PushPacket(std::shared_ptr<TMediaPacket> Packet,std::function<bool()> Block);
	bool							HasPackets() const		{	return !mPackets.IsEmpty();	}
	size_t							GetPacketCount() const	{	return mPackets.GetSize();	}

	void							FlushFrames(SoyTime FlushTime);
	virtual bool					PrePushBuffer(SoyTime Timestamp);

protected:
	virtual void					ReleaseFramesAfter(SoyTime FlushTime);
	void							CorrectIncomingPacketTimecode(TMediaPacket& Packet);

public:
	SoyEvent<std::shared_ptr<TMediaPacket>>	mOnNewPacket;
	
private:
	//	make this a ring buffer of objects
	//	gr: but it also needs to be sorted!
	size_t									mMaxBufferSize;
	Array<std::shared_ptr<TMediaPacket>>	mPackets;
	std::mutex								mPacketsLock;

	SoyTime									mLastPacketTimestamp;	//	for when we have to calculate timecodes ourselves
	SoyTime									mAutoTimestampDuration;
	SoyTime									mFlushFenceTime;		//	if valid, don't allow frames over this, post-seek. Resets when we get a packet under
};



//	rename this and TMediaExtractor to match (muxer and demuxer?)
class TMediaMuxer : public SoyWorkerThread
{
public:
	TMediaMuxer(std::shared_ptr<TStreamWriter> Output,std::shared_ptr<TMediaPacketBuffer>& Input,const std::string& ThreadName="TMediaMuxer");
	~TMediaMuxer();

	void					SetStreams(const ArrayBridge<TStreamMeta>&& Streams);
	virtual void			Finish()=0;

	void					GetMeta(TJsonWriter& Json);

protected:
	virtual void			SetupStreams(const ArrayBridge<TStreamMeta>&& Streams)=0;
	virtual void			ProcessPacket(std::shared_ptr<TMediaPacket> Packet,TStreamWriter& Output)=0;

private:
	virtual bool			Iteration() override;
	virtual bool			CanSleep() override;

	bool					IsStreamsReady(std::shared_ptr<TMediaPacket> Packet);	//	if this returns false, not ready to ProcessPacket yet
	
public:
	std::shared_ptr<TStreamWriter>			mOutput;
	std::shared_ptr<TMediaPacketBuffer>		mInput;
	Array<TStreamMeta>						mStreams;		//	streams, once setup, fixed
	
	Array<std::shared_ptr<TMediaPacket>>	mDefferedPackets;	//	when auto-calculating streams we buffer up some packets

private:
	SoyListenerId							mOnPacketListener;
};


class TMediaExtractorParams
{
public:
	//	copy but override the filename (common use)
	TMediaExtractorParams(const std::string& Filename,const TMediaExtractorParams& OtherParams) :
		TMediaExtractorParams	( OtherParams )
	{
		mFilename = Filename;
	}
	TMediaExtractorParams(const std::string& Filename,const std::string& ThreadName,std::function<void(const SoyTime,size_t)> OnFrameExtracted,SoyTime ReadAheadMs,bool DiscardOldFrames,bool ForceNonPlanarOutput,bool ExtractAudioStreams,bool ApplyHeightPadding) :
		mFilename						( Filename ),
		mOnFrameExtracted				( OnFrameExtracted ),
		mReadAheadMs					( ReadAheadMs ),
		mDiscardOldFrames				( DiscardOldFrames ),
		mForceNonPlanarOutput			( ForceNonPlanarOutput ),
		mDebugIntraFrameRect			( false ),
		mDebugIntraFrameTransparency	( false ),
		mExtractAudioStreams			( ExtractAudioStreams ),
		mOnlyExtractKeyframes			( false ),
		mResetInternalTimestamp			( false ),
		mAudioSampleRate				( 0 ),
		mApplyHeightPadding				( ApplyHeightPadding ),
		mWindowIncludeBorders			( true ),
		mLiveUseClockTime				( false )
	{
	}
	
public:
	std::string					mFilename;
	std::string					mThreadName;
	std::function<void(const SoyTime,size_t)>	mOnFrameExtracted;
	SoyTime						mReadAheadMs;

	SoyTime						mInitialTime;
	size_t						mAudioSampleRate;
	bool						mExtractAudioStreams;
	bool						mOnlyExtractKeyframes;
	bool						mResetInternalTimestamp;
	bool						mApplyHeightPadding;		//	for windows where we need height padding sometimes, can turn off with this
	bool						mWindowIncludeBorders;

	//	some extractors have some decoder-themed params
	bool						mDiscardOldFrames;
	bool						mForceNonPlanarOutput;		//	for some extractors which have pixelly settings
	
	//	for gifs
	bool						mDebugIntraFrameRect;
	bool						mDebugIntraFrameTransparency;

	//	for streams (webcams etc) use Real time (clock) rather than SeekTime
	//	real time works, but when app is paused, threads continue but player time doesnt and it falls behind
	bool						mLiveUseClockTime;		
};



//	demuxer
class TMediaExtractor : public SoyWorkerThread
{
public:
	TMediaExtractor(const TMediaExtractorParams& Params);
	~TMediaExtractor();
	
	void							Seek(SoyTime Time,const std::function<void(SoyTime)>& FlushFrames);				//	keep calling this, controls the packet read-ahead
	virtual void					FlushFrames(SoyTime FlushTime);
	
	virtual void					GetMeta(TJsonWriter& Json);

	virtual void					GetStreams(ArrayBridge<TStreamMeta>&& Streams)=0;
	TStreamMeta						GetStream(size_t Index);
	TStreamMeta						GetVideoStream(size_t Index);
	virtual std::shared_ptr<Platform::TMediaFormat>	GetStreamFormat(size_t StreamIndex)=0;
	bool							HasFatalError(std::string& Error)
	{
		Error += mFatalError;
		return !Error.empty();
	}
	
	std::shared_ptr<TMediaPacketBuffer>	AllocStreamBuffer(size_t StreamIndex);
	std::shared_ptr<TMediaPacketBuffer>	GetStreamBuffer(size_t StreamIndex);

public:
//protected:	//	gr: only for subclasses, but the playlist media extractor needs to call this on it's children
	virtual std::shared_ptr<TMediaPacket>	ReadNextPacket()=0;
	bool							CanPushPacket(SoyTime Time,size_t StreamIndex,bool IsKeyframe);

	void							OnError(const std::string& Error);

protected:
	//	call this when there's a packet ready for ReadNextPacket
	void							OnPacketExtracted(SoyTime& Timecode,size_t StreamIndex);
	void							OnPacketExtracted(std::shared_ptr<TMediaPacket>& Packet);
	void							OnSkippedExtractedPacket(const SoyTime& Timecode);
	SoyTime							GetExtractorRealTimecode(SoyTime,ssize_t StreamIndex=-1);

	void							OnClearError();
	void							OnStreamsChanged(const ArrayBridge<TStreamMeta>&& Streams);
	void							OnStreamsChanged();
	
	//virtual void					ResetTo(SoyTime Time);			//	for when we seek backwards, assume a stream needs resetting
	void							ReadPacketsUntil(SoyTime Time,std::function<bool()> While);
	SoyTime							GetSeekTime() const			{	return mSeekTime;	}
	SoyTime							GetExtractTime() const		{	return mSeekTime + mExtractAheadMs;	}
	
	virtual bool					OnSeek()					{	return false;	}	//	reposition extractors whereever possible. return true to invoke a data flush (ie. if you moved the extractor)
	virtual bool					CanSeekBackwards()			{	return false;	}	//	by default, don't allow this, until it's implemented for that extractor

private:
	virtual bool					Iteration() override;

public:
	SoyEvent<const ArrayBridge<TStreamMeta>>		mOnStreamsChanged;
	SoyTime											mExtractAheadMs;

	TMediaExtractorParams			mParams;
	
protected:
	std::map<size_t,std::shared_ptr<TMediaPacketBuffer>>	mStreamBuffers;
	std::function<void(const SoyTime,size_t)>				mOnPacketExtracted;
	
private:
	std::string						mFatalError;
	SoyTime							mSeekTime;				//	current player time, which we actually want to seek to
	std::map<size_t,SoyTime>		mStreamFirstFrameTime;	//	time correction per-frame
};


//	transcoder
//	todo: change output buffer to a packet buffer - esp when it becomes audio
//	gr: this is dumb atm, it processes packets on the thread ASAP.
//		output decides what's discarded, input decides what's skipped.
//		maybe it should be more intelligent as this should be the SLOWEST part of the chain?...
//	gr: renamed to decoder as it decodes to pixels
class TMediaDecoder : public SoyWorkerThread
{
public:
	TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TPixelBufferManager> OutputBuffer);
	TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TAudioBufferManager> OutputBuffer);
	TMediaDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TTextBufferManager> OutputBuffer);
	virtual ~TMediaDecoder();
	
	bool							HasFatalError(std::string& Error)
	{
		Error += mFatalError.str();
		return !Error.empty();
	}
	
	size_t							GetMinBufferSize() const				{	return IsDecodingFramesInOrder() ? 0 : 5;	}
	
protected:
	virtual bool					IsDecodingFramesInOrder() const			{	return true;	}
	
	void							OnDecodeFrameSubmitted(const SoyTime& Time);
	virtual bool					ProcessPacket(std::shared_ptr<TMediaPacket>& Packet);			//	return false to return the frame to the buffer and not discard
	virtual bool					ProcessPacket(const TMediaPacket& Packet)=0;		//	return false to return the frame to the buffer and not discard
	
	//	gr: added this for android as I'm not sure about the thread safety, but other platforms generally have a callback
	//		if we can do input&output on different threads, remove this
	virtual void					ProcessOutputPacket(TPixelBufferManager& FrameBuffer)
	{
	}
	virtual void					ProcessOutputPacket(TAudioBufferManager& FrameBuffer)
	{
	}
	virtual void					ProcessOutputPacket(TTextBufferManager& FrameBuffer)
	{
	}
	
	TPixelBufferManager&			GetPixelBufferManager();
	TAudioBufferManager&			GetAudioBufferManager();
	TTextBufferManager&				GetTextBufferManager();
	
private:
	virtual bool					Iteration() final;
	virtual bool					CanSleep() override;
	
protected:
	std::shared_ptr<TMediaPacketBuffer>		mInput;
	std::shared_ptr<TPixelBufferManager>	mPixelOutput;
	std::shared_ptr<TAudioBufferManager>	mAudioOutput;
	std::shared_ptr<TTextBufferManager>		mTextOutput;
	std::stringstream						mFatalError;
	
private:
	SoyListenerId							mOnNewPacketListener;
};


//	encode to binary - merge this with TMediaDecoder to arbitraily go from any format to another
class TMediaEncoder
{
public:
	TMediaEncoder(std::shared_ptr<TMediaPacketBuffer>& OutputBuffer) :
		mOutput	( OutputBuffer )
	{
	}
	
	bool							HasFatalError(std::string& Error)
	{
		Error = mFatalError.str();
		return !Error.empty();
	}
	
	virtual void					Write(const Opengl::TTexture& Image,SoyTime Timecode,Opengl::TContext& Context)=0;
	virtual void					Write(const Directx::TTexture& Image,SoyTime Timecode,Directx::TContext& Context)=0;
	virtual void					Write(std::shared_ptr<SoyPixelsImpl> Image,SoyTime Timecode)=0;
	virtual void					GetMeta(TJsonWriter& Json);

	size_t							GetPendingOutputCount() const	{	return mOutput ? mOutput->GetPacketCount() : 0;	}
	virtual size_t					GetPendingEncodeCount() const	{	return 0;	}

protected:
	void							PushFrame(std::shared_ptr<TMediaPacket>& Packet,std::function<bool(void)> Block);

public:
	SoyEvent<SoyTime>					mOnFramePushSkipped;

protected:
	std::shared_ptr<TMediaPacketBuffer>	mOutput;
	std::stringstream					mFatalError;
};



//	"decoder" that passes TPixelBuffers through, or turns raw data into pixel buffers (cannot work on compressed data)
class TMediaPassThroughDecoder : public TMediaDecoder
{
public:
	TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TPixelBufferManager> OutputBuffer);
	TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TAudioBufferManager> OutputBuffer);
	TMediaPassThroughDecoder(const std::string& ThreadName,std::shared_ptr<TMediaPacketBuffer>& InputBuffer,std::shared_ptr<TTextBufferManager> OutputBuffer);
	
	static bool						HandlesCodec(SoyMediaFormat::Type Format);
	
protected:
	virtual bool					ProcessPacket(std::shared_ptr<TMediaPacket>& Packet) override;
	virtual bool					ProcessPacket(const TMediaPacket& Packet) override;
	
	//	return true if handled
	bool							ProcessTextPacket(std::shared_ptr<TMediaPacket>& Packet);
	bool							ProcessAudioPacket(const TMediaPacket& Packet);
	bool							ProcessPixelPacket(const TMediaPacket& Packet);
};


class TMediaPassThroughEncoder : public TMediaEncoder
{
public:
	TMediaPassThroughEncoder(std::shared_ptr<TMediaPacketBuffer>& OutputBuffer,size_t StreamIndex);
	
	virtual void		Write(const Opengl::TTexture& Image,SoyTime Timecode,Opengl::TContext& Context) override;
	virtual void		Write(const Directx::TTexture& Image,SoyTime Timecode,Directx::TContext& Context) override;
	virtual void		Write(std::shared_ptr<SoyPixelsImpl> Image,SoyTime Timecode) override;
	
	void				OnError(const std::string& Error);
	
public:
	SoyPixelsMeta				mOutputMeta;
	TMediaPacketBuffer			mFrames;
	size_t						mStreamIndex;	//	may want to be in base
};


