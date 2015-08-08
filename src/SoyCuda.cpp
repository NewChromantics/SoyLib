#include "SoyCuda.h"
#include <SoyDebug.h>
#include <SoyAssert.h>




std::string Cuda::GetEnumString(CUresult Error)
{
#define CASE_ENUM_STRING(e)	case (e):	return #e;
	switch (Error)
	{
		//	errors
		CASE_ENUM_STRING(CUDA_SUCCESS);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_VALUE);
		CASE_ENUM_STRING(CUDA_ERROR_OUT_OF_MEMORY);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_INITIALIZED);
		CASE_ENUM_STRING(CUDA_ERROR_DEINITIALIZED);
		CASE_ENUM_STRING(CUDA_ERROR_PROFILER_DISABLED);
		CASE_ENUM_STRING(CUDA_ERROR_PROFILER_NOT_INITIALIZED);
		CASE_ENUM_STRING(CUDA_ERROR_PROFILER_ALREADY_STARTED);
		CASE_ENUM_STRING(CUDA_ERROR_PROFILER_ALREADY_STOPPED);
		CASE_ENUM_STRING(CUDA_ERROR_NO_DEVICE);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_DEVICE);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_IMAGE);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_CONTEXT);
		CASE_ENUM_STRING(CUDA_ERROR_CONTEXT_ALREADY_CURRENT);
		CASE_ENUM_STRING(CUDA_ERROR_MAP_FAILED);
		CASE_ENUM_STRING(CUDA_ERROR_UNMAP_FAILED);
		CASE_ENUM_STRING(CUDA_ERROR_ARRAY_IS_MAPPED);
		CASE_ENUM_STRING(CUDA_ERROR_ALREADY_MAPPED);
		CASE_ENUM_STRING(CUDA_ERROR_NO_BINARY_FOR_GPU);
		CASE_ENUM_STRING(CUDA_ERROR_ALREADY_ACQUIRED);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_MAPPED);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_MAPPED_AS_ARRAY);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_MAPPED_AS_POINTER);
		CASE_ENUM_STRING(CUDA_ERROR_ECC_UNCORRECTABLE);
		CASE_ENUM_STRING(CUDA_ERROR_UNSUPPORTED_LIMIT);
		CASE_ENUM_STRING(CUDA_ERROR_CONTEXT_ALREADY_IN_USE);
		CASE_ENUM_STRING(CUDA_ERROR_PEER_ACCESS_UNSUPPORTED);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_PTX);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_GRAPHICS_CONTEXT);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_SOURCE);
		CASE_ENUM_STRING(CUDA_ERROR_FILE_NOT_FOUND);
		CASE_ENUM_STRING(CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND);
		CASE_ENUM_STRING(CUDA_ERROR_SHARED_OBJECT_INIT_FAILED);
		CASE_ENUM_STRING(CUDA_ERROR_OPERATING_SYSTEM);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_HANDLE);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_FOUND);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_READY);
		CASE_ENUM_STRING(CUDA_ERROR_ILLEGAL_ADDRESS);
		CASE_ENUM_STRING(CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
		CASE_ENUM_STRING(CUDA_ERROR_LAUNCH_TIMEOUT);
		CASE_ENUM_STRING(CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING);
		CASE_ENUM_STRING(CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED);
		CASE_ENUM_STRING(CUDA_ERROR_PEER_ACCESS_NOT_ENABLED);
		CASE_ENUM_STRING(CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE);
		CASE_ENUM_STRING(CUDA_ERROR_CONTEXT_IS_DESTROYED);
		CASE_ENUM_STRING(CUDA_ERROR_ASSERT);
		CASE_ENUM_STRING(CUDA_ERROR_TOO_MANY_PEERS);
		CASE_ENUM_STRING(CUDA_ERROR_HOST_MEMORY_ALREADY_REGISTERED);
		CASE_ENUM_STRING(CUDA_ERROR_HOST_MEMORY_NOT_REGISTERED);
		CASE_ENUM_STRING(CUDA_ERROR_HARDWARE_STACK_ERROR);
		CASE_ENUM_STRING(CUDA_ERROR_ILLEGAL_INSTRUCTION);
		CASE_ENUM_STRING(CUDA_ERROR_MISALIGNED_ADDRESS);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_ADDRESS_SPACE);
		CASE_ENUM_STRING(CUDA_ERROR_INVALID_PC);
		CASE_ENUM_STRING(CUDA_ERROR_LAUNCH_FAILED);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_PERMITTED);
		CASE_ENUM_STRING(CUDA_ERROR_NOT_SUPPORTED);
		CASE_ENUM_STRING(CUDA_ERROR_UNKNOWN);
	};
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "Unknown CUDA error " << Error;
	return Unknown.str();
}


bool Cuda::IsOkay(CUresult Error, const std::string& Context, bool ThrowException)
{
	if (Error == CUDA_SUCCESS)
		return true;

	std::stringstream ErrorStr;
	ErrorStr << "Cuda error in " << Context << ": " << Cuda::GetEnumString(Error) << std::endl;

	if (!ThrowException)
	{
		std::Debug << ErrorStr.str() << std::endl;
		return false;
	}

	return Soy::Assert(Error == CUDA_SUCCESS, ErrorStr.str());
}



void Cuda::GetDevices(ArrayBridge<Cuda::TDevice>&& Metas)
{
	int Count = 0;
	auto Error = cuDeviceGetCount( &Count );
	Cuda::IsOkay(Error,"cuDeviceGetCount");

	for ( int i=0;	i<Count;	i++ )
	{
		CUdevice Device;
		auto Error = cuDeviceGet( &Device, i );
		Cuda::IsOkay( Error, "cuDeviceGet" );
		char Buffer[200] = {0};
		Error = cuDeviceGetName( Buffer, sizeof(Buffer), Device );
		Cuda::IsOkay( Error, "cuDeviceGetName" );

		Metas.PushBack( TDevice(i, Device, Buffer ) );
	}
}

Cuda::TContext::TContext() :
	mContext	(CUDA_INVALID_VALUE)
{
	//	init driver
	auto Error = cuInit( 0 );	//	currently flags must be 0
	Cuda::IsOkay( Error, "cuInit" );

	//	grab a device
	Array<Cuda::TDevice> Devices;
	GetDevices( GetArrayBridge(Devices) );
	if ( Devices.IsEmpty() )
		throw Soy::AssertException("No CUDA devices found");

	//	init with first device
	mDevice = Devices[0];

	//	gr: blocking context atm until we need to speed up
	Error = cuCtxCreate( &mContext, CU_CTX_SCHED_BLOCKING_SYNC, mDevice.mHandle );
	Cuda::IsOkay( Error, "cuCtxCreate" );
}

Cuda::TContext::~TContext()
{
	mDevice = TDevice();

	if ( mContext != CUDA_INVALID_VALUE )
	{
		auto Error = cuCtxDestroy( mContext );
		Cuda::IsOkay( Error, "cuCtxDestroy" );
		mContext = CUDA_INVALID_VALUE;
	}

}


bool Cuda::TContext::Lock()
{
	return true;
};


void Cuda::TContext::Unlock()
{
}




Cuda::TBuffer::TBuffer(size_t DataSize) :
	mAllocSize	( 0 ),
	mData		( nullptr )
{
	void* pData = mData;
	auto Error = cuMemAllocHost( &pData, DataSize );
	Cuda::IsOkay( Error, "cuMemAllocHost" );

	mData = reinterpret_cast<uint8*>(pData);
	mAllocSize = DataSize;

	//	init mem for testing
	memset( mData, 0x4f, mAllocSize );
}

Cuda::TBuffer::~TBuffer()
{
	if ( mData )
	{
		cuMemFreeHost( mData );
		mData = nullptr;
		mAllocSize = 0;
	}
}

void Cuda::TBuffer::Read(CUdeviceptr DeviceData,TStream& Stream,bool Block)
{
	CUresult Error;
	if ( Block )
		Error = cuMemcpyDtoH( mData, DeviceData, mAllocSize );
	else
		Error = cuMemcpyDtoHAsync( mData, DeviceData, mAllocSize, Stream.mStream );

	Cuda::IsOkay( Error, "cuMemcpyDtoHAsync" );
}



Cuda::TStream::TStream(bool Blocking) :
	mStream	( CUDA_INVALID_VALUE )
{
	auto Flags = Blocking ? CU_STREAM_DEFAULT : CU_STREAM_NON_BLOCKING;
	auto Error = cuStreamCreate( &mStream, Flags );
	Cuda::IsOkay( Error, "cuStreamCreate" );
}

Cuda::TStream::~TStream()
{
	if ( mStream != CUDA_INVALID_VALUE )
	{
		cuStreamDestroy( mStream );
		mStream = CUDA_INVALID_VALUE;
	}
}

