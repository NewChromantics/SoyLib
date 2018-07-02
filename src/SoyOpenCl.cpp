#include "SoyOpenCl.h"
//#include "SoyApp.h"
#include <SoyDebug.h>
#include <SoyString.h>
#include "SoyOpenglContext.h"


namespace Opencl
{
#define Opencl_IsOkay(Error)	Opencl::IsOkay( (Error), __func__, true )
	bool				IsOkay(cl_int Error,const std::string& Context,bool ThrowException=true);
	
	std::string			EnumToString_EventStatus(cl_uint Error);

	cl_channel_order	GetImageChannelOrder(SoyPixelsFormat::Type Format,cl_channel_type& DataType);
}


std::ostream& operator<<(std::ostream &out,const Opencl::TDeviceMeta& in)
{
	out << in.mName << " ";
	out << in.mVendor << " ";
	out << OpenclDevice::ToString(in.mType) << " ";
	return out;
}
/*

std::ostream& Opencl::operator<<(std::ostream &out,const Opencl::TDeviceMeta& in)
{
	out << in.mName << " ";
	out << in.mVendor << " ";
	out << OpenclDevice::ToString(in.mType) << " ";
	return out;
}
*/

std::map<OpenclDevice::Type,std::string> OpenclDevice::EnumMap =
{
	{ OpenclDevice::Invalid,	"Invalid" },
	{ OpenclDevice::CPU,	"CPU" },
	{ OpenclDevice::GPU,	"GPU" },
	{ OpenclDevice::ANY,	"ANY" },
};


cl_float8 Soy::VectorToCl8(const ArrayBridge<float>& v)
{
	return cl_float8{ .s={ v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7] } };
}

cl_float16 Soy::VectorToCl16(const ArrayBridge<float>& v)
{
	return cl_float16{ .s={ v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15] } };
}

cl_int4 Soy::VectorToCl(const vec4x<int>& v)
{
	return cl_int4{ .s={ v.x, v.y, v.z, v.w } };
}

cl_float2 Soy::VectorToCl(const vec2f& v)
{
	return cl_float2{ .s={ v.x, v.y } };
}

cl_float3 Soy::VectorToCl(const vec3f& v)
{
	return cl_float3{ .s={ v.x, v.y, v.z } };
}

cl_float4 Soy::VectorToCl(const vec4f& v)
{
	return cl_float4{ .s={ v.x, v.y, v.z, v.w } };
}

vec2f Soy::ClToVector(const cl_float2& v)
{
	return vec2f( v.s[0], v.s[1] );
}

vec4f Soy::ClToVector(const cl_float4& v)
{
	return vec4f( v.s[0], v.s[1], v.s[2], v.s[3] );
}


std::ostream& operator<<(std::ostream &out,const cl_float2& in)
{
	out << Soy::ClToVector(in);
	return out;
}

std::ostream& operator<<(std::ostream &out,const cl_float4& in)
{
	out << Soy::ClToVector(in);
	return out;
}





bool Opencl::IsOkay(cl_int Error,const std::string& Context,bool ThrowException)
{
	if ( Error == CL_SUCCESS )
		return true;
	
	std::stringstream ErrorStr;
	ErrorStr << "Opencl error in " << Context << ": " << Opencl::GetErrorString(Error) << std::endl;
	
	if ( !ThrowException )
	{
		std::Debug << ErrorStr.str() << std::endl;
		return false;
	}
	
	throw Soy::AssertException( ErrorStr.str() );
}


std::string Opencl::GetErrorString(cl_int Error)
{
#define CASE_ENUM_STRING(e)	case (e):	return #e;
	switch ( Error )
	{
		//	extension errors
			CASE_ENUM_STRING(CL_INVALID_ARG_NAME_APPLE);
			
		CASE_ENUM_STRING( CL_SUCCESS );
			CASE_ENUM_STRING( CL_DEVICE_NOT_FOUND );
			CASE_ENUM_STRING( CL_DEVICE_NOT_AVAILABLE );
			CASE_ENUM_STRING( CL_COMPILER_NOT_AVAILABLE );
			CASE_ENUM_STRING( CL_MEM_OBJECT_ALLOCATION_FAILURE );
			CASE_ENUM_STRING( CL_OUT_OF_RESOURCES );
			CASE_ENUM_STRING( CL_OUT_OF_HOST_MEMORY );
			CASE_ENUM_STRING( CL_PROFILING_INFO_NOT_AVAILABLE );
			CASE_ENUM_STRING( CL_MEM_COPY_OVERLAP );
			CASE_ENUM_STRING( CL_IMAGE_FORMAT_MISMATCH );
			CASE_ENUM_STRING( CL_IMAGE_FORMAT_NOT_SUPPORTED );
			CASE_ENUM_STRING( CL_BUILD_PROGRAM_FAILURE );
			CASE_ENUM_STRING( CL_MAP_FAILURE );
			CASE_ENUM_STRING( CL_MISALIGNED_SUB_BUFFER_OFFSET );
			CASE_ENUM_STRING( CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST );
			CASE_ENUM_STRING( CL_COMPILE_PROGRAM_FAILURE );
			CASE_ENUM_STRING( CL_LINKER_NOT_AVAILABLE );
			CASE_ENUM_STRING( CL_LINK_PROGRAM_FAILURE );
			CASE_ENUM_STRING( CL_DEVICE_PARTITION_FAILED );
			CASE_ENUM_STRING( CL_KERNEL_ARG_INFO_NOT_AVAILABLE );
			
			CASE_ENUM_STRING( CL_INVALID_VALUE );
			CASE_ENUM_STRING( CL_INVALID_DEVICE_TYPE );
			CASE_ENUM_STRING( CL_INVALID_PLATFORM );
			CASE_ENUM_STRING( CL_INVALID_DEVICE );
			CASE_ENUM_STRING( CL_INVALID_CONTEXT );
			CASE_ENUM_STRING( CL_INVALID_QUEUE_PROPERTIES );
			CASE_ENUM_STRING( CL_INVALID_COMMAND_QUEUE );
			CASE_ENUM_STRING( CL_INVALID_HOST_PTR );
			CASE_ENUM_STRING( CL_INVALID_MEM_OBJECT );
			CASE_ENUM_STRING( CL_INVALID_IMAGE_FORMAT_DESCRIPTOR );
			CASE_ENUM_STRING( CL_INVALID_IMAGE_SIZE );
			CASE_ENUM_STRING( CL_INVALID_SAMPLER );
			CASE_ENUM_STRING( CL_INVALID_BINARY );
			CASE_ENUM_STRING( CL_INVALID_BUILD_OPTIONS );
			CASE_ENUM_STRING( CL_INVALID_PROGRAM );
			CASE_ENUM_STRING( CL_INVALID_PROGRAM_EXECUTABLE );
			CASE_ENUM_STRING( CL_INVALID_KERNEL_NAME );
			CASE_ENUM_STRING( CL_INVALID_KERNEL_DEFINITION );
			CASE_ENUM_STRING( CL_INVALID_KERNEL );
			CASE_ENUM_STRING( CL_INVALID_ARG_INDEX );
			CASE_ENUM_STRING( CL_INVALID_ARG_VALUE );
			CASE_ENUM_STRING( CL_INVALID_ARG_SIZE );
			CASE_ENUM_STRING( CL_INVALID_KERNEL_ARGS );
			CASE_ENUM_STRING( CL_INVALID_WORK_DIMENSION );
			CASE_ENUM_STRING( CL_INVALID_WORK_GROUP_SIZE );
			CASE_ENUM_STRING( CL_INVALID_WORK_ITEM_SIZE );
			CASE_ENUM_STRING( CL_INVALID_GLOBAL_OFFSET );
			CASE_ENUM_STRING( CL_INVALID_EVENT_WAIT_LIST );
			CASE_ENUM_STRING( CL_INVALID_EVENT );
			CASE_ENUM_STRING( CL_INVALID_OPERATION );
			CASE_ENUM_STRING( CL_INVALID_GL_OBJECT );
			CASE_ENUM_STRING( CL_INVALID_BUFFER_SIZE );
			CASE_ENUM_STRING( CL_INVALID_MIP_LEVEL );
			CASE_ENUM_STRING( CL_INVALID_GLOBAL_WORK_SIZE );
			CASE_ENUM_STRING( CL_INVALID_PROPERTY );
			CASE_ENUM_STRING( CL_INVALID_IMAGE_DESCRIPTOR );
			CASE_ENUM_STRING( CL_INVALID_COMPILER_OPTIONS );
			CASE_ENUM_STRING( CL_INVALID_LINKER_OPTIONS );
			CASE_ENUM_STRING( CL_INVALID_DEVICE_PARTITION_COUNT );
	}
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "Unknown cl error " << Error;
	return Unknown.str();
}


std::string Opencl::EnumToString_EventStatus(cl_uint Error)
{
#define CASE_ENUM_STRING(e)	case (e):	return #e;
	switch ( Error )
	{
			//	extension errors
			CASE_ENUM_STRING( CL_COMPLETE );
			CASE_ENUM_STRING( CL_RUNNING );
			CASE_ENUM_STRING( CL_SUBMITTED );
			CASE_ENUM_STRING( CL_QUEUED );
	}
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "Unknown cl event status " << Error;
	return Unknown.str();
}

void GetPlatforms(ArrayBridge<cl_platform_id>&& Platforms)
{
	cl_platform_id PlatformBuffer[100];
	cl_uint PlatformCount = 0;
	auto Error = clGetPlatformIDs( sizeofarray(PlatformBuffer), PlatformBuffer, &PlatformCount );
	if ( !Opencl::IsOkay( Error, "Failed to get opencl platforms" ) )
		return;

	PlatformCount = std::min<cl_uint>( sizeof(PlatformBuffer), PlatformCount );
	
	for ( int p=0;	p<PlatformCount;	p++ )
		Platforms.PushBack( PlatformBuffer[p] );
}

std::string GetString(cl_platform_id Platform,cl_platform_info Info)
{
	cl_char Buffer[100] = {'\0'};
	auto Error = clGetPlatformInfo( Platform, Info, sizeof(Buffer), Buffer, nullptr );
	if ( !Opencl_IsOkay( Error ) )
		return "Error";
	return std::string( reinterpret_cast<char*>(Buffer) );
}

std::string GetString(cl_device_id Device,cl_device_info Info)
{
	cl_char Buffer[100] = {'\0'};
	size_t RealSize = 0;
	auto Error = clGetDeviceInfo( Device, Info, sizeof(Buffer), Buffer, &RealSize );

	//	gr: this can return invalid value if the string is larger than the buffer
	if ( RealSize > sizeof(Buffer ) && Error == CL_INVALID_VALUE )
	{
		Array<cl_char> BigBuffer(RealSize);
		auto Error = clGetDeviceInfo( Device, Info, BigBuffer.GetDataSize(), BigBuffer.GetArray(), &RealSize );

		if ( !Opencl_IsOkay( Error ) )
			return "Error";
		return std::string( reinterpret_cast<char*>( BigBuffer.GetArray() ) );
	}
	
	if ( !Opencl_IsOkay( Error ) )
		return "Error";

	return std::string( reinterpret_cast<char*>(Buffer) );
}

template<typename TYPE>
void GetValue(cl_device_id Device,cl_device_info Info,TYPE& Value)
{
	size_t RealSize = 0;
	auto Error = clGetDeviceInfo( Device, Info, sizeof(TYPE), &Value, &RealSize );
	Opencl_IsOkay( Error );
}


Opencl::TDeviceMeta::TDeviceMeta(cl_device_id Device) :
	mDevice						( Device ),
	mHasOpenglInteroperability	( false ),
	mHasOpenglSyncSupport		( false )
{
	mVendor = GetString( Device, CL_DEVICE_VENDOR );
	mName = GetString( Device, CL_DEVICE_NAME );
	mDriverVersion = GetString( Device, CL_DRIVER_VERSION );
	std::string DeviceVersion = GetString( Device, CL_DEVICE_VERSION );
	mProfile = GetString( Device, CL_DEVICE_PROFILE );

	auto Extensions = GetString( Device, CL_DEVICE_EXTENSIONS );
	Soy::StringSplitByMatches( GetArrayBridge(mExtensions), Extensions, " " );

#if defined(TARGET_OSX)
	mHasOpenglInteroperability = mExtensions.Find("cl_APPLE_gl_sharing");
#endif
	
	mHasOpenglSyncSupport = mExtensions.Find("cl_khr_gl_event");
	
	//	extract version
	mVersion = Soy::TVersion( DeviceVersion, "OpenCL " );
	
	std::Debug << "Device " << mName << "(" << mDriverVersion << ") extensions: " << Extensions << std::endl;

	cl_device_type Type = OpenclDevice::Invalid;
	GetValue( Device, CL_DEVICE_TYPE, Type );
	mType = OpenclDevice::Validate( static_cast<OpenclDevice::Type>(Type) );
	
	GetValue( Device, CL_DEVICE_MAX_COMPUTE_UNITS, maxComputeUnits );
	GetValue( Device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, maxWorkItemDimensions );
	GetValue( Device, CL_DEVICE_MAX_WORK_ITEM_SIZES, maxWorkItemSizes );
	GetValue( Device, CL_DEVICE_MAX_WORK_GROUP_SIZE, maxWorkGroupSize );
	GetValue( Device, CL_DEVICE_MAX_CLOCK_FREQUENCY, maxClockFrequency );
	GetValue( Device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, maxMemAllocSize );
	GetValue( Device, CL_DEVICE_IMAGE_SUPPORT, imageSupport );
	GetValue( Device, CL_DEVICE_MAX_READ_IMAGE_ARGS, maxReadImageArgs );
	GetValue( Device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, maxWriteImageArgs );
	GetValue( Device, CL_DEVICE_IMAGE2D_MAX_WIDTH, image2dMaxWidth );
	GetValue( Device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, image2dMaxHeight );
	GetValue( Device, CL_DEVICE_IMAGE3D_MAX_WIDTH, image3dMaxWidth );
	GetValue( Device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, image3dMaxHeight );
	GetValue( Device, CL_DEVICE_IMAGE3D_MAX_DEPTH, image3dMaxDepth );
	GetValue( Device, CL_DEVICE_MAX_SAMPLERS, maxSamplers );
	GetValue( Device, CL_DEVICE_MAX_PARAMETER_SIZE, maxParameterSize );
	GetValue( Device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, globalMemCacheSize );
	GetValue( Device, CL_DEVICE_GLOBAL_MEM_SIZE, globalMemSize );
	GetValue( Device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, maxConstantBufferSize );
	GetValue( Device, CL_DEVICE_MAX_CONSTANT_ARGS, maxConstantArgs );
	GetValue( Device, CL_DEVICE_LOCAL_MEM_SIZE, localMemSize );
	GetValue( Device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, errorCorrectionSupport );
	GetValue( Device, CL_DEVICE_PROFILING_TIMER_RESOLUTION, profilingTimerResolution );
	GetValue( Device, CL_DEVICE_ENDIAN_LITTLE, endianLittle );
	GetValue( Device, CL_DEVICE_ADDRESS_BITS, deviceAddressBits );


	//	some explicit checks
	if ( maxWorkGroupSize < 1 )
	{
		std::stringstream Error;
		Error << "Max work group size (" << maxWorkGroupSize << ") < 1";
		maxWorkGroupSize = 1;
		throw Soy::AssertException( Error.str() );
	}
}


std::ostream& operator<<(std::ostream &out,const Opencl::TPlatform& in)
{
	out << in.mName << " ";
	out << in.mVendor << " ";
	return out;
}

size_t Opencl::TDeviceMeta::GetMaxGlobalWorkGroupSize() const
{
	//	gr: old code used below, but this looks right. need to figure out why I used the device address bits!?
	return maxWorkGroupSize;
	/*
	int DeviceAddressBits = mDeviceInfo.deviceAddressBits;
	int MaxSize = (1<<(DeviceAddressBits-1))-1;
	//	gr: if this is negative opencl kernels lock up (or massive loops?), code should bail out beforehand
	assert( MaxSize > 0 );
	return MaxSize;
	 */
}


Opencl::TPlatform::TPlatform(cl_platform_id Platform) :
	mPlatform	( Platform )
{
	mVersion = GetString( Platform, CL_PLATFORM_VERSION );
	mName = GetString( Platform, CL_PLATFORM_NAME );
	mVendor = GetString( Platform, CL_PLATFORM_VENDOR );
}





void Opencl::TPlatform::EnumDevices(std::function<void(const TDeviceMeta&)> EnumDevice,OpenclDevice::Type Filter)
{
	cl_device_id DeviceBuffer[100];
	cl_uint DeviceCount = 0;
	auto err = clGetDeviceIDs( mPlatform, Filter, sizeofarray(DeviceBuffer), DeviceBuffer, &DeviceCount );
	
	if ( err != CL_SUCCESS )
	{
		std::stringstream Error;
		Error << "Failed to get devices on " << (*this) << ": " << Opencl::GetErrorString(err);
		throw Soy::AssertException( Error.str() );
		return;
	}
	
	for ( int d=0;	d<DeviceCount;	d++ )
	{
		TDeviceMeta Meta( DeviceBuffer[d] );
		EnumDevice( Meta );
	}
}

void Opencl::TPlatform::GetDevices(ArrayBridge<TDeviceMeta>& Metas,OpenclDevice::Type Filter)
{
	auto EnumDevice = [&](const TDeviceMeta& DeviceMeta)
	{
		Metas.PushBack(DeviceMeta);
	};
	EnumDevices( EnumDevice, Filter );
}
	

void Opencl::GetDevices(ArrayBridge<TDeviceMeta>&& Metas,OpenclDevice::Type Filter)
{
	auto EnumDevice = [&](const TDeviceMeta& DeviceMeta)
	{
		Metas.PushBack(DeviceMeta);
	};
	EnumDevices( EnumDevice );
}


void Opencl::EnumDevices(std::function<void(const TDeviceMeta&)> EnumDevice)
{
	//	windows AMD sdk/ati radeon driver implementation doesn't accept NULL as a platform ID, so fetch the list of platforms first
	Array<cl_platform_id> Platforms;
	GetPlatforms( GetArrayBridge(Platforms) );
	
	//	collect devices
	for ( int p=0;	p<Platforms.GetSize();	p++ )
	{
		TPlatform Platform(Platforms[p]);
		
		Platform.EnumDevices( EnumDevice, OpenclDevice::GPU );
		Platform.EnumDevices( EnumDevice, OpenclDevice::CPU );
	}

}



Opencl::TDevice::TDevice(const ArrayBridge<cl_device_id>& Devices,Opengl::TContext* OpenglContext) :
	mContext				( nullptr ),
	mSharedOpenglContext	( nullptr )
{
	CreateContext( Devices, OpenglContext );
}


Opencl::TDevice::TDevice(const ArrayBridge<TDeviceMeta>& Devices,Opengl::TContext* OpenglContext) :
	mContext				( nullptr ),
	mSharedOpenglContext	( nullptr )
{
	Array<cl_device_id> DeviceIds;
	for ( int i=0;	i<Devices.GetSize();	i++ )
		DeviceIds.PushBack( Devices[i].mDevice );

	CreateContext( GetArrayBridge(DeviceIds), OpenglContext );
}

void Opencl::TDevice::CreateContext(const ArrayBridge<cl_device_id>& Devices,Opengl::TContext* OpenglContext)
{
	if ( Devices.IsEmpty() )
		throw Soy::AssertException("No devices provided");
	
	
	//	gr: just warn?
	//	gr: filter out non interop devices?
	if ( OpenglContext )
	{
	}
	
	//	create context
	//	if we specify any properties we need a platform (and a terminator)
	Array<cl_context_properties> Properties;

#if defined(TARGET_OSX)
	if ( OpenglContext )
	{
		auto CGLContext = OpenglContext->GetPlatformContext();
		CGLShareGroupObj CGLShareGroup = CGLGetShareGroup( CGLContext );
		Properties.PushBack( CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE );
		Properties.PushBack( (cl_context_properties)CGLShareGroup );
	}
#endif

	//	add c-array style list terminator
	if ( !Properties.IsEmpty() )
		Properties.PushBack(0);

	cl_int err;
	mContext = clCreateContext( Properties.GetArray(), size_cast<cl_uint>(Devices.GetSize()), Devices.GetArray(), nullptr, nullptr, &err );
	Opencl::IsOkay( err, "clCreateContext failed" );
	Soy::Assert( mContext != nullptr, "clCreateContext failed to return a context" );
	
	mDevices.Copy( Devices );
	
#if defined(TARGET_OSX)
	if ( OpenglContext )
	{
		mSharedOpenglContext = OpenglContext->GetPlatformContext();
	}
#endif
}


Opencl::TDevice::~TDevice()
{
	if ( mContext )
	{
		clReleaseContext( mContext );
		mContext = nullptr;
	}
}

bool Opencl::TDevice::HasInteroperability(Opengl::TContext &Opengl)
{
#if defined(TARGET_OSX)
	auto Context = Opengl.GetPlatformContext();
	return mSharedOpenglContext == Context;
#else
	return false;
#endif
}

std::shared_ptr<Opencl::TContext> Opencl::TDevice::CreateContext()
{
	if ( mDevices.IsEmpty() )
		return nullptr;
	
	//	pick a device
	cl_device_id Device = mDevices.GetBack().mDevice;
	
	//	create a queue
	std::shared_ptr<TContext> Context( new TContext( *this, Device ) );
	return Context;
}

std::shared_ptr<Opencl::TContextThread> Opencl::TDevice::CreateContextThread(const std::string& Name)
{
	if ( mDevices.IsEmpty() )
		return nullptr;
	
	//	pick a device
	cl_device_id Device = mDevices.GetBack().mDevice;
	
	//	create a queue
	std::shared_ptr<TContextThread> Context( new TContextThread( Name, *this, Device ) );
	return Context;
}


Opencl::TContext::TContext(TDevice& Device,cl_device_id SubDevice) :
	mQueue		( nullptr ),
	mDeviceMeta	( SubDevice ),
	mDevice		( Device )
{
	cl_command_queue_properties Properties = 0;
	cl_int Error;
	mQueue = clCreateCommandQueue( mDevice.GetClContext(), mDeviceMeta.mDevice, Properties, &Error );
	Opencl_IsOkay( Error );
	Soy::Assert( mQueue != nullptr, "Failed to create opencl command queue" );
}

Opencl::TContext::~TContext()
{
	if ( mQueue )
	{
		clReleaseCommandQueue( mQueue );
		mQueue = nullptr;
	}
}

bool Opencl::TContext::IsLocked(std::thread::id Thread)
{
	//	check for any thread
	if ( Thread == std::thread::id() )
		return mLockedThread!=std::thread::id();
	else
		return mLockedThread == Thread;
}

bool Opencl::TContext::Lock()
{
	//	need to set thread for cl_queue?

	//	same as Opengl::TContext::Lock()
	Soy::Assert( mLockedThread == std::thread::id(), "context already locked" );
	mLockedThread = std::this_thread::get_id();
	return true;
}

void Opencl::TContext::Unlock()
{
	//	same as Opengl::TContext::Unlock()
	auto ThisThread = std::this_thread::get_id();
	Soy::Assert( mLockedThread != std::thread::id(), "context not locked to wrong thread" );
	Soy::Assert( mLockedThread == ThisThread, "context not unlocked from wrong thread" );
	mLockedThread = std::thread::id();
}



void GetBuildLog(cl_program Program,cl_device_id Device,std::ostream& Log)
{
	size_t Length = 0;
	auto LengthError = clGetProgramBuildInfo( Program, Device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &Length );
	Opencl_IsOkay(LengthError);
	
	//	no error?
	if ( Length == 0 )
	{
		Log << "<no build log>";
		return;
	}
	
	Array<char> Buffer( Length+1 );
	auto BuildLogError = clGetProgramBuildInfo( Program, Device, CL_PROGRAM_BUILD_LOG, Buffer.GetSize(), Buffer.GetArray(), nullptr );
	Opencl_IsOkay(BuildLogError);
	Buffer[Length] = '\0';
	
	//	errors might contain % symbols, which will screw up the va_args system when it tries to parse them...
	//std::replace( buffer.begin(), buffer.end(), '%', '@' );
	
	Log << Buffer.GetArray();
}

Opencl::TProgram::TProgram(const std::string& Source,TContext& Context) :
	mProgram	( nullptr )
{
	const char* Lines[] =
	{
		Source.c_str(),
	};
	cl_int Error = 0;
	mProgram = clCreateProgramWithSource( Context.GetContext(), sizeofarray(Lines), Lines, nullptr, &Error );
	Opencl_IsOkay(Error);
	Soy::Assert( mProgram != nullptr, "creating program failed" );
	
	//	now compile
	std::stringstream BuildArguments;
	
	//	add kernel info
	BuildArguments << BuildOption_KernelInfo << ' ';
	
	Array<std::string> Paths;
	Paths.PushBack(".");
	for ( int p=0;	p<Paths.GetSize();	p++)
	{
		auto Path = Paths[p];
		//	amd APP sdk on windows requires paths with forward slashes
		std::replace( Path.begin(), Path.end(), '\\', '/' );
		BuildArguments << "-I \"" << Path << "\" ";
	}

	//	build for context's device
	BufferArray<cl_device_id,1> Devices;
	Devices.PushBack( Context.GetDevice().mDevice );
	cl_int Err = clBuildProgram( mProgram, size_cast<cl_uint>(Devices.GetSize()), Devices.GetArray(), BuildArguments.str().c_str(), nullptr, nullptr );
	
	//	get log
	std::stringstream BuildLog;
	GetBuildLog( mProgram, Devices[0], BuildLog );

	//	throw if error
	if ( Err != CL_SUCCESS )
	{
		//	gr: if you're here, possible problem: a kernel with no params doesn't compile on some Radeons";
		//	https://github.com/nengo/nengo-ocl/issues/51
		std::stringstream Error;
		Error << "Failed to compile kernel; " << BuildLog.str() << "; gr: possible problem: a kernel with no params doesn't compile on some Radeons";
		throw Soy::AssertException( Error.str() );
	}
	
	//	print any warnings
	if ( !BuildLog.str().empty() )
		std::Debug << "Opencl kernel build log: " << BuildLog.str() << std::endl;

}

Opencl::TProgram::~TProgram()
{
	if ( mProgram )
	{
		clReleaseProgram( mProgram );
		mProgram = nullptr;
	}
}


std::string GetArgValue(cl_kernel Kernel,cl_uint ArgIndex,cl_kernel_arg_info Info)
{
	//	gr: this returns an empty string, but no error (length = 1) if -cl-kernel-arg-info is missing
	
	//	get string length
	size_t NameLength = 0;
	auto Error = clGetKernelArgInfo( Kernel, ArgIndex, Info, 0, nullptr, &NameLength );
	Opencl_IsOkay( Error );
	
	Array<char> NameBuffer( NameLength + 1 );
	Error = clGetKernelArgInfo( Kernel, ArgIndex, Info, NameBuffer.GetDataSize(), NameBuffer.GetArray(), nullptr );
	Opencl_IsOkay( Error );

	//	just in case
	NameBuffer.GetBack() = '\0';
	return std::string( NameBuffer.GetArray() );
}

Opencl::TKernel::TKernel(const std::string& Kernel,TProgram& Program) :
	mKernelName		( Kernel ),
	mKernel			( nullptr ),
	mLockedContext	( nullptr )
{
	cl_int Error;
	mKernel = clCreateKernel( Program.mProgram, Kernel.c_str(), &Error );
	Opencl_IsOkay( Error );
	Soy::Assert( mKernel!=nullptr, "Expected kernel" );
	
	//	enum kernel args
	cl_uint ArgCount = 0;
	Error = clGetKernelInfo( mKernel, CL_KERNEL_NUM_ARGS, sizeof(ArgCount), &ArgCount, nullptr );
	Opencl::IsOkay( Error, "CL_KERNEL_NUM_ARGS" );

	for ( int i=0;	i<ArgCount;	i++ )
	{
		auto Name = GetArgValue( mKernel, i, CL_KERNEL_ARG_NAME );
		auto Type = GetArgValue( mKernel, i, CL_KERNEL_ARG_TYPE_NAME );
		TUniform Uniform( Name, Type, i );
		mUniforms.PushBack( Uniform );
	}
}

Opencl::TKernel::~TKernel()
{
	//	wait for lock to be finished
	std::lock_guard<std::mutex> Lock( mLock );
	if ( mKernel )
	{
		clReleaseKernel( mKernel );
		mKernel = nullptr;
	}
}

Opencl::TKernelState Opencl::TKernel::Lock(TContext& Context)
{
	mLock.lock();
	Soy::Assert( mLockedContext == nullptr, "Expecting null locked context" );
	mLockedContext = &Context;
	return TKernelState(*this);
}

void Opencl::TKernel::Unlock()
{
	mLockedContext = nullptr;
	mLock.unlock();
}

Opencl::TContext& Opencl::TKernel::GetContext()
{
	Soy::Assert( mLockedContext != nullptr, "Expecting locked context" );
	return *mLockedContext;
}

Opencl::TUniform Opencl::TKernel::GetUniform(const char* Name) const
{
	auto* Uniform = mUniforms.Find( Name );
	return Uniform ? *Uniform : TUniform();
}


Opencl::TSync::TSync() :
	mEvent	( nullptr )
{
}

void Opencl::TSync::Release()
{
	if ( mEvent )
	{
		clReleaseEvent( mEvent );
		mEvent = nullptr;
	}
}

void Opencl::TSync::Wait()
{
	if ( !mEvent )
		return;
	
	cl_uint EventRefCount = 0;
	auto InfoError = clGetEventInfo( mEvent, CL_EVENT_REFERENCE_COUNT, sizeof(EventRefCount), &EventRefCount, nullptr );
	Opencl_IsOkay( InfoError );

	cl_int EventStatus = CL_SUCCESS;
	auto GetEventInfoErr = clGetEventInfo( mEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(EventStatus), &EventStatus, nullptr );
	Opencl_IsOkay( GetEventInfoErr );
	//std::Debug << "Event status: " << EnumToString_EventStatus( EventStatus ) << std::endl;
	
	auto WaitError = clWaitForEvents( 1, &mEvent );

	//	there was an error in execution, let's find what it was
	if ( WaitError == CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST )
	{
		cl_int ExecutionError = CL_SUCCESS;
		auto GetEventInfoErr = clGetEventInfo( mEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(ExecutionError), &ExecutionError, nullptr );
		Opencl::IsOkay( GetEventInfoErr, "Get clWaitForEvents info-error" );
		Opencl::IsOkay( ExecutionError, "Error in execution which we're waiting on" );
	}
	else
	{
		Opencl_IsOkay( WaitError );
	}
}

Opencl::TKernelState::TKernelState(TKernel& Kernel) :
	mKernel	( Kernel )
{
}

Opencl::TKernelState::~TKernelState()
{
	mKernel.Unlock();
}

Opencl::TUniform Opencl::TKernelState::GetUniform(const std::string& Name) const
{
	auto Uniform = mKernel.GetUniform( Name.c_str() );
	if ( !Uniform.IsValid() )
	{
		std::stringstream Error;
		Error << "No uniform matching " << Name;
		throw Soy::AssertException( Error.str() );
	}
	return Uniform;
}


template<typename TYPE>
bool SetKernelArg(Opencl::TKernelState& Kernel,const char* Name,const TYPE& Value)
{
	//	as per opengl, silent error if uniform doesn't exist
	auto Uniform = Kernel.mKernel.GetUniform(Name);
	if ( !Uniform.IsValid() )
		return false;
	
	//	gr: ByNameAPPLE will fail if we haven't compiled kernel info
	//		Opencl::BuildOption_KernelInfo
	auto Error = clSetKernelArgByNameAPPLE( Kernel.mKernel.mKernel, Name, sizeof(TYPE), &Value );
	
	std::stringstream ErrorString;
	ErrorString << "SetKernelArg(" << Name << ", " << Soy::GetTypeName<TYPE>() << ")";
	Opencl::IsOkay(Error, ErrorString.str() );
	return true;
}



cl_channel_order Opencl::GetImageChannelOrder(SoyPixelsFormat::Type Format,cl_channel_type& DataType)
{
	switch ( Format )
	{
		case SoyPixelsFormat::Greyscale:		return CL_R;
		case SoyPixelsFormat::GreyscaleAlpha:	return CL_RA;
		case SoyPixelsFormat::RGB:				return CL_RGB;
		case SoyPixelsFormat::RGBA:				return CL_RGBA;
		case SoyPixelsFormat::BGRA:				return CL_BGRA;
	//	case SoyPixelsFormat::BGR:				return CL_BGR;
			
		case SoyPixelsFormat::KinectDepth:
		case SoyPixelsFormat::FreenectDepth10bit:
		case SoyPixelsFormat::FreenectDepth11bit:
		case SoyPixelsFormat::FreenectDepthmm:
			DataType = CL_UNORM_INT16;
			return CL_R;
			
		default:
		{
			std::stringstream Error;
			Error << "Unsupported pixelformat for opencl; " << Format;
			throw Soy::AssertException( Error.str() );
		}
	}
}

cl_image_format Opencl::GetImageFormat(SoyPixelsFormat::Type Format)
{
	cl_image_format FormatCl;
	FormatCl.image_channel_data_type = CL_UNORM_INT8;
	FormatCl.image_channel_order = Opencl::GetImageChannelOrder( Format, FormatCl.image_channel_data_type );
	return FormatCl;
}


Opencl::TBuffer::TBuffer(const std::string& DebugName) :
	mMem		( nullptr ),
	mBufferSize	( 0 ),
	mDebugName	( DebugName )
{
	
}

Opencl::TBuffer::TBuffer(size_t Size,TContext& Context,const std::string& DebugName) :
	mMem		( nullptr ),
	mBufferSize	( 0 ),
	mDebugName	( DebugName )
{
	//	gr: we cannot create a CL_BUFFER with zero bytes, but I want to allow it in the middleware, so bail early
	//		may abort this in future as we can't use the clmem anyway if its null and we'll just get INVALID_ARGS when we try and execute
	if ( Size == 0 )
		return;

	//	this will just return CL_INVALID_BUFFER_SIZE, but catching it early might be handy
	Soy::Assert( Size > 0, "Cannot create CL buffer of zero bytes");
	
	cl_mem_flags Flags = CL_MEM_READ_WRITE;
	void* HostPtr = nullptr;

	cl_int Error = CL_SUCCESS;
	mMem = clCreateBuffer( Context.GetContext(), Flags, Size, HostPtr, &Error );
	std::stringstream ErrorString;
	ErrorString << __func__ << "(" << Soy::FormatSizeBytes(Size) << ")";
	Opencl::IsOkay( Error, ErrorString.str() );

	Soy::Assert( mMem != nullptr, "clCreateBuffer success but no mem object" );
	mBufferSize = Size;
}

Opencl::TBuffer::~TBuffer()
{
	if ( mMem )
	{
		//std::Debug << "Deleting cl_mem " << mDebugName << std::endl;
		
		auto Error = clReleaseMemObject( mMem );
		Opencl::IsOkay( Error, "clReleaseMemObject" );
		mMem = nullptr;
		mBufferSize = 0;
	}
}


void Opencl::TBuffer::ReadImpl(ArrayInterface<uint8>& Array,TContext& Context,TSync* Sync)
{
	Soy::Assert( mMem != nullptr, mDebugName + " Mem buffer expected" );
	
	//	todo; check size of mem. We should store this on mMem set
	size_t StartBytes = 0;
	size_t ByteCount = Array.GetDataSize();
	static bool Blocking = true;
	
	//	gr: sync's from clEnqueueReadBuffer never seem to finish if bytecount is 0. Should bail out earlier, but just in case...
	if ( ByteCount == 0 )
	{
		Soy::Assert( Array.GetArray() == nullptr, "Array is not null (move to unit test!)" );
		//	end event, just in case its setup
		if ( Sync )
			Sync->Release();
		return;
	}
	
	auto* ArrayPtr = Array.GetArray();
	Soy::Assert( ArrayPtr != nullptr, "Array should not be empty if bytecount>0" );

	if ( ByteCount > mBufferSize )
	{
		std::stringstream Error;
		Error << "Trying to read " << Soy::FormatSizeBytes(ByteCount) << " more than buffer size " << Soy::FormatSizeBytes(mBufferSize) << std::endl;
		throw Soy::AssertException( Error.str() );
		//ByteCount = mBufferSize;
	}

	auto Error = clEnqueueReadBuffer( Context.GetQueue(), mMem, Blocking, StartBytes, ByteCount, ArrayPtr, 0, nullptr, Sync ? &Sync->mEvent : nullptr );
	std::stringstream ErrorString;
	ErrorString << "TBuffer::Read(" << ByteCount << " bytes)";
	Opencl::IsOkay( Error, ErrorString.str() );
}

void Opencl::TBuffer::Write(const uint8 *Array,size_t Size,TContext& Context,Opencl::TSync *Sync)
{
	Soy::Assert( mMem != nullptr, mDebugName + " Mem buffer expected" );
	Soy::Assert( Array != nullptr, mDebugName + " Array should not be empty" );
	Soy::Assert( Size <= mBufferSize, mDebugName + " Trying to write more bytes than in buffer");

	bool Blocking = false;
	size_t StartBytes = 0;
	Size = std::min( Size, mBufferSize );

	auto Error = clEnqueueWriteBuffer( Context.GetQueue(), mMem, Blocking, StartBytes, Size, Array, 0, nullptr,  Sync ? &Sync->mEvent : nullptr );
	std::stringstream ErrorString;
	ErrorString << mDebugName << " TBuffer::Write(" << Size << " bytes)";
	Opencl::IsOkay( Error, ErrorString.str() );
}



Opencl::TBufferImage::TBufferImage(const SoyPixelsMeta& Meta,TContext& Context,const SoyPixelsImpl* ClientStorage,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,TSync* Semaphore) :
	TBuffer			( DebugName + " (TBufferImage)" ),
	mContext		( Context ),
	mOpenglObject	( nullptr ),
	mMeta			( Meta )
{
	Soy::Assert( mMem == nullptr, "clmem already allocated");
	
	auto& Device = Context.GetDevice();
	Soy::Assert( Device.imageSupport, "Device doesn't support images" );
	if ( Device.image2dMaxWidth < Meta.GetWidth() || Device.image2dMaxHeight < Meta.GetHeight() )
	{
		std::stringstream Error;
		Error << "Image(" << Meta << ") is too big for device(" << Device.image2dMaxWidth << "x" << Device.image2dMaxHeight << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	auto Format = Opencl::GetImageFormat( Meta.GetFormat() );
	cl_image_desc Description;
	Description.image_type = CL_MEM_OBJECT_IMAGE2D;
	Description.image_width = Meta.GetWidth();
	Description.image_height = Meta.GetHeight();
	Description.image_depth = 1;
	Description.image_array_size = 1;
	Description.image_row_pitch = 0;
	Description.image_slice_pitch = 0;
	Description.num_mip_levels = 0;
	Description.num_samples = 0;
	Description.buffer = nullptr;
	
	cl_mem_flags Flags = ReadWrite;
	Flags |= ClientStorage ? CL_MEM_USE_HOST_PTR : CL_MEM_ALLOC_HOST_PTR;

	//	note: should combine const & read only flags
	const void* DataConst = ClientStorage ? static_cast<const void*>(ClientStorage->GetPixelsArray().GetArray()) : nullptr;
	void* Data = const_cast<void*>( DataConst );
	cl_int Error;
	mMem = clCreateImage( Context.GetContext(), Flags, &Format, &Description, Data, &Error );
	Opencl::IsOkay( Error, "clCreateImage" );
}

Opencl::TBufferImage::TBufferImage(const SoyPixelsImpl& Image,TContext& Context,bool ClientStorage,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,Opencl::TSync* Semaphore) :
	TBufferImage		( Image.GetMeta(), Context, ClientStorage ? &Image : nullptr, ReadWrite, DebugName, Semaphore )
{
	if ( ReadWrite != OpenclBufferReadWrite::WriteOnly )
		Write( Image, Semaphore );
}


Opencl::TBufferImage::TBufferImage(const Opengl::TTexture& Texture,Opengl::TContext& OpenglContext,TContext& Context,OpenclBufferReadWrite::Type ReadWrite,const std::string& DebugName,TSync* Semaphore) :
	TBuffer			( DebugName + " (TBufferImage)" ),
	mContext		( Context ),
	mOpenglObject	( nullptr ),
	mMeta			( Texture.mMeta )
{
	try
	{
		if ( Context.HasInteroperability(OpenglContext) )
		{
			cl_mem_flags MemFlags = ReadWrite;
			cl_GLint MipLevel = 0;
			cl_int Error = CL_SUCCESS;
			Soy::Assert( mMem == nullptr, "clmem already allocated");
			mMem = clCreateFromGLTexture( mContext.GetContext(), MemFlags, Texture.mType, MipLevel, Texture.mTexture.mName, &Error );
			Opencl::IsOkay( Error, "clCreateFromGLTexture" );

			//	before we lock opengl objects, we need to flush/sync them
			//	gr: use cl_khr_gl_sync to sync individual textures. apparently we're supposed to flush with gl finish, but flush works?
			static bool FlushIfOpenglBusy = true;
			bool DoFlush = true;

			//	gr: to avoid deadlock when this is called thunked from opengl, don't flush if the context is locked in a job
			//	note: we are NOT in the locked thread, so check if locked to any
			//	check this doesn't cause corruption
			if ( !FlushIfOpenglBusy )
			{
				if ( OpenglContext.IsLockedToAnyThread() )
				{
					//std::Debug << "Skipping flush as context is locked to a job" << std::endl;
					DoFlush = false;
				}
			}
			
			BufferArray<cl_event,1> PreAcquireEvents;
		
			if ( DoFlush )
			{
				static int FlushTimerMin = 10;
				static bool TextureSyncCl = false;
				static bool TextureSync = true;
				static bool FlushApple = false;
				static bool Flush = false;
				static bool Finish = false;
				
				bool DoTextureSync = ( TextureSync && Texture.mWriteSync );
				bool DoTextureSyncCl = ( TextureSyncCl && Context.GetDevice().mHasOpenglSyncSupport && Texture.mWriteSync );
				
				std::stringstream SyncTimerName;
				SyncTimerName << "Opengl -> Opencl flush sync " << DebugName;
				if ( DoTextureSyncCl )
					SyncTimerName << " TEXTURE SYNC GL->CL";
				else if ( DoTextureSync )
					SyncTimerName << " TEXTURE SYNC";
				
				Soy::TScopeTimerPrint Timer(SyncTimerName.str().c_str(),FlushTimerMin);
				Soy::TSemaphore Sync;
				
				if ( DoTextureSyncCl )
				{
					cl_int Error = ~CL_SUCCESS;
					auto Event = clCreateEventFromGLsyncKHR( Context.GetContext(), Texture.mWriteSync->mSyncObject, &Error );
					Opencl::IsOkay( Error, "clCreateEventFromGLsyncKHR" );
					PreAcquireEvents.PushBack( Event );

					//	so we don't wait on this sync
					Sync.OnCompleted();
				}
				else if ( DoTextureSync )
				{
					auto TextureSync = [&Texture]
					{
						Texture.mWriteSync->Wait();
					};
					OpenglContext.PushJob( TextureSync, Sync );
				}
				else if ( FlushApple )
				{
					OpenglContext.PushJob( glFlushRenderAPPLE, Sync );
				}
				else if ( Flush )
				{
					OpenglContext.PushJob( glFlush, Sync );
				}
				else if ( Finish )
				{
					OpenglContext.PushJob( glFinish, Sync );
				}
				else
				{
					Sync.OnCompleted();
				}
				Sync.Wait();
			
			}
			
			//	lock opengl objects
			static bool DoAcquire = true;
			if ( DoAcquire )
			{
				static int AcquireTimerMin = 10;
				Soy::TScopeTimerPrint Timer("clEnqueueAcquireGLObjects",AcquireTimerMin);

				Error = clEnqueueAcquireGLObjects( mContext.GetQueue(), 1, &mMem, size_cast<cl_uint>(PreAcquireEvents.GetSize()), PreAcquireEvents.GetArray(), Semaphore ? &Semaphore->mEvent : nullptr );
				Opencl::IsOkay( Error, "clEnqueueAcquireGLObjects" );
			}

			//	gr: set this regardless of the lock
			mOpenglObject = &Texture;

			//	success!
			return;
		}
	}
	catch(std::exception& e)
	{
		std::Debug << "Failed to upload texture to opencl via opengl: " << e.what() << std::endl;
	}

	//	read texture to buffer and upload that
	Soy::TScopeTimerPrint Timer("TBufferImage from opengl texture via read pixels", 10 );
	
	SoyPixels Buffer;
	auto Read = [&Buffer,&Texture]
	{
		Texture.Read( Buffer );
	};
	Soy::TSemaphore ReadSemaphore;
	OpenglContext.PushJob( Read, ReadSemaphore );
	ReadSemaphore.Wait();

	*this = std::move( Opencl::TBufferImage( Buffer, Context, false, ReadWrite, mDebugName, Semaphore ) );
}

Opencl::TBufferImage& Opencl::TBufferImage::operator=(TBufferImage&& Move)
{
	if ( mContext != Move.mContext )
		throw Soy::AssertException("Trying to std::move a buffer image across contexts");
	
	if ( this != &Move )
	{
		if ( mMem )
			throw Soy::AssertException("Overwriting mem with std::move");

		mMem = Move.mMem;
		mOpenglObject = Move.mOpenglObject;
		mBufferSize = Move.mBufferSize;

		//	stolen resources
		Move.mMem = nullptr;
		Move.mOpenglObject = nullptr;
		Move.mBufferSize = 0;
	}
	return *this;
}

Opencl::TBufferImage::~TBufferImage()
{
	//	release any opengl objects in use
	if ( mOpenglObject )
	{
		Soy::Assert( mMem != nullptr, "Need to release GL object lock from membuffer, but mem is gone");
		
		TSync Sync;
		auto Error = clEnqueueReleaseGLObjects( mContext.GetQueue(), 1, &mMem, 0, nullptr, &Sync.mEvent );
		Opencl::IsOkay( Error, "clEnqueueReleaseGLObjects" );
		Sync.Wait();
		mOpenglObject = nullptr;
	}
}

void Opencl::TBufferImage::Write(const SoyPixelsImpl& Image,Opencl::TSync* Semaphore)
{
	size_t Origin[] = { 0,0,0 };
	size_t Region[] = { Image.GetWidth(), Image.GetHeight(), 1 };
	auto Queue = mContext.GetQueue();
	cl_bool Block = CL_TRUE;
	int image_row_pitch = 0;	// TODO
	int image_slice_pitch = 0;
	auto* Data = Image.GetPixelsArray().GetArray();
	auto* Event = Semaphore ? &Semaphore->mEvent : nullptr;

	cl_int Error = clEnqueueWriteImage( Queue, mMem, Block, Origin, Region, image_row_pitch, image_slice_pitch, Data, 0, nullptr, Event );
	Opencl::IsOkay( Error, __func__ );
}


void Opencl::TBufferImage::Write(TBufferImage& Image,Opencl::TSync* Semaphore)
{
	//	copy image to image
	size_t SourceOffset = 0;	//	mem offset, guess this needs calculating if we're doing a sub image
	size_t DestOrigin[] = { 0,0,0 };
	size_t DestRegion[] = { mMeta.GetWidth(), mMeta.GetHeight(), 1 };
	auto Queue = mContext.GetQueue();
	auto* Event = Semaphore ? &Semaphore->mEvent : nullptr;
	auto SourceBuffer = Image.mMem;
	auto DestBuffer = mMem;
	
	cl_int Error = clEnqueueCopyBufferToImage( Queue, SourceBuffer, DestBuffer, SourceOffset, DestOrigin, DestRegion, 0, nullptr, Event );
	
	std::stringstream ErrorContext;
	ErrorContext << "clEnqueueCopyBufferToImage " << this->mDebugName << " --> " << Image.mDebugName;
	Opencl::IsOkay( Error, ErrorContext.str() );
}


void Opencl::TBufferImage::Read(SoyPixelsImpl& Image,Opencl::TSync* Semaphore)
{
	size_t Origin[] = { 0,0,0 };
	size_t Region[] = { Image.GetWidth(), Image.GetHeight(), 1 };
	auto Queue = mContext.GetQueue();
	cl_bool Block = CL_TRUE;
	
	//	driver calcs pitch automatically
	size_t image_row_pitch = 0;
	image_row_pitch = Image.GetRowPitchBytes();
	size_t image_slice_pitch = 0;	//	must be 0 for 2D

	auto* Data = Image.GetPixelsArray().GetArray();
	auto* Event = Semaphore ? &Semaphore->mEvent : nullptr;

	auto Error = clEnqueueReadImage( Queue, mMem, Block, Origin, Region, image_row_pitch, image_slice_pitch, Data, 0, nullptr, Event );
	Opencl::IsOkay( Error, __func__ );
	
	clFinish(Queue);
	Opencl::IsOkay( Error, __func__ );
}


void Opencl::TBufferImage::Read(Opengl::TTexture& Image,Opencl::TSync* Semaphore)
{
	//	read-back the texture we were created with
	if ( mOpenglObject == &Image )
	{
		//	seems to just write immediately, with no need for any flushes...
		static bool Flush = false;
		if ( Flush )
		{
			//	gr: obviously this should be a fence for just this object
			Soy::TScopeTimerPrint Timer("Texture read-back with clFinish",1);
			auto Error = clFinish( mContext.GetQueue() );
			Opencl::IsOkay( Error, "Finish for texture read" );
		}
		return;
	}
	
	throw Soy::AssertException("Trying to read back buffer image to different texture, context required");
}



void Opencl::TBufferImage::Read(Opengl::TTextureAndContext& TextureAndContext,Opencl::TSync* Semaphore)
{
	//	read-back the texture we were created with
	if ( mOpenglObject == &TextureAndContext.mTexture )
	{
		//	seems to just write immediately, with no need for any flushes...
		return;
	}
	
	//	if texture is not valid, create it
	auto& Texture = TextureAndContext.mTexture;
	auto& Context = TextureAndContext.mContext;
	if ( !Texture.IsValid(false) )
	{
		auto CreateTexture = [&Texture,this]
		{
			Texture = std::move( Opengl::TTexture( mMeta, GL_TEXTURE_2D ) );
		};
		Soy::TSemaphore Semaphore;
		Context.PushJob( CreateTexture, Semaphore );
		Semaphore.Wait();
	}
	
	//	create a temporary buffer attached to the texture, and copy it
	Opencl::TBufferImage NewTextureBuffer( Texture, Context, this->mContext, OpenclBufferReadWrite::ReadWrite, std::string("Temporary image to copy from ")+mDebugName );
	NewTextureBuffer.Write( *this, Semaphore );
	NewTextureBuffer.Read( Texture, Semaphore );
}

bool Opencl::TKernelState::SetUniform(const char* Name,const Opengl::TTextureAndContext& Pixels,OpenclBufferReadWrite::Type ReadWriteMode)
{
	//	todo: get uniform and check type is image_2D_t
	//	make image buffer and set that
	Opencl::TSync Sync;
	std::shared_ptr<TBuffer> Buffer( new TBufferImage( Pixels.mTexture, Pixels.mContext, GetContext(), ReadWriteMode, std::string("Uniform ")+Name, &Sync ) );
	
	mBuffers.PushBack( std::make_pair(Name,Buffer) );
/*
	if ( mBuffers.find(Name) != mBuffers.end() )
		std::Debug << "Warning, setting buffer for uniform " << Name << " which already has a buffer..." << std::endl;
	mBuffers[Name] = Buffer;
	*/
	//	set kernel arg
	bool Result = SetKernelArg( *this, Name, Buffer->GetMemBuffer() );
	
	//	we can't tell when caller is going to release the pixels, so wait for it to finish
	Sync.Wait();
	
	OnAssignedUniform( Name, Result );
	return Result;
}


bool Opencl::TKernelState::SetUniform(const char* Name,const SoyPixelsImpl& Pixels,OpenclBufferReadWrite::Type ReadWriteMode)
{
	//	todo: get uniform and check type is image_2D_t
	//	make image buffer and set that
	Opencl::TSync Sync;
	std::shared_ptr<TBuffer> Buffer( new TBufferImage( Pixels, GetContext(), false, ReadWriteMode, std::string("Uniform ") + Name, &Sync ) );

	mBuffers.PushBack( std::make_pair(Name,Buffer) );
	/*
	if ( mBuffers.find(Name) != mBuffers.end() )
		std::Debug << "Warning, setting buffer for uniform " << Name << " which already has a buffer..." << std::endl;
	mBuffers[Name] = Buffer;
*/
	//	set kernel arg
	bool Result = SetKernelArg( *this, Name, Buffer->GetMemBuffer() );
	
	//	we can't tell when caller is going to release the pixels, so wait for it to finish
	Sync.Wait();
	
	OnAssignedUniform( Name, Result );
	return Result;
}


bool Opencl::TKernelState::SetUniform(const char* Name,const vec4f& Value)
{
	auto Value2 = Soy::VectorToCl( Value );
	bool Result = SetKernelArg( *this, Name, Value2 );
	OnAssignedUniform( Name, Result );
	return Result;
}

bool Opencl::TKernelState::SetUniform(const char* Name,const float& Value)
{
	bool Result = SetKernelArg( *this, Name, Value );
	OnAssignedUniform( Name, Result );
	return Result;
}

bool Opencl::TKernelState::SetUniform(const char* Name,const vec2f& Value)
{
	auto Value2 = Soy::VectorToCl( Value );
	bool Result = SetKernelArg( *this, Name, Value2 );
	OnAssignedUniform( Name, Result );
	return Result;
}

bool Opencl::TKernelState::SetUniform(const char* Name,const vec3f& Value)
{
	auto Value3 = Soy::VectorToCl( Value );
	bool Result = SetKernelArg( *this, Name, Value3 );
	OnAssignedUniform( Name, Result );
	return Result;
}

bool Opencl::TKernelState::SetUniform(const char* Name,const int& Value)
{
	static_assert( sizeof(cl_int) == sizeof(Value), "cl_int doesn't match int" );
	bool Result = SetKernelArg( *this, Name, Value );
	OnAssignedUniform( Name, Result );
	return Result;
}

bool Opencl::TKernelState::SetUniform(const char* Name,TBuffer& Buffer)
{
	//	gr: this fails if you assign a read uniform with a write-only buffer! catch this here!
	bool Result = SetKernelArg( *this, Name, Buffer.GetMemBuffer() );
	OnAssignedUniform( Name, Result );
	return Result;
}

Opencl::TBuffer& Opencl::TKernelState::GetUniformBuffer(const char* Name)
{
	for ( int i=0;	i<mBuffers.GetSize();	i++ )
	{
		auto& Pair = mBuffers[i];
		if ( Pair.first == Name )
			return *Pair.second;
	}
	
	std::stringstream Error;
	Error << "No uniform buffer named " << Name;
	throw Soy::AssertException( Error.str() );
}

void Opencl::TKernelState::ReadUniform(const char* Name,Opengl::TTextureAndContext& Texture)
{
	auto& Buffer = GetUniformBuffer( Name );

	//	check types etc!
	Opencl::TSync Semaphore;
	auto& BufferImage = dynamic_cast<TBufferImage&>( Buffer );
	BufferImage.Read( Texture.mTexture, &Semaphore );
	Semaphore.Wait();
}

void Opencl::TKernelState::ReadUniform(const char* Name,SoyPixelsImpl& Pixels)
{
	auto& Buffer = GetUniformBuffer( Name );
	
	//	check types etc!
	Opencl::TSync Semaphore;
	auto& BufferImage = dynamic_cast<TBufferImage&>( Buffer );
	BufferImage.Read( Pixels, &Semaphore );
	Semaphore.Wait();
}

void Opencl::TKernelState::ReadUniform(const char* Name,TBuffer& Buffer)
{
	//	may need to copy?
}


const Opencl::TDeviceMeta& Opencl::TKernelState::GetDevice()
{
	return GetContext().GetDevice();
}

Opencl::TContext& Opencl::TKernelState::GetContext()
{
	return mKernel.GetContext();
}


void ExpandIterations(ArrayBridge<Opencl::TKernelIteration>& IterationSplits,size_t Executions,size_t FirstOffset,size_t KernelWorkGroupMax)
{
	if ( Executions == 0 )
		return;
	
	//	copy originals
	Array<Opencl::TKernelIteration> OriginalIteraions( IterationSplits );
	
	//	first instance
	if ( OriginalIteraions.IsEmpty() )
		OriginalIteraions.PushBack( Opencl::TKernelIteration() );
	
	//	clear output
	IterationSplits.Clear();

	bool Overflow = (Executions % KernelWorkGroupMax) > 0;
	auto Iterations = (Executions / KernelWorkGroupMax) + Overflow;
	
	
	for ( int it=0;	it<Iterations;	it++ )
	{
		for ( int oit=0;	oit<OriginalIteraions.GetSize();	oit++ )
		{
			auto Iteration = OriginalIteraions[oit];
		
			//	add next dimension
			auto First = it * KernelWorkGroupMax;
			auto Count = std::min( KernelWorkGroupMax, Executions - First );
			Iteration.mFirst.PushBack( First + FirstOffset );
			Iteration.mCount.PushBack( Count );
			
			//
			IterationSplits.PushBack( Iteration );
		}
	}
}


void Opencl::TKernelState::GetIterations(ArrayBridge<TKernelIteration>&& IterationSplits,const ArrayBridge<vec2x<size_t>>&& Iterations)
{
	auto& Device = GetDevice();
	
	size_t Firsta = (Iterations.GetSize() >= 1) ? (Iterations[0].x) : (0);
	size_t Firstb = (Iterations.GetSize() >= 2) ? (Iterations[1].x) : (0);
	size_t Firstc = (Iterations.GetSize() >= 3) ? (Iterations[2].x) : (0);
	size_t Execa = (Iterations.GetSize() >= 1) ? (Iterations[0].y-Firsta) : (0);
	size_t Execb = (Iterations.GetSize() >= 2) ? (Iterations[1].y-Firstb) : (0);
	size_t Execc = (Iterations.GetSize() >= 3) ? (Iterations[2].y-Firstc) : (0);

	auto KernelWorkGroupMax = Device.GetMaxGlobalWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = std::max( Execa, Execb, Execc );

	ExpandIterations( IterationSplits, Execa, Firsta, KernelWorkGroupMax );
	ExpandIterations( IterationSplits, Execb, Firstb, KernelWorkGroupMax );
	ExpandIterations( IterationSplits, Execc, Firstc, KernelWorkGroupMax );
}

void Opencl::TKernelState::QueueIteration(const TKernelIteration& Iteration)
{
	QueueIterationImpl( Iteration, nullptr );
}
void Opencl::TKernelState::QueueIteration(const TKernelIteration& Iteration,TSync& Semaphore)
{
	QueueIterationImpl( Iteration, &Semaphore );
}

void Opencl::TKernelState::QueueIterationImpl(const TKernelIteration& Iteration,TSync* Semaphore)
{
	BufferArray<size_t,3> GlobalExec( Iteration.mCount );
	BufferArray<size_t,3> LocalExec;

/*
	for ( int i=0;	i<GlobalExec.GetSize();	i++ )
	{
		auto& ExecCount = GlobalExec[i];
		
		//	dimensions need to be at least 1, zero size is not a failure, just don't execute
		if ( ExecCount <= 0 )
			return true;
		
		if ( IsValidGlobalExecCount(ExecCount) )
			continue;
		
		BufferString<1000> Debug;
		Debug << GetName() << ": Too many iterations for kernel: " << ExecCount << "/" << GetMaxWorkGroupSize() << "... execution count truncated.";
		ofLogWarning( Debug.c_str() );
		ExecCount = std::min( ExecCount, GetMaxGlobalWorkGroupSize() );
	}
*/

	if ( !LocalExec.IsEmpty() )
	{
		Soy_AssertTodo();
		/*
		
		if ( LocalExec.GetSize() != GlobalExec.GetSize() )
		{
			std::stringstream Error;
			Error << "Local execution size must match global execution size if specified";
			throw Soy::AssertException( Error.str() );
		}
	
		if ( !IsValidLocalExecCount( GetArrayBridge(LocalExec), true ) )
			return false;
	
		//	CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and
		//			the total number of work-items in the work-group computed as
		//			local_work_size[0] *... local_work_size[work_dim - 1] is greater than
		//			the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.
		int TotalLocalWorkGroupSize = LocalExec[0];
		for ( int d=1;	d<LocalExec.GetSize();	d++ )
			TotalLocalWorkGroupSize *= LocalExec[d];
		
		if ( TotalLocalWorkGroupSize > GetMaxLocalWorkGroupSize() )
		{
			std::stringstream Error;
			Error << "Too many local work items for device: " << TotalLocalWorkGroupSize << "/" << GetMaxLocalWorkGroupSize();
			throw Soy::AssertException( Error.str() );
		}

		
		//	CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of work-items specified by
		//	global_work_size is not evenly divisable by size of work-group given by local_work_size
		{
			std::stringstream Error;
			for ( int Dim=0;	Dim<LocalExec.GetSize();	Dim++ )
			{
				int Local = LocalExec[Dim];
				int Global = GlobalExec[Dim];
				int Remainer = Global % Local;
				if ( Remainer != 0 )
				{
					Error << "Local work items[" << Dim << "] " << Local << " not divisible by global size " << Global << std::endl;
				}
			}
			
			if ( !Error.str().empty() )
				throw Soy::AssertException( Error.str() );
		}
		*/
	}
	
	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	//if ( !WaitForPendingWrites() )
	//	return false;
	
	auto& Kernel = mKernel.mKernel;
	auto Queue = mKernel.GetContext().GetQueue();
	auto Dimensions = size_cast<cl_uint>(GlobalExec.GetSize());
	const size_t* GlobalWorkOffset = nullptr;
	const size_t* GlobalWorkSize = GlobalExec.GetArray();
	const size_t* LocalWorkSize = LocalExec.GetArray();
	cl_event* WaitEvent = Semaphore ? &Semaphore->mEvent : nullptr;
	
	cl_int Err = clEnqueueNDRangeKernel( Queue, Kernel, Dimensions, GlobalWorkOffset, GlobalWorkSize, LocalWorkSize, 0, nullptr, WaitEvent );

	//	invalid kernel args can mean an arg wasn't set. work it out
	if ( Err == CL_INVALID_KERNEL_ARGS )
	{
		Array<std::string> UnassignedUniforms;
		for ( int u=0;	u<mKernel.mUniforms.GetSize();	u++ )
		{
			auto& Uniform = mKernel.mUniforms[u];
			if ( mAssignedArguments.Find( Uniform.mName ) )
				continue;
			UnassignedUniforms.PushBack( Uniform.mName );
		}

		//	gr: this will fail, so lets throw this
		std::stringstream Error;
		Error << "Detected unassigned uniforms x" << UnassignedUniforms.GetSize() << ": ";
		for ( int i=0;	i<UnassignedUniforms.GetSize();	i++ )
			Error << UnassignedUniforms[i] << ", ";
		throw Soy::AssertException( Error.str() );
	}
	
	Opencl::IsOkay( Err, "clEnqueueNDRangeKernel" );
}


