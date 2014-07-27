#include "SoyPixels.h"
#include "SoyDebug.h"
#include "SoyTime.h"
#include <functional>
#include "SoyPng.h"





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
	case KinectDepth:	return 2;
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

void TPixels::Clear()
{
	GetMeta() = SoyPixelsMeta();
	mPixels.Clear(true);
}

bool TPixels::SetChannels(uint8 Channels)
{
	SoyPixelsFormat::Type Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	return SetFormat( Format );
}

bool ConvertFormat_KinectDepthToGreyscale(ArrayBridge<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	bool GreyscaleAlphaFormat = (NewFormat == SoyPixelsFormat::GreyscaleAlpha);
	int Height = Meta.GetHeight( Pixels.GetDataSize() );
	int PixelCount = Meta.GetWidth() * Height;
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint16 KinectDepth = *reinterpret_cast<uint16*>( &Pixels[p*2] );
		uint8& Greyscale = Pixels[ p * (GreyscaleAlphaFormat?2:1) ];

		int PlayerIndex = KinectDepth & ((1<<3)-1);
		float Depthf = static_cast<float>(KinectDepth >> 3) / static_cast<float>( 1<<13 );
		Greyscale = ofLimit<int>( static_cast<int>(Depthf*255.f), 0, 255 );

		if ( GreyscaleAlphaFormat )
		{
			uint8& GreyscaleAlpha = Pixels[ (p * 2) + 1 ];
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


bool ConvertFormat_BgrToRgb(ArrayBridge<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
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


bool ConvertFormat_RGBAToGreyscale(ArrayBridge<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
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
		
		uint8& Greyscale = Pixels[ p * GreyscaleChannels ];
		Greyscale = ofLimit<int>( static_cast<int>(Intensity), 0, 255 );
	}

	//	half the pixels & change format
	Pixels.SetSize( PixelCount * GreyscaleChannels );
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight( Pixels.GetDataSize() ) == Height );
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
	std::function<bool(ArrayBridge<uint8>&,SoyPixelsMeta&,SoyPixelsFormat::Type)>	mFunction;
};

TConvertFunc gConversionFuncs[] = 
{
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Greyscale, ConvertFormat_KinectDepthToGreyscale ),
	TConvertFunc( SoyPixelsFormat::BGR, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::BGRA, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::RGBA, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::Greyscale, ConvertFormat_RGBAToGreyscale ),
};
int gConversionFuncsCount = sizeofarray(gConversionFuncs);


bool TPixels::SetFormat(SoyPixelsFormat::Type Format)
{
	if ( !SoyPixelsFormat::IsValid( Format ) )
		return false;

	if ( GetFormat() == Format )
		return true;
	if ( !IsValid() )
		return false;

	auto PixelsBridge = GetArrayBridge( mPixels );
	auto ConversionFuncs = GetRemoteArray( gConversionFuncs, gConversionFuncsCount );
	auto* ConversionFunc = ConversionFuncs.Find( std::make_tuple( GetFormat(), Format ) );
	if ( ConversionFunc )
	{
		if ( ConversionFunc->mFunction( PixelsBridge, GetMeta(), Format ) )
			return true;
	}

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::Greyscale )
		return ConvertFormat_KinectDepthToGreyscale( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::GreyscaleAlpha )
		return ConvertFormat_KinectDepthToGreyscale( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::BGRA && Format == SoyPixelsFormat::RGBA )
		return ConvertFormat_BgrToRgb( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::BGR && Format == SoyPixelsFormat::RGB )
		return ConvertFormat_BgrToRgb( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::BGRA )
		return ConvertFormat_BgrToRgb( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::BGR )
		return ConvertFormat_BgrToRgb( PixelsBridge, GetMeta(), Format );

	if ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::BGR )
		return ConvertFormat_BgrToRgb( PixelsBridge, GetMeta(), Format );

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

	int Alloc = w * c * h;
	auto SrcPixelsData = GetRemoteArray( p, Alloc, Alloc );
	mPixels.Copy( SrcPixelsData );
	return true;
}
#endif


bool TPixels::Set(const TPixels& Pixels)
{
	GetMeta() = Pixels.GetMeta();
	mPixels = Pixels.mPixels;
	return true;
}

bool TPixels::Set(const SoyPixelsImpl& Pixels)
{
	GetMeta() = Pixels.GetMeta();
	mPixels = Pixels.GetPixelsArray();
	return true;
}

#if defined(OPENFRAMEWORKS)
bool TPixels::Set(ofxCvImage& Pixels)
{
	ofPixels& RealPixels = Pixels.getPixelsRef();
	Set( RealPixels );
	return true;
}
#endif

#if defined(OPENFRAMEWORKS)
bool TPixels::Set(const IplImage& Pixels)
{
	int w = Pixels.width;
	int h = Pixels.height;
	int c = Pixels.nChannels;
	auto* p = Pixels.imageData;

	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( c );
	if ( !Init( w, h, Format ) )
		return false;

	int Alloc = w * c * h;
	auto SrcPixelsData = GetRemoteArray( p, Alloc, Alloc );
	mPixels.Copy( SrcPixelsData );
	return true;
}
#endif

#if defined(SOY_OPENGL)
static bool treatbgrasrgb = true;
#endif

#if defined(SOY_OPENGL)
bool SoyPixelsMeta::GetOpenglFormat(int& glFormat) const
{
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( GetFormat() )
	{
	case SoyPixelsFormat::Greyscale:	glFormat = GL_LUMINANCE;	return true;
	case SoyPixelsFormat::RGB:			glFormat = GL_RGB;			return true;
	case SoyPixelsFormat::RGBA:			glFormat = GL_RGBA;			return true;
	case SoyPixelsFormat::BGRA:			glFormat = treatbgrasrgb ? GL_RGBA: GL_BGRA;			return true;
	}
	return false;
}
#endif


#if defined(SOY_OPENCL)
bool SoyPixelsMeta::GetOpenclFormat(int& clFormat) const
{
	//	from ofGetGlInternalFormat(const ofPixels& pix)
	switch ( GetFormat() )
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
	RawData.PushBackReinterpret( Meta );
	RawData.PushBackArray( Pixels );
	return true;
}


bool SoyPixelsImpl::SetPng(const ArrayBridge<char>& PngData,std::stringstream& Error)
{
	//	http://stackoverflow.com/questions/7942635/write-png-quickly
	
	//uint8 Magic[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	char _Magic[] = { -119, 'P', 'N', 'G', 13, 10, 26, 10 };
	BufferArray<char,8> __Magic( _Magic );
	auto Magic = GetArrayBridge( __Magic );
	

	TArrayReader Png( PngData );

	if ( !Png.ReadCompare( Magic ) )
	{
		Error << "PNG has invalid magic header";
		return false;
	}
	
	
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
		std::string BlockTypeString = (std::stringstream() << BlockType[0] << BlockType[1] << BlockType[2] << BlockType[3]).str();
		
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
	const int PixelDataSize = RawData.GetDataSize()-HeaderSize;
	auto Pixels = GetRemoteArray( RawData.GetArray()+HeaderSize, PixelDataSize, PixelDataSize );
	
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
		TPixels OtherFormat;
		OtherFormat.Set( *this );
		auto NewFormat = SoyPixelsFormat::RGB;
		switch ( GetFormat() )
		{
			case SoyPixelsFormat::KinectDepth:	NewFormat = SoyPixelsFormat::Greyscale;	break;
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

		return OtherFormat.Def().GetPng( PngData );
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

