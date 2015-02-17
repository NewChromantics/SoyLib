#include "SoyPixels.h"
#include "SoyDebug.h"
#include "SoyTime.h"
#include <functional>
#include "SoyPng.h"
#include "RemoteArray.h"

#if defined(SOY_OPENGL)
#if defined(TARGET_OSX)
#include <Opengl/gl.h>
#include <OpenGL/OpenGL.h>
#endif
#endif

#define USE_STB
#if defined(USE_STB)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

/// Maximum value that a uint16_t pixel will take on in the buffer of any of the FREENECT_DEPTH_MM or FREENECT_DEPTH_REGISTERED frame callbacks
#define FREENECT_DEPTH_MM_MAX_VALUE 10000
/// Value indicating that this pixel has no data, when using FREENECT_DEPTH_MM or FREENECT_DEPTH_REGISTERED depth modes
#define FREENECT_DEPTH_MM_NO_VALUE 0
/// Maximum value that a uint16_t pixel will take on in the buffer of any of the FREENECT_DEPTH_11BIT, FREENECT_DEPTH_10BIT, FREENECT_DEPTH_11BIT_PACKED, or FREENECT_DEPTH_10BIT_PACKED frame callbacks
#define FREENECT_DEPTH_RAW_MAX_VALUE 2048
/// Value indicating that this pixel has no data, when using FREENECT_DEPTH_11BIT, FREENECT_DEPTH_10BIT, FREENECT_DEPTH_11BIT_PACKED, or FREENECT_DEPTH_10BIT_PACKED
#define FREENECT_DEPTH_RAW_NO_VALUE 2047


bool SoyPixelsFormat::GetIsFrontToBackDepth(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:			return true;
		case SoyPixelsFormat::FreenectDepthmm:		return false;
		case SoyPixelsFormat::FreenectDepth10bit:	return false;
		case SoyPixelsFormat::FreenectDepth11bit:	return false;
			
		default:
			return false;
	}
}


int SoyPixelsFormat::GetMaxValue(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:		return (1<<13);
		case SoyPixelsFormat::FreenectDepthmm:	return FREENECT_DEPTH_MM_MAX_VALUE;
		case SoyPixelsFormat::FreenectDepth10bit:	return 1022;
		
		//	test: min: 246, Max: 1120
		//case SoyPixelsFormat::FreenectDepth11bit:	return FREENECT_DEPTH_RAW_MAX_VALUE;
		case SoyPixelsFormat::FreenectDepth11bit:	return 1120;

		default:
			return 0;
	}
}

int SoyPixelsFormat::GetMinValue(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:		return 1;
		case SoyPixelsFormat::FreenectDepthmm:	return 0;
		case SoyPixelsFormat::FreenectDepth10bit:	return 0;

		//	test: min: 246, Max: 1120
		//case SoyPixelsFormat::FreenectDepth11bit:	return 0;
		case SoyPixelsFormat::FreenectDepth11bit:	return 200;
		default:
			return 0;
	}
}


int SoyPixelsFormat::GetInvalidValue(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:			return 0;
		case SoyPixelsFormat::FreenectDepthmm:		return FREENECT_DEPTH_MM_NO_VALUE;
		case SoyPixelsFormat::FreenectDepth11bit:	return FREENECT_DEPTH_RAW_NO_VALUE;
		case SoyPixelsFormat::FreenectDepth10bit:	return 1023;	//	10 bit
		default:
			return 0;
	}
}

int SoyPixelsFormat::GetPlayerIndexFirstBit(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:			return 13;
		default:
			return -1;
	}
}



int SoyPixelsFormat::GetChannelCount(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
	default:
	case Invalid:		return 0;
	case Greyscale:		return 1;
	case GreyscaleAlpha:	return 2;
	case RGB:			return 3;
	case RGBA:			return 4;
	case BGRA:			return 4;
	case KinectDepth:	return 2;	//	only 1 channel, but 16 bit
	case FreenectDepth11bit:	return 2;	//	only 1 channel, but 16 bit
	case FreenectDepth10bit:	return 2;	//	only 1 channel, but 16 bit
	case FreenectDepthmm:	return 2;	//	only 1 channel, but 16 bit
	}
}

SoyPixelsFormat::Type SoyPixelsFormat::GetFormatFromChannelCount(int ChannelCount)
{
	switch ( ChannelCount )
	{
	default:			return SoyPixelsFormat::Invalid;
	case 1:				return SoyPixelsFormat::Greyscale;
	case 2:				return SoyPixelsFormat::GreyscaleAlpha;
	case 3:				return SoyPixelsFormat::RGB;
	case 4:				return SoyPixelsFormat::RGBA;
	}
}


std::ostream& operator<< (std::ostream &out,const SoyPixelsFormat::Type &in)
{
	switch ( in )
	{
		case SoyPixelsFormat::Greyscale:		return out << "Greyscale";
		case SoyPixelsFormat::GreyscaleAlpha:	return out << "GreyscaleAlpha";
		case SoyPixelsFormat::RGB:				return out << "RGB";
		case SoyPixelsFormat::RGBA:				return out << "RGBA";
		case SoyPixelsFormat::BGRA:				return out << "BGRA";
		case SoyPixelsFormat::BGR:				return out << "BGR";
		case SoyPixelsFormat::KinectDepth:		return out << "KinectDepth";
		case SoyPixelsFormat::FreenectDepth11bit:	return out << "FreenectDepth11bit";
		case SoyPixelsFormat::FreenectDepth10bit:	return out << "FreenectDepth10bit";
		case SoyPixelsFormat::FreenectDepthmm:	return out << "FreenectDepthmm";
		default:
			return out << "<unknown SoyPixelsFormat:: " << static_cast<int>(in) << ">";
	}
}

#if defined(OPENFRAMEWORKS)
bool TPixels::Get(ofImage& Pixels) const		
{
	if ( !Get( Pixels.getPixelsRef() ) )
		return false;
	//	update image's params to match pixels
	Pixels.update();
	return true;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Get(ofPixels& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	Pixels.allocate( GetWidth(), GetHeight(), GetChannels() );	//	remove log notice
	Pixels.setFromPixels( mPixels.GetArray(), GetWidth(), GetHeight(), GetChannels() );
	return true;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Get(ofTexture& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	int glFormat;
	if ( !GetMeta().GetOpenglFormat(glFormat) )
	{
		Pixels.clear();
		return false;
	}
	
	Pixels.allocate( GetWidth(), GetHeight(), glFormat );
	Pixels.loadData( mPixels.GetArray(), GetWidth(), GetHeight(), glFormat );
	return true;
}
#endif


#if defined(OPENFRAMEWORKS)
bool TPixels::Get(ofxCvImage& Pixels) const
{
	if ( !IsValid() )
	{
		Pixels.clear();
		return false;
	}

	Pixels.allocate( GetWidth(), GetHeight() );

	//	check channels
	if ( Pixels.getPixelsRef().getNumChannels() != GetChannels() )
	{
		//	convert number of channels
		ofPixels PixelsN;
		if ( !Get( PixelsN ) )
			return false;
		PixelsN.setNumChannels( Pixels.getPixelsRef().getNumChannels() );
		Pixels.setFromPixels( PixelsN );
	}
	else
	{
		Pixels.setFromPixels( mPixels.GetArray(), GetWidth(), GetHeight() );
	}
	return true;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Get(ofxCvGrayscaleImage& Pixels,SoyOpenClManager& OpenClManager) const
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
	if ( Pixels.getPixelsRef().getNumChannels() != GetChannels() )
	{
		//	use shader to convert
		TPixels Bri;
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


#if defined(SOY_OPENCL)
bool TPixels::Get(msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel,cl_int clMemMode) const
{
	if ( !IsValid() )
		return false;
	if ( !Kernel.IsValid() )
		return false;
	
	return Get( Pixels, Kernel.GetQueue(), clMemMode );
}
#endif

#if defined(SOY_OPENCL)
bool TPixels::Get(msa::OpenCLImage& Pixels,cl_command_queue Queue,cl_int clMemMode) const
{
	assert( Queue );
	if ( !Queue )
		return false;
	
	cl_int ChannelOrder;
	if ( !GetMeta().GetOpenclFormat( ChannelOrder ) )
		return false;

	//	can't use 8 bit, 3 channels
	//	http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/cl_image_format.html
	//	so fail (need to convert this to RGBA)
	//	gr: auto convert here with temp pixels?
	if ( ChannelOrder == CL_RGB )
		return false;

	//	block write on our thread's queue
	bool BlockingWrite = true;
	if ( !Pixels.initWithoutTexture( Queue, GetWidth(), GetHeight(), 1, ChannelOrder, CL_UNORM_INT8, clMemMode, const_cast<uint8*>(mPixels.GetArray()), BlockingWrite ) )
		return false;
	
	return true;
}
#endif


bool SoyPixelsImpl::SetChannels(uint8 Channels)
{
	SoyPixelsFormat::Type Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	return SetFormat( Format );
}


void SetDepthColour(char& Red,char& Green,char& Blue,float Depth,int PlayerIndex,bool NoDepthValue)
{
	//	yelow if invalid
	if ( NoDepthValue )
	{
		Red = 255;
		Blue = 0;
		Green = 255;
		return;
	}
	
	//	magenta for invalid depth
	if ( Depth > 1.f || Depth < 0.f )
	{
		Red = 255;
		Blue = 255;
		Green = 20;
		return;
	}
	
	static int MinBrightness = 0;
	//	gr: invert to distinguish invalid from close
	uint8 Greyscale = 255 - ofLimit<int>( static_cast<int>(Depth*255.f), MinBrightness, 255 );

	static bool UseRainbowScale = true;
	if ( UseRainbowScale )
	{
		if ( Depth < 1.f/3.f )
		{
			float d = ofGetMathTime( Depth, 0.f, 1.f/3.f );
			//	red to green
			Red = ofLerp( 0, 255, 1.f-d );
			Green = ofLerp( 0, 255, d );
			Blue = 0;
		}
		else if ( Depth < 2.f/3.f )
		{
			float d = ofGetMathTime( Depth, 1.f/3.f, 2.f/3.f );
			//	yellow to blue
			Green = ofLerp( 0, 255, 1.f-d );
			Blue = ofLerp( 0, 255, d );
			Red = 0;
		}
		else
		{
			float d = ofGetMathTime( Depth, 2.f/3.f, 3.f/3.f );
			//	blue to red
			Blue = ofLerp( 0, 255, 1.f-d );
			Red = ofLerp( 0, 255, d );
			Green = 0;
		}
		return;
	}

	static bool UsePlayerColourGreyScale = true;
	if ( UsePlayerColourGreyScale )
	{
		Red = 0;
		Green = 0;
		Blue = 0;
		
		//	make 0 (no user) white
		if ( PlayerIndex == 0 )
			PlayerIndex = 7;
		
		if ( PlayerIndex & 1 )
			Red = Greyscale;
		if ( PlayerIndex & 2 )
			Green = Greyscale;
		if ( PlayerIndex & 4 )
			Blue = Greyscale;
		return;
	}
}

bool ConvertFormat_KinectDepthToGreyscale(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	bool GreyscaleAlphaFormat = (NewFormat == SoyPixelsFormat::GreyscaleAlpha);
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint16 KinectDepth = *reinterpret_cast<uint16*>( &Pixels[p*2] );
		char& Greyscale = Pixels[ p * (GreyscaleAlphaFormat?2:1) ];

		int PlayerIndex = KinectDepth & ((1<<3)-1);
		float Depthf = static_cast<float>(KinectDepth >> 3) / static_cast<float>( 1<<13 );
		Greyscale = ofLimit<int>( static_cast<int>(Depthf*255.f), 0, 255 );

		if ( GreyscaleAlphaFormat )
		{
			char& GreyscaleAlpha = Pixels[ (p * 2) + 1 ];
			GreyscaleAlpha = PlayerIndex;
		}
	}

	//	half the pixels & change format
	if ( !GreyscaleAlphaFormat )
		Pixels.SetSize( PixelCount );
	Meta.DumbSetFormat( GreyscaleAlphaFormat ? SoyPixelsFormat::GreyscaleAlpha : SoyPixelsFormat::Greyscale );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight( Pixels.GetDataSize() ) == Height );
	return true;
}

bool ConvertFormat_KinectDepthToRgb(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	Array<uint16> DepthPixels;
	DepthPixels.SetSize( Pixels.GetSize() / 2 );
	memcpy( DepthPixels.GetArray(), Pixels.GetArray(), DepthPixels.GetDataSize() );
	
	int Components = SoyPixelsFormat::GetChannelCount( NewFormat );
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	
	//	realloc
	Pixels.SetSize( Components * PixelCount );
	
	static bool Debug = false;
	
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint16 KinectDepth = DepthPixels[p];
		if ( Debug )
			std::Debug << KinectDepth << " ";
		
		static int DepthBits = 13;
		uint16 PlayerIndex = KinectDepth >> DepthBits;
		uint16 MaxDepth = (1<<DepthBits)-1;
		uint16 MinDepth = 0;
		KinectDepth &= MaxDepth;
		
		bool DepthInvalid = (KinectDepth == 0);
		float Depthf = ofGetMathTime( KinectDepth, MinDepth, MaxDepth );
		
		char& Red = Pixels[ p * (Components) + 0 ];
		char& Green = Pixels[ p * (Components) + 1 ];
		char& Blue = Pixels[ p * (Components) + 2 ];
		SetDepthColour( Red, Green, Blue, Depthf, PlayerIndex, DepthInvalid );
	}
	
	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight( Pixels.GetDataSize() ) == Height );
	return true;
}



bool DepthToGreyOrRgb(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	bool RawDepth = (Meta.GetFormat() != SoyPixelsFormat::FreenectDepthmm);
	uint16 MinDepth = SoyPixelsFormat::GetMinValue( Meta.GetFormat() );
	uint16 MaxDepth = SoyPixelsFormat::GetMaxValue( Meta.GetFormat() );
	uint16 InvalidDepth = SoyPixelsFormat::GetInvalidValue( Meta.GetFormat() );
	int PlayerIndexFirstBit = SoyPixelsFormat::GetPlayerIndexFirstBit( Meta.GetFormat() );
 
	Array<uint16> DepthPixels;
	DepthPixels.SetSize( Pixels.GetSize() / 2 );
	memcpy( DepthPixels.GetArray(), Pixels.GetArray(), DepthPixels.GetDataSize() );
	
	int Components = SoyPixelsFormat::GetChannelCount( NewFormat );
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	
	//	realloc
	Pixels.SetSize( Components * PixelCount );
	
	static bool Debug = false;
	
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint16 KinectDepth = DepthPixels[p];
		int PlayerIndex = 0;

		if ( PlayerIndexFirstBit >= 0 )
		{
			PlayerIndex = KinectDepth >> PlayerIndexFirstBit;
			KinectDepth &= (1<<PlayerIndexFirstBit) -1;
		}

		if ( Debug )
			std::Debug << KinectDepth << " ";
		

		bool DepthInvalid = (KinectDepth == InvalidDepth);
		float Depthf = ofGetMathTime( KinectDepth, MinDepth, MaxDepth );
		
		char& Red = Pixels[ p * (Components) + 0 ];
		
		//	if RGB then we do different colours for raw and mm
		if ( Components >= 3 )
		{
			int CompZero = RawDepth ? 1 : 2;
			int CompDepth = RawDepth ? 2 : 1;
			char& Green = Pixels[ p * (Components) + CompZero ];
			char& Blue = Pixels[ p * (Components) + CompDepth ];

			SetDepthColour( Red, Green, Blue, Depthf, PlayerIndex, DepthInvalid );

			/*
			
			Green = 0;
			Blue = DepthInvalid ? 0 : 255-Red;
			
			if ( Components > 3 )
			{
				int c = 3;
				uint8& Alpha = Pixels[ p * (Components) + c ];
				Alpha = DepthInvalid ? 0 : 255;
			}
			 */
		}
		else
		{
			//	greyscale...
			static int GreyInvalid = 0;
			static int GreyMin = 1;
			uint8 Greyscale = ofLimit<int>( static_cast<int>(Depthf*255.f), GreyMin, 255 );

			//	set first component to greyscale
			Red = DepthInvalid ? GreyInvalid : Greyscale;
			
			//	other components just valid/not
			for ( int c=1;	c<Components;	c++ )
			{
				char& Blue = Pixels[ p * (Components) + c ];
				Blue = DepthInvalid ? 0 : 255;
			}
		}
		
	}

	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight( Pixels.GetDataSize() ) == Height );
	return true;
}


bool ConvertFormat_BgrToRgb(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	int SrcChannels = Meta.GetChannels();
	int DestChannels = SoyPixelsFormat::GetChannelCount( NewFormat );
	if ( SrcChannels != DestChannels )
		return false;

	for ( int p=0;	p<PixelCount;	p++ )
	{
		auto& b = Pixels[(p*SrcChannels)+0];
		//auto& g = Pixels[(p*SrcChannels)+1];
		auto& r = Pixels[(p*SrcChannels)+2];
	
		uint8 Swap = b;
		b = r;
		r = Swap;
	}

	Meta.DumbSetFormat( NewFormat );
	return true;
}


bool ConvertFormat_RGBAToGreyscale(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	int Channels = Meta.GetChannels();
	int GreyscaleChannels = SoyPixelsFormat::GetChannelCount(NewFormat);

	//	todo: store alpha in loop
	assert( GreyscaleChannels == 1 );

	for ( int p=0;	p<PixelCount;	p++ )
	{
		float Intensity = 0.f;
		int ReadChannels = 3;
		for ( int c=0;	c<ReadChannels;	c++ )
			Intensity += Pixels[ p*Channels + c ];
		Intensity /= ReadChannels;
		
		char& Greyscale = Pixels[ p * GreyscaleChannels ];
		Greyscale = ofLimit<int>( static_cast<int>(Intensity), 0, 255 );
	}

	//	half the pixels & change format
	Pixels.SetSize( PixelCount * GreyscaleChannels );
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight( Pixels.GetDataSize() ) == Height );
	return true;
}

int GetDepthFormatBits(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::KinectDepth:		return 13;
		default:
			return 16;
	}
}

bool ConvertDepth16(ArrayInterface<char>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto OldFormat = Meta.GetFormat();

	//	assume this is a valid depth format...
	if ( !Soy::Assert(SoyPixelsFormat::GetChannelCount(OldFormat) == 2, "expected 2-channel 16 bit depth format" ) )
		return false;
	if ( !Soy::Assert(SoyPixelsFormat::GetChannelCount(NewFormat) == 2, "expected 2-channel 16 bit depth format" ) )
		return false;
	
	int OldInvalid = SoyPixelsFormat::GetInvalidValue( OldFormat );
	int OldMin = SoyPixelsFormat::GetMinValue( OldFormat );
	int OldMax = SoyPixelsFormat::GetMaxValue( OldFormat );
	bool OldFrontToBack = SoyPixelsFormat::GetIsFrontToBackDepth( OldFormat );

	int NewInvalid = SoyPixelsFormat::GetInvalidValue( NewFormat );
	int NewMin = SoyPixelsFormat::GetMinValue( NewFormat );
	int NewMax = SoyPixelsFormat::GetMaxValue( NewFormat );
	bool NewFrontToBack = SoyPixelsFormat::GetIsFrontToBackDepth( NewFormat );
	
	int OldDepthBits = GetDepthFormatBits( OldFormat );
	//int NewDepthBits = GetDepthFormatBits( NewFormat );
	
	uint16* DepthPixels = reinterpret_cast<uint16*>( Pixels.GetArray() );
	int PixelCount = Meta.GetWidth() * Meta.GetHeight( Pixels.GetDataSize() );

	static bool Debug = false;
	
	for ( int p=0;	p<PixelCount;	p++ )
	{
		auto& DepthValue = DepthPixels[p];
		uint16 Depth16 = DepthValue & ((1<<OldDepthBits)-1);
		//uint16 Player16 = DepthValue >> OldDepthBits;
		bool InvalidDepth = (Depth16 == OldInvalid);
		float Depthf = ofGetMathTime( Depth16, OldMin, OldMax );
		Depthf = ofLimit<float>( Depthf, 0.f, 1.f );
		
		if ( OldFrontToBack != NewFrontToBack )
			Depthf = 1.f - Depthf;
		
		if ( InvalidDepth )
		{
			//	todo: write player index if both formats have it
			DepthValue = NewInvalid;
		}
		else
		{
			//	todo: write player index if both formats have it
			DepthValue = static_cast<uint16>( ofLerp( NewMin, NewMax, Depthf ) );
		}
		
		if ( Debug )
			std::Debug << Depth16 << "/" << DepthValue << "   ";
	}
	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
	return true;
}

class TConvertFunc
{
public:
	TConvertFunc()	{}

	template<typename FUNCTYPE>
	TConvertFunc(SoyPixelsFormat::Type SrcFormat,SoyPixelsFormat::Type DestFormat,FUNCTYPE Func) :
		mSrcFormat	( SrcFormat ),
		mDestFormat	( DestFormat ),
		mFunction	( Func )
	{
	}

	inline bool		operator==(const std::tuple<SoyPixelsFormat::Type,SoyPixelsFormat::Type>& SrcToDestFormat) const
	{
		return (std::get<0>( SrcToDestFormat )==mSrcFormat) && (std::get<1>( SrcToDestFormat )==mDestFormat);
	}

	SoyPixelsFormat::Type	mSrcFormat;
	SoyPixelsFormat::Type	mDestFormat;
	std::function<bool(ArrayInterface<char>&,SoyPixelsMeta&,SoyPixelsFormat::Type)>	mFunction;
};


TConvertFunc gConversionFuncs[] =
{
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::FreenectDepth10bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::FreenectDepth11bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::FreenectDepthmm, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::FreenectDepthmm, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::FreenectDepth11bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::KinectDepth, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::FreenectDepthmm, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::FreenectDepth10bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::KinectDepth, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::FreenectDepth10bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::FreenectDepth11bit, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::KinectDepth, ConvertDepth16 ),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::RGB, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::RGBA, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Greyscale, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::RGB, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::RGBA, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::Greyscale, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::RGB, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::RGBA, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::Greyscale, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::RGB, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::RGBA, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::FreenectDepthmm, SoyPixelsFormat::Greyscale, DepthToGreyOrRgb ),
	TConvertFunc( SoyPixelsFormat::BGR, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::BGRA, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::RGBA, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::BGRA, SoyPixelsFormat::RGBA, ConvertFormat_BgrToRgb ),
	TConvertFunc( SoyPixelsFormat::BGR, SoyPixelsFormat::RGB, ConvertFormat_BgrToRgb ),
	TConvertFunc( SoyPixelsFormat::RGBA, SoyPixelsFormat::BGRA, ConvertFormat_BgrToRgb ),
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::BGR, ConvertFormat_BgrToRgb ),
};
int gConversionFuncsCount = sizeofarray(gConversionFuncs);


bool SoyPixelsImpl::SetFormat(SoyPixelsFormat::Type Format)
{
	auto OldFormat = GetFormat();
	if ( !SoyPixelsFormat::IsValid( Format ) )
		return false;

	if ( OldFormat == Format )
		return true;
	if ( !IsValid() )
		return false;

	auto& PixelsArray = GetPixelsArray();
	auto ConversionFuncs = GetRemoteArray( gConversionFuncs );
	auto* ConversionFunc = GetArrayBridge(ConversionFuncs).Find( std::make_tuple( OldFormat, Format ) );
	if ( ConversionFunc )
	{
		if ( ConversionFunc->mFunction( PixelsArray, GetMeta(), Format ) )
			return true;
	}
	
	//	report missing, but desired conversions
	std::Debug << "No soypixel conversion from " << OldFormat << " to " << Format << std::endl;

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::Greyscale )
		return ConvertFormat_KinectDepthToGreyscale( PixelsArray, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::GreyscaleAlpha )
		return ConvertFormat_KinectDepthToGreyscale( PixelsArray, GetMeta(), Format );


	//	see if we can use of simple channel-count change
	bool UseOfPixels = false;
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::RGBA );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::RGB );
	//UseOfPixels |= ( GetFormat() == SoyPixelsFormat::BGRA && Format == SoyPixelsFormat::BGR );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::Greyscale );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::Greyscale );

	if ( !UseOfPixels )
		return false;

#if defined(OPENFRAMEWORKS)
	int Channels = GetChannels();
	ofScopeTimerWarning Timer("TPixels::SetChannels",1);
	ofPixels PixelsX;
	Get( PixelsX );
	PixelsX.setNumChannels( Channels );
	Set( PixelsX );
	return true;
#else
	return false;
#endif
}

#if defined(SOY_OPENCL)
bool TPixels::SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const ofColour& FillerColour)
{
	clSetPixelParams Params = _clSetPixelParams();
	Params.mDefaultColour = ofToCl_Rgba_int4( FillerColour );
	return SetChannels( Channels, OpenClManager, Params );
}
#endif

#if defined(SOY_OPENCL)
bool TPixels::SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const clSetPixelParams& Params)
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

	
#if defined(SOY_OPENCL)
bool TPixels::RgbaToHsla(SoyOpenClManager& OpenClManager,ofPtr<SoyClDataBuffer>& Hsla) const
{
	if ( !IsValid() )
		return false;

	ClShaderRgbaToHsla Shader( OpenClManager );
	if ( !Shader.Run( *this, Hsla ) )
		return false;

	return true;
}
#endif

#if defined(SOY_OPENCL)
bool TPixels::RgbaToHsla(SoyOpenClManager& OpenClManager,ofPtr<msa::OpenCLImage>& Hsla) const
{
	if ( !IsValid() )
		return false;

	ClShaderRgbaToHsla Shader( OpenClManager );
	if ( !Shader.Run( *this, Hsla ) )
		return false;

	return true;
}
#endif

#if defined(SOY_OPENCL)
bool TPixels::RgbaToHsla(SoyOpenClManager& OpenClManager)
{
	if ( !IsValid() )
		return false;

	ClShaderRgbaToHsla Shader( OpenClManager );
	if ( !Shader.Run( *this ) )
		return false;
	return true;
}
#endif

#if defined(SOY_OPENCL)
bool TPixels::Set(const msa::OpenCLImage& PixelsConst,SoyOpenClKernel& Kernel)
{
	if ( !Kernel.IsValid() )
		return false;

	return Set( PixelsConst, Kernel.GetQueue() );
}
#endif


#if defined(SOY_OPENCL)
bool TPixels::Set(const msa::OpenCLImage& PixelsConst,cl_command_queue Queue)
{
	if ( !Queue )
		return false;

	uint8 Channels = PixelsConst.getDepth();
	auto& Pixels = const_cast<msa::OpenCLImage&>( PixelsConst );
	//	clformat is either RGBA(4 chan) or A/LUM(1 chan)
	if ( Channels != 1 && Channels != 4 )
	{
		assert( false );
		Clear();
		return false;
	}
	
	mMeta.DumbSetWidth( Pixels.getWidth() );
	mMeta.DumbSetFormat( SoyPixelsFormat::GetFormatFromChannelCount( Channels ) );
	int Alloc = GetWidth() * GetChannels() * Pixels.getHeight();
	if ( !mPixels.SetSize( Alloc, false, true ) )
		return false;

	bool Blocking = true;
	if ( !Pixels.read( Queue, mPixels.GetArray(), Blocking ) )
	{
		Clear();
		return false;
	}

	//	check for corruption as there is no size checking in the read :/
	mPixels.GetHeap().Debug_Validate( mPixels.GetArray() );
	return true;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Set(const ofPixels& Pixels)
{
	int w = Pixels.getWidth();
	int h = Pixels.getHeight();
	int c = Pixels.getNumChannels();
	auto* p = Pixels.getPixels();

	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( c );
	if ( !Init( w, h, Format ) )
		return false;

	size_t Alloc = w * c * h;
	auto SrcPixelsData = GetRemoteArray( p, Alloc );
	mPixels.Copy( SrcPixelsData );
	return true;
}
#endif



bool SoyPixelsFormat::GetOpenglFormat(int& glFormat,SoyPixelsFormat::Type Format)
{
#if defined(SOY_OPENGL)

	glFormat = GL_INVALID_VALUE;
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( Format )
	{
#if defined(TARGET_OSX)
		case SoyPixelsFormat::RGB:			glFormat = GL_RGB;		return true;
		case SoyPixelsFormat::RGBA:			glFormat = GL_RGBA;	return true;
#else
		case SoyPixelsFormat::RGB:			glFormat = GL_RGB;			return true;
		case SoyPixelsFormat::RGBA:			glFormat = GL_RGBA;			return true;
#endif
		case SoyPixelsFormat::Greyscale:	glFormat = GL_LUMINANCE;	return true;
		case SoyPixelsFormat::BGRA:			glFormat = GL_BGRA;			return true;
			
		default:
			glFormat = GL_INVALID_VALUE;
			return false;
	}
#endif

	glFormat = -1;
	return false;
}

SoyPixelsFormat::Type SoyPixelsFormat::GetFormatFromOpenglFormat(int glFormat)
{
#if defined(SOY_OPENGL)
	switch ( glFormat )
	{
		//	gr: osx returns these values hmmmm
		case GL_RGBA8:	return SoyPixelsFormat::RGBA;
		case GL_RGB8:	return SoyPixelsFormat::RGB;
			
		case GL_LUMINANCE:	return SoyPixelsFormat::Greyscale;
		case GL_RGB:	return SoyPixelsFormat::RGB;
		case GL_RGBA:	return SoyPixelsFormat::RGBA;
		case GL_BGRA:	return SoyPixelsFormat::BGRA;
	}
#endif
	return SoyPixelsFormat::Invalid;
}


#if defined(SOY_OPENCL)
bool SoyPixelsFormat::GetOpenclFormat(int& clFormat,SoyPixelsFormat::Type Format)
{
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( Format )
	{
		//	clSURF code uses CL_R...
	case SoyPixelsFormat::Greyscale:	clFormat = CL_R;	return true;
//	case SoyPixelsFormat::Greyscale:	clFormat = CL_LUMINANCE;	return true;
	case SoyPixelsFormat::RGB:			clFormat = CL_RGB;			return true;
	case SoyPixelsFormat::RGBA:			clFormat = CL_RGBA;			return true;
	case SoyPixelsFormat::BGRA:			clFormat = treatbgrasrgb ? CL_RGBA : CL_BGRA;			return true;
	}
	return false;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Init(uint16 Width,uint16 Height,uint8 Channels,const ofColour& DefaultColour)
{
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	return Init( Width, Height, Format, DefaultColour );
}
#endif


#if defined(OPENFRAMEWORKS)
bool TPixels::Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format,const ofColour& DefaultColour)
{
	if ( !Init( Width, Height, Format ) )
		return false;

	int Channels = GetChannels();
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
#endif



bool SoyPixelsImpl::Init(uint16 Width,uint16 Height,uint8 Channels)
{
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	return Init( Width, Height, Format );
}

bool SoyPixelsImpl::Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format)
{
	//	alloc
	GetMeta().DumbSetWidth( Width );
	GetMeta().DumbSetFormat( Format );
	int Alloc = GetWidth() * GetChannels() * Height;
	auto& Pixels = GetPixelsArray();
	Pixels.SetSize( Alloc, false );
	assert( this->GetHeight() == Height );
	return true;	
}

bool SoyPixelsImpl::GetRawSoyPixels(ArrayBridge<char>& RawData) const
{
	if ( !IsValid() )
		return false;

	//	write header/meta
	auto& Pixels = GetPixelsArray();
	auto& Meta = GetMeta();
	
	//	alloc all data in one go (need this for memfiles!)
	//	gr: previously we APPENDED data. now clear. Has this broken anything?
	RawData.Clear(false);
	RawData.Reserve( sizeof(SoyPixelsMeta) + Pixels.GetDataSize() );
	
	if ( !RawData.PushBackReinterpret( Meta ) )
		return false;
	if ( !RawData.PushBackArray( Pixels ) )
		return false;
	
	return true;
}


bool SoyPixelsImpl::SetPng(const ArrayBridge<char>& PngData,std::stringstream& Error)
{
	//	http://stackoverflow.com/questions/7942635/write-png-quickly
	TArrayReader Png( PngData );

	if ( !TPng::CheckMagic( Png ) )
	{
		Error << "PNG has invalid magic header";
		return false;
	}
	
	//	use stb
#if defined(USE_STB)
	const stbi_uc* Buffer = reinterpret_cast<const stbi_uc*>( PngData.GetArray() );
	int BufferSize = PngData.GetDataSize();
	int Width = 0;
	int Height = 0;
	int Components = 0;
	int RequestComponents = 0;
	auto* DecodedPixels = stbi_load_from_memory( Buffer, BufferSize, &Width,&Height, &Components, RequestComponents );
	if ( !DecodedPixels )
	{
		Error << "Failed to decode png pixels";
		return false;
	}
	
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount(Components);
	if ( !this->Init( Width, Height, Format ) )
	{
		Error << "Failed to init pixels from png (" << Width << "x" << Height << " " << Format << ")";
		return false;
	}
	auto* ThisPixels = this->GetPixelsArray().GetArray();
	memcpy( ThisPixels, DecodedPixels, this->GetPixelsArray().GetDataSize() );
	return true;
#else
	
	TPng::THeader Header;
	assert( !Header.IsValid() );
	//	all the extracted IDAT chunks together
	Array<char> ImageData;
	bool FoundEnd = false;
	
	//	read until out of data
	//	gr: had a png with junk after IEND... so abort after that
	while ( !Png.Eod() && !FoundEnd )
	{
		uint32 BlockLength;
		if ( !Png.ReadReinterpretReverse<BufferArray<char,20>>( BlockLength ) )
		{
			Error << "Failed to read block length";
			return false;
		}
		BufferArray<char,4> BlockType(4);
		auto BlockTypeBridge = GetArrayBridge( BlockType );
		if ( !Png.Read( BlockTypeBridge ) )
		{
			Error << "Failed to read block type";
			return false;
		}
		std::stringstream BlockTypeStream;
		BlockTypeStream << BlockType[0] << BlockType[1] << BlockType[2] << BlockType[3];
		std::string BlockTypeString = BlockTypeStream.str();
	
		Array<char> BlockData( BlockLength );
		auto BlockDataBridge = GetArrayBridge( BlockData );
		if ( BlockLength > 0 && !Png.Read( BlockDataBridge ) )
		{
			Error << "Failed to read block " << BlockTypeString << " data (" << BlockLength << " bytes)";
			return false;
		}
		uint32 BlockCrc;
		if ( !Png.ReadReinterpretReverse<BufferArray<char,20>>( BlockCrc ) )
		{
			Error << "Failed to read block " << BlockTypeString << " CRC";
			return false;
		}
		//	gr: crc doesn't match, needs to include more than just the data I think
		uint32 BlockDataCrc = BlockDataBridge.GetCrc32();
		if ( BlockDataCrc != BlockCrc )
		{
			std::Debug << "Block " << BlockTypeString << " CRC (" << BlockDataCrc << ") doesn't match header CRC (" << BlockCrc << ")" << std::endl;
		//	Error << "Block " << BlockTypeString << " CRC (" << BlockDataCrc << ") doesn't match header CRC (" << BlockCrc << ")";
		//	return false;
		}
		
		if ( BlockTypeString == "IHDR" )
		{
			if ( BlockDataBridge.IsEmpty() )
			{
				Error << "Header data is empty, aborting";
				return false;
			}

			if ( Header.IsValid() )
			{
				Error << "Found 2nd header, aborting";
				return false;
			}
			
			if ( !TPng::ReadHeader( *this, Header, BlockDataBridge, Error ) )
				return false;
			assert( Header.IsValid() );
		}
		else if ( BlockTypeString == "IDAT" )
		{
			if ( BlockDataBridge.IsEmpty() )
			{
				Error << "Block data is empty, aborting";
				return false;
			}
			//	these are supposed to be consecutive but I guess it doesn't matter
			ImageData.PushBackArray( BlockDataBridge );
		}
		else if ( BlockTypeString == "IEND" )
		{
			if ( !TPng::ReadTail( *this, BlockDataBridge, Error ) )
				return false;
			FoundEnd = true;
		}
		else
		{
			std::Debug << "Ignored unhandled PNG block: " << BlockTypeString << ", " << BlockLength << " bytes" << std::endl;
		}
	}
	
	if ( !Header.IsValid() )
	{
		Error << "PNG missing header";
		return false;
	}
	
	if ( ImageData.IsEmpty() )
	{
		Error << "PNG missing image data";
		return false;
	}
	
	//	read the image data
	auto ImageDataBridge = GetArrayBridge( ImageData );
	if ( !TPng::ReadData( *this, Header, ImageDataBridge, Error ) )
		return false;
	
	return true;
#endif
}

bool SoyPixelsImpl::SetRawSoyPixels(const ArrayBridge<char>& RawData)
{
	int HeaderSize = sizeof(SoyPixelsMeta);
	if ( RawData.GetDataSize() < HeaderSize )
		return false;
	auto& Header = *reinterpret_cast<const SoyPixelsMeta*>( RawData.GetArray() );
	if ( !Header.IsValid() )
		return false;

	//	rest of the data as an array
	const size_t PixelDataSize = RawData.GetDataSize()-HeaderSize;
	auto Pixels = GetRemoteArray( RawData.GetArray()+HeaderSize, PixelDataSize );
	
	//	todo: verify size! (alignment!)
	GetMeta() = Header;
	GetPixelsArray().Clear(false);
	GetPixelsArray().PushBackArray( Pixels );

	if ( !IsValid() )
		return false;

	return true;
}



bool SoyPixelsImpl::GetPng(ArrayBridge<char>& PngData) const
{
	//	if non-supported PNG colour format, then convert to one that is
	auto PngColourType = TPng::GetColourType( GetFormat() );
	if ( PngColourType == TPng::TColour::Invalid )
	{
		//	do conversion here
		SoyPixels OtherFormat;
		OtherFormat.Copy( *this );
		auto NewFormat = SoyPixelsFormat::RGB;
		
		//	gr: this conversion should be done in soypixels, with a list of compatible output formats
		switch ( GetFormat() )
		{
			//case SoyPixelsFormat::KinectDepth:	NewFormat = SoyPixelsFormat::Greyscale;	break;
			case SoyPixelsFormat::BGRA:			NewFormat = SoyPixelsFormat::RGBA;	break;
			default:break;
		}

		//	new format MUST be compatible or we'll get stuck in a loop
		if ( TPng::GetColourType( NewFormat ) == TPng::TColour::Invalid )
		{
			assert( false );
			return false;
		}
		
		//	attempt conversion
		if ( !OtherFormat.SetFormat( NewFormat ) )
			return false;

		return OtherFormat.GetPng( PngData );
	}

	//	http://stackoverflow.com/questions/7942635/write-png-quickly

	//uint8 Magic[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	char Magic[] = { -119, 80, 78, 71, 13, 10, 26, 10 };
	char IHDR[] = { 73, 72, 68, 82 };	//'I', 'H', 'D', 'R'
	const char IEND[] = { 73, 69, 78, 68 }; //("IEND")
	const char IDAT[] = { 73, 68, 65, 84 };// ("IDAT") 

	//	write header chunk
	uint32 Width = GetWidth();
	uint32 Height = GetHeight();
	uint8 BitDepth = GetBitDepth();
	uint8 ColourType = PngColourType;
	assert( PngColourType != TPng::TColour::Invalid );
	uint8 Compression = TPng::TCompression::DEFLATE;
	uint8 Filter = TPng::TFilter::None;
	uint8 Interlace = TPng::TInterlace::None;
	Array<char> Header;
	Header.PushBackArray( IHDR );
	Header.PushBackReinterpretReverse( Width );
	Header.PushBackReinterpretReverse( Height );
	Header.PushBack( BitDepth );
	Header.PushBack( ColourType );
	Header.PushBack( Compression );
	Header.PushBack( Filter );
	Header.PushBack( Interlace );

	//	write data chunks
	Array<char> PixelData;
	PixelData.PushBackArray( IDAT );
	if ( !TPng::GetPngData( PixelData, *this, static_cast<TPng::TCompression::Type>(Compression) ) )
		return false;

	//	write Tail chunks
	Array<char> Tail;
	Tail.PushBackArray( IEND );

	//	put it all together!
	PngData.Reserve( Header.GetDataSize() + 12 );
	PngData.Reserve( PixelData.GetDataSize() + 12 );
	PngData.Reserve( Tail.GetDataSize() + 12 );

	uint32 HeaderLength = Header.GetDataSize() - sizeofarray(IHDR);
	assert( HeaderLength == 13 );
	uint32 HeaderCrc = GetArrayBridge(Header).GetCrc32();
	PngData.PushBackArray( Magic );
	PngData.PushBackReinterpretReverse( HeaderLength );
	PngData.PushBackArray( Header );
	PngData.PushBackReinterpretReverse( HeaderCrc );

	uint32 PixelDataLength = PixelData.GetDataSize() - sizeofarray(IDAT);
	uint32 PixelDataCrc = GetArrayBridge(PixelData).GetCrc32();
	PngData.PushBackReinterpretReverse( PixelDataLength );
	PngData.PushBackArray( PixelData );
	PngData.PushBackReinterpretReverse( PixelDataCrc );

	uint32 TailLength = Tail.GetDataSize() - sizeofarray(IEND);
	uint32 TailCrc = GetArrayBridge(Tail).GetCrc32();
	assert( TailCrc == 0xAE426082 );
	PngData.PushBackReinterpretReverse( TailLength );
	PngData.PushBackArray( Tail );
	PngData.PushBackReinterpretReverse( TailCrc );
	
	return true;
}


const uint8& SoyPixelsImpl::GetPixel(uint16 x,uint16 y,uint16 Channel) const
{
	int w = GetWidth();
	int h = GetHeight();
	int Channels = GetChannels();
	if ( x < 0 || x >= w || y<0 || y>=h || Channel<0 || Channel>=Channels )
	{
		assert(false);
		static uint8 Fake = 0;
		return Fake;
	}
	int Index = x + (y*w);
	Index *= Channels;
	Index += Channel;
	return GetPixelsArray()[Index];
}

bool SoyPixelsImpl::SetPixel(uint16 x,uint16 y,uint16 Channel,const uint8& Component)
{
	int w = GetWidth();
	int h = GetHeight();
	int Channels = GetChannels();
	if ( x < 0 || x >= w || y<0 || y>=h || Channel<0 || Channel>=Channels )
	{
		assert(false);
		return false;
	}
	int Index = x + (y*w);
	Index *= Channels;
	Index += Channel;
	GetPixelsArray()[Index] = Component;
	return true;
}



void SoyPixelsImpl::ResizeClip(uint16 Width,uint16 Height)
{
	auto& Pixels = GetPixelsArray();
	
	//	simply add/remove rows
	if ( Height > GetHeight() )
	{
		int RowBytes = GetChannels() * GetWidth();
		RowBytes *= Height - GetHeight();
		Pixels.PushBlock( RowBytes );
	}
	else if ( Height < GetHeight() )
	{
		int RowBytes = GetChannels() * GetWidth();
		RowBytes *= GetHeight() - Height;
		Pixels.SetSize( Pixels.GetDataSize() - RowBytes );
	}
	
	//	insert/remove data on the end of each row
	if ( Width > GetWidth() )
	{
		//	working backwards makes it easy & fast
		int Stride = GetChannels() * GetWidth();
		int ColBytes = GetChannels() * (Width - GetWidth());
		for ( int p=Pixels.GetDataSize();	p>=0;	p-=Stride )
			Pixels.InsertBlock( p, ColBytes );
		GetMeta().DumbSetWidth( Width );
	}
	else if ( Width < GetWidth() )
	{
		//	working backwards makes it easy & fast
		int Stride = GetChannels() * GetWidth();
		int ColBytes = GetChannels() * (GetWidth() - Width);
		for ( int p=Pixels.GetDataSize()-ColBytes;	p>=0;	p-=Stride )
			Pixels.RemoveBlock( p, ColBytes );
		GetMeta().DumbSetWidth( Width );
	}
	
	assert( Height == GetHeight() );
	assert( Width = GetWidth() );
}


void SoyPixelsImpl::ResizeFastSample(uint16 NewWidth, uint16 NewHeight)
{
	//	copy old data
	SoyPixels Old;
	Old.Copy(*this);
	
	auto& New = *this;
	auto NewChannelCount = GetChannels();
	//	resize buffer
	ResizeClip( NewWidth, NewHeight );
	
	auto OldHeight = Old.GetHeight();
	auto OldWidth = Old.GetWidth();
	auto OldChannelCount = Old.GetChannels();
	auto MinChannelCount = std::min( OldChannelCount, NewChannelCount );
	
	auto& OldPixels = Old.GetPixelsArray();
	auto& NewPixels = New.GetPixelsArray();
	
	for ( int ny=0;	ny<NewHeight;	ny++ )
	{
		float yf = ny/(float)NewHeight;
		int oy = OldHeight * yf;

		auto OldLineSize = OldWidth * OldChannelCount;
		auto NewLineSize = NewWidth * NewChannelCount;
		auto OldRow = GetRemoteArray( &OldPixels.GetArray()[oy*OldLineSize], OldLineSize );
		auto NewRow = GetRemoteArray( &NewPixels.GetArray()[ny*NewLineSize], NewLineSize );
		
		for ( int nx=0;	nx<NewWidth;	nx++ )
		{
			float xf = nx/(float)NewWidth;
			int ox = OldWidth * xf;
			auto* OldPixel = &OldRow[ox*OldChannelCount];
			auto* NewPixel = &NewRow[nx*NewChannelCount];

			memcpy( NewPixel, OldPixel, MinChannelCount );
		}
	}
	
	assert( NewHeight == GetHeight() );
	assert( NewWidth == GetWidth() );
}


void SoyPixelsImpl::RotateFlip()
{
	if ( !IsValid() )
		return;
	
	//	buffer a line so we don't need to realloc for the temp line
	auto LineSize = GetWidth() * GetChannels();
	Array<char> TempLine( LineSize );
	auto* Pixels = const_cast<char*>( GetPixelsArray().GetArray() );
	
	auto Height = GetHeight();
	for ( int y=0;	y<Height/2;	y++ )
	{
		//	swap lines
		int TopY = y;
		int BottomY = (Height-1) - y;

		auto* TopRow = &Pixels[TopY * LineSize];
		auto* BottomRow = &Pixels[BottomY  * LineSize];
		memcpy( TempLine.GetArray(), TopRow, LineSize );
		memcpy( TopRow, BottomRow, LineSize );
		memcpy( BottomRow, TempLine.GetArray(), LineSize );
	}
}

bool SoyPixelsImpl::Copy(const SoyPixelsImpl &that)
{
	if ( &that == this )
		return true;
	
	this->GetMeta() = that.GetMeta();
	this->GetPixelsArray().Copy( that.GetPixelsArray() );
	return true;
}
