#pragma once

#include "SoyPixels.h"
#include "SoyThread.h"

class TStreamWriter;
class TStreamBuffer;
class TMediaPacket;

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

//	merge this + pixel format at some point
namespace SoyMediaFormat
{
	enum Type
	{
		Invalid = SoyPixelsFormat::Invalid,
		
		Greyscale = SoyPixelsFormat::Greyscale,
		GreyscaleAlpha = SoyPixelsFormat::GreyscaleAlpha,
		RGB = SoyPixelsFormat::RGB,
		RGBA = SoyPixelsFormat::RGBA,
		BGRA = SoyPixelsFormat::BGRA,
		BGR = SoyPixelsFormat::BGR,
		ARGB = SoyPixelsFormat::ARGB,
		KinectDepth = SoyPixelsFormat::KinectDepth,
		FreenectDepth10bit = SoyPixelsFormat::FreenectDepth10bit,
		FreenectDepth11bit = SoyPixelsFormat::FreenectDepth11bit,
		FreenectDepthmm = SoyPixelsFormat::FreenectDepthmm,
		LumaFull = SoyPixelsFormat::LumaFull,
		LumaVideo = SoyPixelsFormat::LumaVideo,
		Yuv_8_88_Full = SoyPixelsFormat::Yuv_8_88_Full,
		Yuv_8_88_Video = SoyPixelsFormat::Yuv_8_88_Video,
		Yuv_8_8_8_Full = SoyPixelsFormat::Yuv_8_8_8_Full,
		Yuv_8_8_8_Video = SoyPixelsFormat::Yuv_8_8_8_Video,
		ChromaUV_8_8 = SoyPixelsFormat::ChromaUV_8_8,
		ChromaUV_88 = SoyPixelsFormat::ChromaUV_88,
		
		NotPixels = SoyPixelsFormat::Count,
		
		//	file:///Users/grahamr/Downloads/513_direct_access_to_media_encoding_and_decoding.pdf
		//	gr: too specific? extended/sub modes?... prefer this really...
		H264_8,			//	AVCC format (length8+payload)
		H264_16,		//	AVCC format (length16+payload)
		H264_32,		//	AVCC format (length32+payload)
		H264_ES,		//	ES format (0001+payload)	elementry/transport/annexb/Nal stream (H264 ES in MF)
		H264_SPS_ES,	//	SPS data, nalu
		H264_PPS_ES,	//	PPS data, nalu
		
		Mpeg2TS,
		Mpeg2,
		Mpeg4,
		
		//	audio
		Wave,
		Aac,
		PcmAndroidRaw,		//	temp until I work out what this actually is
		PcmLinear_8,
		PcmLinear_16,		//	signed, see SoyWave
		PcmLinear_20,
		PcmLinear_24,
		PcmLinear_float,	//	-1..1 see SoyWave
		
		Text,
		Subtitle,
		ClosedCaption,
		Timecode,
		MetaData,
		Muxed,
	};
	
	SoyPixelsFormat::Type	GetPixelFormat(Type MediaFormat);
	Type					FromPixelFormat(SoyPixelsFormat::Type PixelFormat);
	
	bool		IsVideo(Type Format);	//	or pixels
	inline bool	IsPixels(Type Format)	{	return GetPixelFormat( Format ) != SoyPixelsFormat::Invalid;	}
	bool		IsAudio(Type Format);
	bool		IsH264(Type Format);
	Type		FromFourcc(uint32 Fourcc,int H264LengthSize=-1);
	uint32		ToFourcc(Type Format);
	bool		IsH264Fourcc(uint32 Fourcc);
	std::string	ToMime(Type Format);
	Type		FromMime(const std::string& Mime);
	
	DECLARE_SOYENUM(SoyMediaFormat);
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
	mYuvMatrix			( Soy::TYuvParams::Full() ),
	mEncodingBitRate	( 0 ),
	mAudioSampleRate	( 0 ),
	mAudioBytesPerPacket	( 0 ),
	mAudioBytesPerFrame		( 0 ),
	mAudioFramesPerPacket	( 0 ),
	mAudioSampleCount		( 0 )
	{
	};
	
	void				SetMime(const std::string& Mime)	{	mCodec = SoyMediaFormat::FromMime( Mime );	}
	std::string			GetMime() const						{	return SoyMediaFormat::ToMime( mCodec );	}
	
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
	
	//	windows media foundation
	size_t				mStreamIndex;		//	windows MediaFoundation can have multiple metas for a single stream (MediaType index splits this), otherwise this would be EXTERNAL from the meta
	size_t				mMediaTypeIndex;	//	applies to Windows MediaFoundation streams
	
	//	video
	SoyPixelsMeta		mPixelMeta;			//	could be invalid format (if unknown, so just w/h) or because its audio etc
	bool				mInterlaced;
	float				mVideoClockWiseRotationDegrees;	//	todo: change for a 3x3 matrix
	bool				m3DVideo;
	Soy::TYuvParams		mYuvMatrix;
	bool				mDrmProtected;
	size_t				mMaxKeyframeSpacing;	//	gr: not sure of entropy yet
	size_t				mAverageBitRate;		//	gr: not sure of entropy yet
	float3x3			mTransform;
	
	//	audio
	size_t				mChannelCount;			//	for audio. Maybe expand to planes? but mPixelMeta tell us this
	float				mAudioSampleRate;		//	todo: standardise this to khz?
	size_t				mAudioBytesPerPacket;
	size_t				mAudioBytesPerFrame;
	size_t				mAudioFramesPerPacket;
	size_t				mAudioBitsPerChannel;	//	change this to be like H264 different formats; AAC_8, AAC_16, AAC_float etc
	
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
	//	if 1 texture assume RGB/BGR greyscale etc
	//	if multiple, assuming YUV
	//virtual void		Lock(ArrayBridge<Metal::TTexture>&& Textures)=0;
	//virtual void		Lock(ArrayBridge<Cuda::TTexture>&& Textures)=0;
	//virtual void		Lock(ArrayBridge<Opencl::TTexture>&& Textures)=0;
	virtual void		Lock(ArrayBridge<Opengl::TTexture>&& Textures,Opengl::TContext& Context)=0;
	virtual void		Lock(ArrayBridge<Directx::TTexture>&& Textures,Directx::TContext& Context)=0;
	virtual void		Lock(ArrayBridge<SoyPixelsImpl*>&& Textures)=0;
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
	mResetInternalTimestamp	( false ),
	mBufferFrameSize		( 10 ),
	mPopNearestFrame		( false )
	{
	}
	
	size_t		mPushBlockSleepMs;
	bool		mDebugFrameSkipping;
	size_t		mBufferFrameSize;
	bool		mResetInternalTimestamp;
	SoyTime		mPreSeek;					//	gr: put this somewhere else!
	bool		mPopFrameSync;				//	false to pop ASAP (out of sync)
	bool		mPopNearestFrame;			//	instead of skipping to the latest frame (<= now), we find the nearest frame (98 will pop frame 99) and allow looking ahead
	bool		mAllowPushRejection;		//	early frame rejection
};



//	gr: I want to merge these pixel & audio types into the TPacketMediaBuffer type
class TMediaBufferManager
{
public:
	TMediaBufferManager(const TPixelBufferParams& Params) :
		mParams	( Params )
	{
		
	}
	
	virtual void				SetPlayerTime(const SoyTime& Time);			//	maybe turn this into a func to PULL the time rather than maintain it in here...
	virtual void				CorrectDecodedFrameTimestamp(SoyTime& Timestamp);	//	adjust timestamp if neccessary
	virtual void				ReleaseFrames()=0;
	virtual bool				PrePushPixelBuffer(SoyTime Timestamp)=0;

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

	SoyTime							mFirstTimestamp;
	SoyTime							mAdjustmentTimestamp;
	SoyTime							mLastTimestamp;		//	to cope with errors with incoming timestamps, we record the last output timestamp to re-adjust against
};


//	gr: abstracted so we can share or redirect pixel buffers
class TPixelBufferManager : public TMediaBufferManager
{
public:
	TPixelBufferManager(const TPixelBufferParams& Params) :
		TMediaBufferManager	( Params )
	{
	}
	
	//	gr: most of these will move to base
	SoyTime				GetNextPixelBufferTime(bool Safe=true);
	std::shared_ptr<TPixelBuffer>	PopPixelBuffer(SoyTime& Timestamp);
	bool				PushPixelBuffer(TPixelBufferFrame& PixelBuffer,std::function<bool()> Block);
	virtual bool		PrePushPixelBuffer(SoyTime Timestamp) override;
	bool				PeekPixelBuffer(SoyTime Timestamp);	//	is there a new pixel buffer?
	bool				IsPixelBufferFull() const;
	
	virtual void		ReleaseFrames() override;
	
	
private:
	std::mutex						mFrameLock;
	std::vector<TPixelBufferFrame>	mFrames;
	
};


//	gr: hacky for now but work it back to a media packet buffer
class TAudioBufferBlock
{
public:
	//	consider using stream meta here
	size_t				mChannels;
	size_t				mFrequency;
	
	//	gr: maybe change this into some format where we can access mData[Time]
	SoyTime				mStartTime;
	SoyTime				mEndTime;
	Array<float>		mData;
};

class TAudioBufferManager : public TMediaBufferManager
{
public:
	TAudioBufferManager(const TPixelBufferParams& Params) :
		TMediaBufferManager	( Params )
	{
	}

	void			PushAudioBuffer(const TAudioBufferBlock& AudioData);
	void			PopAudioBuffer(ArrayBridge<float>&& Data,size_t Channels,SoyTime StartTime,SoyTime EndTime);
	virtual void	ReleaseFrames() override;
	virtual bool	PrePushPixelBuffer(SoyTime Timestamp)	{	return true;	}	//	no skipping atm

private:
	std::mutex					mBlocksLock;
	Array<TAudioBufferBlock>	mBlocks;
};


class TMediaPacket
{
public:
	TMediaPacket() :
	mIsKeyFrame		( false ),
	mEncrypted		( false ),
	mEof			( false )
	{
	}

	//	gr: re-instating this, we should enforce decode timecodes in the extractor.
	SoyTime					GetSortingTimecode() const	{	return mDecodeTimecode.IsValid() ? mDecodeTimecode : mTimecode;	}
	
public:
	bool					mEof;
	SoyTime					mTimecode;	//	presentation time
	SoyTime					mDuration;
	SoyTime					mDecodeTimecode;
	bool					mIsKeyFrame;
	bool					mEncrypted;
	
	std::shared_ptr<Platform::TMediaFormat>	mFormat;
	TStreamMeta				mMeta;			//	format info
	
	Array<uint8>			mData;
};
std::ostream& operator<<(std::ostream& out,const TMediaPacket& in);



//	abstracted so later we can handle multiple streams at once as seperate buffers
class TMediaPacketBuffer
{
public:
	TMediaPacketBuffer(size_t MaxBufferSize=10) :
		mMaxBufferSize			( MaxBufferSize ),
		mAutoTimestampDuration	( 33ull )
	{
	}
	
	//	todo: options here so we can get the next packet we need
	//		where we skip over all frames until the prev-to-Time keyframe
	//std::shared_ptr<TMediaPacket>	PopPacket(SoyTime Time);
	std::shared_ptr<TMediaPacket>	PopPacket();
	void							UnPopPacket(std::shared_ptr<TMediaPacket> Packet);		//	gr: force a packet back into the buffer at the start; todo; enforce a decoder timestamp and just use push(but with a forced re-insertion)
	void							PushPacket(std::shared_ptr<TMediaPacket> Packet,std::function<bool()> Block);
	bool							HasPackets() const	{	return !mPackets.IsEmpty();	}
	
protected:
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
};



//	rename this and TMediaExtractor to match (muxer and demuxer?)
class TMediaMuxer : public SoyWorkerThread
{
public:
	TMediaMuxer(std::shared_ptr<TStreamWriter>& Output,std::shared_ptr<TMediaPacketBuffer>& Input,const std::string& ThreadName="TMediaMuxer");
	~TMediaMuxer();

	void					SetStreams(const ArrayBridge<TStreamMeta>&& Streams);
	virtual void			Finish()=0;

protected:
	virtual void			SetupStreams(const ArrayBridge<TStreamMeta>&& Streams)=0;
	virtual void			ProcessPacket(std::shared_ptr<TMediaPacket> Packet,TStreamWriter& Output)=0;

private:
	virtual bool			Iteration() override;
	virtual bool			CanSleep() override;

	bool					IsStreamsReady(std::shared_ptr<TMediaPacket> Packet);	//	if this returns false, not ready to ProcessPacket yet
	
public:
	SoyListenerId							mOnPacketListener;
	std::shared_ptr<TStreamWriter>			mOutput;
	std::shared_ptr<TMediaPacketBuffer>		mInput;
	Array<TStreamMeta>						mStreams;		//	streams, once setup, fixed
	
	Array<std::shared_ptr<TMediaPacket>>	mDefferedPackets;	//	when auto-calculating streams we buffer up some packets
};





//	demuxer
class TMediaExtractor : public SoyWorkerThread
{
public:
	TMediaExtractor(const std::string& ThreadName,SoyTime ReadAheadMs,std::function<void(const SoyTime,size_t)> OnFrameExtracted);
	~TMediaExtractor();
	
	void							Seek(SoyTime Time);				//	keep calling this, controls the packet read-ahead
	
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
	


//protected:	//	gr: only for subclasses, but the playlist media extractor needs to call this on it's children
	virtual std::shared_ptr<TMediaPacket>	ReadNextPacket()=0;
	bool							CanPushPacket(SoyTime Time,size_t StreamIndex,bool IsKeyframe);

protected:
	void							OnEof();
	void							OnError(const std::string& Error);
	void							OnClearError();
	void							OnStreamsChanged(const ArrayBridge<TStreamMeta>&& Streams)	{	mOnStreamsChanged.OnTriggered( Streams );	}
	
	//virtual void					ResetTo(SoyTime Time);			//	for when we seek backwards, assume a stream needs resetting
	void							ReadPacketsUntil(SoyTime Time,std::function<bool()> While);

private:
	virtual bool					Iteration() override;
	
public:
	SoyEvent<const ArrayBridge<TStreamMeta>>		mOnStreamsChanged;
	SoyTime											mExtractAheadMs;
	
protected:
	std::map<size_t,std::shared_ptr<TMediaPacketBuffer>>	mStreamBuffers;
	
private:
	std::function<void(const SoyTime,size_t)>	mOnPacketExtracted;
	std::string						mFatalError;
	SoyTime							mSeekTime;
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
	virtual ~TMediaDecoder();
	
	bool							HasFatalError(std::string& Error)
	{
		Error += mFatalError.str();
		return !Error.empty();
	}
	
protected:
	void							OnDecodeFrameSubmitted(const SoyTime& Time);
	virtual bool					ProcessPacket(const TMediaPacket& Packet)=0;		//	return false to return the frame to the buffer and not discard
	
	//	gr: added this for android as I'm not sure about the thread safety, but other platforms generally have a callback
	//		if we can do input&output on different threads, remove this
	virtual void					ProcessOutputPacket(TPixelBufferManager& FrameBuffer)
	{
	}
	virtual void					ProcessOutputPacket(TAudioBufferManager& FrameBuffer)
	{
	}
	
	TPixelBufferManager&			GetPixelBufferManager();
	TAudioBufferManager&			GetAudioBufferManager();
	
private:
	virtual bool					Iteration() final;
	virtual bool					CanSleep() override;
	
protected:
	std::shared_ptr<TMediaPacketBuffer>		mInput;
	std::shared_ptr<TPixelBufferManager>	mPixelOutput;
	std::shared_ptr<TAudioBufferManager>	mAudioOutput;
	std::stringstream					mFatalError;
	
private:
	SoyListenerId						mOnNewPacketListener;
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
	virtual void					Write(const std::shared_ptr<SoyPixelsImpl> Image,SoyTime Timecode)=0;
	
protected:
	void							PushFrame(std::shared_ptr<TMediaPacket>& Packet,std::function<bool(void)> Block);
	
protected:
	std::shared_ptr<TMediaPacketBuffer>	mOutput;
	std::stringstream					mFatalError;
};




