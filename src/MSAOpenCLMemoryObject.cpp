#include "MSAOpenCL.h"
#include "MSAOpenCLMemoryObject.h"

namespace msa { 

#if defined(ENABLE_OPENCL_RELEASE_LOCK)
	ofMutex OpenCLMemoryObject::gReleaseLock;
#endif

	OpenCLMemoryObject::OpenCLMemoryObject() :
		clMemObject	( NULL )
	{
	}
	
	OpenCLMemoryObject::~OpenCLMemoryObject() 
	{
		if(clMemObject) 
		{
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
			ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
			auto Result = clReleaseMemObject(clMemObject);
			assert( Result == CL_SUCCESS );
		}
	}
}