/***********************************************************************
 
 OpenCL Buffer Memory Object
 You can either instantiate this class directly. e.g.:
 OpenCLBuffer myBuffer;
 myImage.initFromGLObject(myVbo);
 
 or create it via Instead use OpenCL::createBufferXXXX. e.g.:
 OpenCLBuffer *myBuffer = openCL.createBufferFromGLObject(myVBO);
 (this method is here for backwards compatibility with previous versions of OpenCL)
 
 ************************************************************************/

#pragma once

#include <ofxSoylent.h>
#include "MSAOpenCLMemoryObject.h"

namespace msa {
 	
	class OpenCLBuffer : public OpenCLMemoryObject {
	public:
		OpenCLBuffer(OpenCL& Parent);

		// if dataPtr parameter is passed in, data is uploaded immediately
		// parameters with default values can be omited
		bool initBuffer(	int numberOfBytes,
						cl_mem_flags memFlags,
						void *dataPtr,
						bool blockingWrite,cl_command_queue Queue);
		
#if defined(OPENCL_OPENGL_INTERPOLARITY)
		// create buffer from the GL Object - e.g. VBO (they share memory space on device)
		// parameters with default values can be omited
		void initFromGLObject(GLuint glBufferObject,
							  cl_mem_flags memFlags = CL_MEM_READ_WRITE);
#endif
		
		// read from device memory, into main memoy (into dataPtr)
		bool read(void *dataPtr,
				  int startOffsetBytes,
				  int numberOfBytes,
				  bool blockingRead,cl_command_queue Queue);
		
		// write from main memory (dataPtr), into device memory
		bool write(void *dataPtr,
				   int startOffsetBytes,
				   int numberOfBytes,
				   bool blockingWrite,cl_command_queue Queue);
		
		bool writeAsync(void *dataPtr,
				   int startOffsetBytes,
				   int numberOfBytes,
				   cl_event* Event,cl_command_queue Queue);
		
		
		// copy data from another object on device memory
		bool copyFrom(OpenCLBuffer &srcBuffer,
					  int srcOffsetBytes,
					  int dstOffsetBytes,
					  int numberOfBytes,cl_command_queue Queue);

	public:
		OpenCL&		mParent;
	};
}
