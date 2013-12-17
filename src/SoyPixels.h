#pragma once

#include "memheap.hpp"
#include "SoyOpenCl.h"

//#define ENABLE_OPENCV
//#define ENABLE_OPENCL

class ofTexture;

#if defined(ENABLE_OPENCV)
class ofxCvImage;
class ofxCvGrayscaleImage;
class IplImage;
#endif

#if defined(ENABLE_OPENCL)
class SoyOpenClManager;
class SoyOpenClKernel;
class clSetPixelParams;
#endif

namespace msa
{
	class OpenCLImage;
}

class SoyPixels
{
public:
	SoyPixels(prmem::Heap& Heap=prcore::Heap) :
		mChannels	( 0 ),
		mWidth		( 0 ),
		mPixels		( Heap )
	{
	}

	bool		Init(uint16 Width,uint16 Height,uint8 Channels,const ofColour& DefaultColour);
	bool		Init(uint16 Width,uint16 Height,uint8 Channels);

	bool		Get(ofPixels& Pixels) const;
#if defined(ENABLE_OPENCV)
	bool		Get(ofxCvImage& Pixels) const;
	bool		Get(ofxCvGrayscaleImage& Pixels,SoyOpenClManager& OpenClManager) const;
#endif
	bool		Get(ofTexture& Pixels) const;
#if defined(ENABLE_OPENCL)
	bool		Get(msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel,cl_int clMemMode=CL_MEM_READ_WRITE) const;
#endif

	bool		Set(const ofPixels& Pixels);
#if defined(ENABLE_OPENCV)
	bool		Set(ofxCvImage& Pixels);
	bool		Set(const IplImage& Pixels);	//	opencv internal image
#endif
	bool		Set(const SoyPixels& Pixels);
	bool		Set(const SoyPixels& Pixels,uint8 Channel);
	bool		SetChannels(uint8 Channels);
#if defined(ENABLE_OPENCL)
	bool		Set(const msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel,uint8 Channels);
	bool		SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const ofColour& DefaultColour=ofColour(255,255,255,255));
	bool		SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const clSetPixelParams& Params);
#endif
	void		Clear();						//	delete data

	bool		IsValid() const		{	return (mWidth>0) && (mChannels>0);	}
	uint8		GetChannels() const	{	return mChannels;	}
	uint16		GetWidth() const	{	return mWidth;	}
	uint16		GetHeight() const	{	return IsValid() ? mPixels.GetSize() / (mChannels*mWidth) : 0;	}
	bool		GetOpenglFormat(int& glFormat) const;
	bool		GetOpenclFormat(int& ChannelOrder) const;
	const Array<uint8>&	GeSoyPixelsArray() const	{	return mPixels;	}

	Array<uint8>&	GeSoyPixelsArray()			{	return mPixels;	}
	void			DumbSetChannels(int Channels)	{	mChannels = Channels;	}

private:
	uint8			mChannels;
	uint16			mWidth;
	Array<uint8>	mPixels;
};
DECLARE_TYPE_NAME( SoyPixels );
