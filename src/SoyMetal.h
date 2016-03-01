#pragma once

#include "SoyThread.h"
#include "SortArray.h"
#include "SoyString.h"
#include "SoyEnum.h"
#include "SoyPixels.h"
#include "SoyUniform.h"



namespace Metal
{
	class TDeviceMeta;
	class TDevice;
	class TContext;
	class TContextThread;		//	job queue
	class TTexture;

	void		EnumDevices(ArrayBridge<std::shared_ptr<TDevice>>&& Devices);
};


//	forward declarations matching unity's helpers
#if defined(__OBJC__)
@class CAMetalLayer;
@protocol CAMetalDrawable;
@protocol MTLDrawable;
@protocol MTLDevice;
@protocol MTLTexture;
@protocol MTLCommandBuffer;
@protocol MTLCommandQueue;
@protocol MTLCommandEncoder;

typedef id<CAMetalDrawable>		CAMetalDrawableRef;
typedef id<MTLDevice>			MTLDeviceRef;
typedef id<MTLTexture>			MTLTextureRef;
typedef id<MTLCommandBuffer>	MTLCommandBufferRef;
typedef id<MTLCommandQueue>		MTLCommandQueueRef;
typedef id<MTLCommandEncoder>	MTLCommandEncoderRef;
#else
typedef struct objc_object		CAMetalLayer;
typedef struct objc_object*		CAMetalDrawableRef;
typedef struct objc_object*		MTLDeviceRef;
typedef struct objc_object*		MTLTextureRef;
typedef struct objc_object*		MTLCommandBufferRef;
typedef struct objc_object*		MTLCommandQueueRef;
typedef struct objc_object*		MTLCommandEncoderRef;
#endif


class Metal::TDeviceMeta
{
public:
	TDeviceMeta()
	{
	}
	
public:
	std::string			mName;
};
std::ostream& operator<<(std::ostream &out,const Metal::TDeviceMeta& in);


class Metal::TDevice : public TDeviceMeta
{
public:
	TDevice(void* DevicePtr);

	std::shared_ptr<TContext>		CreateContext();
	std::shared_ptr<TContextThread>	CreateContextThread(const std::string& Name);
	
	TDeviceMeta			GetMeta() const	{	return *this;	}
	
	bool				operator==(void* DevicePtr) const		{	return mDevice == DevicePtr;	}
	
protected:
	MTLDeviceRef		mDevice;
};


class Metal::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext(void* DevicePtr);	//	unity's anonymous interface
	//TContext(TDevice& Device);
	
	virtual bool	Lock() override;
	virtual void	Unlock() override;
	virtual bool	IsLocked(std::thread::id Thread) override;
	void			Iteration()			{	Flush(*this);	}
	
protected:
	std::shared_ptr<TDevice>	mDevice;
	
private:
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
};

/*
class Metal::TContextThread : public SoyWorkerThread, public Metal::TContext
{
public:
	TContextThread(const std::string& Name,TDevice& Device) :
		SoyWorkerThread		( Name, SoyWorkerWaitMode::Wake ),
		TContext			( Device )
	{
		WakeOnEvent( PopWorker::TJobQueue::mOnJobPushed );
		Start();
	}
	
	//	thread
	virtual bool		Iteration() override	{	PopWorker::TJobQueue::Flush(*this);	return true;	}
	virtual bool		CanSleep() override		{	return !PopWorker::TJobQueue::HasJobs();	}	//	break out of conditional with this
};
*/

class Metal::TTexture
{
public:
	//	referece from external (unity)
	TTexture(void* TexturePtr);
	
	SoyPixelsMeta	GetMeta() const;
	
private:
	MTLTextureRef	mTexture;
};


