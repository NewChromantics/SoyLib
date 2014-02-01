#include "MSAOpenCL.h"
#include "MSAOpenCLBuffer.h"

namespace msa {

	static bool CHECK_DEVICE_MAX_ALLOC_SIZE = true;

	OpenCLBuffer::OpenCLBuffer(OpenCL& Parent) :
		mParent	( Parent )
	{
	}
	
	bool OpenCLBuffer::initBuffer(int numberOfBytes,
								  cl_mem_flags memFlags,
								  void *dataPtr,
								  bool blockingWrite,cl_command_queue Queue)
	{
		//	need a queue
		assert( Queue );
		if ( !Queue )
			return false;

		//	buffer of zero will fail
		if ( numberOfBytes <= 0 )
			return false;

		//	gr: only in debug, as GetDevice is a bit expensive?
		if ( CHECK_DEVICE_MAX_ALLOC_SIZE )
		{
			//	check with device that bytes isn't too big
			auto* pDevice = mParent.GetDevice( Queue );
			assert(pDevice);
			if ( !pDevice )
				return false;

			//	trying to allocate something we KNOW is too big
			if ( numberOfBytes > pDevice->mInfo.maxMemAllocSize )
				return false;
		}

		//	need a ptr if mapping
		if ( memFlags & CL_MEM_USE_HOST_PTR && !dataPtr )
		{
			assert( memFlags & CL_MEM_USE_HOST_PTR && dataPtr );
			return false;
		}

		//	do not lose existing pointers
		assert( !clMemObject );
		cl_int err = CL_SUCCESS;
		clMemObject = clCreateBuffer( mParent.getContext(), memFlags, numberOfBytes, memFlags & CL_MEM_USE_HOST_PTR ? dataPtr : NULL, &err);
		assert( clMemObject && err == CL_SUCCESS);
		if ( err != CL_SUCCESS || !clMemObject )
			return false;
		
		if(dataPtr) 
			if ( !write( dataPtr, 0, numberOfBytes, blockingWrite, Queue ) )
				return false;

		return true;
	}
	
	
#if defined(OPENCL_OPENGL_INTERPOLARITY)
	void OpenCLBuffer::initFromGLObject(GLuint glBufferObject,
										cl_mem_flags memFlags)
	{	
		cl_int err;
		clMemObject= clCreateFromGLBuffer( mParent.getContext(), memFlags, glBufferObject, &err);
		assert(err != CL_INVALID_CONTEXT);
		assert(err != CL_INVALID_VALUE);
		assert(err != CL_INVALID_GL_OBJECT);
		assert(err != CL_OUT_OF_HOST_MEMORY);
		assert(err == CL_SUCCESS);
		assert(clMemObject);	
	}
#endif
	
	bool OpenCLBuffer::read(void *dataPtr, int startOffsetBytes, int numberOfBytes, bool blockingRead,cl_command_queue Queue) {
		if ( !Queue )
		{
			assert( Queue );
			return false;
		}
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueReadBuffer( Queue, clMemObject, blockingRead, startOffsetBytes, numberOfBytes, dataPtr, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	
	bool OpenCLBuffer::write(void *dataPtr, int startOffsetBytes, int numberOfBytes, bool blockingWrite,cl_command_queue Queue) {
		if ( !Queue )
		{
			assert( Queue );
			return false;
		}
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueWriteBuffer( Queue, clMemObject, blockingWrite, startOffsetBytes, numberOfBytes, dataPtr, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	bool OpenCLBuffer::writeAsync(void *dataPtr, int startOffsetBytes, int numberOfBytes,cl_event* Event,cl_command_queue Queue) {
		if ( !Queue )
		{
			assert( Queue );
			return false;
		}
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		bool blockingWrite = false;
		cl_int err = clEnqueueWriteBuffer( Queue, clMemObject, blockingWrite, startOffsetBytes, numberOfBytes, dataPtr, 0, NULL, Event );
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	bool OpenCLBuffer::copyFrom(OpenCLBuffer &srcBuffer, int srcOffsetBytes, int dstOffsetBytes, int numberOfBytes,cl_command_queue Queue) {
		if ( !Queue )
		{
			assert( Queue );
			return false;
		}
		cl_int err = clEnqueueCopyBuffer( Queue, srcBuffer.getCLMem(), clMemObject, srcOffsetBytes, dstOffsetBytes, numberOfBytes, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
}
