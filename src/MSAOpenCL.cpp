#include "MSAOpenCL.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLKernel.h"
#include "SortArray.h"
//#define ENABLE_SETUP_FROM_OPENGL


class TSortPolicy_BestDevice
{
public:
	static int		GetPriority(msa::OpenClDevice::Type DeviceType)
	{
		switch ( DeviceType )
		{
		case msa::OpenClDevice::GPU:	return 0;
		case msa::OpenClDevice::CPU:	return 1;
		default:	return 2;
		}
	}
	static int		Compare(const msa::OpenClDevice& a,const msa::OpenClDevice& b)
	{
		int Prioritya = GetPriority( a.GetType() );
		int Priorityb = GetPriority( b.GetType() );
		
		if ( Prioritya < Priorityb )		return -1;
		if ( Prioritya > Priorityb )		return 1;
		return 0;
	}
};

namespace msa {
	
bool OpenClDevice::Init()
{
	size_t	size;
	auto& d = mDeviceId;
	auto& info = mInfo;
			
	cl_int err = clGetDeviceInfo(d, CL_DEVICE_VENDOR, sizeof(info.vendorName), info.vendorName, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_NAME, sizeof(info.deviceName), info.deviceName, &size);
	err |= clGetDeviceInfo(d, CL_DRIVER_VERSION, sizeof(info.driverVersion), info.driverVersion, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_VERSION, sizeof(info.deviceVersion), info.deviceVersion, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(info.maxComputeUnits), &info.maxComputeUnits, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(info.maxWorkItemDimensions), &info.maxWorkItemDimensions, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(info.maxWorkItemSizes), &info.maxWorkItemSizes, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(info.maxWorkGroupSize), &info.maxWorkGroupSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(info.maxClockFrequency), &info.maxClockFrequency, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(info.maxMemAllocSize), &info.maxMemAllocSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE_SUPPORT, sizeof(info.imageSupport), &info.imageSupport, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(info.maxReadImageArgs), &info.maxReadImageArgs, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(info.maxWriteImageArgs), &info.maxWriteImageArgs, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(info.image2dMaxWidth), &info.image2dMaxWidth, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(info.image2dMaxHeight), &info.image2dMaxHeight, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(info.image3dMaxWidth), &info.image3dMaxWidth, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(info.image3dMaxHeight), &info.image3dMaxHeight, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(info.image3dMaxDepth), &info.image3dMaxDepth, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_SAMPLERS, sizeof(info.maxSamplers), &info.maxSamplers, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(info.maxParameterSize), &info.maxParameterSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(info.globalMemCacheSize), &info.globalMemCacheSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(info.globalMemSize), &info.globalMemSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(info.maxConstantBufferSize), &info.maxConstantBufferSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(info.maxConstantArgs), &info.maxConstantArgs, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(info.localMemSize), &info.localMemSize, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(info.errorCorrectionSupport), &info.errorCorrectionSupport, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(info.profilingTimerResolution), &info.profilingTimerResolution, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_ENDIAN_LITTLE, sizeof(info.endianLittle), &info.endianLittle, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_PROFILE, sizeof(info.profile), info.profile, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_EXTENSIONS, sizeof(info.extensions), info.extensions, &size);
	err |= clGetDeviceInfo(d, CL_DEVICE_TYPE, sizeof(info.type), &info.type, &size);
			
	if(err != CL_SUCCESS) 
	{
		BufferString<1000> Debug;
		Debug << "Error getting clDevice information: " << OpenCL::getErrorAsString(err);
		ofLogError( Debug.c_str() );
		return false;
	}
			
	ofLogNotice( OpenCL::getInfoAsString(info) );
	return true;
}


const char* OpenClDevice::ToString(OpenClDevice::Type type)
{
	switch ( type ) 
	{
	case Invalid:	return "Invalid";
	case All:		return "All";
	case Any:		return "Any";
	case CPU:		return "CPU";
	case GPU:		return "GPU";
	default:
		return "Unhandled Opencl Device Type";
	}
}

clPlatformInfo::clPlatformInfo(cl_platform_id Platform)
{
	mName[0] = '\0';
	mVendor[0] = '\0';
	mVersion[0] = '\0';

	if ( !Platform )
		return;

	clGetPlatformInfo( Platform, CL_PLATFORM_VERSION, sizeof(mVersion), mVersion, NULL );
	clGetPlatformInfo( Platform, CL_PLATFORM_NAME,sizeof(mName),  mName, NULL );
	clGetPlatformInfo( Platform, CL_PLATFORM_VENDOR, sizeof(mVendor), mVendor, NULL );
}

	OpenCL::OpenCL() :
		mContext	( NULL )
	{
	}
	
	OpenCL::~OpenCL() 
	{
		for ( int q=0;	q<mQueues.GetSize();	q++ )
			clFinish( mQueues[q] );
		
		ofMutex::ScopedLock MemObjectsLock(mMemObjectsLock);
		for(int i=0; i<memObjects.GetSize(); i++) 
			delete memObjects[i];

		ofMutex::ScopedLock KernelsLock(mKernelsLock);
		for(int i=0;	i<kernels.GetSize();	i++ )
			delete kernels[i];

		ofMutex::ScopedLock ProgramsLock(mProgramsLock);
		for(int i=0; i<programs.GetSize(); i++) 
			delete programs[i];

		for ( int q=0;	q<mQueues.GetSize();	q++ )
			clReleaseCommandQueue( mQueues[q] );

		clReleaseContext( mContext );
		mContext = nullptr;
	}
	
	
	bool OpenCL::setup(const char* PlatformName)
	{
		if( isInitialised() )
		{
			ofLogNotice("... already setup. returning");
			return true;
		}
		
		if ( !createDevices(PlatformName) )
			return false;
		
		//	make array of all our devices
		cl_device_id DeviceBuffer[100];
		int DeviceBufferSize = sizeof(DeviceBuffer)/sizeof(DeviceBuffer[0]);
		int DeviceCount = 0;
		for ( int d=0;	DeviceCount<DeviceBufferSize && d<mDevices.GetSize();	d++ )
			DeviceBuffer[DeviceCount++] = mDevices[d].mDeviceId;

		//	create context with all our devices
		cl_int err;
		mContext = clCreateContext(NULL, DeviceCount, DeviceBuffer, NULL, NULL, &err);
		if( !mContext || err != CL_SUCCESS )
		{
			BufferString<1000> Debug;
			Debug << "Error creating clContext:" << getErrorAsString(err);
			ofLogError( Debug.c_str() );
			assert(false);
			return false;
		}		
		
		return true;
	}	
	
	
	bool OpenCL::setupFromOpenGL() {
#if defined(ENABLE_SETUP_FROM_OPENGL)
		if(isSetup) {
			ofLogNotice("... already setup. returning");
			return true;
		}
		
		cl_int err;
		
		createDevice(CL_DEVICE_TYPE_GPU, 1);
		
#ifdef TARGET_OSX	
		CGLContextObj kCGLContext = CGLGetCurrentContext();
		CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
		cl_context_properties properties[] = { CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup, 0 };
#else
		ofLogError("OpenCL::setupFromOpenGL() only implemented for mac osx at the moment.\nIf you know how to do this for windows/linux please go ahead and implement it here.");
		assert(false);
#endif
		
		mContext = clCreateContext(properties, 0, 0, NULL, NULL, &err);
		if(mContext == NULL) {
			ofLogError("Error creating clContext.");
			assert(err != CL_INVALID_PLATFORM);
			assert(err != CL_INVALID_VALUE);
			assert(err != CL_INVALID_DEVICE);
			assert(err != CL_INVALID_DEVICE_TYPE);
			assert(err != CL_DEVICE_NOT_AVAILABLE);
			assert(err != CL_DEVICE_NOT_FOUND);
			assert(err != CL_OUT_OF_HOST_MEMORY);
			assert(false);
		}
		
		createQueue();
#endif
		return false;
	}	
	
	OpenClDevice* OpenCL::GetDevice(cl_command_queue Queue)
	{
		cl_device_id Device = NULL;
		auto err = clGetCommandQueueInfo( Queue, CL_QUEUE_DEVICE, sizeof(Device), &Device, NULL );
		if ( err != CL_SUCCESS)
		{
			BufferString<1000> Debug;
			Debug << "Error getting queue's device; " << getErrorAsString( err );
			ofLogError( Debug.c_str() );
			return NULL;
		}
		return GetDevice( Device );
	}

	OpenClDevice* OpenCL::GetDevice(OpenClDevice::Type DeviceType)
	{
		//	find matching type
		for ( int d=0;	d<mDevices.GetSize();	d++ )
		{
			auto& Device = mDevices[d];
			if ( DeviceType == OpenClDevice::Any )
				return &Device;
			if ( Device.GetType() == DeviceType )
				return &Device;
		}
		return NULL;
	}
	
	OpenClDevice* OpenCL::GetDevice(cl_device_id DeviceId)
	{
		//	find matching type
		for ( int d=0;	d<mDevices.GetSize();	d++ )
		{
			auto& Device = mDevices[d];
			if ( Device.mDeviceId == DeviceId )
				return &Device;
		}
		return NULL;
	}
	
	OpenCLProgram* OpenCL::loadProgramFromFile(std::string filename, bool isBinary,const char* BuildOptions) { 
		assert( isInitialised() );
		BufferString<1000> Debug;
		Debug << __FUNCTION__ << " " << filename;
		ofLogNotice( Debug.c_str() );
		
		OpenCLProgram *p = new OpenCLProgram( *this );
		p->loadFromFile(filename, isBinary, BuildOptions );

		ofMutex::ScopedLock Lock(mProgramsLock);
		programs.PushBack(p);
		return p;
	}
	
	
	OpenCLProgram* OpenCL::loadProgramFromSource(std::string source) {
		assert( isInitialised() );
		ofLogNotice( __FUNCTION__ );
		OpenCLProgram *p = new OpenCLProgram( *this );
		p->loadFromSource(source,NULL,NULL);

		ofMutex::ScopedLock Lock(mProgramsLock);
		programs.PushBack(p);
		return p;
	} 
	
	
	OpenCLKernel* OpenCL::loadKernel(std::string kernelName,OpenCLProgram& program,cl_command_queue Queue) 
	{
		assert( isInitialised() );
		assert( Queue );
		if ( !Queue )
			return nullptr;
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + kernelName + ", " + program.getName() );
		OpenCLKernel *k = program.loadKernel( kernelName, Queue );
		if ( !k )
			return nullptr;
		
		ofMutex::ScopedLock Lock(mKernelsLock);
		kernels.PushBack(k);
		return k;
	}
	
	void OpenCL::deleteKernel(OpenCLKernel& Kernel)
	{
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		delete &Kernel;
		ofMutex::ScopedLock Lock(mKernelsLock);
		kernels.Remove( &Kernel );
	}

	void OpenCL::deleteBuffer(OpenCLMemoryObject& Object)
	{
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		delete &Object;
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.Remove( &Object );
	}

	OpenCLBuffer* OpenCL::createBuffer(cl_command_queue Queue,int numberOfBytes, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite) {
		if ( numberOfBytes <= 0 )
			return nullptr;
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + ofToString(numberOfBytes,0) + " bytes; blocking: " + ofToString(blockingWrite,0)  );
		assert( isInitialised() );
		OpenCLBuffer *clBuffer = new OpenCLBuffer(*this);
		if (!clBuffer )
			return nullptr;
		if ( !clBuffer->initBuffer(numberOfBytes, memFlags, dataPtr, blockingWrite, Queue ) )
		{
			delete clBuffer;
			return nullptr;
		}

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.PushBack(clBuffer);
		return clBuffer;
	}
	
	
#if defined(OPENCL_OPENGL_INTERPOLARITY)
	OpenCLBuffer* OpenCL::createBufferFromGLObject(GLuint glBufferObject, cl_mem_flags memFlags) {
		ofLogNotice( __FUNCTION__ );
		assert( isInitialised() );
		OpenCLBuffer *clBuffer = new OpenCLBuffer(*this);
		clBuffer->initFromGLObject(glBufferObject, memFlags);

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clBuffer);
		return clBuffer;
	}
#endif
	
	
	OpenCLImage* OpenCL::createImage2D(cl_command_queue Queue,int width, int height, cl_channel_order imageChannelOrder, cl_channel_type imageChannelDataType, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite) {
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		return createImage3D(Queue,width, height, 1, imageChannelOrder, imageChannelDataType, memFlags, dataPtr, blockingWrite);
	}
	
	
	OpenCLImage* OpenCL::createImageFromTexture(ofTexture &tex, cl_mem_flags memFlags, int mipLevel) {
		//ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage( *this );
		clImage->initFromTexture(tex, memFlags, mipLevel);

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.PushBack(clImage);
		return clImage;
	}
	
#if defined(OPENCL_OPENGL_INTERPOLARITY)
	OpenCLImage* OpenCL::createImageWithTexture(cl_command_queue Queue,int width, int height, int glType, cl_mem_flags memFlags) {
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage( *this );
		clImage->initWithTexture(Queue,width, height, glType, memFlags);
		
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.PushBack(clImage);
		return clImage;
	}
#endif
	
	OpenCLImage* OpenCL::createImage3D(cl_command_queue Queue,int width, int height, int depth, cl_channel_order imageChannelOrder, cl_channel_type imageChannelDataType, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite) {
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage( *this );
		clImage->initWithoutTexture(Queue,width, height, depth, imageChannelOrder, imageChannelDataType, memFlags, dataPtr, blockingWrite);
	
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.PushBack(clImage);
		return clImage;
	}
	
	int OpenClDevice::GetPlatformCount(const Array<OpenClDevice>& Devices)
	{
		Array<cl_platform_id> Platforms;
		for ( int i=0;	i<Devices.GetSize();	i++ )
		{
			auto& Device = Devices[i];
			Platforms.PushBackUnique( Device.mPlatform );
		}
		return Platforms.GetSize();
	}

	bool OpenCL::EnumDevices(Array<OpenClDevice>& Devices,const char* PlatformNameFilter,OpenClDevice::Type DeviceFilter)
	{
		cl_int err = CL_SUCCESS;
		
		cl_platform_id PlatformBuffer[100];
		cl_uint PlatformCount = 0;

		//	windows AMD sdk/ati radeon driver implementation doesn't accept NULL as a platform ID, so fetch the list of platforms first
		err = clGetPlatformIDs(	sizeofarray(PlatformBuffer), PlatformBuffer, &PlatformCount );
		assert( PlatformCount >= 0 && PlatformCount <= sizeofarray(PlatformBuffer) );
		if ( err != CL_SUCCESS || PlatformCount == 0 )
		{
			BufferString<1000> Debug;
			Debug << "Failed to get opencl platforms; " << OpenCL::getErrorAsString(err);
			ofLogError( Debug.c_str() );
			return false;
		}

		//	filter out platforms
		for ( int p=PlatformCount-1;	PlatformNameFilter && p>=0;	p-- )
		{
			cl_platform_id Platform = PlatformBuffer[p];
			clPlatformInfo PlatformInfo( Platform );

			//	need to filter platform
			if ( Soy::StringContains( PlatformInfo.GetName(), PlatformNameFilter, false ) )
				continue;

			//	remove from array
			for ( int i=p+1;	i<static_cast<int>(PlatformCount);	i++ )
				PlatformBuffer[i-1] = PlatformBuffer[i];
			PlatformCount--;
			continue;
		}

		//	no platforms
		if ( PlatformCount == 0 )
		{
			BufferString<1000> Debug;
			Debug << __FUNCTION__ << " no opencl platforms found";
			ofLogError( Debug.c_str() );
			return false;
		}

		//	collect devices
		for ( int p=0;	p<PlatformCount;	p++ )
		{
			cl_platform_id Platform = PlatformBuffer[p];

			//	get platform info
			clPlatformInfo PlatformInfo( Platform );

			cl_device_id DeviceBuffer[100];
			cl_uint DeviceCount = 0;
			err = clGetDeviceIDs( Platform, OpenClDevice::Any, sizeofarray(DeviceBuffer), DeviceBuffer, &DeviceCount);
			assert( DeviceCount >=0 && DeviceCount <= sizeofarray(DeviceBuffer) );
			if ( err != CL_SUCCESS )
			{
				BufferString<1000> Debug;
				Debug << "Failed to get devices; " << OpenCL::getErrorAsString( err );
				ofLogError( Debug.c_str() );
				return false;
			}

			for ( int d=0;	d<DeviceCount;	d++ )
			{
				OpenClDevice Device( Platform, DeviceBuffer[d] );
				if ( !Device.Init() )
					continue;

				Devices.PushBack( Device );
			}
		}

		return true;
	}
	
	bool OpenCL::createDevices(const char* PlatformName) 
	{
		Array<OpenClDevice> Devices;
		if ( !EnumDevices( Devices, PlatformName, OpenClDevice::Any ) )
			return false;

		//	have more than one platform, need to abort and make the user pick a platform
		int PlatformCount = OpenClDevice::GetPlatformCount( Devices );
		if ( PlatformCount < 1 )
		{
			ofLogError("No opencl devices found");
			return false;
		}
		else if ( PlatformCount > 1 )
		{
			std::string Debug = __FUNCTION__;
			Debug += " More than one opencl platform found. Need to specify which platform name to use; ";
			for ( int d=0;	d<Devices.GetSize();	d++ )
			{
				cl_platform_id Platform = Devices[d].mPlatform;
				clPlatformInfo PlatformInfo( Platform );
				Debug += "\n";
				Debug += PlatformInfo.GetName();
			}
			ofLogError( Debug.c_str() );
			return false;
		}
		auto& PlatformInfo = Devices[0].mPlatformInfo;

		//	save devices
		for ( int d=0;	d<Devices.GetSize();	d++ )
		{
			auto& Device = Devices[d];

			//	sorted by preferred type
			auto& SortedDevices = GetSortArray( mDevices, TSortPolicy_BestDevice() );
			SortedDevices.Push( Device );
		}				

		BufferString<1000> Debug;
		Debug << mDevices.GetSize() << " devices found, on " << PlatformInfo.GetName();
		ofLogNotice( Debug.c_str() );
		if ( mDevices.IsEmpty() )
			return false;
		
		return true;
	}
	
	std::string OpenCL::getInfoAsString(const clDeviceInfo& info) 
	{
		return std::string( (char*)info.vendorName );
		/*
		TString Debug;
		Debug << "OpenCL Device information:"	<<
		"\n\tvendorName.................." + string((char*)info.vendorName) + 
		"\n\tdeviceName.................." + string((char*)info.deviceName) + 
		"\n\tdriverVersion..............." + string((char*)info.driverVersion) +
		"\n\tdeviceVersion..............." + string((char*)info.deviceVersion) +
		"\n\tmaxComputeUnits............." + ofToString(info.maxComputeUnits, 0) +
		"\n\tmaxWorkItemDimensions......." + ofToString(info.maxWorkItemDimensions, 0) +
		"\n\tmaxWorkItemSizes[0]........." + ofToString(info.maxWorkItemSizes[0], 0) + 
		"\n\tmaxWorkGroupSize............" + ofToString(info.maxWorkGroupSize, 0) +
		"\n\tmaxClockFrequency..........." + ofToString(info.maxClockFrequency, 0) +
		"\n\tmaxMemAllocSize............." + ofToString(info.maxMemAllocSize/1024.0f/1024.0f, 3) + " MB" + 
		"\n\timageSupport................" + (info.imageSupport ? "YES" : "NO") +
		"\n\tmaxReadImageArgs............" + ofToString(info.maxReadImageArgs, 0) +
		"\n maxWriteImageArgs..........." + ofToString(info.maxWriteImageArgs, 0) +
		"\n image2dMaxWidth............." + ofToString(info.image2dMaxWidth, 0) +
		"\n image2dMaxHeight............" + ofToString(info.image2dMaxHeight, 0) +
		"\n image3dMaxWidth............." + ofToString(info.image3dMaxWidth, 0) +
		"\n image3dMaxHeight............" + ofToString(info.image3dMaxHeight, 0) +
		"\n image3dMaxDepth............." + ofToString(info.image3dMaxDepth, 0) +
		"\n maxSamplers................." + ofToString(info.maxSamplers, 0) +
		"\n maxParameterSize............" + ofToString(info.maxParameterSize, 0) +
		"\n globalMemCacheSize.........." + ofToString(info.globalMemCacheSize/1024.0f/1024.0f, 3) + " MB" + 
		"\n globalMemSize..............." + ofToString(info.globalMemSize/1024.0f/1024.0f, 3) + " MB" +
		"\n maxConstantBufferSize......." + ofToString(info.maxConstantBufferSize/1024.0f, 3) + " KB"
		"\n maxConstantArgs............." + ofToString(info.maxConstantArgs, 0) +
		"\n localMemSize................" + ofToString(info.localMemSize/1024.0f, 3) + " KB"
		"\n errorCorrectionSupport......" + (info.errorCorrectionSupport ? "YES" : "NO") +
		"\n profilingTimerResolution...." + ofToString(info.profilingTimerResolution, 0) +
		"\n endianLittle................" + ofToString(info.endianLittle, 0) +
		"\n profile....................." + string((char*)info.profile) +
		"\n extensions.................." + string((char*)info.extensions) +
		"\n*********\n\n";
		*/
	}
	
	const char* OpenCL::getErrorAsString(cl_int err)
	{
		switch ( err )
		{
			case CL_SUCCESS:					return "Success";
			case CL_DEVICE_NOT_FOUND:			return "Device not found";
			case CL_DEVICE_NOT_AVAILABLE:		return "Device not available";
			case CL_COMPILER_NOT_AVAILABLE:		return "Compiler not available";
			case CL_MEM_OBJECT_ALLOCATION_FAILURE:	return "Memory object allocation failure";
			case CL_OUT_OF_RESOURCES:			return "Out of resources";
			case CL_OUT_OF_HOST_MEMORY:			return "Out of host memory";
			case CL_PROFILING_INFO_NOT_AVAILABLE:	return "Profiling info not available";
			case CL_MEM_COPY_OVERLAP:			return "Memory copy overlap";
			case CL_IMAGE_FORMAT_MISMATCH:		return "Image format mismatch";
			case CL_IMAGE_FORMAT_NOT_SUPPORTED:	return "Image format not supported";
			case CL_BUILD_PROGRAM_FAILURE:		return "Build program failure";
			case CL_MAP_FAILURE:				return "Map failure";
			case CL_INVALID_VALUE:				return "Invalid value";
			case CL_INVALID_DEVICE_TYPE:		return "Invalid device type";
			case CL_INVALID_PLATFORM:			return "Invalid platform";
			case CL_INVALID_DEVICE:				return "Invalid device";
			case CL_INVALID_CONTEXT:			return "Invalid context";
			case CL_INVALID_QUEUE_PROPERTIES:	return "Invalid queue properties";
			case CL_INVALID_COMMAND_QUEUE:		return "Invalid command queue";
			case CL_INVALID_HOST_PTR:			return "Invalid host pointer";
			case CL_INVALID_MEM_OBJECT:			return "Invalid memory object";
			case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:	return "Invalid image format descriptor";
			case CL_INVALID_IMAGE_SIZE:			return "Invalid image size";
			case CL_INVALID_SAMPLER:			return "Invalid sampler";
			case CL_INVALID_BINARY:				return "Invalid binary";
			case CL_INVALID_BUILD_OPTIONS:		return "Invalid build options";
			case CL_INVALID_PROGRAM:			return "Invalid program";
			case CL_INVALID_PROGRAM_EXECUTABLE:	return "Invalid program executable";
			case CL_INVALID_KERNEL_NAME:		return "Invalid kernel name";
			case CL_INVALID_KERNEL_DEFINITION:	return "Invalid kernel definition";
			case CL_INVALID_KERNEL:				return "Invalid kernel";
			case CL_INVALID_ARG_INDEX:			return "Invalid argument index";
			case CL_INVALID_ARG_VALUE:			return "Invalid argument value";
			case CL_INVALID_ARG_SIZE:			return "Invalid argument size";
			case CL_INVALID_KERNEL_ARGS:		return "Invalid kernel arguments";
			case CL_INVALID_WORK_DIMENSION:		return "Invalid work dimension";
			case CL_INVALID_WORK_GROUP_SIZE:	return "Invalid work group size";
			case CL_INVALID_WORK_ITEM_SIZE:		return "invalid work item size";
			case CL_INVALID_GLOBAL_OFFSET:		return "Invalid global offset";
			case CL_INVALID_EVENT_WAIT_LIST:	return "Invalid event wait list";
			case CL_INVALID_EVENT:				return "Invalid event";
			case CL_INVALID_OPERATION:			return "Invalid operation";
			case CL_INVALID_GL_OBJECT:			return "Invalid OpenGL object";
			case CL_INVALID_BUFFER_SIZE:		return "Invalid buffer size";
			case CL_INVALID_MIP_LEVEL:			return "Invalid MIP level";
		}
    
		//	unhandled case
		assert(false);
		return "Unhandled opencl error";
	}


	
	cl_command_queue OpenCL::createQueue(OpenClDevice::Type DeviceType) 
	{
		assert( isInitialised() );
		ofLogNotice( __FUNCTION__ );

		//	pick a device
		auto* pDevice = GetDevice( DeviceType );
		if ( !pDevice )
		{
			ofLogError("Failed to find device of specified type.");
			return false;
		}

		auto DeviceId = pDevice->mDeviceId;
		cl_int Err = CL_SUCCESS;
		cl_command_queue Queue = clCreateCommandQueue(mContext, DeviceId, 0, &Err );
		if ( !Queue || Err != CL_SUCCESS )
		{
			BufferString<100> Debug;
			Debug << "Failed to create queue; " << getErrorAsString( Err );
			ofLogError( Debug.c_str() );
			return NULL;
		}

		ofMutex::ScopedLock lock(mQueuesLock);
		mQueues.PushBack( Queue );
		return Queue;
	}

}
