/***********************************************************************
 
 OpenCL Image Memory Object
 You can either instantiate this class directly. e.g.:
 OpenCLImage myImage;
 myImage.initWithGLTexture(320, 240, GL_RGBA);
 
 or create it via Instead use OpenCL::createImageXXXX. e.g.:
 OpenCLImage *myImage = openCL.createImageWithTexture(320, 240, GL_RGBA);
 (this method is here for backwards compatibility with previous versions of OpenCL)

 ************************************************************************/ 


#pragma once

#include <ofxSoylent.h>
#include "MSAOpenCLMemoryObject.h"


namespace msa { 
	
	class OpenCLImage : public OpenCLMemoryObject
//	, public ofBaseDraws
	{
	public:
		OpenCLImage(OpenCL& Parent);
		
		
		// create an Image with given properties
		// Image is not linked to an OpenGL texture
		// for 2D image, leave depth as 1
		// parameters with default values can be omited
		bool initWithoutTexture(cl_command_queue Queue,int width,
								int height,
								int depth = 1,
								cl_channel_order imageChannelOrder = CL_RGBA,
								cl_channel_type imageChannelDataType = CL_FLOAT,
								cl_mem_flags memFlags = CL_MEM_READ_WRITE,
								void *dataPtr = NULL,
								bool blockingWrite = CL_FALSE);
		
		// create a 2D Image from the ofTexture passed in (they share memory space on device)
		// parameters with default values can be omited
		bool initFromTexture(ofTexture &tex,
							 cl_mem_flags memFlags = CL_MEM_READ_WRITE,
							 int mipLevel = 0);
		
#if defined(OPENCL_OPENGL_INTERPOLARITY)
		// create both a 2D Image AND an ofTexture at the same time (they share memory space on device)
		// parameters with default values can be omited
		bool initWithTexture(cl_command_queue Queue,int width,
							 int height,
							 int glTypeInternal = GL_RGBA,
							 cl_mem_flags memFlags = CL_MEM_READ_WRITE);
#endif
		
		
		
		// read from device memory, into main memoy (into dataPtr)
		// if origin and/or region is NULL, entire image is read
		bool read(cl_command_queue Queue,void *dataPtr,
				  bool blockingRead = CL_TRUE,
				  size_t *pOrigin = NULL,
				  size_t *pRegion = NULL,
				  size_t rowPitch = 0,
				  size_t slicePitch = 0);
		
		// write from main memory (dataPtr), into device memory
		// if origin and/or region is NULL, entire image is written
		bool write(cl_command_queue Queue,void *dataPtr,
				   bool blockingWrite = CL_FALSE,
				   size_t *pOrigin = NULL,
				   size_t *pRegion = NULL,
				   size_t rowPitch = 0,
				   size_t slicePitch = 0);
		
		
		// copy data from another image in device memory
		// if origin and/or region is NULL, entire image is written
		bool copyFrom(cl_command_queue Queue,OpenCLImage &srcImage,
					  size_t *pSrcOrigin = NULL, 
					  size_t *pDstOrigin = NULL, 
					  size_t *pRegion = NULL);
		
		
		
		// return reference to related ofTexture
		// this may be NULL if no ofTexture was setup
		ofTexture &getTexture()	{	return *texture;	}
		
		// draw the ofTexture
		// this may be NULL if no ofTexture was setup
		// this may not be up-to-date...
		// ...need to make sure the openCL kernels are finished with OpenCL::finish() or sync with cl_events
#if !defined(NO_OPENFRAMEWORKS)
		void draw(float x, float y);
		void draw(float x, float y, float w, float h);
#endif

#if !defined(NO_OPENFRAMEWORKS)
		void reset(cl_command_queue Queue);
#endif
		
		int getWidth() const {
			return width;
		}
		
		int getHeight() const {
			return height;
		}
		
		int getDepth() const {
			return depth;
		}
		
		
	protected:
		void	init(int width, int height, int depth);

	protected:
		OpenCL&			mParent;

		int				width;
		int				height;
		int				depth;
		
		size_t			origin[3];
		size_t			region[3];
		
		ofTexture		*texture;
	};
}
