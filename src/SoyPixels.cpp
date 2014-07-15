#include "SoyPixels.h"
#include "miniz/miniz.h"
#include "SoyDebug.h"
#include "SoyTime.h"
#include <functional>


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


namespace TPng
{
	namespace TColour {	enum Type
	{
		Invalid			= -1,	//	unsupported (do not write to PNG!)
		Greyscale		= 0,
		RGB				= 2,
		Palette			= 3,
		GreyscaleAlpha	= 4,
		RGBA			= 6,
	};};

	namespace TCompression { enum Type
	{
		DEFLATE = 0,
	};};

	namespace TFilter { enum Type
	{
		None = 0,
	};};
	
	namespace TInterlace { enum Type
	{
		None = 0,
	};};

	TColour::Type	GetColourType(SoyPixelsFormat::Type Format)
	{
		switch ( Format )
		{
		case SoyPixelsFormat::Greyscale:		return TColour::Greyscale;
		case SoyPixelsFormat::GreyscaleAlpha:	return TColour::GreyscaleAlpha;
		case SoyPixelsFormat::RGB:				return TColour::RGB;
		case SoyPixelsFormat::RGBA:				return TColour::RGBA;
		default:
			return TColour::Invalid;
		}
	}

	bool		GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression);
	bool		GetDeflateData(Array<char>& ChunkData,const ArrayBridge<uint8>& PixelBlock,bool LastBlock,int WindowSize); 
};


bool TPng::GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression)
{
	if ( Compression == TCompression::DEFLATE )
	{
		//	we need to add a filter value at the start of each row, so calculate all the byte indexes
		auto& OrigPixels = Image.GetPixelsArray();
		Array<uint8> FilteredPixels;
		FilteredPixels.Reserve( OrigPixels.GetDataSize() + Image.GetHeight() );
		FilteredPixels.PushBackArray( OrigPixels );
		int Stride = Image.GetChannels()*Image.GetWidth();
		for ( int i=OrigPixels.GetSize()-Stride;	i>=0;	i-=Stride )
		{
			//	insert filter value/code
			static char FilterValue = 0;
			auto* RowStart = FilteredPixels.InsertBlock(i,1);
			RowStart[0] = FilterValue;
		}

		//	use miniz to compress with deflate
		auto CompressionLevel = MZ_NO_COMPRESSION;
		BufferString<100> Debug_TimerName;
		Debug_TimerName << "Deflate compression; " << Soy::FormatSizeBytes(FilteredPixels.GetDataSize()) << ". Compression level: " << CompressionLevel;
		ofScopeTimerWarning DeflateCompressTimer( Debug_TimerName, 3 );
	
		uLong DefAllocated = static_cast<int>( 1.2f * FilteredPixels.GetDataSize() );
		uLong DefUsed = DefAllocated;
		auto* DefData = PngData.PushBlock(DefAllocated);
		auto Result = mz_compress2( reinterpret_cast<Byte*>(DefData), &DefUsed, FilteredPixels.GetArray(), FilteredPixels.GetDataSize(), CompressionLevel );
		assert( Result == MZ_OK );
		if ( Result != MZ_OK )
			return false;
		assert( DefUsed <= DefAllocated );
		//	trim data
		int Overflow = DefAllocated - DefUsed;
		PngData.SetSize( PngData.GetSize() - Overflow );
		return true;

		/*


		//	split into sections of 64k bytes
		static int WindowSizeShift = 1;
		//	gr: windowsize is just for decoder?
//		http://www.libpng.org/pub/png/spec/1.2/PNG-Compression.html
		//If the data to be compressed contains 16384 bytes or fewer, the encoder can set the window size by rounding up to a power of 2 (256 minimum). This decreases the memory required not only for encoding but also for decoding, without adversely affecting the compression ratio.
		static int WindowSize = 65535;//32768;
		//int WindowSize = (1 << (WindowSizeShift+8))-minus;
		Array<char> DeflateData;
		bool BlockCountOverflow = (FilteredPixels.GetDataSize() % WindowSize) > 0;
		int BlockCount = (FilteredPixels.GetDataSize() / WindowSize) + BlockCountOverflow;
		assert( BlockCount > 0 );
		DeflateData.Reserve( FilteredPixels.GetSize() + (BlockCount*10) );
		for ( int b=0;	b<BlockCount;	b++ )
		{
			int FirstPixel = b*WindowSize;
			const int PixelCount = ofMin( WindowSize, FilteredPixels.GetDataSize()-FirstPixel );
			bool LastBlock = (b==BlockCount-1);
			auto BlockPixelData = GetRemoteArray( &FilteredPixels[FirstPixel], PixelCount, PixelCount );
			if ( !GetDeflateData( DeflateData, GetArrayBridge(BlockPixelData), LastBlock, WindowSize ) )
				return false;
		}

		//	write deflate data and decompressed checksum
		static uint8 CompressionMethod = 8;	//	deflate
		static uint8 CInfo = WindowSizeShift;	//	
		uint8 Cmf = 0;
		Cmf |= CompressionMethod << 0;
		Cmf |= CInfo << 4;
		
		static uint8 fcheck = 0;
		static uint8 fdict = 0;
		static uint8 flevel = 0;
		uint8 Flg = 0;

		//	The FCHECK value must be such that CMF and FLG, when viewed as
        //	a 16-bit unsigned integer stored in MSB order (CMF*256 + FLG),
        //	is a multiple of 31.
		int Checksum = (Cmf * 256) + Flg;
		if ( Checksum % 31 != 0 )
		{
			fcheck = 31 - (Checksum % 31);
			Checksum = (Cmf * 256) + (Flg|fcheck);
			assert( Checksum % 31 == 0 );
		}
		assert( fcheck < (1<<5) );
		//if ( Checksum == 0 )
		//	fcheck = 31;

		Flg |= fcheck << 0;
		Flg |= fdict << 5;
		Flg |= flevel << 6;
	
		PngData.Reserve( DeflateData.GetSize() + 6 );
		PngData.PushBack( Cmf );
		PngData.PushBack( Flg );
		if ( Flg & 1<<5 )	//	fdict, bit 5
		{
			uint32 Dict = 0;
			PngData.PushBack( Dict );
		}
		PngData.PushBackArray( DeflateData );
		uint32 DecompressedCrc = GetArrayBridge(OrigPixels).GetCrc32();
		PngData.PushBackReinterpretReverse( DecompressedCrc );
		return true;
		*/
	}
	else
	{
		//	unsupported
		return false;
	}
}

bool TPng::GetDeflateData(Array<char>& DeflateData,const ArrayBridge<uint8>& PixelBlock,bool LastBlock,int WindowSize)
{
	//	pixel block should have already been split
	assert( PixelBlock.GetSize() <= WindowSize );
	if ( PixelBlock.GetSize() > WindowSize )
		return false;

	//	if it's NOT the last block, it MUST be full-size
	if ( !LastBlock )
	{
		assert( PixelBlock.GetSize() == WindowSize );
		if ( PixelBlock.GetSize() != WindowSize )
			return false;
	}

	//	http://en.wikipedia.org/wiki/DEFLATE
	//	http://www.ietf.org/rfc/rfc1951.txt
	uint8 Header = 0x0;
	if ( LastBlock )
		Header |= 1<<0;
	uint16 Len = PixelBlock.GetDataSize();
	uint16 NLen = 0;	//	compliment

	DeflateData.PushBack( Header );
	static bool reverse = false;
	if ( reverse)
		DeflateData.PushBackReinterpretReverse( Len );
	else
		DeflateData.PushBackReinterpret( Len );
	if ( sizeof(Len)==2 )
		DeflateData.PushBackReinterpret( NLen );
	DeflateData.PushBackArray( PixelBlock );
	return true;
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
		auto& g = Pixels[(p*SrcChannels)+1];
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
	auto* ConversionFunc = ConversionFuncs.Find( std::tuple<SoyPixelsFormat::Type,SoyPixelsFormat::Type>( GetFormat(), Format ) );
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

static bool treatbgrasrgb = true;

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



bool TPixels::Init(uint16 Width,uint16 Height,uint8 Channels)
{
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	return Init( Width, Height, Format );
}

bool TPixels::Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format)
{
	//	alloc
	GetMeta().DumbSetWidth( Width );
	GetMeta().DumbSetFormat( Format );
	int Alloc = GetWidth() * GetChannels() * Height;
	mPixels.SetSize( Alloc, false );
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

bool SoyPixelsImpl::SetPng(const ArrayBridge<char>& PngData)
{
	assert(false);
	return false;
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

