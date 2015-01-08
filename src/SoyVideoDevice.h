#pragma once

#include <array.hpp>
#include <heaparray.hpp>
#include <SoyEvent.h>
#include <SoyPixels.h>
#include <SoyTime.h>
#include <SoyMemFile.h>


class TVideoDeviceMeta
{
public:
	TVideoDeviceMeta(const std::string& Serial="",const std::string& Name="") :
	mSerial			( Serial ),
	mName			( Name ),
	mVideo			( false ),
	mAudio			( false ),
	mText			( false ),
	mClosedCaption	( false ),
	mSubtitle		( false ),
	mTimecode		( false ),
	mTimedMetadata	( false ),
	mMetadata		( false ),
	mMuxed			( false )
	{
		//Soy::Assert( !IsValid(), "expected invalid" );
	}
	
	bool		IsValid() const		{	return !mSerial.empty();	}
	bool		operator==(const std::string& SerialOrName) const;
	
public:
	std::string	mName;
	std::string	mModel;
	std::string	mVendor;
	std::string	mSerial;
	bool		mConnected;
	bool		mVideo;
	bool		mAudio;
	bool		mText;
	bool		mClosedCaption;
	bool		mSubtitle;
	bool		mTimecode;
	bool		mTimedMetadata;
	bool		mMetadata;
	bool		mMuxed;
};

namespace TVideoQuality
{
	enum Type
	{
		Low,
		Medium,
		High,
	};
}
namespace TVideoOption
{
	enum Type
	{
		LockedFocus,
		LockedWhiteBalance,
		LockedExposure,
	};
}


class TVideoDeviceParams
{
public:
	TVideoDeviceParams() :
	mDiscardOldFrames	( true ),
	mQuality			( TVideoQuality::Low )
	{
	}
	
	bool				mDiscardOldFrames;
	TVideoQuality::Type	mQuality;	//	replce with resolution!
};


//	seperate type for meta so we can have timecode
//	gr: change this so we store directly to a memfile so we're constantly updating a shared membuffer
class TVideoFrameImpl
{
public:
	bool							IsValid() const		{	return GetPixelsConst().IsValid();	}
	virtual SoyPixelsImpl&			GetPixels() =0;
	virtual const SoyPixelsImpl&	GetPixelsConst() const=0;
	
public:
	SoyTime			mTimecode;
};

class TVideoFrame : public TVideoFrameImpl
{
public:
	virtual SoyPixelsImpl&			GetPixels() override		{	return mPixels;	}
	virtual const SoyPixelsImpl&	GetPixelsConst() const override 	{	return mPixels;	}
	
	SoyPixels		mPixels;
};

class SoyPixelsMemFile : public SoyPixelsImpl
{
public:
	SoyPixelsMemFile(std::string Filename,bool AllowOtherFilenames) :
	mMemFileArray		( Filename, AllowOtherFilenames ),
	mPixelBufferBridge	( mPixelBuffer )
	{
	}
	
	virtual bool						Copy(const SoyPixelsImpl& that) override
	{
		if ( !SoyPixelsImpl::Copy( that ) )
			return false;
		
		//	update memfile
		return GetRawSoyPixels( GetArrayBridge( mMemFileArray ) );
	}
	
	virtual SoyPixelsMeta&				GetMeta()						{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const					{	return mMeta;	}
	virtual ArrayInterface<char>&		GetPixelsArray() override		{	return mPixelBufferBridge;	}
	virtual const ArrayInterface<char>&	GetPixelsArray() const override	{	return mPixelBufferBridge;	}
	
public:
	SoyPixelsMeta		mMeta;
	Array<char>			mPixelBuffer;
	ArrayBridgeDef<Array<char>>	mPixelBufferBridge;
	
	//	gr: we need to have some sub-array type where Meta sits at the start of the memfile...
	MemFileArray		mMemFileArray;
};

class TVideoFrameMemFile : public TVideoFrameImpl
{
public:
	TVideoFrameMemFile(std::string Filename,bool AllowOtherFilenames) :
	mPixels	( Filename, AllowOtherFilenames )
	{
	}
	
	virtual SoyPixelsImpl&			GetPixels() override			{	return mPixels;	}
	virtual const SoyPixelsImpl&	GetPixelsConst() const override	{	return mPixels;	}
	
public:
	SoyPixelsMemFile	mPixels;
};

//	gr: currently RAII so no play/pause virtuals...
class TVideoDevice
{
public:
	TVideoDevice(std::string Serial,std::stringstream& Error);
	virtual ~TVideoDevice();
	
	virtual TVideoDeviceMeta	GetMeta() const=0;		//	gr: make this dynamic so other states might change
	std::string					GetSerial() const		{	return GetMeta().mSerial;	}
	const TVideoFrameMemFile&		GetLastFrame(std::stringstream& Error) const	{	Error << mLastError;	return mLastFrame;	}
	float						GetFps() const;			//	how many frames per sec are we averaging?
	int							GetFrameMs() const;		//	how long does each frame take to recieve
	void						ResetFrameCounter();	//	reset the fps counter
	
	//	gr: might need to report if supported
	virtual bool				GetOption(TVideoOption::Type Option,bool Default=false)	{	return Default;	}
	virtual bool				SetOption(TVideoOption::Type Option,bool Enable)		{	return false;	}
	
	bool						operator==(const std::string& Serial) const				{	return GetMeta() == Serial;	}
	
protected:
	void						OnFailedFrame(const std::string& Error);
	void						OnNewFrame(const SoyPixelsImpl& Pixels,SoyTime Timecode);
	
public:
	SoyEvent<TVideoDevice>		mOnNewFrame;
	
private:
	//	gr: video frame can cope without a lock,(no realloc) but the string will probably crash
	std::string					mLastError;		//	should be empty if last frame was okay
	TVideoFrameMemFile			mLastFrame;
	
	//	fps counting
	SoyTime						mFirstFrameTime;	//	time we got first frame
	SoyTime						mLastFrameTime;
	uint64						mFrameCount;
};



//	need a better name, generic interface to specific implementations (AVCApture, kinect etc)
class SoyVideoContainer
{
public:
	bool									HasDevice(const std::string& Serial);
	inline void								GetDevices(ArrayBridge<TVideoDeviceMeta>&& Metas)	{	return GetDevices( Metas );	}

	virtual void							GetDevices(ArrayBridge<TVideoDeviceMeta>& Metas)=0;
	virtual std::shared_ptr<TVideoDevice>	AllocDevice(const std::string& Serial,std::stringstream& Error)=0;
};


class SoyVideoCapture //: public EventReceiver
{
public:
	SoyVideoCapture();
	virtual ~SoyVideoCapture();
	
	std::shared_ptr<TVideoDevice>	GetDevice(std::string Serial,std::stringstream& Error);
	void							GetDevices(ArrayBridge<TVideoDeviceMeta>&& Metas);
	TVideoDeviceMeta				GetDeviceMeta(std::string Serial);
	void							CloseDevice(std::string Serial);
	
	static TVideoDeviceMeta			GetBestDeviceMeta(std::string Serial,ArrayBridge<TVideoDeviceMeta>&& Metas);	//	gr: abstracted to static so we can use it in a unit test
	
	void							AddContainer(std::shared_ptr<SoyVideoContainer> Container);
	
private:
	Array<std::shared_ptr<TVideoDevice>>		mDevices;
	Array<std::shared_ptr<SoyVideoContainer>>	mContainers;
};

