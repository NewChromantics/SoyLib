/***********************************************************************
 
 OpenCL Memory Object base class for Images and Buffers
 Do not instantiate this class

 ************************************************************************/

#pragma once

#include <ofxSoylent.h>

//#define ENABLE_OPENCL_RELEASE_LOCK

namespace msa { 
	class OpenCL;
	
	class OpenCLMemoryObject {
		
	public:
		virtual ~OpenCLMemoryObject();
		
		cl_mem		&getCLMem()	{	return clMemObject;	}
		operator	cl_mem&()	{	return getCLMem();	}
		
	public:
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		static ofMutex	gReleaseLock;
#endif

	protected:
		OpenCLMemoryObject();
		cl_mem		clMemObject;
	};
}