#include "SoyPixels.h"

#if defined(ENABLE_OPENCV)
#include "ofxOpenCV.h"
#endif

#if defined(ENABLE_OPENCL)
#include "SoyOpenCl.h"
#endif




bool SoyPixels::Get(ofPixels& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	Pixels.allocate( GetWidth(), GetHeight(), mChannels );	//	remove log notice
	Pixels.setFromPixels( mPixels.GetArray(), GetWidth(), GetHeight(), mChannels );
	return true;
}

bool SoyPixels::Get(ofTexture& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	int glFormat;
	if ( !GetOpenglFormat(glFormat) )
	{
		Pixels.clear();
		return false;
	}
	
	Pixels.allocate( GetWidth(), GetHeight(), glFormat );
	Pixels.loadData( mPixels.GetArray(), GetWidth(), GetHeight(), glFormat );
	return true;
}

#if defined(ENABLE_OPENCV)
bool SoyPixels::Get(ofxCvImage& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	Pixels.allocate( GetWidth(), GetHeight() );

	//	check channels
	if ( Pixels.getChannels() != GetChannels() )
	{
		//	convert number of channels
		ofPixels PixelsN;
		if ( !Get( PixelsN ) )
			return false;
		PixelsN.setNumChannels( Pixels.getChannels() );
		Pixels.setFromPixels( PixelsN );
	}
	else
	{
		Pixels.setFromPixels( mPixels.GetArray(), GetWidth(), GetHeight() );
	}
	return true;
}
#endif


#if defined(ENABLE_OPENCV)
bool SoyPixels::Get(ofxCvGrayscaleImage& Pixels,SoyOpenClManager& OpenClManager) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	if ( Pixels.getWidth() != GetWidth() && Pixels.getHeight() != GetHeight() )
	{
		Pixels.allocate( GetWidth(), GetHeight() );
	}

	//	check channels
	if ( Pixels.getChannels() != GetChannels() )
	{
		//	use shader to convert
		SoyPixels Bri;
		ClShaderRgbToBri RgbToBriShader( OpenClManager );
		if ( !RgbToBriShader.Run( *this, Bri ) )
		{
			//	if failed, use non-shader version
			return Get( Pixels );
		}

		Pixels.setFromPixels( Bri.mPixels.GetArray(), GetWidth(), GetHeight() );
	}
	else
	{
		Pixels.setFromPixels( mPixels.GetArray(), GetWidth(), GetHeight() );
	}
	return true;
}
#endif


#if defined(ENABLE_OPENCL)
bool SoyPixels::Get(msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel,cl_int clMemMode) const
{
	if ( !IsValid() )
		return false;
	if ( !Kernel.IsValid() )
		return false;
	
	cl_int ChannelOrder;
	if ( !GetOpenclFormat( ChannelOrder ) )
		return false;

	//	can't use 8 bit, 3 channels
	//	http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/cl_image_format.html
	//	so fail (need to convert this to RGBA
	if ( ChannelOrder == CL_RGB )
		return false;

	//	block write on our thread's queue
	bool BlockingWrite = true;
	if ( !Pixels.initWithoutTexture( Kernel.GetQueue(), GetWidth(), GetHeight(), 1, ChannelOrder, CL_UNORM_INT8, clMemMode, const_cast<uint8*>(mPixels.GetArray()), BlockingWrite ) )
		return false;
	
	return true;
}
#endif

void SoyPixels::Clear()
{
	mChannels = 0;
	mWidth = 0;
	mPixels.Clear(true);
}

bool SoyPixels::SetChannels(uint8 Channels)
{
	if ( Channels < 1 )
		return false;
	if ( Channels == GetChannels() )
		return true;
	if ( !IsValid() )
		return false;

	//	slow
	ofScopeTimerWarning Timer("SoyPixels::SetChannels",1);
	ofPixels PixelsX;
	Get( PixelsX );
	PixelsX.setNumChannels( Channels );
	Set( PixelsX );
	return true;
}


#if defined(ENABLE_OPENCL)
bool SoyPixels::SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const ofColour& FillerColour)
{
	clSetPixelParams Params = _clSetPixelParams();
	Params.mDefaultColour = ofToCl_Rgba_int4( FillerColour );
	return SetChannels( Channels, OpenClManager, Params );
}
#endif

#if defined(ENABLE_OPENCL)
bool SoyPixels::SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const clSetPixelParams& Params)
{
	if ( Channels < 1 )
		return false;

	//	skip if no change in channels, unless we have other options
	if ( Channels == GetChannels() )
		return true;

	if ( !IsValid() )
		return false;

	//	use the lovely fast shader!
	//	non-shader version takes about 340ms for 1080p
	ClShaderSetPixelChannels Shader( OpenClManager );
	if ( !Shader.Run( *this, Channels, Params ) )
		return false;
	return true;
}
#endif


#if defined(ENABLE_OPENCL)
bool SoyPixels::Set(const msa::OpenCLImage& PixelsConst,SoyOpenClKernel& Kernel,uint8 Channels)
{
	if ( !Kernel.IsValid() )
		return false;

	auto& Pixels = const_cast<msa::OpenCLImage&>( PixelsConst );
	//	clformat is either RGBA(4 chan) or A/LUM(1 chan)
	if ( Channels != 1 && Channels != 4 )
	{
		assert( false );
		Clear();
		return false;
	}
	
	mWidth = Pixels.getWidth();
	mChannels = Channels;
	int Alloc = mWidth * mChannels * Pixels.getHeight();
	if ( !mPixels.SetSize( Alloc, false, true ) )
		return false;

	bool Blocking = true;
	if ( !Pixels.read( Kernel.GetQueue(), mPixels.GetArray(), Blocking ) )
	{
		Clear();
		return false;
	}

	//	check for corruption as there is no size checking in the read :/
	mPixels.GetHeap().Debug_Validate( mPixels.GetArray() );
	return true;
}
#endif

bool SoyPixels::Set(const ofPixels& Pixels)
{
	mWidth = Pixels.getWidth();
	mChannels = Pixels.getNumChannels();
	int Alloc = mWidth * mChannels * Pixels.getHeight();
	mPixels.SetSize( Alloc, false, true );
	memcpy( mPixels.GetArray(), Pixels.getPixels(), Alloc );
	return true;
}

bool SoyPixels::Set(const SoyPixels& Pixels)
{
	mWidth = Pixels.GetWidth();
	mChannels = Pixels.mChannels;
	mPixels = Pixels.mPixels;
	return true;
}

#if defined(ENABLE_OPENCV)
bool SoyPixels::Set(ofxCvImage& Pixels)
{
	ofPixels& RealPixels = Pixels.geSoyPixelsRef();
	Set( RealPixels );
	return true;
}
#endif

#if defined(ENABLE_OPENCV)
bool SoyPixels::Set(const IplImage& Pixels)
{
	mWidth = Pixels.width;
	mChannels = Pixels.nChannels;
	int Alloc = mWidth * mChannels * Pixels.height;
	mPixels.SetSize( Alloc, false, true );
	memcpy( mPixels.GetArray(), Pixels.imageData, Alloc );
	return true;
}
#endif

bool SoyPixels::Set(const SoyPixels& That,uint8 Channel)
{
	//	non existant channel
	if ( Channel >= That.GetChannels() )
		return false;

	//	alloc
	mWidth = That.GetWidth();
	mChannels = 1;
	int Alloc = mWidth * mChannels * That.GetHeight();
	mPixels.Clear(false);
	mPixels.Reserve( Alloc, false );

	//	copy each pixel from source's channel
	int Step = That.GetChannels();
	auto& ThaSoyPixels = That.GeSoyPixelsArray();
	for ( int p=Channel;	p<ThaSoyPixels.GetSize();	p+=Step )
	{
		uint8 Component = ThaSoyPixels[p];
		mPixels.PushBack( Component );
	}
	assert( this->GetHeight() == That.GetHeight() );

	return true;
}

bool SoyPixels::GetOpenglFormat(int& glFormat) const
{
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( mChannels )
	{
	case 1:	glFormat = GL_LUMINANCE;	return true;
	case 3:	glFormat = GL_RGB;			return true;
	case 4:	glFormat = GL_RGBA;			return true;
	}
	return false;
}


bool SoyPixels::GetOpenclFormat(int& clFormat) const
{
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( mChannels )
	{
	case 1:	clFormat = CL_LUMINANCE;	return true;
	case 3:	clFormat = CL_RGB;			return true;
	case 4:	clFormat = CL_RGBA;			return true;
	}
	return false;
}


bool SoyPixels::Init(uint16 Width,uint16 Height,uint8 Channels,const ofColour& DefaultColour)
{
	if ( !Init( Width, Height, Channels ) )
		return false;

	//	init
	BufferArray<uint8,255> Components;
	if ( Channels >= 1 )	Components.PushBack( DefaultColour.r );
	if ( Channels >= 2 )	Components.PushBack( DefaultColour.g );
	if ( Channels >= 3 )	Components.PushBack( DefaultColour.b );
	if ( Channels >= 4 )	Components.PushBack( DefaultColour.a );
	for ( int c=5;	c<Channels;	c++ )
		Components.PushBack( DefaultColour.r );

	//	are all the components the same? use SetAll
	bool AllSame = true;
	for ( int c=1;	c<Components.GetSize();	c++ )
		AllSame &= (Components[0] == Components[c]);
	
	if ( AllSame )
	{
		mPixels.SetAll( Components[0] );
	}
	else
	{
		for ( int pc=0;	pc<mPixels.GetSize();	pc++ )
			mPixels[pc] = Components[pc%Components.GetSize()];
	}

	return true;	
}



bool SoyPixels::Init(uint16 Width,uint16 Height,uint8 Channels)
{
	//	alloc
	mWidth = Width;
	mChannels = Channels;
	int Alloc = mWidth * mChannels * Height;
	mPixels.SetSize( Alloc, false );
	assert( this->GetHeight() == Height );
	return true;	
}

