#include "MSAOpenCL.h"
#include "MSAOpenCLImage.h"

namespace msa {
	
	OpenCLImage::OpenCLImage(OpenCL& Parent) :
		mParent	( Parent ),
		texture	( NULL )
	{
		ofLogNotice( __FUNCTION__ );
	}
	
	
	bool OpenCLImage::initWithoutTexture(cl_command_queue Queue,int w,
										 int h,
										 int d,
										 cl_channel_order imageChannelOrder,
										 cl_channel_type imageChannelDataType,
										 cl_mem_flags memFlags,
										 void *dataPtr,
										 bool blockingWrite)
	{
		ofLogNotice( __FUNCTION__ );
		
		init(w, h, d);
		
		cl_int err;
		cl_image_format imageFormat;
		imageFormat.image_channel_order		= imageChannelOrder;
		imageFormat.image_channel_data_type	= imageChannelDataType;
		
		int image_row_pitch = 0;	// TODO
		int image_slice_pitch = 0;
		
		if(clMemObject) 
		{
			clReleaseMemObject(clMemObject);
			clMemObject = NULL;
		}
		
		if(depth == 1) {
			clMemObject = clCreateImage2D( mParent.getContext(), memFlags, &imageFormat, width, height, image_row_pitch, memFlags & CL_MEM_USE_HOST_PTR ? dataPtr : NULL, &err);
		} else {
			clMemObject = clCreateImage3D( mParent.getContext(), memFlags, &imageFormat, width, height, depth, image_row_pitch, image_slice_pitch, memFlags & CL_MEM_USE_HOST_PTR ? dataPtr : NULL, &err);
		}
		assert(err != CL_INVALID_CONTEXT);
		assert(err != CL_INVALID_VALUE);
		assert(err != CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
		assert(err != CL_INVALID_IMAGE_SIZE );
		assert(err != CL_INVALID_HOST_PTR);
		assert(err != CL_IMAGE_FORMAT_NOT_SUPPORTED);
		assert(err != CL_MEM_OBJECT_ALLOCATION_FAILURE);
		assert(err != CL_INVALID_OPERATION);
		assert(err != CL_OUT_OF_HOST_MEMORY );
		assert(err == CL_SUCCESS);
		assert(clMemObject);
		
		bool WriteSuccess = true;
		if(dataPtr) 
		{
			WriteSuccess = write(Queue,dataPtr, blockingWrite);
		}
		
		if(texture) 
		{
			delete texture;
			texture = NULL;
		}

		return WriteSuccess && clMemObject && (err == CL_SUCCESS);
	}
	
	
	
	bool OpenCLImage::initFromTexture(ofTexture &tex,
									  cl_mem_flags memFlags,
									  int mipLevel)
	{
		ofLogNotice( __FUNCTION__ );
		
		init(tex.getWidth(), tex.getHeight(), 1);
		
		cl_int err = CL_SUCCESS;
		if(clMemObject) 
		{
			clReleaseMemObject(clMemObject);
			clMemObject = NULL;
		}
		
		//	gr: missing from amd sdk?
		//clMemObject = clCreateFromGLTexture2D(pOpenCL->getContext(), memFlags, tex.getTextureData().textureTarget, mipLevel, tex.getTextureData().textureID, &err);
		assert(err != CL_INVALID_CONTEXT);
		assert(err != CL_INVALID_VALUE);
		//	assert(err != CL_INVALID_MIPLEVEL);
		assert(err != CL_INVALID_GL_OBJECT);
		assert(err != CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
		assert(err != CL_OUT_OF_HOST_MEMORY);
		assert(err == CL_SUCCESS);
		assert(clMemObject);
		
		texture = &tex;

		return clMemObject && (err == CL_SUCCESS);
	}
	
	
	
	
#if defined(OPENCL_OPENGL_INTERPOLARITY)
	bool OpenCLImage::initWithTexture(cl_command_queue Queue,int w,
									  int h,
									  int glTypeInternal,
									  cl_mem_flags memFlags)
	{
		ofLogNotice( __FUNCTION__ );
		
		if(texture)
		{
			delete texture;
			texture = NULL;
		}
		texture = new ofTexture();
		texture->allocate(w, h, glTypeInternal);
		if ( !initFromTexture(*texture, memFlags, 0) )
			return false;
		reset(Queue);
		return true;
	}
#endif
	
	
	void OpenCLImage::init(int w, int h, int d) 
	{
		d = ofMax( d, 1 );
		
		this->width			= w;
		this->height		= h;
		this->depth			= d;
		
		origin[0] = 0; 
		origin[1] = 0;
		origin[2] = 0;
		
		region[0] = width; 
		region[1] = height;
		region[2] = depth;
		
		BufferString<1000> Debug;
		Debug << __FUNCTION__ << width << ", " << height << ", " << depth;
		ofLogNotice( Debug.c_str() );
	}
	
#if !defined(NO_OPENFRAMEWORKS)
	void OpenCLImage::reset(cl_command_queue Queue) {
		ofLogNotice( __FUNCTION__ );
		int numElements = width * height * 4; // TODO, make real
		if(texture->getTextureData().pixelType == GL_FLOAT) 
			numElements *= sizeof(cl_float);
		char *data = new char[numElements];
		memset(data, 0, numElements);
		write(Queue,data, true);
		delete []data;
	}
#endif
	
	bool OpenCLImage::read(cl_command_queue Queue,void *dataPtr, bool blockingRead, size_t *pOrigin, size_t *pRegion, size_t rowPitch, size_t slicePitch) {
		assert( Queue );
		if ( !Queue )
			return false;
		if(pOrigin == NULL) pOrigin = origin;
		if(pRegion == NULL) pRegion = region;
		
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueReadImage( Queue, clMemObject, blockingRead, pOrigin, pRegion, rowPitch, slicePitch, dataPtr, 0, NULL, NULL);
		return (err==CL_SUCCESS);
	}
	
	
	bool OpenCLImage::write(cl_command_queue Queue,void *dataPtr, bool blockingWrite, size_t *pOrigin, size_t *pRegion, size_t rowPitch, size_t slicePitch) {
		assert( Queue );
		if ( !Queue )
			return false;
		if(pOrigin == NULL) pOrigin = origin;
		if(pRegion == NULL) pRegion = region;
		
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueWriteImage( Queue, clMemObject, blockingWrite, pOrigin, pRegion, rowPitch, slicePitch, dataPtr, 0, NULL, NULL);
		return (err==CL_SUCCESS);
	}
	
	bool OpenCLImage::copyFrom(cl_command_queue Queue,OpenCLImage &srcImage, size_t *pSrcOrigin, size_t *pDstOrigin, size_t *pRegion) {
		assert( Queue );
		if ( !Queue )
			return false;
		if(pSrcOrigin == NULL) pSrcOrigin = origin;
		if(pDstOrigin == NULL) pDstOrigin = origin;
		if(pRegion == NULL) pRegion = region;
		
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueCopyImage( Queue, srcImage.getCLMem(), clMemObject, pSrcOrigin, pDstOrigin, pRegion, 0, NULL, NULL);
		return (err==CL_SUCCESS);
	}
	
		
#if !defined(NO_OPENFRAMEWORKS)
	void OpenCLImage::draw(float x, float y) {
		if(texture) texture->draw(x, y);
	}
#endif

#if !defined(NO_OPENFRAMEWORKS)
	void OpenCLImage::draw(float x, float y, float w, float h) {
		if(texture) texture->draw(x, y, w, h);
	}
#endif
}

