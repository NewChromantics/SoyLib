#include "MSAOpenCL.h"
#include "MSAOpenCLKernel.h"

namespace msa { 
	
	OpenCLKernel::OpenCLKernel(OpenCL& Parent,cl_kernel Kernel,cl_command_queue Queue,std::string Name) :
		mParent		( Parent ),
		mName		( Name ),
		mKernel		( Kernel ),
		mQueue		( Queue )
	{
		assert( mKernel );
		assert( mQueue );
	}
	
	
	OpenCLKernel::~OpenCLKernel()
	{
		if ( mKernel )
		{
			clReleaseKernel( mKernel );
		}
	}
	

	bool OpenCLKernel::run(bool Blocking,int numDimensions, size_t *globalSize, size_t *localSize) {
		assert(mKernel);
		if ( !mKernel )
			return false;
		
		//	size_t localSize = MIN(n, info.maxWorkGroupSize);
		cl_event WaitEvent = NULL;
		cl_int err = clEnqueueNDRangeKernel( getQueue(), mKernel, numDimensions, NULL, globalSize, localSize, 0, NULL, Blocking ? &WaitEvent : NULL );
		assert(err == CL_SUCCESS);
		if ( err != CL_SUCCESS )
			return false;

		//	block by waiting for the execute-event
		if ( Blocking && WaitEvent )
			err = clWaitForEvents( 1, &WaitEvent );

		return (err == CL_SUCCESS);
	}
	
	bool OpenCLKernel::run1D(bool Blocking,size_t globalSize, size_t localSize) {
		size_t globalSizes[1];
		globalSizes[0] = globalSize;
		if(localSize) {
			size_t localSizes[1];
			localSizes[0] = localSize;
			return run( Blocking, 1, globalSizes, localSizes);
		} else {
			return run( Blocking, 1, globalSizes, NULL);
		}
	}
	
	bool OpenCLKernel::run2D(bool Blocking,size_t globalSizeX, size_t globalSizeY, size_t localSizeX, size_t localSizeY) {
		size_t globalSizes[2];
		globalSizes[0] = globalSizeX;
		globalSizes[1] = globalSizeY;
		if(localSizeY && localSizeX) {
			size_t localSizes[2];
			localSizes[0] = localSizeX;
			localSizes[1] = localSizeY;
			return run( Blocking, 2, globalSizes, localSizes);
		} else {
			return run( Blocking, 2, globalSizes, NULL);
		}
	}
	
	bool OpenCLKernel::run3D(bool Blocking,size_t globalSizeX, size_t globalSizeY, size_t globalSizeZ, size_t localSizeX, size_t localSizeY, size_t localSizeZ) {
		size_t globalSizes[3];
		globalSizes[0] = globalSizeX;
		globalSizes[1] = globalSizeY;
		globalSizes[2] = globalSizeZ;
		if(localSizeZ && localSizeY && localSizeX) {
			size_t localSizes[3];
			localSizes[0] = localSizeX;
			localSizes[1] = localSizeY;
			localSizes[2] = localSizeZ;
			return run( Blocking, 3, globalSizes, localSizes);
		} else {
			return run( Blocking, 3, globalSizes, NULL);
		}
	}
	
}
