#include "MSAOpenCL.h"
#include "MSAOpenCLMemoryObject.h"

namespace msa { 

#if defined(ENABLE_OPENCL_RELEASE_LOCK)
	ofMutex OpenCLMemoryObject::gReleaseLock;
#endif

	OpenCLMemoryObject::OpenCLMemoryObject() :
		clMemObject	( NULL )
	{
		ofLogNotice( __FUNCTION__ );
	}
	
	OpenCLMemoryObject::~OpenCLMemoryObject() {
		ofLogNotice( __FUNCTION__ );
		if(clMemObject) 
		{
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
			ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
			clReleaseMemObject(clMemObject);
		}
	}
}