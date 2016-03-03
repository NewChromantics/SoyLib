#pragma once

#include "SoyThread.h"
#include "SortArray.h"
#include "SoyString.h"
#include "SoyEnum.h"
#include "SoyPixels.h"
#include "SoyUniform.h"


namespace Opengl
{
	class TTextureUploadParams;
}

namespace Metal
{
	class TDeviceMeta;
	class TDevice;
	class TContext;				//	Metal queue
	class TContextThread;		//	job queue
	class TTexture;
	class TJob;					//	metal command buffer
	class TBuffer;				//	MTLBuffer

	void		EnumDevices(ArrayBridge<std::shared_ptr<TDevice>>&& Devices);
};


template<typename TYPE_impl>
class Objc
{
public:
	Objc(std::shared_ptr<TYPE_impl> Ptr) :
		mPtr	( Ptr )
	{
	}
	Objc(const Objc& that)	{	*this = that;	}
	Objc()	{}
	
	TYPE_impl*		operator->() const			{	return mPtr.get(); }
	Objc&			operator=(const Objc& that)	{	mPtr = that.mPtr;	return *this;	}
	operator		bool() const				{	return mPtr!=nullptr;	}
	
	void			reset()						{	mPtr.reset();	}
	
private:

	
private:
	std::shared_ptr<TYPE_impl>	mPtr;
};


//	MTLXXXWrapper objective c id<XXX> wrapper defined externally
//	shared_ptr to wrapper

#if defined(__OBJC__)
#define DECALRE_PROTOCOL(T)	@protocol T;
#else
#define DECALRE_PROTOCOL(T)	;
#endif

#define FWD_DECLARE_OBJC_WRAPPER(TYPE)	\
DECALRE_PROTOCOL( TYPE )	\
class TYPE ## _impl;	\
typedef Objc<TYPE ## _impl> TYPE ## Ptr;\


FWD_DECLARE_OBJC_WRAPPER( MTLDevice );
FWD_DECLARE_OBJC_WRAPPER( MTLTexture );
FWD_DECLARE_OBJC_WRAPPER( MTLBuffer );
FWD_DECLARE_OBJC_WRAPPER( MTLCommandBuffer );
FWD_DECLARE_OBJC_WRAPPER( MTLCommandQueue );
FWD_DECLARE_OBJC_WRAPPER( MTLCommandEncoder );



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
	TDevice(void* DevicePtr);	//	horrible void* lack of casting

	std::shared_ptr<TContext>		CreateContext();
	std::shared_ptr<TContextThread>	CreateContextThread(const std::string& Name);
	
	TDeviceMeta			GetMeta() const	{	return *this;	}
	
	bool				operator==(void* DevicePtr) const;
#if defined(__OBJC__)
	id<MTLDevice>		GetDevice();
#endif
	
protected:
	MTLDevicePtr		mDevice;
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
	
	bool			HasMultithreadAccess() const		{	return false;	}
	
	std::shared_ptr<TJob>		AllocJob();

	std::shared_ptr<TBuffer>	AllocBuffer(const uint8* Data,size_t DataSize);
	template<typename TYPE>
	std::shared_ptr<TBuffer>	AllocBuffer(const ArrayBridge<TYPE>&& Data)
	{
		return AllocBuffer( reinterpret_cast<const uint8*>(Data.GetArray()), Data.GetDataSize() );
	}
#if defined(__OBJC__)
	id<MTLCommandQueue>			GetQueue();
	id<MTLDevice>				GetDevice();	//	throws if null
#endif
	
protected:
	std::shared_ptr<TDevice>	mDevice;
	MTLCommandQueuePtr			mQueue;			//	command queue
	
private:
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
};

class Metal::TJob
{
public:
	TJob(TContext& Context);
	~TJob();
	
	void						Execute()								{	Execute( nullptr );	}
	void						Execute(Soy::TSemaphore& Semaphore)		{	Execute( &Semaphore );	}
	
#if defined(__OBJC__)
	id<MTLCommandBuffer>		GetCommandBuffer();
#endif
	
private:
	void						Execute(Soy::TSemaphore* Semaphore);
	
private:
	MTLCommandBufferPtr			mCommandBuffer;
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
	TTexture() :
		mTexture	( nullptr ),
		mAutoRelease	( true )
	{
	}
	TTexture(TTexture&& Move)			{	*this = std::move(Move);	}
	TTexture(const TTexture& Reference)	{	*this = Reference;	}
	TTexture(void* TexturePtr);			//	referece from external (unity)
	explicit TTexture(const SoyPixelsMeta& Meta,TContext& Context);	//	alloc
	~TTexture()
	{
		if ( mAutoRelease )
			Delete();
	}
	
	TTexture& operator=(const TTexture& Weak)
	{
		if ( this != &Weak )
		{
			mAutoRelease = false;
			mTexture = Weak.mTexture;
		}
		return *this;
	}
	
	TTexture& operator=(TTexture&& Move)
	{
		if ( this != &Move )
		{
			mAutoRelease = Move.mAutoRelease;
			mTexture = Move.mTexture;
			
			//	stolen the resource
			Move.mAutoRelease = false;
		}
		return *this;
	}
	
	void			Delete();
	void			Write(const SoyPixelsImpl& Pixels,const Opengl::TTextureUploadParams& Params,TContext& Context);
	void			Write(const TTexture& That,const Opengl::TTextureUploadParams& Params,TContext& Context);

	SoyPixelsMeta	GetMeta() const;
	bool			IsValid() const		{	return mTexture;	}
	bool			operator==(const SoyPixelsMeta& Meta) const	{	return Meta == GetMeta();	}
	bool			operator!=(const SoyPixelsMeta& Meta) const	{	return Meta != GetMeta();	}
	bool			operator==(const TTexture& that) const		{	return mTexture == that.mTexture;	}
	bool			operator!=(const TTexture& that) const		{	return mTexture != that.mTexture;	}
	
private:
	bool			mAutoRelease;
	MTLTexturePtr	mTexture;
};


class Metal::TBuffer
{
public:
	TBuffer(const uint8* Data,size_t DataSize,TDevice& Device);
	
#if defined(__OBJC__)
	id<MTLBuffer>		GetCommandBuffer();
#endif
	
private:
	MTLBufferPtr		mBuffer;
};

