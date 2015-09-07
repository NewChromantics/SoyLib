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

	vec2f		ClToVector(const cl_float2& v);
	vec4f		ClToVector(const cl_float4& v);
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
	virtual bool	SetUniform(const char* Name,const float& v) override;
	virtual bool	SetUniform(const char* Name,const vec2f& v) override;
	virtual bool	SetUniform(const char* Name,const vec4f& v) override;
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v) override
	{
		return SetUniform( Name, v, OpenclBufferReadWrite::ReadWrite );
	}
	bool			SetUniform(const char* Name,const Opengl::TTextureAndContext& v,OpenclBufferReadWrite::Type ReadWriteMode);
	bool			SetUniform(const char* Name,const SoyPixelsImpl& Pixels,OpenclBufferReadWrite::Type ReadWriteMode);
	bool			SetUniform(const char* Name,cl_int Value);
	bool			SetUniform(const char* Name,TBuffer& Buffer);
	
	//	throw on error, assuming wrong uniform is fatal
	//	read back data from a buffer that was used as a uniform
	void			ReadUniform(const char* Name,SoyPixelsImpl& Pixels);
	void			ReadUniform(const char* Name,Opengl::TTextureAndContext& Texture);
	template<typename TYPE>
	void			ReadUniform(const char* Name,ArrayBridge<TYPE>&& Data,size_t ElementsToRead);
	void			ReadUniform(const char* Name,TBuffer& Buffer);

	void			GetIterations(ArrayBridge<TKernelIteration>&& IterationSplits,const ArrayBridge<size_t>&& Iterations);

	void			QueueIteration(const TKernelIteration& Iteration);
	void			QueueIteration(const TKernelIteration& Iteration,TSync& Semaphore);

	const TDeviceMeta&	GetDevice();
	TContext&		GetContext();

private:
	void			QueueIterationImpl(const TKernelIteration& Iteration,TSync* Semaphore);

	//	get buffer for a uniform - only applies to temporary ones we created
	TBuffer&		GetUniformBuffer(const char* Name);	//	throw if non-existant. assuming fatal if we're trying to read data from a uniform

public:
	TKernel&		mKernel;
	
private:
	//	gr: changed to array because of map crashes... hopefully this will find out why
	Array<std::pair<std::string,std::shared_ptr<TBuffer>>>	mBuffers;
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
	~TSync();
	
	void	Wait();
	
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



/*
class SoyOpenClKernel
{
public:
	SoyOpenClKernel(std::string Name,SoyOpenClShader& Parent);
	~SoyOpenClKernel()
	{
		DeleteKernel();
	}

	void			OnLoaded();				//	kernel will be null if it failed
	void			OnUnloaded();
	std::string		GetName() const					{	return mName;	}
	bool			IsValid() const					{	return mKernel!=NULL;	}
	bool			IsValidExecCount(int ExecCount)	{	return IsValidGlobalExecCount( ExecCount );	}
	bool			IsValidGlobalExecCount(int ExecCount)	{	return (GetMaxWorkGroupSize()==-1) ? true : (ExecCount<=GetMaxWorkGroupSize());	}
	bool			IsValidLocalExecCount(ArrayBridge<int>& LocalExec,bool ForceCorrection);
	int				GetMaxWorkGroupSize() const				{	return GetMaxGlobalWorkGroupSize();	}
	int				GetMaxGlobalWorkGroupSize() const;
	int				GetMaxLocalWorkGroupSize() const;
	int				GetMaxBufferSize() const		{	return mDeviceInfo.maxMemAllocSize;	}
	cl_command_queue	GetQueue()					{	return mKernel ? mKernel->getQueue() : NULL;	}
	msa::OpenCL&	GetOpenCL() const;
	msa::OpenClDevice::Type	GetDevice() const		{	return static_cast<msa::OpenClDevice::Type>( mDeviceInfo.type );	}

	void			DeleteKernel();
	inline bool		operator==(const char* Name) const	{	return mName == Name;	}
	bool			Begin();
	bool			End(bool Blocking,const ArrayBridge<int>& GlobalExec,const ArrayBridge<int>& LocalExec);
	template<int DIMENSIONS>
	bool			End(const SoyOpenclKernelIteration<DIMENSIONS>& Iteration,const ArrayBridge<int>& LocalIterations)	{	return End( Iteration.mBlocking, GetArrayBridge(Iteration.mCount), LocalIterations );	}
	template<int DIMENSIONS>
	bool			End(const SoyOpenclKernelIteration<DIMENSIONS>& Iteration)	{	Array<int> LocalCount;	return End( Iteration, GetArrayBridge( LocalCount ) );	}
	
	bool			End1D(int Exec1)							{	return End1D( SoyOpenCl::DefaultExecuteBlocking, Exec1 );	}
	bool			End2D(int Exec1,int Exec2)					{	return End2D( SoyOpenCl::DefaultExecuteBlocking, Exec1, Exec2 );	}
	bool			End3D(int Exec1,int Exec2,int Exec3)		{	return End3D( SoyOpenCl::DefaultExecuteBlocking, Exec1, Exec2, Exec3 );	}
	bool			End1D(bool Blocking,int Exec1)						{	SoyOpenclKernelIteration<1> It( Exec1, Blocking );	return End(It);	}
	bool			End2D(bool Blocking,int Exec1,int Exec2)			{	SoyOpenclKernelIteration<2> It( Exec1, Exec2, Blocking );	return End(It);	}
	bool			End3D(bool Blocking,int Exec1,int Exec2,int Exec3)	{	SoyOpenclKernelIteration<3> It( Exec1, Exec2, Exec3, Blocking );	return End(It);	}
	
	void			GetIterations(Array<SoyOpenclKernelIteration<1>>& Iterations,int Exec1,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);
	void			GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);
	void			GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);

	void			OnBufferPendingWrite(cl_event PendingWriteEvent);

	bool									CheckPaddingChecksum(const int* Padding,int Length);
	template<typename TYPE> bool			CheckPaddingChecksum(const TYPE& Object)						{	return CheckPaddingChecksum( Object.mPadding, sizeofarray(Object.mPadding) );	}
	template<typename ARRAYTYPE> bool		CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray);
	template<typename TYPE> bool			CheckPaddingChecksum(const Array<TYPE>& ObjectArray)			{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,int SIZE> bool	CheckPaddingChecksum(const BufferArray<TYPE,SIZE>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE> bool			CheckPaddingChecksum(const RemoteArray<TYPE>& ObjectArray)		{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,class SORT> bool	CheckPaddingChecksum(const SortArray<TYPE,SORT>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray.mArray ) );	}
	
protected:
	bool			WaitForPendingWrites();

private:
	msa::clDeviceInfo	mDeviceInfo;
	std::string			mName;
	Array<cl_event>		mPendingBufferWrites;	//	wait for all these to finish before executing

public:
	SoyOpenClManager&	mManager;
	SoyOpenClShader&	mShader;				//	parent
	msa::OpenCLKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
};




template<typename ARRAYTYPE>
inline bool SoyOpenClKernel::CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray)	
{	
	for ( int i=0;	i<ObjectArray.GetSize();	i++ )
		if ( !CheckPaddingChecksum( ObjectArray[i] ) )
			return false;
	return true;
}


//	soy-cl-program which auto-reloads if the file changes
class SoyOpenClShader : public SoyFileChangeDetector
{
public:
	SoyOpenClShader(SoyRef Ref,std::string Filename,std::string BuildOptions,SoyOpenClManager& Manager) :
		SoyFileChangeDetector	( Filename ),
		mRef					( Ref ),
		mManager				( Manager ),
		mProgram				( NULL ),
		mBuildOptions			( BuildOptions )
	{
	}
	~SoyOpenClShader()	{	UnloadShader();	}

	bool				IsLoading();
	bool				LoadShader();
	void				UnloadShader();
	SoyOpenClKernel*	GetKernel(std::string Name,cl_command_queue Queue);			//	load and return a kernel
	SoyRef				GetRef() const							{	return mRef;	}
	std::string 		GetBuildOptions() const					{	return mBuildOptions;	}
	inline bool			operator==(const char* Filename) const	{	return Soy::StringBeginsWith( GetFilename(), Filename, false );	}
	inline bool			operator==(std::string Filename) const	{	return Soy::StringBeginsWith( GetFilename(), Filename, false );	}
	inline bool			operator==(const SoyRef& Ref) const		{	return GetRef() == Ref;	}

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyRef					mRef;
	std::string				mBuildOptions;

public:
	SoyOpenClManager&		mManager;
};

class SoyThreadQueue
{
public:
	SoyThreadQueue() :
		mQueue		( NULL ),
		mDeviceType	( msa::OpenClDevice::Invalid )
	{
	}

public:
	std::thread::id			mThreadId;
	cl_command_queue		mQueue;
	msa::OpenClDevice::Type	mDeviceType;
};

class SoyOpenClManager : public SoyThread
{
public:
	SoyOpenClManager(std::string PlatformName,prmem::Heap& Heap=prcore::Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	SoyOpenClKernel*		GetKernel(SoyOpenClKernelRef KernelRef,cl_command_queue Queue);
	void					DeleteKernel(SoyOpenClKernel* Kernel);
	bool					IsValid();			//	if not, cannot use opencl
	void					PreLoadShader(std::string Filename,std::string BuildOptions);	//	load in the background
	SoyOpenClShader*		LoadShader(std::string Filename,std::string BuildOptions);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyOpenClShader*		GetShader(std::string Filename);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));
	prmem::Heap&			GetHeap()		{	return mHeap;	}
	msa::OpenCL&			GetOpenCL()		{	return mOpencl;	}

	cl_command_queue		GetQueueForThread(msa::OpenClDevice::Type DeviceType);				//	get/alloc a specific queue for the current thread
	cl_command_queue		GetQueueForThread(std::thread::id ThreadId,msa::OpenClDevice::Type DeviceType);	//	get/alloc a specific queue for a thread

private:
	SoyOpenClShader*		AllocShader(std::string Filename,std::string BuildOptions);

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;
	ofMutexT<Array<SoyThreadQueue>>	mThreadQueues;

	ofMutexT<Array<SoyPair<BufferString<MAX_PATH>,BufferString<MAX_PATH>>>>	mPreloadShaders;	//	filename,buildoptions

public:
	msa::OpenCL				mOpencl;
	ofEvent<SoyOpenClShader*>	mOnShaderLoaded;	//	or [successfully] reloaded - after kernels have loaded
};



class SoyClShaderRunner
{
public:
	SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device=SoyOpenCl::DefaultDeviceType,const char* BuildOptions="");
	SoyClShaderRunner(const char* Shader,const char* Kernel,bool UseThreadQueue,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device=SoyOpenCl::DefaultDeviceType,const char* BuildOptions="");
	virtual ~SoyClShaderRunner()				{	DeleteKernel();	}

	bool				IsValid()		
	{
		auto* pKernel = GetKernel();	
		return pKernel ? pKernel->IsValid() : false;
	}
	SoyOpenClKernel*	GetKernel();
	void				DeleteKernel()			{	return mManager.DeleteKernel( mKernel );	}
	BufferString<200>	Debug_GetName() const	{	return BufferString<200>() << mKernelRef.Debug_GetName() << '[' << msa::OpenClDevice::ToString( mKernel ? mKernel->GetDevice() : msa::OpenClDevice::Invalid ) << ']';	}

public:
	bool				mUseThreadQueue;
	msa::OpenClDevice::Type	mRequestDevice;	//	device we requested at construction if not default/any. actual device used is stored in kernel
	SoyOpenClKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
	SoyOpenClManager&	mManager;
	prmem::Heap&		mHeap;
};




class SoyClDataBuffer
{
private:
	SoyClDataBuffer(const SoyClDataBuffer& That) :
		mManager	( *(SoyOpenClManager*)nullptr ),
		mKernel		( nullptr )
	{
	}
public:
	explicit SoyClDataBuffer(SoyOpenClManager& Manager,cl_command_queue Queue) :
		mBuffer		( nullptr ),
		mManager	( Manager ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,uint32 SIZE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const RemoteArray<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,class SORT>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,uint32 SIZE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const RemoteArray<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,class SORT>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}

	template<typename TYPE>
	bool		Write(const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode)
	{
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(TYPE) > 8 )
		{
			int Remainder = sizeof(TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<TYPE>() << " not aligned to 16 bytes; " << sizeof(TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}
		return Write( Array.GetArray(), Array.GetDataSize(), SoyOpenCl::DefaultWriteBlocking, ReadWriteMode );
	}
	template<typename ARRAYTYPE>
	bool		Write(const ArrayBridgeDef<ARRAYTYPE>& Array,cl_int ReadWriteMode)
	{
		auto& Bridge = static_cast<const ArrayBridge<typename ARRAYTYPE::TYPE>&>( Array );
		return Write( Bridge, ReadWriteMode );
	}
	template<typename TYPE>				bool	Write(const Array<TYPE>& Array,cl_int ReadWriteMode)			{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,int SIZE>	bool	Write(const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE>				bool	Write(const RemoteArray<TYPE>& Array,cl_int ReadWriteMode)		{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,class SORT>	bool	Write(const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array.mArray ), ReadWriteMode );	}
	
	template<typename TYPE>
	bool		Write(const TYPE& Data,cl_int ReadWriteMode)
	{
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(TYPE) > 8 )
		{
			int Remainder = sizeof(TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<TYPE>() << " not aligned to 16 bytes; " << sizeof(TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}
		return Write( &Data, sizeof(TYPE), SoyOpenCl::DefaultWriteBlocking, ReadWriteMode );
	}

	~SoyClDataBuffer()
	{
		if ( mBuffer )
		{
			mManager.mOpencl.deleteBuffer( *mBuffer );
		}
	}

	template<typename TYPE>
	bool		Read(ArrayBridge<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		if ( !mBuffer )
			return false;
		if ( ElementCount < 0 )
			ElementCount = Array.GetSize();
		Array.SetSize( ElementCount );
		if ( Array.IsEmpty() )
			return true;
		if ( !GetQueue() )
			return false;
		return mBuffer->read( Array.GetArray(), 0, Array.GetDataSize(), Blocking, GetQueue() );
	}
	template<typename ARRAYTYPE>
	bool		Read(ArrayBridgeDef<ARRAYTYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		auto& Bridge = static_cast<ArrayBridge<typename ARRAYTYPE::TYPE>&>( Array );
		return Read( Bridge, ElementCount, Blocking );
	}
	template<typename TYPE>				bool	Read(Array<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)				{	auto Bridge = GetArrayBridge(Array);	return Read( Bridge, ElementCount, Blocking );	}
	template<typename TYPE,int SIZE>	bool	Read(BufferArray<TYPE,SIZE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)	{	auto Bridge = GetArrayBridge(Array);	return Read( Bridge, ElementCount, Blocking );	}
	template<typename TYPE>				bool	Read(RemoteArray<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)			{	auto Bridge = GetArrayBridge(Array);	return Read( Bridge, ElementCount, Blocking );	}
	template<typename TYPE,class SORT>	bool	Read(SortArray<TYPE,SORT>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)		{	auto Bridge = GetArrayBridge(Array.mArray);	return Read( Bridge, ElementCount, Blocking );	}
		
	template<typename TYPE>
	bool		Read(TYPE& Data,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		if ( !mBuffer )
			return false;
		if ( !GetQueue() )
			return false;
		return mBuffer->read( &Data, 0, sizeof(TYPE), Blocking, GetQueue() );
	}

	bool		IsValid() const		{	return mBuffer!=nullptr;	}
	operator	bool()				{	return IsValid();	}
	cl_mem&		getCLMem()			{	static cl_mem Dummy;	assert( mBuffer );	return mBuffer ? mBuffer->getCLMem() : Dummy;	}

private:
	template<typename TYPE>
	bool	Write(const TYPE* pDataConst,int DataSize,bool Blocking,int ReadWriteMode)
	{
		assert( GetQueue() );
		if ( !GetQueue() )
			return false;
		assert( !mBuffer );
		mBuffer = mManager.mOpencl.createBuffer( GetQueue(), DataSize, ReadWriteMode, NULL, Blocking );
		if ( !mBuffer )
			return false;

		//	gr: only write data if we're NOT write-only memory
		bool WriteOnly = (ReadWriteMode & CL_MEM_WRITE_ONLY)!=0;
		if ( WriteOnly )
			return true;

		void* pData = const_cast<TYPE*>( pDataConst );
		if ( Blocking )
		{
			if ( !mBuffer->write( pData, 0, DataSize, Blocking, GetQueue() ) )
				return false;
		}
		else
		{
			//	write async and give an event to the kernel to wait for before executing
			cl_event Event;
			if ( !mBuffer->writeAsync( pData, 0, DataSize, &Event, GetQueue() ) )
				return false;
			if ( mKernel )
				mKernel->OnBufferPendingWrite( Event );
		}
		return true;
	}

	cl_command_queue	GetQueue()	{	return mKernel ? (mKernel->IsValid()?mKernel->GetQueue():nullptr) : mQueue;	}

public:
	msa::OpenCLBuffer*	mBuffer;
	SoyOpenClManager&	mManager;
	SoyOpenClKernel*	mKernel;
	cl_command_queue	mQueue;
};


*/

