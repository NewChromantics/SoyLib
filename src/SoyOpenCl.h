#pragma once

#include "SoyThread.h"
#include "SortArray.h"
#include "SoyString.h"
#include "SoyEnum.h"
#include "SoyPixels.h"
#include "SoyUniform.h"

namespace Opengl
{
	class TTexture;
	class TContext;
}



#if defined(TARGET_WINDOWS)
//	set OPENCL_PATH in macros 
//	gr: amd APP sdk, other includes/libs may be different?
#include <cl/Opencl.h>
#pragma comment( lib, "OpenCL.lib" )
#endif

#if defined(TARGET_OSX)
//	add the OpenCL.framework
#include <opencl/opencl.h>
#endif


#define CL_DEVICE_TYPE_INVALID		0
#define CL_UNIFORM_INVALID_INDEX	CL_UINT_MAX


class SoyPixelsImpl;


namespace OpenclDevice
{
	enum Type
	{
		Invalid = CL_DEVICE_TYPE_INVALID,
		CPU = CL_DEVICE_TYPE_CPU,
		GPU = CL_DEVICE_TYPE_GPU,
		ANY = CL_DEVICE_TYPE_CPU|CL_DEVICE_TYPE_GPU,
	};
	DECLARE_SOYENUM(OpenclDevice);
}

namespace OpenclBufferReadWrite
{
	enum Type
	{
		ReadWrite = CL_MEM_READ_WRITE,
		ReadOnly = CL_MEM_READ_ONLY,
		WriteOnly = CL_MEM_WRITE_ONLY,
	};
}

namespace Opencl
{
	class TPlatform;	//	API, has multiple devices
	class TDeviceMeta;
	class TDevice;		//	context to interface with 1 or more devices (one cl context)
	class TContext;		//	a command queue/thread on a device (cl queue)
	class TContextThread;	//	context with a built in thread for processing jobs
	class TProgram;		//	compilable shader with multiple kernels
	class TKernel;		//	individual kernel from a program
	class TUniform;
	class TSync;		//	cl_wait_event

	class TJob;			//	execute a kernel, on a context
	
	class TBuffer;
	class TBufferImage;
	template<typename TYPE>
	class TBufferArray;

	class TKernelState;	//	current binding to a kernel
	class TKernelIteration;

	void				GetDevices(ArrayBridge<TDeviceMeta>&& Metas,OpenclDevice::Type Filter);
	std::string			GetErrorString(cl_int Error);
	cl_image_format		GetImageFormat(SoyPixelsFormat::Type Format);
	
	static const char*	BuildOption_KernelInfo = "-cl-kernel-arg-info";

};


//	type conversions
namespace Soy
{
	cl_float2	VectorToCl(const vec2f& v);
	cl_float3	VectorToCl(const vec3f& v);
	cl_float4	VectorToCl(const vec4f& v);
	cl_float8	VectorToCl8(const ArrayBridge<float>& v);
	cl_int4		VectorToCl(const vec4x<int>& v);

	vec2f		ClToVector(const cl_float2& v);
	vec4f		ClToVector(const cl_float4& v);
	mathfu::Vector<float,8>		ClToVector(const cl_float8& v);
}

std::ostream& operator<<(std::ostream &out,const cl_float2& in);
std::ostream& operator<<(std::ostream &out,const cl_float4& in);



class Opencl::TPlatform
{
public:
	TPlatform(cl_platform_id Platform);
	
	void			GetDevices(ArrayBridge<TDeviceMeta>& Metas,OpenclDevice::Type Filter);
	
	std::string		mName;
	std::string		mVersion;
	std::string		mVendor;
	cl_platform_id	mPlatform;
};
std::ostream& operator<<(std::ostream &out,const Opencl::TPlatform& in);


class Opencl::TDeviceMeta
{
public:
	TDeviceMeta() :
		mDevice		( nullptr )
	{
	}
	TDeviceMeta(cl_device_id Device);
	
	bool				IsValid() const		{	return mDevice != nullptr;	}

	
	size_t				GetMaxGlobalWorkGroupSize() const;

public:
	Soy::TVersion		mVersion;
	cl_device_id		mDevice;
	std::string			mVendor;
	std::string			mName;
	std::string			mDriverVersion;
	std::string			mDeviceVersion;
	std::string			mProfile;
	Array<std::string>	mExtensions;
	OpenclDevice::Type	mType;
	bool				mHasOpenglInteroperability;
	bool				mHasOpenglSyncSupport;
	
	cl_uint		maxComputeUnits;
	cl_uint		maxWorkItemDimensions;
	size_t		maxWorkItemSizes[32];
	size_t		maxWorkGroupSize;
	cl_uint		maxClockFrequency;
	cl_ulong	maxMemAllocSize;
	cl_bool		imageSupport;
	cl_uint		maxReadImageArgs;
	cl_uint		maxWriteImageArgs;
	size_t		image2dMaxWidth;
	size_t		image2dMaxHeight;
	size_t		image3dMaxWidth;
	size_t		image3dMaxHeight;
	size_t		image3dMaxDepth;
	cl_uint		maxSamplers;
	size_t		maxParameterSize;
	cl_ulong	globalMemCacheSize;
	cl_ulong	globalMemSize;
	cl_ulong	maxConstantBufferSize;
	cl_uint		maxConstantArgs;
	cl_ulong	localMemSize;
	cl_bool		errorCorrectionSupport;
	size_t		profilingTimerResolution;
	cl_bool		endianLittle;
	cl_uint		deviceAddressBits;
};
std::ostream& operator<<(std::ostream &out,const Opencl::TDeviceMeta& in);


class Opencl::TDevice
{
public:
	TDevice(const ArrayBridge<cl_device_id>& Devices,Opengl::TContext* OpenglContext=nullptr);
	TDevice(const ArrayBridge<TDeviceMeta>& Devices,Opengl::TContext* OpenglContext=nullptr);
	~TDevice();
	
	std::shared_ptr<TContext>		CreateContext();
	std::shared_ptr<TContextThread>	CreateContextThread(const std::string& Name);
	cl_context						GetClContext()		{	return mContext;	}
	bool							HasInteroperability(Opengl::TContext& Opengl);
	
private:
	void				CreateContext(const ArrayBridge<cl_device_id>& Devices,Opengl::TContext* OpenglContext);

protected:
	Array<TDeviceMeta>	mDevices;	//	devices attached to this context
	cl_context			mContext;	//	binding to a device
	
#if defined(TARGET_OSX)
	CGLContextObj		mSharedOpenglContext;
#endif
};


class Opencl::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext(TDevice& Device,cl_device_id SubDevice);
	~TContext();
	
	std::shared_ptr<TBuffer>		CreateBuffer();
	std::shared_ptr<TBufferImage>	CreateBufferImage();

	virtual bool	Lock() override;
	virtual void	Unlock() override;
	virtual bool	IsLocked(std::thread::id Thread) override;
	
	const TDeviceMeta&	GetDevice() const	{	return mDeviceMeta;		}
	cl_context			GetContext()		{	return mDevice.GetClContext();	}	//	get the opencl context
	cl_command_queue	GetQueue() const	{	return mQueue;	}

	bool				HasInteroperability(Opengl::TContext& Opengl)	{	return mDevice.HasInteroperability(Opengl);	}
	
	bool				operator==(const TContext& that) const
	{
		return mDevice.GetClContext() == that.mDevice.GetClContext();
	}
	bool				operator!=(const TContext& that) const	{	return !(*this == that);	}
	
protected:
	TDevice&			mDevice;
	TDeviceMeta			mDeviceMeta;	//	useful to cache to read vars
	cl_command_queue	mQueue;
	
private:
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
};



class Opencl::TContextThread : public SoyWorkerThread, public Opencl::TContext
{
public:
	TContextThread(const std::string& Name,TDevice& Device,cl_device_id SubDevice) :
		SoyWorkerThread		( Name, SoyWorkerWaitMode::Wake ),
		Opencl::TContext	( Device, SubDevice )
	{
		WakeOnEvent( PopWorker::TJobQueue::mOnJobPushed );
		Start();
	}
	
	//	thread
	virtual bool		Iteration() override	{	PopWorker::TJobQueue::Flush(*this);	return true;	}
	virtual bool		CanSleep() override		{	return !PopWorker::TJobQueue::HasJobs();	}	//	break out of conditional with this
};


class Opencl::TProgram
{
public:
	TProgram(const std::string& Source,TContext& Context);
	~TProgram();
	
public:
	cl_program	mProgram;
};




class Opencl::TBuffer
{
protected:
	TBuffer(const std::string& DebugName);
	TBuffer(size_t Size,TContext& Context,const std::string& DebugName);
public:
	virtual ~TBuffer();
	
	cl_mem		GetMemBuffer() 	{	return mMem;	}
	
	template<typename TYPE>
	void		Read(ArrayBridge<TYPE>& Array,TContext& Context,TSync* Sync)
	{
		//	construct a new array
		auto* ArrayPtr = reinterpret_cast<uint8*>( Array.GetArray() );
		size_t ByteCount = Array.GetDataSize();
		auto Array8 = GetRemoteArray( ArrayPtr, ByteCount );
		auto Array8Bridge = GetArrayBridge( Array8 );
		ReadImpl( Array8Bridge, Context, Sync );
	}
	
	template<typename TYPE>
	void		Read(ArrayBridge<TYPE>&& Array,TContext& Context,TSync* Sync)
	{
		Read( Array, Context, Sync );
	}
	
	template<typename TYPE>
	void		Read(TYPE& Item,TContext& Context,TSync* Sync)
	{
		auto ItemArray = GetRemoteArray( &Item, 1 );
		Read( GetArrayBridge( ItemArray ), Context, Sync );
	}
	
	size_t		GetMaxSize() const	{	return mBufferSize;	}
	
protected:
	void		ReadImpl(ArrayInterface<uint8>& Array,TContext& Context,TSync* Sync);
	void		Write(const uint8* Array,size_t Size,TContext& Context,TSync* Sync);
	
protected:
	size_t			mBufferSize;
	cl_mem			mMem;
	std::string		mDebugName;
};


template<typename TYPE>
class Opencl::TBufferArray : public TBuffer
{
public:
	TBufferArray(ArrayBridge<TYPE>&& Array,TContext& Context,const std::string& DebugName,TSync* Sync=nullptr) :
		TBuffer			( Array.GetDataSize(), Context, DebugName ),
		mElementSize	( 0 )
	{
		Write( reinterpret_cast<uint8*>( Array.GetArray() ), Array.GetDataSize(), Context, Sync );
		mElementSize = Array.GetElementSize();
	}
	TBufferArray(ArrayBridge<TYPE>& Array,TContext& Context,const std::string& DebugName,TSync* Sync=nullptr) :
		TBuffer			( Array.GetDataSize(), Context, DebugName ),
		mElementSize	( 0 )
	{
		Write( reinterpret_cast<uint8*>( Array.GetArray() ), Array.GetDataSize(), Context, Sync );
		mElementSize = Array.GetElementSize();
	}
	
	size_t	GetSize() const	{	return mBufferSize / mElementSize;	}
	
private:
	size_t	mElementSize;
};



class Opencl::TBufferImage : public TBuffer
{
public:
	TBufferImage(const SoyPixelsMeta& Meta,TContext& Context,const SoyPixelsImpl* ClientStorage,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,Opencl::TSync* Semaphore=nullptr);
	TBufferImage(const SoyPixelsImpl& Image,TContext& Context,bool ClientStorage,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,Opencl::TSync* Semaphore=nullptr);
	TBufferImage(const Opengl::TTexture& Image,Opengl::TContext& OpenglContext,TContext& Context,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,Opencl::TSync* Semaphore=nullptr);
	~TBufferImage();
	
	TBufferImage&	operator=(TBufferImage&& Move);
	
	void		Write(const Opengl::TTexture& Image,Opencl::TSync* Semaphore);
	void		Write(const SoyPixelsImpl& Image,Opencl::TSync* Semaphore);
	void		Write(TBufferImage& Image,Opencl::TSync* Semaphore);
	void		Read(SoyPixelsImpl& Image,Opencl::TSync* Semaphore);
	void		Read(Opengl::TTexture& Image,Opencl::TSync* Semaphore);
	void		Read(Opengl::TTextureAndContext& TextureAndContext,Opencl::TSync* Semaphore);
	
	SoyPixelsMeta	GetMeta() const			{	return mMeta;	}
	
private:
	TContext&				mContext;
	const Opengl::TTexture*	mOpenglObject;	//	pointer to reduce dependancy. If texture is deleted in the meantime results are undefined anyway. maybe make this safer sometime
	SoyPixelsMeta			mMeta;
};

class Opencl::TKernelIteration
{
	const static size_t DIMENSIONS=3;
public:
	TKernelIteration() :
		mFirst		( 0 ),
		mCount		( 0 ),
		mBlocking	( true )
	{
		mFirst.SetAll(0);
		mCount.SetAll(0);
	}
	explicit TKernelIteration(size_t Exec1,bool Blocking) :
		mFirst		( 1 ),
		mCount		( 1 ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		assert( DIMENSIONS == 1 );
		mCount[0] = Exec1;
	}
	explicit TKernelIteration(size_t Exec1,size_t Exec2,bool Blocking) :
		mFirst		( 2 ),
		mCount		( 2 ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		mCount[0] = Exec1;
		mCount[1] = Exec2;
	}
	explicit TKernelIteration(size_t Exec1,size_t Exec2,size_t Exec3,bool Blocking) :
		mFirst		( 3 ),
		mCount		( 3 ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		mCount[0] = Exec1;
		mCount[1] = Exec2;
		mCount[2] = Exec3;
	}
public:
	BufferArray<size_t,DIMENSIONS>		mFirst;
	BufferArray<size_t,DIMENSIONS>		mCount;
	bool								mBlocking;
};



class Opencl::TKernelState : public Soy::TUniformContainer
{
	friend class TKernel;
protected:
	TKernelState(TKernel& Kernel);
public:
	virtual ~TKernelState();
	
	//	gr: not uniforms, but matching name of opengl
	//	gr: like opengl, these now throw on error, silent(return) if uniform doesn't exist
	virtual bool	SetUniform(const char* Name,const int& v) override;
	virtual bool	SetUniform(const char* Name,const float& v) override;
	virtual bool	SetUniform(const char* Name,const vec2f& v) override;
	virtual bool	SetUniform(const char* Name,const vec3f& v) override;
	virtual bool	SetUniform(const char* Name,const vec4f& v) override;
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v) override
	{
		return SetUniform( Name, v, OpenclBufferReadWrite::ReadWrite );
	}
	bool			SetUniform(const char* Name,const Opengl::TTextureAndContext& v,OpenclBufferReadWrite::Type ReadWriteMode);
	bool			SetUniform(const char* Name,const SoyPixelsImpl& Pixels) override		{	return SetUniform( Name, Pixels, OpenclBufferReadWrite::ReadWrite );	}
	bool			SetUniform(const char* Name,const SoyPixelsImpl& Pixels,OpenclBufferReadWrite::Type ReadWriteMode);
	bool			SetUniform(const char* Name,TBuffer& Buffer);
	
	//	throw on error, assuming wrong uniform is fatal
	//	read back data from a buffer that was used as a uniform
	void			ReadUniform(const char* Name,SoyPixelsImpl& Pixels);
	void			ReadUniform(const char* Name,Opengl::TTextureAndContext& Texture);
	template<typename TYPE>
	void			ReadUniform(const char* Name,ArrayBridge<TYPE>&& Data,size_t ElementsToRead);
	void			ReadUniform(const char* Name,TBuffer& Buffer);

	void			GetIterations(ArrayBridge<TKernelIteration>&& IterationSplits,const ArrayBridge<vec2x<size_t>>&& Iterations);

	void			QueueIteration(const TKernelIteration& Iteration);
	void			QueueIteration(const TKernelIteration& Iteration,TSync& Semaphore);

	const TDeviceMeta&	GetDevice();
	TContext&		GetContext();

private:
	void			QueueIterationImpl(const TKernelIteration& Iteration,TSync* Semaphore);

	//	get buffer for a uniform - only applies to temporary ones we created
	TBuffer&		GetUniformBuffer(const char* Name);	//	throw if non-existant. assuming fatal if we're trying to read data from a uniform
	void			OnAssignedUniform(const char* Name,bool Success)
	{
		if ( Success )
			mAssignedArguments.PushBack( Name );
	}

public:
	TKernel&		mKernel;
	
private:
	//	gr: changed to array because of map crashes... hopefully this will find out why
	Array<std::pair<std::string,std::shared_ptr<TBuffer>>>	mBuffers;
	Array<std::string>										mAssignedArguments;	//	for detecting when an argument hasn't been set
	//std::map<std::string,std::shared_ptr<TBuffer>>	mBuffers;	//	temporarily allocated buffers for uniforms
};



class Opencl::TUniform : public Soy::TUniform
{
public:
	TUniform() :
		mIndex		( CL_UNIFORM_INVALID_INDEX )
	{
	}
	TUniform(const std::string& Name,const std::string& Type,cl_uint Index) :
		Soy::TUniform	( Name, Type ),
		mIndex			( Index )
	{
	}
	
	bool		IsValid() const	{	return mIndex != CL_UNIFORM_INVALID_INDEX;	}
	bool		operator==(const std::string& Name) const	{	return mName == Name;	}
	
public:
	cl_uint		mIndex;
};
std::ostream& operator<<(std::ostream &out,const Opencl::TUniform& in);


class Opencl::TKernel
{
public:
	TKernel(const std::string& Kernel,TProgram& Program);
	~TKernel();

	//	cl_kernel's are the only things that aren't thread safe(re-entrant safe)
	//	when setting arguments
	TKernelState	Lock(TContext& Context);
	void			Unlock();
	TContext&		GetContext();
	TUniform		GetUniform(const char* Name) const;
	bool			HasUniform(const char* Name) const	{	return GetUniform(Name).IsValid();	}

public:
	std::string		mKernelName;		//	only for debug
	cl_kernel		mKernel;
	Array<TUniform>	mUniforms;
	
protected:
	std::mutex	mLock;
	TContext*	mLockedContext;	//	kernels need to be locked to a queue
};


class Opencl::TJob : public PopWorker::TJob
{
public:
	TJob(TKernel& Kernel,TContext& Context);	//	should be able to get context from run()

protected:
	TKernel&	mKernel;
	TContext&	mContext;
};


class Opencl::TSync
{
public:
	TSync();
	~TSync()	{	Release();	}
	
	void		Release();	//	occasionally we need to invalidate a sync
	void		Wait();
	
public:
	cl_event	mEvent;
};



template<typename TYPE>
inline void Opencl::TKernelState::ReadUniform(const char* Name,ArrayBridge<TYPE>&& Data,size_t ElementsToRead)
{
	auto ArrayWrapper = GetRemoteArray( Data.GetArray(), ElementsToRead );
	auto& Buffer = GetUniformBuffer(Name);
	Opencl::TSync Semaphore;
	Buffer.Read( GetArrayBridge(ArrayWrapper), GetContext(), &Semaphore );
	Semaphore.Wait();
}

