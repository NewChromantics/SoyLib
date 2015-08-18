#include "SoyOpenCl.h"
//#include "SoyApp.h"
#include <SoyDebug.h>
#include <SoyString.h>
#include "SoyOpenglContext.h"


namespace Opencl
{
#define Opencl_IsOkay(Error)	Opencl::IsOkay( (Error), __func__, true )
	bool				IsOkay(cl_int Error,const std::string& Context,bool ThrowException=true);
	
	
	cl_channel_order	GetImageChannelOrder(SoyPixelsFormat::Type Format,cl_channel_type& DataType);
}


std::map<OpenclDevice::Type,std::string> OpenclDevice::EnumMap =
{
	{ OpenclDevice::Invalid,	"Invalid" },
	{ OpenclDevice::CPU,	"CPU" },
	{ OpenclDevice::GPU,	"GPU" },
	{ OpenclDevice::ANY,	"ANY" },
};



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


std::ostream& operator<<(std::ostream &out,const Opencl::TDeviceMeta& in)
{
	out << in.mName << " ";
	out << in.mVendor << " ";
	out << OpenclDevice::ToString(in.mType) << " ";
	return out;
}


Opencl::TDeviceMeta::TDeviceMeta(cl_device_id Device) :
	mDevice		( Device )
{
	mVendor = GetString( Device, CL_DEVICE_VENDOR );
	mName = GetString( Device, CL_DEVICE_NAME );
	mDriverVersion = GetString( Device, CL_DRIVER_VERSION );
	std::string DeviceVersion = GetString( Device, CL_DEVICE_VERSION );
	mProfile = GetString( Device, CL_DEVICE_PROFILE );
	mExtensions = GetString( Device, CL_DEVICE_EXTENSIONS );

	//	extract version
	mVersion = Soy::TVersion( DeviceVersion, "OpenCL " );
	
	std::Debug << "Device " << mName << "(" << mDriverVersion << ") extensions: " << mExtensions << std::endl;

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


void Opencl::TPlatform::GetDevices(ArrayBridge<TDeviceMeta>& Metas,OpenclDevice::Type Filter)
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
		Metas.PushBack( Meta );
	}
}
	

void Opencl::GetDevices(ArrayBridge<TDeviceMeta>&& Metas,OpenclDevice::Type Filter)
{
	//	windows AMD sdk/ati radeon driver implementation doesn't accept NULL as a platform ID, so fetch the list of platforms first
	Array<cl_platform_id> Platforms;
	GetPlatforms( GetArrayBridge(Platforms) );

	//	collect devices
	for ( int p=0;	p<Platforms.GetSize();	p++ )
	{
		TPlatform Platform(Platforms[p]);
		
		Platform.GetDevices( Metas, Filter );
	}
}



Opencl::TDevice::TDevice(const ArrayBridge<cl_device_id>& Devices) :
	mContext	( nullptr )
{
	CreateContext( Devices );
}


Opencl::TDevice::TDevice(const ArrayBridge<TDeviceMeta>& Devices) :
	mContext	( nullptr )
{
	Array<cl_device_id> DeviceIds;
	for ( int i=0;	i<Devices.GetSize();	i++ )
		DeviceIds.PushBack( Devices[i].mDevice );

	CreateContext( GetArrayBridge(DeviceIds) );
}

void Opencl::TDevice::CreateContext(const ArrayBridge<cl_device_id>& Devices)
{
	if ( Devices.IsEmpty() )
		throw Soy::AssertException("No devices provided");
	
	//	create context
	//	if we specify any properties we need a platform (and a terminator)
	Array<cl_context_properties> Properties;

#if defined(TARGET_OSX)
	/* opengl interop
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
	Properties.PushBack( CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE );
	Properties.PushBack( (cl_context_properties)kCGLShareGroup );
	 */
#endif

	//	c-array style list terminator
	if ( !Properties.IsEmpty() )
		Properties.PushBack(0);

	cl_int err;
	mContext = clCreateContext( Properties.GetArray(), size_cast<cl_uint>(Devices.GetSize()), Devices.GetArray(), nullptr, nullptr, &err );
	Opencl::IsOkay( err, "clCreateContext failed" );
	Soy::Assert( mContext != nullptr, "clCreateContext failed to return a context" );
	
	mDevices.Copy( Devices );
}


Opencl::TDevice::~TDevice()
{
	if ( mContext )
	{
		clReleaseContext( mContext );
		mContext = nullptr;
	}
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

bool Opencl::TContext::Lock()
{
	//	need to set thread for queue?
	return true;
}

void Opencl::TContext::Unlock()
{
	
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
		std::stringstream Error;
		Error << "Failed to compile kernel; " << BuildLog.str();
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

Opencl::TSync::~TSync()
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

Opencl::TBuffer::TBuffer() :
	mMem	( nullptr )
{
	
}

Opencl::TBuffer::TBuffer(size_t Size,TContext& Context) :
	mMem	( nullptr )
{
	cl_mem_flags Flags = 0;
	void* HostPtr = nullptr;

	cl_int Error = CL_SUCCESS;
	mMem = clCreateBuffer( Context.GetContext(), Flags, Size, HostPtr, &Error );
	std::stringstream ErrorString;
	ErrorString << __func__ << "(" << Soy::FormatSizeBytes(Size) << ")";
	Opencl::IsOkay( Error, ErrorString.str() );

	Soy::Assert( mMem != nullptr, "clCreateBuffer success but no mem object" );
}

Opencl::TBuffer::~TBuffer()
{
	if ( mMem )
	{
		clReleaseMemObject( mMem );
		mMem = nullptr;
	}
}


void Opencl::TBuffer::ReadImpl(ArrayInterface<uint8>& Array,TContext& Context,TSync* Sync)
{
	Soy::Assert( mMem != nullptr, "Mem buffer expected" );

	auto* ArrayPtr = Array.GetArray();
	Soy::Assert( ArrayPtr != nullptr, "Array should not be empty" );
	
	//	todo; check size of mem. We should store this on mMem set
	size_t StartBytes = 0;
	size_t ByteCount = Array.GetDataSize();
	bool Blocking = false;
	
	auto Error = clEnqueueReadBuffer( Context.GetQueue(), mMem, Blocking, StartBytes, ByteCount, ArrayPtr, 0, nullptr, Sync ? &Sync->mEvent : nullptr );
	std::stringstream ErrorString;
	ErrorString << "TBuffer::Read(" << ByteCount << " bytes)";
	Opencl::IsOkay( Error, ErrorString.str() );
}

void Opencl::TBuffer::Write(const uint8 *Array,size_t Size,TContext& Context,Opencl::TSync *Sync)
{
	Soy::Assert( mMem != nullptr, "Mem buffer expected" );
	Soy::Assert( Array != nullptr, "Array should not be empty" );

	bool Blocking = false;
	size_t StartBytes = 0;

	auto Error = clEnqueueWriteBuffer( Context.GetQueue(), mMem, Blocking, StartBytes, Size, Array, 0, nullptr,  Sync ? &Sync->mEvent : nullptr );
	std::stringstream ErrorString;
	ErrorString << "TBuffer::Write(" << Size << " bytes)";
	Opencl::IsOkay( Error, ErrorString.str() );
}



Opencl::TBufferImage::TBufferImage(const SoyPixelsMeta& Meta,TContext& Context,const SoyPixelsImpl* ClientStorage,OpenclBufferReadWrite::Type ReadWrite,TSync* Semaphore) :
	mContext	( Context )
{
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

Opencl::TBufferImage::TBufferImage(const SoyPixelsImpl& Image,TContext& Context,bool ClientStorage,OpenclBufferReadWrite::Type ReadWrite,Opencl::TSync* Semaphore) :
	TBufferImage	( Image.GetMeta(), Context, ClientStorage ? &Image : nullptr, ReadWrite, Semaphore )
{
	if ( ReadWrite != OpenclBufferReadWrite::WriteOnly )
		Write( Image, Semaphore );
}


Opencl::TBufferImage::TBufferImage(const Opengl::TTexture& Texture,Opengl::TContext& OpenglContext,TContext& Context,OpenclBufferReadWrite::Type ReadWrite,TSync* Semaphore) :
	mContext	( Context )
{
	//	check for opengl interoperability
	SoyPixels Buffer;
	auto Read = [&Buffer,&Texture]
	{
		Texture.Read( Buffer );
	};
	Soy::TSemaphore ReadSemaphore;
	OpenglContext.PushJob( Read, ReadSemaphore );
	//ReadSemaphore.Wait("TBufferImage read pixels from texture");
	ReadSemaphore.Wait();
	
	*this = std::move( Opencl::TBufferImage( Buffer, Context, false, ReadWrite, Semaphore ) );
}

Opencl::TBufferImage& Opencl::TBufferImage::operator=(TBufferImage&& Move)
{
	if ( mContext != Move.mContext )
		throw Soy::AssertException("Trying to std::move a buffer image across contexts");
	
	if ( this != &Move )
	{
		mMem = Move.mMem;

		//	stolen the resource
		Move.mMem = nullptr;
	}
	return *this;
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


bool Opencl::TKernelState::SetUniform(const char* Name,const Opengl::TTextureAndContext& Pixels)
{
	//	todo: get uniform and check type is image_2D_t
	//	make image buffer and set that
	Opencl::TSync Sync;
	std::shared_ptr<TBuffer> Buffer( new TBufferImage( Pixels.mTexture, Pixels.mContext, GetContext(), OpenclBufferReadWrite::ReadWrite, &Sync ) );
	
	if ( mBuffers.find(Name) != mBuffers.end() )
		std::Debug << "Warning, setting buffer for uniform " << Name << " which already has a buffer..." << std::endl;
	
	mBuffers[Name] = Buffer;
	
	//	set kernel arg
	bool Result = SetKernelArg( *this, Name, Buffer->GetMemBuffer() );
	
	//	we can't tell when caller is going to release the pixels, so wait for it to finish
	Sync.Wait();
	
	return Result;
}


bool Opencl::TKernelState::SetUniform(const char* Name,const SoyPixelsImpl& Pixels)
{
	//	todo: get uniform and check type is image_2D_t
	//	make image buffer and set that
	Opencl::TSync Sync;
	std::shared_ptr<TBuffer> Buffer( new TBufferImage( Pixels, GetContext(), false, OpenclBufferReadWrite::WriteOnly, &Sync ) );

	if ( mBuffers.find(Name) != mBuffers.end() )
		std::Debug << "Warning, setting buffer for uniform " << Name << " which already has a buffer..." << std::endl;

	mBuffers[Name] = Buffer;

	//	set kernel arg
	bool Result = SetKernelArg( *this, Name, Buffer->GetMemBuffer() );
	
	//	we can't tell when caller is going to release the pixels, so wait for it to finish
	Sync.Wait();
	
	return Result;
}


bool Opencl::TKernelState::SetUniform(const char* Name,const vec4f& Value)
{
	auto Value2 = Soy::VectorToCl( Value );
	return SetKernelArg( *this, Name, Value2 );
}

bool Opencl::TKernelState::SetUniform(const char* Name,const float& Value)
{
	return SetKernelArg( *this, Name, Value );
}

bool Opencl::TKernelState::SetUniform(const char* Name,const vec2f& Value)
{
	auto Value2 = Soy::VectorToCl( Value );
	return SetKernelArg( *this, Name, Value2 );
}

bool Opencl::TKernelState::SetUniform(const char* Name,cl_int Value)
{
	return SetKernelArg( *this, Name, Value );
}

bool Opencl::TKernelState::SetUniform(const char* Name,TBuffer& Buffer)
{
	return SetKernelArg( *this, Name, Buffer.GetMemBuffer() );
}


void Opencl::TKernelState::ReadUniform(const char* Name,SoyPixelsImpl& Pixels)
{
	//	see if there's a buffer
	auto BufferIt = mBuffers.find( Name );
	if ( BufferIt == mBuffers.end() )
	{
		std::stringstream Error;
		Error << "No buffer for uniform " << Name;
		throw Soy::AssertException( Error.str() );
	}

	auto& pBuffer = BufferIt->second;
	if ( !pBuffer )
	{
		std::stringstream Error;
		Error << "Missing data buffer for uniform " << Name;
		throw Soy::AssertException( Error.str() );
	}
	
	//	check types etc!
	Opencl::TSync Semaphore;
	auto& BufferImage = dynamic_cast<TBufferImage&>( *pBuffer );
	BufferImage.Read( Pixels, &Semaphore );
	Semaphore.Wait();
}


const Opencl::TDeviceMeta& Opencl::TKernelState::GetDevice()
{
	return GetContext().GetDevice();
}

Opencl::TContext& Opencl::TKernelState::GetContext()
{
	return mKernel.GetContext();
}


void ExpandIterations(ArrayBridge<Opencl::TKernelIteration>& IterationSplits,size_t Executions,size_t KernelWorkGroupMax)
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
			Iteration.mFirst.PushBack( First );
			Iteration.mCount.PushBack( Count );
			
			//
			IterationSplits.PushBack( Iteration );
		}
	}
}


void Opencl::TKernelState::GetIterations(ArrayBridge<TKernelIteration>&& IterationSplits,const ArrayBridge<size_t>&& Iterations)
{
	auto& Device = GetDevice();
	
	BufferArray<size_t,3> Exec(3);
	auto& Execa = Exec[0];
	auto& Execb = Exec[1];
	auto& Execc = Exec[2];
	Execa = (Iterations.GetSize() >= 1) ? Iterations[0] : 0;
	Execb = (Iterations.GetSize() >= 2) ? Iterations[1] : 0;
	Execc = (Iterations.GetSize() >= 3) ? Iterations[2] : 0;
	
	auto KernelWorkGroupMax = Device.GetMaxGlobalWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = std::max( Execa, Execb, Execc );

	ExpandIterations( IterationSplits, Execa, KernelWorkGroupMax );
	ExpandIterations( IterationSplits, Execb, KernelWorkGroupMax );
	ExpandIterations( IterationSplits, Execc, KernelWorkGroupMax );
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

	Opencl::IsOkay( Err, "clEnqueueNDRangeKernel" );
}


/*

//	default settings
bool SoyOpenCl::DefaultReadBlocking = true;
bool SoyOpenCl::DefaultWriteBlocking = false;
bool SoyOpenCl::DefaultExecuteBlocking = true;
msa::OpenClDevice::Type SoyOpenCl::DefaultDeviceType = msa::OpenClDevice::Any;



class TSortPolicy_SoyThreadQueue
{
public:
	//template<typename TYPEB>
	static int		Compare(const SoyThreadQueue& a,const SoyThreadQueue& b)
	{
		if ( a.mThreadId < b.mThreadId )		return -1;
		if ( a.mThreadId > b.mThreadId )		return 1;
		if ( a.mDeviceType < b.mDeviceType )	return -1;
		if ( a.mDeviceType > b.mDeviceType )	return 1;
		return 0;
	}
};



SoyFileChangeDetector::SoyFileChangeDetector(std::string Filename) :
	mFile			( Filename )
{
}


bool SoyFileChangeDetector::HasChanged()
{
	auto CurrentTimestamp = GetCurrentTimestamp();
	return mLastModified != CurrentTimestamp;
}

void SoyFileChangeDetector::SetLastModified(SoyFilesystem::Timestamp Timestamp)
{
	mLastModified = Timestamp;
}



SoyOpenClManager::SoyOpenClManager(std::string PlatformName,prmem::Heap& Heap) :
	SoyThread	( "SoyOpenClManager" ),
	mHeap		( Heap ),
	mShaders	( mHeap )
{
	if ( !mOpencl.setup(PlatformName) )
	{
		assert( false );
		ofLogError("Failed to initialise opencl");
	}

	startThread( true, false );
}

SoyOpenClManager::~SoyOpenClManager()
{
	waitForThread();
}

		
void SoyOpenClManager::threadedFunction()
{
	while ( isThreadRunning() )
	{
		//	setup some shaders for pre-loading
		{
			ofMutex::ScopedLock PreLoadLock( mPreloadShaders );
			while ( mPreloadShaders.GetSize() )
			{
				auto FilenameAndBuildOptions = mPreloadShaders.PopBack();

				//	create a simple shader entry which "needs reloading"
				AllocShader( FilenameAndBuildOptions.mFirst.c_str(), FilenameAndBuildOptions.mSecond.c_str() );
			}
		}

		//	check if any shader's files have changed
		Array<SoyOpenClShader*> Shaders;
		mShaderLock.lock();
		Shaders = mShaders;
		mShaderLock.unlock();
		
		for ( int i=0;	i<Shaders.GetSize();	i++ )
		{
			auto& Shader = *Shaders[i];

			if ( !Shader.HasChanged() )
				continue;
			
			if ( Shader.IsLoading() )
				continue;

			//	reload shader
			if ( Shader.LoadShader() )
			{
				auto* pShader = &Shader;
				ofNotifyEvent( mOnShaderLoaded, pShader );
			}
		}

		Sleep(1);
		
	}
}


bool SoyOpenClManager::IsValid()
{
	auto Context = mOpencl.getContext();
	if ( Context == NULL )
		return false;
	return true;
}

SoyOpenClKernel* SoyOpenClManager::GetKernel(SoyOpenClKernelRef KernelRef,cl_command_queue Queue)
{
	auto* pShader = GetShader( KernelRef.mShader );
	if ( !pShader )
		return NULL;

	return pShader->GetKernel( KernelRef.mKernel, Queue );
}

void SoyOpenClManager::DeleteKernel(SoyOpenClKernel* Kernel)
{
	if ( !Kernel )
		return;
	
	delete Kernel;
}

SoyOpenClShader* SoyOpenClManager::AllocShader(std::string Filename,std::string BuildOptions)
{
	if ( !IsValid() )
		return nullptr;

	ofMutex::ScopedLock Lock( mShaderLock );

	//	see if it already exists
	auto* pShader = GetShader( Filename );

	//	make new one if it doesnt exist
	bool IsNewShader = (!pShader);
	if ( !pShader )
	{
		//	generate a ref
		BufferString<100> BaseFilename = ofFilePath::getFileName(Filename).c_str();
		SoyRef ShaderRef = GetUniqueRef( SoyRef(BaseFilename) );
		if ( !ShaderRef.IsValid() )
			return nullptr;

		//	make new shader
		pShader = mHeap.Alloc<SoyOpenClShader>( ShaderRef, Filename, BuildOptions, *this );
		if ( !pShader )
			return nullptr;

		mShaders.PushBack( pShader );
	}

	return pShader;
}

void SoyOpenClManager::PreLoadShader(std::string Filename,std::string BuildOptions)
{
	ofMutex::ScopedLock Lock( mPreloadShaders );

	auto& FilenameAndBuildOptions = mPreloadShaders.PushBack();
	FilenameAndBuildOptions.mFirst = Filename;
	FilenameAndBuildOptions.mSecond = BuildOptions;

	//	gr: could just AllocShader() here??
}

SoyOpenClShader* SoyOpenClManager::LoadShader(std::string Filename,std::string BuildOptions)
{
	auto* pShader = AllocShader( Filename, BuildOptions );
	if ( !pShader )
		return nullptr;

	//	load (in case it needs it)
	if ( pShader->HasChanged() && !pShader->IsLoading() )
	{
		if ( pShader->LoadShader() )
		{
			ofNotifyEvent( mOnShaderLoaded, pShader );
		}
	}

	return pShader;
}

SoyRef SoyOpenClManager::GetUniqueRef(SoyRef BaseRef)
{
	ofMutex::ScopedLock Lock( mShaderLock );

	//	check each shader for dupe ref
	int s = 0;
	while ( s < mShaders.GetSize() )
	{
		auto& Shader = *mShaders[s];

		//	ref used, try next and re-start the search
		if ( Shader == BaseRef )
		{
			BaseRef.Increment();
			s = 0;
			continue;
		}

		//	try next
		s++;
	}
	return BaseRef;
}

SoyOpenClShader* SoyOpenClManager::GetShader(SoyRef ShaderRef)
{
	ofMutex::ScopedLock Lock( mShaderLock );
	for ( int s=0;	s<mShaders.GetSize();	s++ )
	{
		auto& Shader = *mShaders[s];
		if ( Shader == ShaderRef )
			return &Shader;
	}
	return NULL;
}


SoyOpenClShader* SoyOpenClManager::GetShader(std::string Filename)
{
	ofMutex::ScopedLock Lock( mShaderLock );
	for ( int s=0;	s<mShaders.GetSize();	s++ )
	{
		auto& Shader = *mShaders[s];
		if ( Soy::StringBeginsWith( Shader.GetFilename(), Filename, false ) )
			return &Shader;
	}
	return NULL;
}

cl_command_queue SoyOpenClManager::GetQueueForThread(msa::OpenClDevice::Type DeviceType)
{
	auto CurrentThreadId = SoyThread::GetCurrentThreadId();
	return GetQueueForThread( CurrentThreadId, DeviceType );
}

cl_command_queue SoyOpenClManager::GetQueueForThread(std::thread::id ThreadId,msa::OpenClDevice::Type DeviceType)
{
	ofMutex::ScopedLock Lock( mThreadQueues );

	//	look for matching queue meta
	SoyThreadQueue MatchQueue;
	MatchQueue.mDeviceType = DeviceType;
	MatchQueue.mThreadId = ThreadId;
	auto ThreadQueues = GetSortArray( mThreadQueues, TSortPolicy_SoyThreadQueue() );
	auto* pQueue = ThreadQueues.Find( MatchQueue );

	//	create new entry
	if ( !pQueue )
	{
		MatchQueue.mQueue = mOpencl.createQueue( DeviceType );

		std::Debug << "Created opencl queue " << (ptrdiff_t)(MatchQueue.mQueue) << " for thread id " << std::endl;
	
		if ( !MatchQueue.mQueue )
			return NULL;
		
		pQueue = &ThreadQueues.Push( MatchQueue );
	}
	return pQueue->mQueue;
}


bool SoyOpenClShader::IsLoading()
{
	//	if currently locked then it's loading
	if ( !mLock.tryLock() )
		return true;

	mLock.unlock();
	return false;
}

void SoyOpenClShader::UnloadShader()
{
	ofMutex::ScopedLock Lock(mLock);

	//	delete program
	if ( mProgram )
	{
		delete mProgram;
		mProgram = NULL;
	}
}


bool SoyOpenClShader::LoadShader()
{
	ofMutex::ScopedLock Lock(mLock);
	UnloadShader();

	auto CurrentTimestamp = GetCurrentTimestamp();

	//	file gone missing
	if ( !CurrentTimestamp.IsValid() )
		return false;

	//	let this continue if we have build errors? so it doesnt keep trying to reload...
	mProgram = mManager.mOpencl.loadProgramFromFile( GetFilename(), false, GetBuildOptions() );
	if ( !mProgram )
		return false;
	SetLastModified( CurrentTimestamp );

	//	gr: todo: mark kernels to reload

	return true;
}


SoyOpenClKernel* SoyOpenClShader::GetKernel(std::string Name,cl_command_queue Queue)
{
	ofMutex::ScopedLock Lock(mLock);
	ofScopeTimerWarning Warning( BufferString<1000>()<<__FUNCTION__<<" "<<Name, 3 );
	SoyOpenClKernel* pKernel = new SoyOpenClKernel( Name, *this );
	if ( !pKernel )
		return NULL;

	//	try to load
	if ( mProgram )
	{
		pKernel->mKernel = mManager.mOpencl.loadKernel( pKernel->GetName(), *mProgram, Queue );
		pKernel->OnLoaded();
	}
	
	return pKernel;
}







SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,bool UseThreadQueue,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device,const char* BuildOptions) :
	mManager		( Manager ),
	mKernel			( NULL ),
	mRequestDevice	( Device ),
	mHeap			( mManager.GetHeap() ),
	mUseThreadQueue	( UseThreadQueue )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader, BuildOptions );
	if ( pShader )
		mKernelRef.mShader = pShader->GetRef();

	mKernelRef.mKernel = Kernel;
}

SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device,const char* BuildOptions) :
	mManager		( Manager ),
	mKernel			( NULL ),
	mRequestDevice	( Device ),
	mHeap			( mManager.GetHeap() ),
	mUseThreadQueue	( true )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader, BuildOptions );
	if ( pShader )
		mKernelRef.mShader = pShader->GetRef();

	mKernelRef.mKernel = Kernel;
}


SoyOpenClKernel* SoyClShaderRunner::GetKernel()
{
	if ( !mKernel )
	{
		cl_command_queue Queue = mUseThreadQueue ? mManager.GetQueueForThread( mRequestDevice ) : NULL;
		mKernel = mManager.GetKernel( mKernelRef, Queue );
	}
	return mKernel;
}


SoyOpenClKernel::SoyOpenClKernel(std::string Name,SoyOpenClShader& Parent) :
	mName				( Name ),
	mKernel				( NULL ),
	mShader				( Parent ),
	mManager			( Parent.mManager ),
	mKernelRef			( Parent.GetRef(), Name )
{
}
	
msa::OpenCL& SoyOpenClKernel::GetOpenCL() const
{
	return mManager.mOpencl;
}

void SoyOpenClKernel::DeleteKernel()
{
	if ( !mKernel )
		return;

	mManager.mOpencl.deleteKernel( *mKernel );
	mKernel = NULL;
}

void SoyOpenClKernel::OnBufferPendingWrite(cl_event PendingWriteEvent)
{
	assert( PendingWriteEvent != NULL );
	mPendingBufferWrites.PushBack( PendingWriteEvent );
}
	
bool SoyOpenClKernel::WaitForPendingWrites()
{
	if ( !mKernel )
		return false;

	static bool UseEvents = true;
	if ( UseEvents )
	{
		if ( mPendingBufferWrites.IsEmpty() )
			return true;
		auto Err = clWaitForEvents( mPendingBufferWrites.GetSize(), mPendingBufferWrites.GetArray() );
		return (Err == CL_SUCCESS);
	}
	else
	{
		mPendingBufferWrites.Clear();
		auto Err = clFinish( mKernel->getQueue() );
		return (Err == CL_SUCCESS);
	}
}


bool SoyOpenClKernel::Begin()
{
	if ( !IsValid() )
		return false;

	return true;
}

bool SoyOpenClKernel::IsValidLocalExecCount(ArrayBridge<int>& LocalExec,bool ForceCorrection)
{
	if ( LocalExec.IsEmpty() )
		return true;
	
	std::stringstream Debug;

	//	check against device max's
	//	gr: don't think i need to check mDevice.maxWorkGroupSize as it's implied by the individual limits...
	bool Error = false;
	for ( int d=0;	d<LocalExec.GetSize();	d++ )
	{
		int MaxSize = mDeviceInfo.maxWorkItemSizes[d];
		int Size = LocalExec[d];
		
		Debug << Size << "/" << MaxSize << " ";
		
		if ( Size > MaxSize )
		{
			Error = true;
		
			if ( ForceCorrection )
				LocalExec[d] = MaxSize;
		}
	}
	
	if ( Error )
	{
		std::Debug << GetName() << ": too many local iterations for kernel; " << Debug.str();
		if ( ForceCorrection )
			std::Debug << " (corrected)";
		std::Debug << std::endl;
		return ForceCorrection;
	}
	
	return true;
}

bool SoyOpenClKernel::End(bool Blocking,const ArrayBridge<int>& OrigGlobalExec,const ArrayBridge<int>& OrigLocalExec)
{
	BufferArray<int,3> GlobalExec( OrigGlobalExec );
	BufferArray<int,6> LocalExec( OrigLocalExec );
	assert( !GlobalExec.IsEmpty() );
	if ( GlobalExec.IsEmpty() )
		return true;

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

	//	local size must match global size if specified
	if ( !LocalExec.IsEmpty() && LocalExec.GetSize() != GlobalExec.GetSize() )
	{
		assert( false );
		return false;
	}

	auto LocalExecBridge = GetArrayBridge(LocalExec);
	if ( !IsValidLocalExecCount( LocalExecBridge, true ) )
		return false;


	//	check 		CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and
	//			the total number of work-items in the work-group computed as
	//			local_work_size[0] *... local_work_size[work_dim - 1] is greater than
	//			the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.
	if ( !LocalExec.IsEmpty() )
	{
		int TotalLocalWorkGroupSize = LocalExec[0];
		for ( int d=1;	d<LocalExec.GetSize();	d++ )
			TotalLocalWorkGroupSize *= LocalExec[d];

		if ( TotalLocalWorkGroupSize > GetMaxLocalWorkGroupSize() )
		{
			std::Debug << "Too many local work items for device: " << TotalLocalWorkGroupSize << "/" << GetMaxLocalWorkGroupSize() << std::endl;
			return false;
		}
	}

	//	CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of work-items specified by
	//	global_work_size is not evenly divisable by size of work-group given by local_work_size
	if ( !LocalExec.IsEmpty() )
	{
		bool Error = false;
		for ( int Dim=0;	Dim<LocalExec.GetSize();	Dim++ )
		{
			int Local = LocalExec[Dim];
			int Global = GlobalExec[Dim];
			int Remainer = Global % Local;
			if ( Remainer != 0 )
			{
				std::Debug << "Local work items[" << Dim << "] " << Local << " not divisible by global size " << Global << std::endl;
				Error = true;
			}
		}
		if ( Error )
			return false;
	}
	

	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	if ( !WaitForPendingWrites() )
		return false;

	//	pad out local exec in case it's not specified
	LocalExec.PushBack(0);
	LocalExec.PushBack(0);
	LocalExec.PushBack(0);

	//	execute
	if ( GlobalExec.GetSize() == 3 )
	{
		if ( !mKernel->run3D( Blocking, GlobalExec[0], GlobalExec[1], GlobalExec[2], LocalExec[0], LocalExec[1], LocalExec[2] ) )
			return false;
	}
	else if ( GlobalExec.GetSize() == 2 )
	{
		if ( !mKernel->run2D( Blocking, GlobalExec[0], GlobalExec[1], LocalExec[0], LocalExec[1] ) )
			return false;
	}
	else if ( GlobalExec.GetSize() == 1 )
	{
		if ( !mKernel->run1D( Blocking, GlobalExec[0], LocalExec[0] ) )
			return false;
	}

	return true;
}

void SoyOpenClKernel::OnUnloaded()
{
	mDeviceInfo = msa::clDeviceInfo();
}

void SoyOpenClKernel::OnLoaded()
{
	//	failed to load (syntax error etc)
	if ( !mKernel )
	{
		OnUnloaded();
		return;
	}

	//	query for max work group items
	//	http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/clGetKernelWorkGroupInfo.html
	auto Queue = GetQueue();
	auto* Device = mManager.mOpencl.GetDevice( Queue );
	if ( !Device )
	{
		OnUnloaded();
		return;
	}
	
	mDeviceInfo = Device->mInfo;
}



bool SoyOpenClKernel::CheckPaddingChecksum(const int* Padding,int Length)
{
#if !defined(PADDING_CHECKSUM_1)
	#define PADDING_CHECKSUM_1		123
	#define PADDING_CHECKSUM_2		456
	#define PADDING_CHECKSUM_3		789
#endif
	BufferArray<int,3> Checksums;
	Checksums.PushBack( PADDING_CHECKSUM_1 );
	Checksums.PushBack( PADDING_CHECKSUM_2 );
	Checksums.PushBack( PADDING_CHECKSUM_3 );

	for ( int i=0;	i<Length;	i++ )
	{
		int Pad = Padding[i];
		int Checksum = Checksums[i];
		assert( Pad == Checksum );
		if ( Pad != Checksum )
			return false;
	}
	return true;
}


void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<1>>& Iterations,int Exec1,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = Exec1;

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		SoyOpenclKernelIteration<1> Iteration;
		Iteration.mFirst[0] = ita * KernelWorkGroupMax;
		Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
		Iteration.mBlocking = BlockLast && (ita==Iterationsa-1);
		assert( Iteration.mCount[0] > 0 );
		Iterations.PushBack( Iteration );
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = ofMax( Exec1, Exec2 );

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	bool Overflowb = (Exec2 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;
	int Iterationsb = (Exec2 / KernelWorkGroupMax) + Overflowb;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		for ( int itb=0;	itb<Iterationsb;	itb++ )
		{
			SoyOpenclKernelIteration<2> Iteration;
			Iteration.mFirst[0] = ita * KernelWorkGroupMax;
			Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
			Iteration.mFirst[1] = itb * KernelWorkGroupMax;
			Iteration.mCount[1] = ofMin( KernelWorkGroupMax, Exec2 - Iteration.mFirst[1] );
			Iteration.mBlocking = BlockLast && (ita==Iterationsa-1) && (itb==Iterationsb-1);
			assert( Iteration.mCount[0] > 0 );
			assert( Iteration.mCount[1] > 0 );
			Iterations.PushBack( Iteration );
		}
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = ofMax( Exec1, ofMax( Exec2, Exec3 ) );

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	bool Overflowb = (Exec2 % KernelWorkGroupMax) > 0;
	bool Overflowc = (Exec3 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;
	int Iterationsb = (Exec2 / KernelWorkGroupMax) + Overflowb;
	int Iterationsc = (Exec3 / KernelWorkGroupMax) + Overflowc;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		for ( int itb=0;	itb<Iterationsb;	itb++ )
		{
			for ( int itc=0;	itc<Iterationsc;	itc++ )
			{
				SoyOpenclKernelIteration<3> Iteration;
				Iteration.mFirst[0] = ita * KernelWorkGroupMax;
				Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
				Iteration.mFirst[1] = itb * KernelWorkGroupMax;
				Iteration.mCount[1] = ofMin( KernelWorkGroupMax, Exec2 - Iteration.mFirst[1] );
				Iteration.mFirst[2] = itc * KernelWorkGroupMax;
				Iteration.mCount[2] = ofMin( KernelWorkGroupMax, Exec3 - Iteration.mFirst[2] );
				Iteration.mBlocking = BlockLast && (ita==Iterationsa-1) && (itb==Iterationsb-1) && (itc==Iterationsc-1);
				assert( Iteration.mCount[0] > 0 );
				assert( Iteration.mCount[1] > 0 );
				assert( Iteration.mCount[2] > 0 );
				Iterations.PushBack( Iteration );
			}
		}
	}
}


int SoyOpenClKernel::GetMaxGlobalWorkGroupSize() const		
{
	int DeviceAddressBits = mDeviceInfo.deviceAddressBits;
	int MaxSize = (1<<(DeviceAddressBits-1))-1;	
	//	gr: if this is negative opencl kernels lock up (or massive loops?), code should bail out beforehand
	assert( MaxSize > 0 );
	return MaxSize;
}

int SoyOpenClKernel::GetMaxLocalWorkGroupSize() const		
{
	return mDeviceInfo.maxWorkGroupSize;	
}

*/
