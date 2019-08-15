#include "SoyPixels.h"
#include "SoyDebug.h"
#include "SoyTime.h"
#include <functional>
#include "RemoteArray.h"
#include "SoyMath.h"
#include "SoyStream.h"
#include "SoyImage.h"


/// Maximum value that a uint16_t pixel will take on in the buffer of any of the FREENECT_DEPTH_MM or FREENECT_DEPTH_REGISTERED frame callbacks
#define FREENECT_DEPTH_MM_MAX_VALUE 10000
/// Value indicating that this pixel has no data, when using FREENECT_DEPTH_MM or FREENECT_DEPTH_REGISTERED depth modes
#define FREENECT_DEPTH_MM_NO_VALUE 0
/// Maximum value that a uint16_t pixel will take on in the buffer of any of the FREENECT_DEPTH_11BIT, FREENECT_DEPTH_10BIT, FREENECT_DEPTH_11BIT_PACKED, or FREENECT_DEPTH_10BIT_PACKED frame callbacks
#define FREENECT_DEPTH_RAW_MAX_VALUE 2048
/// Value indicating that this pixel has no data, when using FREENECT_DEPTH_11BIT, FREENECT_DEPTH_10BIT, FREENECT_DEPTH_11BIT_PACKED, or FREENECT_DEPTH_10BIT_PACKED
#define FREENECT_DEPTH_RAW_NO_VALUE 2047


std::ostream& operator<< (std::ostream &out,const SoyPixelsMeta &in)
{
	out << in.GetWidth() << 'x' << in.GetHeight() << '^' << in.GetFormat();
	return out;
}

std::istream& operator>>( std::istream &in,SoyPixelsMeta &out)
{
	bool GotWidth = false;
	bool GotHeight = false;
	bool GotFormat = false;
	auto InString = Soy::StreamToString( in );

	try
	{
		auto HandleChunk = [&](const std::string& Chunk,char Delin)
		{
			if ( !GotWidth )
			{
				out.DumbSetWidth( Soy::StringToType<size_t>( Chunk ) );
				GotWidth = true;
				return true;
			}
			if ( !GotHeight )
			{
				out.DumbSetHeight( Soy::StringToType<size_t>( Chunk ) );
				GotHeight = true;
				return true;
			}
			if ( !GotFormat )
			{
				out.DumbSetFormat( SoyPixelsFormat::ToType( Chunk ) );
				GotFormat = true;
				return true;
			}

			std::stringstream Error;
			Error << "Too many parts (" << Chunk << ")";
			throw Soy::AssertException( Error.str() );
		};
		Soy::StringSplitByMatches( HandleChunk, InString, "x^", true );
	}
	catch(std::exception& e)
	{
		in.setstate( std::ios::failbit );
		std::Debug << "Error parsing " << Soy::GetTypeName(out) << ": " << e.what() << std::endl;
	}
	return in;
}




prmem::Heap& SoyPixels::GetDefaultHeap()
{
	static auto* Heap = new prmem::Heap( true, true, "SoyPixels::DefaultHeap" );
	return *Heap;
}
	

SoyPixelsFormat::Type SoyPixelsFormat::GetYuvFull(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case Luma_Full:
		case Luma_Ntsc:
		case Luma_Smptec:
			return Luma_Full;
	
		case Yuv_8_88_Full:
		case Yuv_8_88_Ntsc:
		case Yuv_8_88_Smptec:
			return Yuv_8_88_Full;

		case Yuv_8_8_8_Full:
		case Yuv_8_8_8_Ntsc:
		case Yuv_8_8_8_Smptec:
			return Yuv_8_8_8_Full;

		case YYuv_8888_Full:
		case YYuv_8888_Ntsc:
		case YYuv_8888_Smptec:
			return YYuv_8888_Full;

		case Yuv_844_Ntsc:
		case Yuv_844_Full:
		case Yuv_844_Smptec:
			return Yuv_844_Full;

		case Uvy_844_Full:
			return Uvy_844_Full;
			
		default:
			break;
	}

	throw Soy::AssertException( std::string(__func__) + " no equivilent");
}


SoyPixelsFormat::Type SoyPixelsFormat::GetYuvNtsc(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case Luma_Full:
		case Luma_Ntsc:
		case Luma_Smptec:
			return Luma_Ntsc;
	
		case Yuv_8_88_Full:
		case Yuv_8_88_Ntsc:
		case Yuv_8_88_Smptec:
			return Yuv_8_88_Ntsc;

		case Yuv_8_8_8_Full:
		case Yuv_8_8_8_Ntsc:
		case Yuv_8_8_8_Smptec:
			return Yuv_8_8_8_Ntsc;

		case YYuv_8888_Full:
		case YYuv_8888_Ntsc:
		case YYuv_8888_Smptec:
			return YYuv_8888_Ntsc;

		case Yuv_844_Ntsc:
		case Yuv_844_Full:
		case Yuv_844_Smptec:
			return Yuv_844_Ntsc;

		default:
			break;
	}

	throw Soy::AssertException( std::string(__func__) + " no equivilent");
}

SoyPixelsFormat::Type SoyPixelsFormat::GetYuvSmptec(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case Luma_Full:
		case Luma_Ntsc:
		case Luma_Smptec:
			return Luma_Smptec;
	
		case Yuv_8_88_Full:
		case Yuv_8_88_Ntsc:
		case Yuv_8_88_Smptec:
			return Yuv_8_88_Smptec;

		case Yuv_8_8_8_Full:
		case Yuv_8_8_8_Ntsc:
		case Yuv_8_8_8_Smptec:
			return Yuv_8_8_8_Smptec;

		case YYuv_8888_Full:
		case YYuv_8888_Ntsc:
		case YYuv_8888_Smptec:
			return YYuv_8888_Smptec;

		case Yuv_844_Full:
		case Yuv_844_Ntsc:
		case Yuv_844_Smptec:
			return Yuv_844_Smptec;
			
		default:
			break;
	}

	throw Soy::AssertException( std::string(__func__) + " no equivilent");
}

SoyPixelsFormat::Type SoyPixelsFormat::ChangeYuvColourRange(Type Format,Type YuvColourRange)
{
	switch ( YuvColourRange )
	{
		case Luma_Full:
		case Yuv_8_88_Full:
		case Yuv_8_8_8_Full:
		case YYuv_8888_Full:
		case Yuv_844_Full:
			return GetYuvFull( Format );

		case Luma_Ntsc:
		case Yuv_8_88_Ntsc:
		case Yuv_8_8_8_Ntsc:
		case YYuv_8888_Ntsc:
		case Yuv_844_Ntsc:
			return GetYuvNtsc( Format );

		case Luma_Smptec:
		case Yuv_8_88_Smptec:
		case Yuv_8_8_8_Smptec:
		case YYuv_8888_Smptec:
		case Yuv_844_Smptec:
			return GetYuvSmptec( Format );


		default:
			break;
	}

	std::stringstream Error;
	Error << std::string(__func__) << " " << Format << "," << YuvColourRange << " no conversion";
	throw Soy::AssertException( Error.str() );
}

SoyPixelsFormat::Type SoyPixelsFormat::GetFloatFormat(Type Format)
{
	switch ( Format )
	{
		case Float1:
		case Float2:
		case Float3:
		case Float4:
			return Format;
			
		case Greyscale:
		case ChromaU_8:
		case ChromaV_8:
			return Float1;
			
		case GreyscaleAlpha:
		case ChromaUV_88:
		case ChromaUV_8_8:
			return Float2;
			
		case RGB:
		case BGR:
			return Float3;
			
		case RGBA:
		case ARGB:
		case BGRA:
			return Float4;
			
		default:
			break;
	}
	
	
	std::stringstream Error;
	Error << std::string(__func__) << " " << Format << " no conversion";
	throw Soy::AssertException( Error.str() );

}

SoyPixelsFormat::Type SoyPixelsFormat::GetByteFormat(Type Format)
{
	switch ( Format )
	{
		case Float1:
			return Greyscale;
			
		case Float2:
			return GreyscaleAlpha;
			
		case Float3:
			return RGB;
			
		case Float4:
			return RGBA;
		
		default:
			return Format;
	}
}



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



size_t SoyPixelsFormat::GetChannelCount(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
	case Greyscale:		return 1;
	case Luma_Ntsc:		return 1;
	case Luma_Smptec:	return 1;
	case GreyscaleAlpha:	return 2;
	case RGB:			return 3;
	case BGR:			return 3;
	case RGBA:			return 4;
	case BGRA:			return 4;
	case ARGB:			return 4;
	
		//	16 bit, but 1 channel
	case KinectDepth:	return 1;
	case FreenectDepth11bit:	return 1;
	case FreenectDepth10bit:	return 21;	
	case FreenectDepthmm:	return 1;

	case ChromaUV_8_8:	return 1;
	case ChromaUV_88:	return 2;
	case ChromaUV_44:	return 1;	//	actually 2 channels, but not supporting nibbles
	case ChromaU_8:		return 1;
	case ChromaV_8:		return 1;

	//	yuv 844 is interlaced luma & chroma, so kinda have 2 channels (helps with a lot of things when it aligns even though we have technically 3 channels)
	case Uvy_844_Full:
	case Yuv_844_Full:
	case Yuv_844_Ntsc:
	case Yuv_844_Smptec:
	case YYuv_8888_Full:
	case YYuv_8888_Ntsc:
	case YYuv_8888_Smptec:
		return 2;

	case uyvy:
		return 2;

	case Float1:	return 1;
	case Float2:	return 2;
	case Float3:	return 3;
	case Float4:	return 4;

	//	throw if we try and get a channel count of zero
	case Invalid:
	default:
		break;
	}

	std::stringstream Error;
	Error << __func__ << " not implemented for " << Format;
	throw Soy::AssertException( Error.str() );
}


bool SoyPixelsFormat::IsFloatChannel(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case Float1:
		case Float2:
		case Float3:
		case Float4:
			return true;

		case Greyscale:
		case Luma_Ntsc:
		case Luma_Smptec:
		case GreyscaleAlpha:
		case RGB:
		case BGR:
		case RGBA:
		case BGRA:
		case ARGB:
		case KinectDepth:
		case FreenectDepth11bit:
		case FreenectDepth10bit:
		case FreenectDepthmm:
		case ChromaUV_8_8:
		case ChromaUV_88:
		case ChromaUV_44:
		case ChromaU_8:
		case ChromaV_8:
		case Uvy_844_Full:
		case Yuv_844_Full:
		case Yuv_844_Ntsc:
		case Yuv_844_Smptec:
		case YYuv_8888_Full:
		case YYuv_8888_Ntsc:
		case YYuv_8888_Smptec:
		case uyvy:
			return false;
			
		default:
			break;
	}
	
	std::stringstream Error;
	Error << __func__ << " not implemented for " << Format;
	throw Soy::AssertException( Error.str() );
}


uint8_t SoyPixelsFormat::GetBytesPerChannel(SoyPixelsFormat::Type Format)
{
	switch (Format)
	{
	case Float1:
	case Float2:
	case Float3:
	case Float4:
		return sizeof(float);

	case KinectDepth:
	case FreenectDepth11bit:
	case FreenectDepth10bit:
	case FreenectDepthmm:
		return sizeof(uint16_t);

	default:
		return sizeof(uint8_t);
	}
}


SoyPixelsFormat::Type SoyPixelsFormat::GetFormatFromChannelCount(size_t ChannelCount)
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

namespace SoyPixelsFormat
{
	const std::map<Type,BufferArray<Type,2>>&	GetMergedFormatMap();
}

const std::map<SoyPixelsFormat::Type,BufferArray<SoyPixelsFormat::Type,2>>& SoyPixelsFormat::GetMergedFormatMap()
{
	static std::map<Type,BufferArray<Type,2>> Map;

	if ( Map.empty() )
	{
		Map[Yuv_8_88_Full].PushBackArray( { Luma_Full, ChromaUV_88 } );
		Map[Yuv_8_88_Ntsc].PushBackArray( { Luma_Ntsc, ChromaUV_88 } );
		Map[Yuv_8_88_Smptec].PushBackArray( { Luma_Smptec, ChromaUV_88 } );

		Map[Yuv_8_8_8_Full].PushBackArray( { Luma_Full, ChromaUV_8_8 } );
		Map[Yuv_8_8_8_Ntsc].PushBackArray( { Luma_Ntsc, ChromaUV_8_8 } );
		Map[Yuv_8_8_8_Smptec].PushBackArray( { Luma_Smptec, ChromaUV_8_8 } );

		Map[Yuv_844_Full].PushBackArray( { Luma_Full, ChromaUV_44 } );
		Map[Yuv_844_Ntsc].PushBackArray( { Luma_Ntsc, ChromaUV_44 } );
		Map[Yuv_844_Smptec].PushBackArray( { Luma_Smptec, ChromaUV_44 } );

		Map[ChromaUV_8_8].PushBackArray( { ChromaU_8, ChromaV_8 } );
	}

	return Map;
}



void SoyPixelsFormat::GetFormatPlanes(Type Format,ArrayBridge<Type>&& PlaneFormats)
{
	auto& Map = GetMergedFormatMap();
	auto it = Map.find( Format );

	//	single plane type, return self
	if ( it == Map.end() )
	{
		PlaneFormats.PushBack(Format);
		return;
	}

	PlaneFormats.Copy( it->second );
}


SoyPixelsFormat::Type SoyPixelsFormat::GetMergedFormat(SoyPixelsFormat::Type Formata,SoyPixelsFormat::Type Formatb)
{
	BufferArray<Type,2> Formatab( { Formata, Formatb } );

	auto& Map = GetMergedFormatMap();
	for ( auto it=Map.begin();	it!=Map.end();	it++ )
	{
		auto& Matchab = it->second;
		auto& MergedFormat = it->first;
		if ( Matchab == Formatab )
			return MergedFormat;
	}

	//	no merged version
	return SoyPixelsFormat::Invalid;
}

//	merge index & palette into Paletteised_8_8
void SoyPixelsFormat::MakePaletteised(SoyPixelsImpl& PalettisedImage,const SoyPixelsImpl& IndexedImage,const SoyPixelsImpl& Palette,uint8 TransparentIndex)
{
	Soy::Assert( IndexedImage.GetFormat() == SoyPixelsFormat::Greyscale, "Expected IndexedImage to be Greyscale" );
	Soy::Assert( Palette.GetFormat() == SoyPixelsFormat::RGB || Palette.GetFormat() == SoyPixelsFormat::RGBA, "Expected Palette to be RGB or RGBA" );
	Soy::Assert( Palette.GetHeight() == 1, "Expected palette to have height of 1" );
	Soy::Assert( Palette.GetWidth() <= ((1<<16)-1), "Expected palette to have max width of 65535" );

	auto& PaletteArray = Palette.GetPixelsArray();
	auto& IndexedArray = IndexedImage.GetPixelsArray();
	
	//	manually construct this image
	auto& PiMeta = PalettisedImage.GetMeta();
	auto& PiArray = PalettisedImage.GetPixelsArray();

	if ( Palette.GetFormat() == SoyPixelsFormat::RGB )
		PiMeta.DumbSetFormat( SoyPixelsFormat::Palettised_RGB_8 );
	if ( Palette.GetFormat() == SoyPixelsFormat::RGBA )
		PiMeta.DumbSetFormat( SoyPixelsFormat::Palettised_RGBA_8 );
	PiMeta.DumbSetWidth( IndexedImage.GetWidth() );
	PiMeta.DumbSetHeight( IndexedImage.GetHeight() );

	PiArray.Clear();
	
	//	write header
	uint8 PaletteSizeA = (Palette.GetWidth()>>0) & 0xff;
	uint8 PaletteSizeB = (Palette.GetWidth()>>8) & 0xff;
	GetArrayBridge(PiArray).PushBack( PaletteSizeA );
	GetArrayBridge(PiArray).PushBack( PaletteSizeB );
	GetArrayBridge(PiArray).PushBack( TransparentIndex );

	//	write data
	PiArray.PushBackArray( PaletteArray );
	PiArray.PushBackArray( IndexedArray );
	
	//	todo: verify by splitting
	static bool Verify = false;
	if ( Verify )
	{
		Array<std::shared_ptr<SoyPixelsImpl>> Planes;
		PalettisedImage.SplitPlanes( GetArrayBridge(Planes) );
		Soy::Assert( Planes.GetSize() == 2, "Palettised image not split into 2");
		Soy::Assert( Planes[0]->GetMeta() == Palette.GetMeta(), "Palettised image split; palette meta check failed");
		Soy::Assert( Planes[1]->GetMeta() == IndexedImage.GetMeta(), "Palettised image split; index meta check failed");
	}
}

size_t SoyPixelsFormat::GetHeaderSize(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		case SoyPixelsFormat::Palettised_RGB_8:
		case SoyPixelsFormat::Palettised_RGBA_8:
			return 3;
			
		default:
			return 0;
	}
}

void SoyPixelsFormat::GetHeaderPalettised(const ArrayBridge<uint8>&& Data,size_t& PaletteSize,size_t& TransparentIndex)
{
	//	gr: note no distinction between the paletised formats here
	auto HeaderSize = GetHeaderSize( SoyPixelsFormat::Palettised_RGB_8 );
	Soy::Assert( HeaderSize == 3, "Header size mismatch");
	Soy::Assert( Data.GetSize() >= HeaderSize, "SoyPixelsFormat::GetHeaderPalettised Data underrun" );
	
	PaletteSize = Data[0];
	PaletteSize |= Data[1] << 8;
	TransparentIndex = Data[2];
}



std::map<SoyPixelsFormat::Type, std::string> SoyPixelsFormat::EnumMap =
{
	{ SoyPixelsFormat::Invalid,				"Invalid" },
//	{ SoyPixelsFormat::UnityUnknown,		"UnityUnknown" },
	{ SoyPixelsFormat::Greyscale,			"Greyscale" },
	{ SoyPixelsFormat::GreyscaleAlpha,		"GreyscaleAlpha"	},
	{ SoyPixelsFormat::RGB,					"RGB"	},
	{ SoyPixelsFormat::RGBA,				"RGBA"	},
	{ SoyPixelsFormat::BGRA,				"BGRA"	},
	{ SoyPixelsFormat::BGR,					"BGR"	},
	{ SoyPixelsFormat::ARGB,				"ARGB"	},
	{ SoyPixelsFormat::KinectDepth,			"KinectDepth"	},
	{ SoyPixelsFormat::FreenectDepth10bit,	"FreenectDepth10bit"	},
	{ SoyPixelsFormat::FreenectDepth11bit,	"FreenectDepth11bit"	},
	{ SoyPixelsFormat::FreenectDepthmm,		"FreenectDepthmm"	},
	{ SoyPixelsFormat::uyvy,				"uyvy"	},
	{ SoyPixelsFormat::Yuv_8_88_Full,		"Yuv_8_88_Full"	},
	{ SoyPixelsFormat::Yuv_8_88_Ntsc,		"Yuv_8_88_Ntsc"	},
	{ SoyPixelsFormat::Yuv_8_88_Smptec,		"Yuv_8_88_Smptec"	},
	{ SoyPixelsFormat::Yuv_8_8_8_Full,		"Yuv_8_8_8_Full"	},
	{ SoyPixelsFormat::Yuv_8_8_8_Ntsc,		"Yuv_8_8_8_Ntsc"	},
	{ SoyPixelsFormat::Yuv_8_8_8_Smptec,	"Yuv_8_8_8_Smptec"	},
	{ SoyPixelsFormat::YYuv_8888_Full,		"YYuv_8888_Full"	},
	{ SoyPixelsFormat::YYuv_8888_Ntsc,		"YYuv_8888_Ntsc"	},
	{ SoyPixelsFormat::YYuv_8888_Smptec,	"YYuv_8888_Smptec"	},
	{ SoyPixelsFormat::Uvy_844_Full,		"Uvy_844_Full"	},
	{ SoyPixelsFormat::Yuv_844_Full,		"Yuv_844_Full"	},
	{ SoyPixelsFormat::Yuv_844_Ntsc,		"Yuv_844_Ntsc"	},
	{ SoyPixelsFormat::Yuv_844_Smptec,		"Yuv_844_Smptec"	},
	{ SoyPixelsFormat::Luma_Full,			"LumaFull"	},
	{ SoyPixelsFormat::Luma_Ntsc,			"Luma_Ntsc"	},
	{ SoyPixelsFormat::Luma_Smptec,			"Luma_Smptec"	},
	{ SoyPixelsFormat::ChromaUV_8_8,		"ChromaUV_8_8"	},
	{ SoyPixelsFormat::ChromaUV_88,			"ChromaUV_88"	},
	{ SoyPixelsFormat::ChromaUV_44,			"ChromaUV_44"	},
	{ SoyPixelsFormat::ChromaU_8,			"ChromaU_8"	},
	{ SoyPixelsFormat::ChromaV_8,			"ChromaV_8"	},
	{ SoyPixelsFormat::Palettised_RGB_8,	"Palettised_RGB_8"	},
	{ SoyPixelsFormat::Palettised_RGBA_8,	"Palettised_RGBA_8"	},
	{ SoyPixelsFormat::Float1,				"Float1"	},
	{ SoyPixelsFormat::Float2,				"Float2"	},
	{ SoyPixelsFormat::Float3,				"Float3"	},
	{ SoyPixelsFormat::Float4,				"Float4"	},
};


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


void SoyPixelsImpl::SetChannels(uint8 Channels)
{
	SoyPixelsFormat::Type Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	SetFormat( Format );
}


void SetDepthColour(uint8& Red,uint8& Green,uint8& Blue,float Depth,int PlayerIndex,bool NoDepthValue)
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
	uint8 Greyscale = 255 - std::clamped<int>( static_cast<int>(Depth*255.f), MinBrightness, 255 );

	static bool UseRainbowScale = true;
	if ( UseRainbowScale )
	{
		if ( Depth < 1.f/3.f )
		{
			float d = Soy::Range( Depth, 0.f, 1.f/3.f );
			//	red to green
			Red = Soy::Lerp( 0, 255, 1.f-d );
			Green = Soy::Lerp( 0, 255, d );
			Blue = 0;
		}
		else if ( Depth < 2.f/3.f )
		{
			float d = Soy::Range( Depth, 1.f/3.f, 2.f/3.f );
			//	yellow to blue
			Green = Soy::Lerp( 0, 255, 1.f-d );
			Blue = Soy::Lerp( 0, 255, d );
			Red = 0;
		}
		else
		{
			float d = Soy::Range( Depth, 2.f/3.f, 3.f/3.f );
			//	blue to red
			Blue = Soy::Lerp( 0, 255, 1.f-d );
			Red = Soy::Lerp( 0, 255, d );
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

void ConvertFormat_KinectDepthToGreyscale(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	bool GreyscaleAlphaFormat = (NewFormat == SoyPixelsFormat::GreyscaleAlpha);
	auto Height = Meta.GetHeight();
	auto PixelCount = Meta.GetWidth() * Height;
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint16 KinectDepth = *reinterpret_cast<uint16*>( &Pixels[p*2] );
		uint8& Greyscale = Pixels[ p * (GreyscaleAlphaFormat?2:1) ];

		int PlayerIndex = KinectDepth & ((1<<3)-1);
		float Depthf = static_cast<float>(KinectDepth >> 3) / static_cast<float>( 1<<13 );
		Greyscale = std::clamped<int>( static_cast<int>(Depthf*255.f), 0, 255 );

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
}

void ConvertFormat_KinectDepthToRgb(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	Array<uint16> DepthPixels;
	DepthPixels.SetSize( Pixels.GetSize() / 2 );
	memcpy( DepthPixels.GetArray(), Pixels.GetArray(), DepthPixels.GetDataSize() );
	
	auto Components = SoyPixelsFormat::GetChannelCount( NewFormat );
	auto Height = Meta.GetHeight();
	auto PixelCount = Meta.GetWidth() * Height;
	
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
		float Depthf = Soy::Range( KinectDepth, MinDepth, MaxDepth );
		
		uint8& Red = Pixels[ p * (Components) + 0 ];
		uint8& Green = Pixels[ p * (Components) + 1 ];
		uint8& Blue = Pixels[ p * (Components) + 2 ];
		SetDepthColour( Red, Green, Blue, Depthf, PlayerIndex, DepthInvalid );
	}
	
	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight() == Height );
}



bool DepthToGreyOrRgb(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	bool RawDepth = (Meta.GetFormat() != SoyPixelsFormat::FreenectDepthmm);
	uint16 MinDepth = SoyPixelsFormat::GetMinValue( Meta.GetFormat() );
	uint16 MaxDepth = SoyPixelsFormat::GetMaxValue( Meta.GetFormat() );
	uint16 InvalidDepth = SoyPixelsFormat::GetInvalidValue( Meta.GetFormat() );
	int PlayerIndexFirstBit = SoyPixelsFormat::GetPlayerIndexFirstBit( Meta.GetFormat() );
 
	Array<uint16> DepthPixels;
	DepthPixels.SetSize( Pixels.GetSize() / 2 );
	memcpy( DepthPixels.GetArray(), Pixels.GetArray(), DepthPixels.GetDataSize() );
	
	auto Components = SoyPixelsFormat::GetChannelCount( NewFormat );
	auto Height = Meta.GetHeight();
	auto PixelCount = Meta.GetWidth() * Height;
	
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
		float Depthf = Soy::Range( KinectDepth, MinDepth, MaxDepth );
		
		uint8& Red = Pixels[ p * (Components) + 0 ];
		
		//	if RGB then we do different colours for raw and mm
		if ( Components >= 3 )
		{
			int CompZero = RawDepth ? 1 : 2;
			int CompDepth = RawDepth ? 2 : 1;
			uint8& Green = Pixels[ p * (Components) + CompZero ];
			uint8& Blue = Pixels[ p * (Components) + CompDepth ];

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
			uint8 Greyscale = std::clamped<int>( static_cast<int>(Depthf*255.f), GreyMin, 255 );

			//	set first component to greyscale
			Red = DepthInvalid ? GreyInvalid : Greyscale;
			
			//	other components just valid/not
			for ( int c=1;	c<Components;	c++ )
			{
				uint8& Blue = Pixels[ p * (Components) + c ];
				Blue = DepthInvalid ? 0 : 255;
			}
		}
		
	}

	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight() == Height );
	return true;
}


bool ConvertFormat_BgrToRgb(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto Height = Meta.GetHeight();
	auto PixelCount = Meta.GetWidth() * Height;
	auto SrcChannels = Meta.GetChannels();
	auto DestChannels = SoyPixelsFormat::GetChannelCount( NewFormat );
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

bool ConvertFormat_RgbToRgba(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto& RgbMeta = Meta;
	auto w = RgbMeta.GetWidth();
	auto h = RgbMeta.GetHeight();
	SoyPixelsMeta RgbaMeta( w, h, NewFormat );
	auto PixelCount = size_cast<int>(w*h);
	auto RgbStride = RgbMeta.GetPixelDataSize();
	auto RgbaStride = RgbaMeta.GetPixelDataSize();
	Pixels.SetSize( RgbaMeta.GetDataSize() );
	
	if ( RgbStride != 3 )
		throw Soy::AssertException("ConvertFormat_RgbToRgba: Expected source stride of 3 bytes");
	if ( RgbaStride != 4 )
		throw Soy::AssertException("ConvertFormat_RgbToRgba: Expected destination stride of 4 bytes");
	
	//	fill backwards and we won't overwrite anything
	for ( int p=PixelCount-1;	p>=0;	p-- )
	{
		uint8_t* OldPos = &Pixels[p*RgbStride];
		uint8_t* NewPos = &Pixels[p*RgbaStride];
		NewPos[0] = OldPos[0];
		NewPos[1] = OldPos[1];
		NewPos[2] = OldPos[2];
		NewPos[3] = 255;
	}
	
	Meta = RgbaMeta;
	return true;
}

bool ConvertFormat_GreyscaleToRgb(ArrayInterface<uint8>& PixelsArray,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto& GreyMeta = Meta;
	auto w = GreyMeta.GetWidth();
	auto h = GreyMeta.GetHeight();
	SoyPixelsMeta RgbMeta( w, h, NewFormat );
	auto PixelCount = size_cast<int>(w*h);
	auto GreyStride = GreyMeta.GetPixelDataSize();
	auto RgbStride = RgbMeta.GetPixelDataSize();
	PixelsArray.SetSize( RgbMeta.GetDataSize() );
	
	if ( GreyStride != 1 )
		throw Soy::AssertException("ConvertFormat_GreyscaleToRgb: Expected source stride of 1 bytes");
	if ( RgbStride != 3 )
		throw Soy::AssertException("ConvertFormat_GreyscaleToRgb: Expected destination stride of 3 bytes");
	
	auto* Pixels = PixelsArray.GetArray();
	
	//	fill backwards and we won't overwrite anything
	for ( int p=PixelCount-1;	p>=0;	p-- )
	{
		uint8_t* OldPos = &Pixels[p*GreyStride];
		uint8_t* NewPos = &Pixels[p*RgbStride];
		NewPos[0] = OldPos[0];
		NewPos[1] = OldPos[0];
		NewPos[2] = OldPos[0];
	}
	
	Meta = RgbMeta;
	return true;
}


void ConvertFormat_Greyscale_To_Yuv_8_8_8(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	we just need to ADD another pair of planes, so luma plane stays the same
	SoyPixelsMeta YuvMeta(Meta.GetWidth(), Meta.GetHeight(), NewFormat);

	//	split the planes and then write the new data to them
	PixelsArray.SetSize(YuvMeta.GetDataSize());
	SoyPixelsRemote YuvPixels(PixelsArray.GetArray(), YuvMeta.GetWidth(), YuvMeta.GetHeight(), YuvMeta.GetDataSize(), YuvMeta.GetFormat());
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> Planes;
	YuvPixels.SplitPlanes(GetArrayBridge(Planes));

	/*
	//	write luma
	auto& Luma = *Planes[0];
	auto& LumaArray = Luma.GetPixelsArray();
	for (auto i = 0; i < LumaArray.GetDataSize(); i++)
	{
		auto rgbi = i * GreyscaleStride;
		auto r = GreyscalePixels[rgbi + 0];
		//auto g = RgbPixels[rgbi + 1];
		//auto b = RgbPixels[rgbi + 2];
		//auto Grey = (r + g + b) / 3;
		LumaArray[i] = r;
	}
	*/

	//	write chromas
	auto& ChromaU = *Planes[1];
	auto& ChromaUArray_Array = ChromaU.GetPixelsArray();
	auto* ChromaUArray = ChromaU.GetPixelsArray().GetArray();
	auto& ChromaV = *Planes[2];
	auto* ChromaVArray = ChromaV.GetPixelsArray().GetArray();
	for (auto i = 0; i < ChromaUArray_Array.GetDataSize(); i++)
	{
		uint8_t u = 128;
		uint8_t v = 128;
		ChromaUArray[i] = u;
		ChromaVArray[i] = v;
	}

	Meta = YuvMeta;
}

void ConvertFormat_RGB_To_Yuv_8_8_8(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	SoyPixelsMeta YuvMeta(Meta.GetWidth(), Meta.GetHeight(), NewFormat);

	//	copy rgb
	Array<uint8_t> RgbPixels;
	RgbPixels.Copy(PixelsArray);
	auto RgbStride = 3;

	//	split the planes and then write the new data to them
	PixelsArray.SetSize(YuvMeta.GetDataSize());
	SoyPixelsRemote YuvPixels(PixelsArray.GetArray(), YuvMeta.GetWidth(), YuvMeta.GetHeight(), YuvMeta.GetDataSize(), YuvMeta.GetFormat());
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> Planes;
	YuvPixels.SplitPlanes(GetArrayBridge(Planes));

	//	write luma
	auto& Luma = *Planes[0];
	auto& LumaArray = Luma.GetPixelsArray();
	for (auto i = 0; i < LumaArray.GetDataSize(); i++)
	{
		auto rgbi = i * RgbStride;
		auto r = RgbPixels[rgbi + 0];
		auto g = RgbPixels[rgbi + 1];
		auto b = RgbPixels[rgbi + 2];
		auto Grey = (r + g + b) / 3;
		LumaArray[i] = Grey;
	}

	//	write chromas
	auto& ChromaU = *Planes[1];
	auto& ChromaUArray = ChromaU.GetPixelsArray();
	auto& ChromaV = *Planes[2];
	auto& ChromaVArray = ChromaV.GetPixelsArray();
	for (auto i = 0; i < ChromaUArray.GetDataSize(); i++)
	{
		uint8_t u = 128;
		uint8_t v = 128;
		ChromaUArray[i] = u;
		ChromaVArray[i] = v;
	}

	Meta = YuvMeta;
}



void ConvertFormat_Uvy844_To_Luma(ArrayInterface<uint8>& PixelsArray,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto& YuvMeta = Meta;
	auto w = YuvMeta.GetWidth();
	auto h = YuvMeta.GetHeight();
	SoyPixelsMeta LumaMeta( w, h, NewFormat );
	auto PixelCount = size_cast<int>(w*h);
	/*
	auto YuvStride = YuvMeta.GetPixelDataSize();
	auto LumaStride = LumaMeta.GetPixelDataSize();
	PixelsArray.SetSize( LumaMeta.GetDataSize() );
	
	*/
	auto YuvStride = 2;
	auto LumaStride = 1;
	auto* Pixels = PixelsArray.GetArray();
	
	//	when shrinking, we go forward through the array
	for ( int p=0;	p<PixelCount;	p++ )
	{
		uint8_t* OldPos = &Pixels[p*YuvStride];
		uint8_t* NewPos = &Pixels[p*LumaStride];
		
		//	invert
		//auto Luma = 255 - OldPos[1];
		auto Luma = OldPos[1];
		
		//	threshold
		//Luma = (Luma < 100) ? 0 : 255;

		NewPos[0] = Luma;
	}
	
	PixelsArray.SetSize( LumaMeta.GetDataSize() );

	Meta = LumaMeta;
}

void ConvertFormat_TwoChannelToFour(ArrayInterface<uint8>& PixelsArray,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto& TwoMeta = Meta;
	auto w = TwoMeta.GetWidth();
	auto h = TwoMeta.GetHeight();
	SoyPixelsMeta FourMeta( w, h, NewFormat );
	auto PixelCount = size_cast<int>(w*h);
	auto TwoStride = TwoMeta.GetPixelDataSize();
	auto FourStride = FourMeta.GetPixelDataSize();
	PixelsArray.SetSize( FourMeta.GetDataSize() );
	
	if ( TwoStride != 2 )
		throw Soy::AssertException("ConvertFormat_TwoChannelToFour: Expected source stride of 2 bytes");
	if ( FourStride != 4 )
		throw Soy::AssertException("ConvertFormat_TwoChannelToFour: Expected destination stride of 4 bytes");
	
	auto* Pixels = PixelsArray.GetArray();
	
	//	fill backwards and we won't overwrite anything
	for ( int p=PixelCount-1;	p>=0;	p-- )
	{
		uint8_t* OldPos = &Pixels[p*TwoStride];
		uint8_t* NewPos = &Pixels[p*FourStride];
		NewPos[0] = OldPos[0];
		NewPos[1] = OldPos[1];
		NewPos[2] = 0;
		NewPos[3] = 255;
	}
	
	Meta = FourMeta;
}

void ConvertFormat_GreyscaleToRgba(ArrayInterface<uint8>& PixelsArray,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto& GreyMeta = Meta;
	auto w = GreyMeta.GetWidth();
	auto h = GreyMeta.GetHeight();
	SoyPixelsMeta RgbaMeta( w, h, NewFormat );
	auto PixelCount = size_cast<int>(w*h);
	auto GreyStride = GreyMeta.GetPixelDataSize();
	auto RgbaStride = RgbaMeta.GetPixelDataSize();
	PixelsArray.SetSize( RgbaMeta.GetDataSize() );
	
	if ( GreyStride != 1 )
		throw Soy::AssertException("ConvertFormat_GreyscaleToRgba: Expected source stride of 1 bytes");
	if ( RgbaStride != 4 )
		throw Soy::AssertException("ConvertFormat_GreyscaleToRgba: Expected destination stride of 4 bytes");
	
	auto* Pixels = PixelsArray.GetArray();
	
	//	fill backwards and we won't overwrite anything
	for ( int p=PixelCount-1;	p>=0;	p-- )
	{
		uint8_t* OldPos = &Pixels[p*GreyStride];
		uint8_t* NewPos = &Pixels[p*RgbaStride];
		NewPos[0] = OldPos[0];
		NewPos[1] = OldPos[0];
		NewPos[2] = OldPos[0];
		NewPos[3] = 255;
	}
	
	Meta = RgbaMeta;
}


bool ConvertFormat_RGBAToGreyscale(ArrayInterface<uint8>& PixelsArray,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto Height = Meta.GetHeight();
	auto PixelCount = Meta.GetWidth() * Height;
	auto Channels = Meta.GetChannels();
	auto GreyscaleChannels = SoyPixelsFormat::GetChannelCount(NewFormat);

	//	todo: store alpha in loop
	assert( GreyscaleChannels == 1 );

	uint8_t* Pixels = PixelsArray.GetArray();
	
	for ( int p=0;	p<PixelCount;	p++ )
	{
		//	todo: ignore alpha if present
		float Intensity = 0.f;
		int ReadChannels = 3;
		for ( int c=0;	c<ReadChannels;	c++ )
			Intensity += Pixels[ p*Channels + c ];
		Intensity /= ReadChannels;
		
		uint8& Greyscale = Pixels[ p * GreyscaleChannels ];
		//Greyscale = std::clamped<int>( static_cast<int>(Intensity), 0, 255 );
		Greyscale = static_cast<int>(Intensity);
	}

	//	half the pixels & change format
	PixelsArray.SetSize( PixelCount * GreyscaleChannels );
	Meta.DumbSetFormat( NewFormat );
	assert( Meta.IsValid() );
	assert( Meta.GetHeight() == Height );
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

void ConvertDepth16(ArrayInterface<uint8>& Pixels,SoyPixelsMeta& Meta,SoyPixelsFormat::Type NewFormat)
{
	auto OldFormat = Meta.GetFormat();

	//	assume this is a valid depth format...
	Soy::Assert(SoyPixelsFormat::GetChannelCount(OldFormat) == 2, "expected 2-channel 16 bit depth format" );
	Soy::Assert(SoyPixelsFormat::GetChannelCount(NewFormat) == 2, "expected 2-channel 16 bit depth format" );
	
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
	auto PixelCount = Meta.GetWidth() * Meta.GetHeight();

	static bool Debug = false;
	
	for ( int p=0;	p<PixelCount;	p++ )
	{
		auto& DepthValue = DepthPixels[p];
		uint16 Depth16 = DepthValue & ((1<<OldDepthBits)-1);
		//uint16 Player16 = DepthValue >> OldDepthBits;
		bool InvalidDepth = (Depth16 == OldInvalid);
		float Depthf = Soy::Range<uint16>( Depth16, OldMin, OldMax );
		Depthf = std::clamped<float>( Depthf, 0.f, 1.f );
		
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
			DepthValue = static_cast<uint16>( Soy::Lerp( NewMin, NewMax, Depthf ) );
		}
		
		if ( Debug )
			std::Debug << Depth16 << "/" << DepthValue << "   ";
	}
	if ( Debug )
		std::Debug << std::endl;
	
	Meta.DumbSetFormat( NewFormat );
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
	std::function<void(ArrayInterface<uint8>&,SoyPixelsMeta&,SoyPixelsFormat::Type)>	mFunction;
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
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::RGBA, ConvertFormat_RgbToRgba ),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::RGB, ConvertFormat_GreyscaleToRgb ),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::RGBA, ConvertFormat_GreyscaleToRgba ),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8_Full, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8_Ntsc, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8_Smptec, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::ChromaUV_88, SoyPixelsFormat::RGBA, ConvertFormat_TwoChannelToFour ),
	TConvertFunc( SoyPixelsFormat::Uvy_844_Full, SoyPixelsFormat::Greyscale, ConvertFormat_Uvy844_To_Luma),
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::Yuv_8_8_8_Ntsc, ConvertFormat_RGB_To_Yuv_8_8_8),
};


void SoyPixelsImpl::SetFormat(SoyPixelsFormat::Type Format)
{
	auto OldFormat = GetFormat();
	if ( !SoyPixelsFormat::IsValid( Format ) )
	{
		std::stringstream Error;
		Error << "Trying to change to " << Format << " (invalid)";
		throw Soy::AssertException(Error.str());
	}

	//	allow these to be the same!
	auto IsGrey = [](SoyPixelsFormat::Type Format)
	{
		switch(Format)
		{
			case SoyPixelsFormat::Luma_Full:
			case SoyPixelsFormat::Luma_Ntsc:
			case SoyPixelsFormat::Luma_Smptec:
			//case SoyPixelsFormat::Greyscale:
				return true;
			default:
				return false;
		}
	};
	if ( IsGrey(OldFormat) && IsGrey(Format) )
	{
		this->GetMeta().DumbSetFormat(Format);
		return;
	}
	if ( OldFormat == Format )
		return;
	if ( !IsValid() )
		throw Soy::AssertException("Pixels are not valid");

	auto& PixelsArray = GetPixelsArray();
	auto ConversionFuncs = GetRemoteArray( gConversionFuncs );
	auto* ConversionFunc = GetArrayBridge(ConversionFuncs).Find( std::make_tuple( OldFormat, Format ) );
	if ( ConversionFunc )
	{
		std::stringstream TimerName;
		TimerName << "SoyPixel::SetFormat( " << OldFormat << " to " << Format << " )";
		Soy::TScopeTimerPrint Timer( TimerName.str().c_str(), 5 );

		ConversionFunc->mFunction( PixelsArray, GetMeta(), Format );
		return;
	}
	

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::Greyscale )
	{
		ConvertFormat_KinectDepthToGreyscale( PixelsArray, GetMeta(), Format );
		return;
	}

	if ( GetFormat() == SoyPixelsFormat::KinectDepth && Format == SoyPixelsFormat::GreyscaleAlpha )
	{
		ConvertFormat_KinectDepthToGreyscale( PixelsArray, GetMeta(), Format );
		return;
	}

	//	see if we can use of simple channel-count change
	bool UseOfPixels = false;
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::RGBA );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::RGB );
	//UseOfPixels |= ( GetFormat() == SoyPixelsFormat::BGRA && Format == SoyPixelsFormat::BGR );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::Greyscale );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::Greyscale );
	
	//	report missing, but desired conversions
	std::stringstream Error;
	Error << "No soypixel conversion from " << OldFormat << " to " << Format;
	throw Soy::AssertException(Error.str());
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
bool TPixels::RgbaToHsla(SoyOpenClManager& OpenClManager,std::shared_ptr<SoyClDataBuffer>& Hsla) const
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
bool TPixels::RgbaToHsla(SoyOpenClManager& OpenClManager,std::shared_ptr<msa::OpenCLImage>& Hsla) const
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



void SoyPixelsImpl::Init(size_t Width, size_t Height, size_t Channels)
{
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	Init( Width, Height, Format );
}

void SoyPixelsImpl::Init(size_t Width, size_t Height,SoyPixelsFormat::Type Format)
{
	Init( SoyPixelsMeta( Width, Height, Format ) );
}


void SoyPixelsImpl::Init(const SoyPixelsMeta& Meta)
{
	auto Width = Meta.GetWidth();
	auto Height = Meta.GetHeight();
	auto Format = Meta.GetFormat();
	
	//	alloc
	GetMeta().DumbSetWidth( Width );
	GetMeta().DumbSetHeight( Height );
	GetMeta().DumbSetFormat( Format );
	auto Alloc = GetMeta().GetDataSize();
	auto& Pixels = GetPixelsArray();
	Pixels.SetSize( Alloc, false );
}


void SoyPixelsImpl::Clear(bool Dealloc)
{
	//	gr: leaving format as it might be useful as ghost meta
	//GetMeta().DumbSetFormat( SoyPixelsFormat::Invalid );
	GetMeta().DumbSetWidth( 0 );
	auto& Pixels = GetPixelsArray();
	Pixels.Clear( Dealloc );

	Soy::Assert( GetHeight() == 0, "Height should be 0 after clear");
	Soy::Assert( !IsValid(), "should be invalid after clear");
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
	
	//	gr: not sure how safe this is... vtables etc...
	auto& CastArray = *reinterpret_cast<ArrayInterface<uint8>*>( &RawData );
	if ( !CastArray.PushBackArray( Pixels ) )
	//if ( !RawData.PushBackArray( Pixels ) )
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
	const size_t PixelDataSize = RawData.GetDataSize()-HeaderSize;
	auto* RawData8 = reinterpret_cast<const uint8*>(RawData.GetArray());
	auto Pixels = GetRemoteArray( RawData8+HeaderSize, PixelDataSize );
	
	//	todo: verify size! (alignment!)
	GetMeta() = Header;
	GetPixelsArray().Clear(false);
	GetPixelsArray().PushBackArray( Pixels );

	if ( !IsValid() )
		return false;

	return true;
}




size_t SoyPixelsImpl::GetIndex(size_t x,size_t y,size_t ChannelOffset) const
{
	auto w = GetWidth();
	auto h = GetHeight();
	auto ChannelCount = GetChannels();
	if ( x >= w || y >= h || ChannelOffset >= ChannelCount )
	{
		std::stringstream Error;
		Error << "Pixel OOB x=" << x << '/' << w << " y=" << y << '/' << h << " ch=" << ChannelOffset << '/' << ChannelCount;
		throw Soy::AssertException( Error.str() );
	}
	
	auto Index = y * (w*ChannelCount);
	Index += x * ChannelCount;
	Index += ChannelOffset;
	return Index;
}

uint8& SoyPixelsImpl::GetPixelPtr(size_t x,size_t y,size_t ChannelOffset)
{
	auto Index = GetIndex( x, y, ChannelOffset );
	auto& Pixels = GetPixelsArray();
	auto* PixelsPtr = Pixels.GetArray();
	return PixelsPtr[Index];
}

const uint8& SoyPixelsImpl::GetPixelPtr(size_t x,size_t y,size_t ChannelOffset) const
{
	auto Index = GetIndex( x, y, ChannelOffset );
	auto& Pixels = GetPixelsArray();
	auto* PixelsPtr = Pixels.GetArray();
	return PixelsPtr[Index];
}

uint8 SoyPixelsImpl::GetPixel(size_t x,size_t y,size_t Channel) const
{
	return GetPixelPtr( x, y, Channel );
}

vec2x<uint8> SoyPixelsImpl::GetPixel2(size_t x,size_t y) const
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 2, "Accessing channel OOB");
	auto* Pixel = &GetPixelPtr( x, y, 0 );
	return vec2x<uint8>( Pixel[0], Pixel[1] );
}

vec3x<uint8> SoyPixelsImpl::GetPixel3(size_t x,size_t y) const
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 3, "Accessing channel OOB");
	auto* Pixel = &GetPixelPtr( x, y, 0 );
	return vec3x<uint8>( Pixel[0], Pixel[1], Pixel[2] );
}

vec4x<uint8> SoyPixelsImpl::GetPixel4(size_t x,size_t y) const
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 4, "Accessing channel OOB");
	auto* Pixel = &GetPixelPtr( x, y, 0 );
	return vec4x<uint8>( Pixel[0], Pixel[1], Pixel[2], Pixel[3] );
}

void SoyPixelsImpl::SetPixel(size_t x,size_t y,size_t Channel,uint8 Component)
{
	auto& Pixel = GetPixelPtr( x, y, Channel );
	Pixel = Component;
}

void SoyPixelsImpl::SetPixel(size_t x,size_t y,const vec2x<uint8>& Colour)
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 2, "Accessing channel OOB");

	auto* Pixel = &GetPixelPtr( x, y, 0 );
	Pixel[0] = Colour.x;
	Pixel[1] = Colour.y;
}

void SoyPixelsImpl::SetPixel(size_t x,size_t y,const vec3x<uint8>& Colour)
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 3, "Accessing channel OOB");
	
	auto* Pixel = &GetPixelPtr( x, y, 0 );
	Pixel[0] = Colour.x;
	Pixel[1] = Colour.y;
	Pixel[2] = Colour.z;
}

void SoyPixelsImpl::SetPixel(size_t x,size_t y,const vec4x<uint8>& Colour)
{
	auto ChannelCount = GetChannels();
	Soy::Assert( ChannelCount >= 4, "Accessing channel OOB");

	auto* Pixel = &GetPixelPtr( x, y, 0 );
	Pixel[0] = Colour.x;
	Pixel[1] = Colour.y;
	Pixel[2] = Colour.z;
	Pixel[3] = Colour.w;
}




void SoyPixelsImpl::Clip(size_t Left,size_t Top,size_t Width,size_t Height)
{
	if ( Width == 0 || Height == 0 )
		throw Soy::AssertException("Cannot size image to 0 width or height");
	
	auto& Pixels = GetPixelsArray();
	auto Channels = GetChannels();
	//	we'll get stuck in loops if stride is zero
	Channels = std::max<uint8_t>( Channels, 1 );
	
	//	remove top rows
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::RemoveTop",5);
		auto RowBytes = Channels * GetWidth();
		auto TotalRowBytes = RowBytes * Top;
		Pixels.RemoveBlock( 0, TotalRowBytes );
		GetMeta().DumbSetHeight( GetHeight()-Top );
	}
	
	//	remove bottom rows (before removing columns, ResizeClip just won't do anything)
	if ( Height < GetHeight() )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::RemoveRows (early)",5);
		auto RowBytes = Channels * GetWidth();
		RowBytes *= GetHeight() - Height;
		Pixels.SetSize( Pixels.GetDataSize() - RowBytes );
		GetMeta().DumbSetHeight( Height );
	}

	//	gr: faster to write new rows if we're clipping
	if ( Left > 0 || Width < GetWidth() )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::Realign rows",5);
		auto OldWidth = GetWidth();
		auto NewWidth = Width;
		
		for ( auto y=0;	y<GetHeight();	y++ )
		{
			auto OldX = Left;
			auto NewX = 0;
			auto OldIndex = (OldX + (y * OldWidth) ) * Channels;
			auto NewIndex = (NewX + (y * NewWidth) ) * Channels;
			auto CopyBytes = Width * Channels;
			Pixels.MoveBlock( OldIndex, NewIndex, CopyBytes );
		}
		auto NewSize = Channels * Width * Height;
		Pixels.SetSize( NewSize );
		
		//	don't do it again
		Left = 0;
		GetMeta().DumbSetWidth( Width );
	}
	
	//	remove start of rows
	if ( Left > 0 )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::RemoveLeft",5);
		//	working backwards makes it easy & fast
		auto Stride = Channels * GetWidth();
		auto RemoveBytes = Channels * Left;
		for ( ssize_t p=Pixels.GetDataSize()-Stride;	p>=0;	p-=Stride )
			Pixels.RemoveBlock( p, RemoveBytes );
		GetMeta().DumbSetWidth( GetWidth() - Left );
	}
	
	ResizeClip( Width, Height );
}

void SoyPixelsImpl::ResizeClip(size_t Width,size_t Height)
{
	if ( Width == 0 || Height == 0 )
		throw Soy::AssertException("Cannot size image to 0 width or height");

	auto& Pixels = GetPixelsArray();
	auto Channels = GetChannels();
	//	we'll get stuck in loops if stride is zero
	Channels = std::max<uint8_t>( Channels, 1 );
	
	//	simply add/remove rows
	if ( Height > GetHeight() )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::AddRows",5);
		auto RowBytes = Channels * GetWidth();
		RowBytes *= Height - GetHeight();
		Pixels.PushBlock( RowBytes );
		GetMeta().DumbSetHeight( Height );
	}
	else if ( Height < GetHeight() )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::RemoveRows",5);
		auto RowBytes = Channels * GetWidth();
		RowBytes *= GetHeight() - Height;
		Pixels.SetSize( Pixels.GetDataSize() - RowBytes );
		GetMeta().DumbSetHeight( Height );
	}
	
	//	insert/remove data on the end of each row
	if ( Width > GetWidth() )
	{//	todo: prealloc!
		//	working backwards makes it easy & fast
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::AddColumns",5);
		auto Stride = Channels * GetWidth();
		Stride = std::max<size_t>( Stride, Channels*1 );
		
		auto ColBytes = Channels * (Width - GetWidth());
		for ( ssize_t p=Pixels.GetDataSize();	p>=0;	p-=Stride )
			Pixels.InsertBlock( p, ColBytes );
		GetMeta().DumbSetWidth( Width );
	}
	else if ( Width < GetWidth() )
	{
		Soy::TScopeTimerPrint Timer("SoyPixels::Clip::RemoveColumns",5);
		//	working backwards makes it easy & fast
		auto Stride = Channels * GetWidth();
		auto ColBytes = Channels * (GetWidth() - Width);
		for ( ssize_t p=Pixels.GetDataSize()-ColBytes;	p>=0;	p-=Stride )
			Pixels.RemoveBlock( p, ColBytes );
		GetMeta().DumbSetWidth( Width );
	}
	
	if ( Height != GetHeight() || Width != GetWidth() )
	{
		std::stringstream Error;
		Error << "Wrong height(" << Height << " not " << GetHeight() << ") width (" << Width << " not " << GetWidth() << ") after " << __func__;
		throw Soy::AssertException( Error.str() );
	}
}

void SoyPixelsMeta::SplitPlanes(size_t PixelDataSize,ArrayBridge<std::tuple<size_t,size_t,SoyPixelsMeta>>&& PlaneOffsetSizeAndMetas,const ArrayInterface<uint8>* Data) const
{
	//	get the mid-formats
	auto& ThisMeta = *this;
	BufferArray<SoyPixelsMeta,10> Formats;
	ThisMeta.GetPlanes( GetArrayBridge(Formats), Data );

	//	build error as we go in case we assert mid-way
	std::stringstream Error;
	Error << "Split pixel planes (" << ThisMeta << " x" << PixelDataSize << " bytes -> " << Soy::StringJoin( GetArrayBridge(Formats), "," ) << ") but data hasn't aligned after split; ";

	size_t DataOffset = SoyPixelsFormat::GetHeaderSize( ThisMeta.GetFormat() );
	for ( int p=0;	p<Formats.GetSize();	p++ )
	{
		auto PlaneMeta = Formats[p];
		auto PlaneDataOffset = DataOffset;
		auto PlaneDataSize = PlaneMeta.GetDataSize();

		auto PlaneOffsetSizeAndMeta = std::make_tuple( PlaneDataOffset, PlaneDataSize, PlaneMeta );
		DataOffset += PlaneDataSize;

		Error << "#" << p << "/" << Formats.GetSize() << " " << PlaneMeta << " = " << PlaneDataSize << " bytes; ";
		//	check for overflow
		if (DataOffset > PixelDataSize )
			throw Soy_AssertException( Error );

		PlaneOffsetSizeAndMetas.PushBack(PlaneOffsetSizeAndMeta);
	}

	//	error, but don't fail if underrun. overrun should be caught in the loop
	Error << "total " << DataOffset << " out of " << PixelDataSize << " bytes";
	static bool ThrowOnUnderflow = false;
	static bool ThrowOnOverflow = true;
	if ( DataOffset < PixelDataSize && ThrowOnUnderflow )
	{
		throw Soy::AssertException( Error.str() );
	}
	else if ( DataOffset > PixelDataSize && ThrowOnOverflow )
	{
		throw Soy::AssertException( Error.str() );
	}
	else if ( DataOffset != PixelDataSize )
	{
		std::Debug << Error.str() << std::endl;
	}
}

void SoyPixelsImpl::SplitPlanes(ArrayBridge<std::shared_ptr<SoyPixelsImpl>>&& Planes) const
{
	//	get the plane layouts
	auto& ThisMeta = GetMeta();
	auto& ThisArray = GetPixelsArray();
	auto PixelDataSize = ThisArray.GetDataSize();
	BufferArray<std::tuple<size_t,size_t,SoyPixelsMeta>,10> PlaneOffsetSizeAndMetas;
	ThisMeta.SplitPlanes( PixelDataSize, GetArrayBridge(PlaneOffsetSizeAndMetas), &ThisArray );
	
	//	make up the remote pixels
	for ( int p=0;	p<PlaneOffsetSizeAndMetas.GetSize();	p++ )
	{
		auto& PlaneOffsetSizeAndMeta = PlaneOffsetSizeAndMetas[p];
		auto PlaneDataOffset = std::get<0>( PlaneOffsetSizeAndMeta );
		auto PlaneDataSize = std::get<1>( PlaneOffsetSizeAndMeta );
		auto PlaneMeta = std::get<2>( PlaneOffsetSizeAndMeta );
		auto* PlaneData = ThisArray.GetArray() + PlaneDataOffset;

		//	lets allow const use of this func. But SoyPixelsRemote may need a non-mutable version?
		auto* PlaneDataMutable = const_cast<uint8_t*>(PlaneData);
		std::shared_ptr<SoyPixelsRemote> Pixels(new SoyPixelsRemote( PlaneDataMutable, PlaneDataSize, PlaneMeta ) );
		Planes.PushBack(Pixels);
	}
}


template<size_t COMPONENTS>
void SetPixelComponents(ArrayInterface<uint8>& Pixels,const ArrayBridge<uint8>& Components)
{
	BufferArray<uint8,COMPONENTS> BufferComponents;
	for ( int i=0;	i<COMPONENTS;	i++ )
	{
		if ( i < Components.GetSize() )
			BufferComponents.PushBack( Components[i] );
		else
			BufferComponents.PushBack( 255 );
	}
	
	for ( int p=0;	p<Pixels.GetSize();	p++ )
	{
		Pixels[p] = BufferComponents[p%COMPONENTS];
	}
	
}

void SoyPixelsImpl::SetPixels(const ArrayBridge<uint8>& Components)
{
	if ( !IsValid() )
		return;
	
	auto& Pixels = GetPixelsArray();
	
	switch ( GetChannels() )
	{
		case 1:	SetPixelComponents<1>( Pixels, Components );	return;
		case 2:	SetPixelComponents<2>( Pixels, Components );	return;
		case 3:	SetPixelComponents<3>( Pixels, Components );	return;
		case 4:	SetPixelComponents<4>( Pixels, Components );	return;
		case 5:	SetPixelComponents<5>( Pixels, Components );	return;
	};
}

vec2f SoyPixelsImpl::GetUv(size_t PixelIndex) const
{
	auto xy = GetXy( PixelIndex );
	float u = xy.x / static_cast<float>( GetWidth() );
	float v = xy.y / static_cast<float>( GetHeight() );
	return vec2f( u, v );
}

vec2x<size_t> SoyPixelsImpl::GetXy(size_t PixelIndex) const
{
	auto x = PixelIndex % GetWidth();
	auto y = PixelIndex / GetWidth();
	return vec2x<size_t>( x, y );
}

void SoyPixelsImpl::ResizeFastSample(size_t NewWidth, size_t NewHeight)
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
	
	auto& OldPixelsArray = Old.GetPixelsArray();
	auto& NewPixelsArray = New.GetPixelsArray();
	
	auto* OldPixels = OldPixelsArray.GetArray();
	auto* NewPixels = NewPixelsArray.GetArray();

	for ( int ny=0;	ny<NewHeight;	ny++ )
	{
		float yf = ny/static_cast<float>(NewHeight);
		int oy = static_cast<int>(OldHeight * yf);

		auto OldLineSize = OldWidth * OldChannelCount;
		auto NewLineSize = NewWidth * NewChannelCount;
//#define SAFE_RESIZE
#if defined(SAFE_RESIZE)
		auto OldRow = GetRemoteArray( &OldPixels[oy*OldLineSize], OldLineSize );
		auto NewRow = GetRemoteArray( &NewPixels[ny*NewLineSize], NewLineSize );
#else
		auto OldRow = &OldPixels[oy*OldLineSize];
		auto NewRow = &NewPixels[ny*NewLineSize];
#endif
		for ( int nx=0;	nx<NewWidth;	nx++ )
		{
			float xf = nx / static_cast<float>(NewWidth);
			int ox = static_cast<int>(OldWidth * xf);
			auto* OldPixel = &OldRow[ox*OldChannelCount];
			auto* NewPixel = &NewRow[nx*NewChannelCount];

			memcpy( NewPixel, OldPixel, MinChannelCount );
		}
	}
	
	assert( NewHeight == GetHeight() );
	assert( NewWidth == GetWidth() );
}


void SoyPixelsImpl::Flip()
{
	if ( !IsValid() )
		return;
	
	Soy::TScopeTimerPrint Timer("SoyPixelsImpl::Flip", 5);
	
	//	buffer a line so we don't need to realloc for the temp line
	auto LineSize = GetWidth() * GetChannels();
	Array<char> TempLine( size_cast<size_t>(LineSize) );
	auto* Pixels = GetPixelsArray().GetArray();
	
	auto Height = GetHeight();
	for ( int y=0;	y<Height/2;	y++ )
	{
		//	swap lines
		int TopY = y;
		ssize_t BottomY = (Height-1) - y;

		auto* TopRow = &Pixels[TopY * LineSize];
		auto* BottomRow = &Pixels[BottomY  * LineSize];
		memcpy( TempLine.GetArray(), TopRow, LineSize );
		memcpy( TopRow, BottomRow, LineSize );
		memcpy( BottomRow, TempLine.GetArray(), LineSize );
	}
}



void SoyPixelsImpl::Copy(const SoyPixelsImpl& That,const TSoyPixelsCopyParams& Params)
{
	//	same data
	if ( &That == this )
		return;
	auto& This = *this;
	if ( That.GetPixelsArray().GetArray() == This.GetPixelsArray().GetArray() )
		return;

	bool Flip = Params.mFlipDestination||Params.mFlipSource;

	//	simple copy if we can realloc
	if ( Params.mAllowRealloc )
	{
		if ( Flip )
			std::Debug << "SoyPixelsImpl::Copy realloc copy, flip ignored" << std::endl;

		This.GetMeta() = That.GetMeta();
		This.GetPixelsArray().Copy( That.GetPixelsArray() );
		return;
	}

	auto ThisWidth = This.GetMeta().GetWidth();
	auto ThatWidth = That.GetMeta().GetWidth();
	auto ThisHeight = This.GetMeta().GetHeight();
	auto ThatHeight = That.GetMeta().GetHeight();
	auto ThisChannels = This.GetMeta().GetChannels();
	auto ThatChannels = That.GetMeta().GetChannels();

	//	not allowed to realloc, so require components to match
	if ( That.GetMeta().GetFormat() != This.GetMeta().GetFormat() )
	{
		if ( !Params.mAllowComponentSwizzle )
		{
			std::stringstream Error;
			Error << "Cannot copy " << That.GetMeta() << " into " << This.GetMeta() << " because !AllowComponentSwizzle";
			throw Soy::AssertException( Error.str() );
		}

		//	we can swizzle, but require components to match
		if ( ThisChannels != ThatChannels )
		{
			std::stringstream Error;
			Error << "Cannot copy " << That.GetMeta() << " into " << This.GetMeta() << " with swizzle because channel counts don't match";
			throw Soy::AssertException( Error.str() );
		}
	}

	//	global rejection for height difference
	if ( ThisHeight != ThatHeight )
	{
		if ( !Params.mAllowHeightClip )
		{
			std::stringstream Error;
			Error << "Cannot copy " << That.GetMeta() << " into " << This.GetMeta() << ", heights don't align";
			throw Soy::AssertException( Error.str() );
		}
	}

	if ( ThisWidth == ThatWidth && !Flip )
	{
		auto* This00 = &This.GetPixelPtr( 0, 0, 0 );
		auto* That00 = &That.GetPixelPtr( 0, 0, 0 );

		auto Stride = ThisChannels * ThisWidth;
		auto Height = std::min( ThisHeight, ThatHeight );
		auto CopyLength = Stride * Height;

		//std::Debug << __func__ << " full copy stride=" << Stride << " Height=" << Height << " " << CopyLength << std::endl;

		memcpy( This00, That00, CopyLength );
	}
	else
	{
		//	slow path where widths doesn't align
		if ( !Params.mAllowWidthClip )
		{
			std::stringstream Error;
			Error << "Cannot copy " << That.GetMeta() << " into " << This.GetMeta() << " widths don't align";
			throw Soy::AssertException( Error.str() );
		}

		//	copy row by row
		//auto CopyWidth = std::min( ThisWidth, ThatWidth );
		auto CopyHeight = std::min( ThisHeight, ThatHeight );
		
		auto ThisStride = ThisChannels * ThisWidth;
		auto ThatStride = ThatChannels * ThatWidth;
		auto CopyStride = std::min( ThisStride, ThatStride );
		auto* This00 = &This.GetPixelPtr( 0, 0, 0 );
		auto* That00 = &That.GetPixelPtr( 0, 0, 0 );
		for ( int y=0;	y<CopyHeight;	y++ )
		{
			auto ThisY = Params.mFlipDestination ? (ThisHeight-1-y) : y;
			auto ThatY = Params.mFlipSource ? (ThatHeight-1-y) : y;
			//std::Debug << __func__ << " scanlinecopy row y=" << y << " ThatStride=" << ThatStride << " ThisStride=" << ThisStride << " CopyStride=" << CopyStride << std::endl;

			auto* Src = &That00[ThatStride * ThatY];
			auto* Dst = &This00[ThisStride * ThisY];
			memcpy( Dst, Src, CopyStride );
		}
	}
}


void SoyPixelsImpl::PrintPixels(const std::string& Prefix,std::ostream& Stream,bool Hex,const char* PixelSuffix) const
{
	auto Meta = GetMeta();
	Stream << Prefix << " " << Meta << std::endl;
	auto ComponentCount = GetChannels();
	auto Stride = ComponentCount * Meta.GetWidth();
	auto* Pixels = GetPixelsArray().GetArray();
	
	if ( Hex )
		Stream << std::hex;
	for ( int p=0;	p<Meta.GetDataSize();	p++ )
	{
		int PixelValue = (int)Pixels[p];
		Stream << PixelValue;
		if ( PixelSuffix )
			Stream << PixelSuffix;
		
		if ( p % Stride == 0 )
			Stream << std::endl;
	}
	Stream << std::dec;
	Stream << std::endl;
}


size_t SoyPixelsMeta::GetDataSize() const
{
	BufferArray<SoyPixelsMeta,3> Planes;
	GetPlanes( GetArrayBridge(Planes) );
	size_t TotalDataSize = 0;
	for ( int p=0;	p<Planes.GetSize();	p++ )
	{
		//	gr: should be recursive really... but I don't think we ever want that case
		TotalDataSize += Planes[p].GetSelfDataSize();
	}
	return TotalDataSize;
}

void SoyPixelsMeta::GetPlanes(ArrayBridge<SoyPixelsMeta>&& Planes,const ArrayInterface<uint8>* Data) const
{
	switch ( GetFormat() )
	{
		case SoyPixelsFormat::Yuv_8_88_Full:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Full ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaUV_88 ) );
			break;
			
		case SoyPixelsFormat::Yuv_8_88_Ntsc:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Ntsc ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaUV_88 ) );
			break;
			
		case SoyPixelsFormat::Yuv_8_88_Smptec:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Smptec ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaUV_88 ) );
			break;
			
		case SoyPixelsFormat::Yuv_8_8_8_Full:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Full ) );
			//	each plane is half width, half height, but next to each other, so double height, and 8 bits per pixel
			//Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight(), SoyPixelsFormat::ChromaUV_8_8 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaU_8 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaV_8 ) );
			break;
			
		case SoyPixelsFormat::Yuv_8_8_8_Ntsc:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Ntsc ) );
			//	each plane is half width, half height, but next to each other, so double height, and 8 bits per pixel
			Planes.PushBack(SoyPixelsMeta(GetWidth() / 2, GetHeight() / 2, SoyPixelsFormat::ChromaU_8));
			Planes.PushBack(SoyPixelsMeta(GetWidth() / 2, GetHeight() / 2, SoyPixelsFormat::ChromaV_8));
			break;
			
		case SoyPixelsFormat::Yuv_8_8_8_Smptec:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Smptec ) );
			//	each plane is half width, half height, but next to each other, so double height, and 8 bits per pixel
			Planes.PushBack(SoyPixelsMeta(GetWidth() / 2, GetHeight() / 2, SoyPixelsFormat::ChromaU_8));
			Planes.PushBack(SoyPixelsMeta(GetWidth() / 2, GetHeight() / 2, SoyPixelsFormat::ChromaV_8));
			break;
			
		case SoyPixelsFormat::ChromaUV_8_8:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaU_8 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaV_8 ) );
			break;

		//	gr: these are interlaced, so don't split
		/*
		//	need to handle these horizontally interlaced formats better
		case SoyPixelsFormat::Uvy_844_Full:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaUV_44 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Full ) );
			break;

		case SoyPixelsFormat::Yuv_844_Full:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Full ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaUV_44 ) );
			break;
			
		case SoyPixelsFormat::Yuv_844_Ntsc:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Ntsc ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaUV_44 ) );
			break;
			
		case SoyPixelsFormat::Yuv_844_Smptec:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma_Smptec ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaUV_44 ) );
			break;
		*/
		case SoyPixelsFormat::Palettised_RGB_8:
		{
			Soy::Assert( Data!=nullptr, "Cannot split format of Palettised_8_8 without data");
			size_t PaletteSize = 0;
			size_t TransparentIndex = 0;
			SoyPixelsFormat::GetHeaderPalettised( GetArrayBridge(*Data), PaletteSize, TransparentIndex );
			//	original meta is the index w/h.
			//	header of palette is the length of the palette
			Planes.PushBack( SoyPixelsMeta( PaletteSize, 1, SoyPixelsFormat::RGB ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Greyscale ) );
			break;
		}

		case SoyPixelsFormat::Palettised_RGBA_8:
		{
			Soy::Assert( Data!=nullptr, "Cannot split format of Palettised_8_8 without data");
			size_t PaletteSize = 0;
			size_t TransparentIndex = 0;
			SoyPixelsFormat::GetHeaderPalettised( GetArrayBridge(*Data), PaletteSize, TransparentIndex );
			//	original meta is the index w/h.
			//	header of palette is the length of the palette
			Planes.PushBack( SoyPixelsMeta( PaletteSize, 1, SoyPixelsFormat::RGBA ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Greyscale ) );
			break;
		}

		case SoyPixelsFormat::Invalid:
			//	invalid format now returns NO planes instead of including self.
			return;
			
		default:
			Planes.PushBack( *this );
			break;
	};
}

void SoyPixelsRemote::CheckDataSize()
{
	auto DataSize = this->mArray.GetDataSize();
	auto ExpectedSize = this->GetMeta().GetDataSize();
	if (DataSize == ExpectedSize)
		return;

	std::stringstream Error;
	Error << "SoyPixelsRemote meta size(" << ExpectedSize << ") different to data size (" << DataSize << ")";
	throw Soy::AssertException(Error);
}
