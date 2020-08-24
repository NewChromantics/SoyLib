#include "SoyPixels.h"
#include "SoyDebug.h"
#include "SoyTime.h"
#include <functional>
#include "RemoteArray.h"
//#include "SoyMath.h"
//#include "SoyStream.h"
#include "SoyImage.h"
#include "magic_enum/include/magic_enum.hpp"



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



size_t SoyPixelsFormat::GetChannelCount(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
	case Greyscale:		return 1;
	case GreyscaleAlpha:	return 2;
	case RGB:			return 3;
	case BGR:			return 3;
	case RGBA:			return 4;
	case BGRA:			return 4;
	case ARGB:			return 4;
	
		//	16 bit, but 1 channel
	case KinectDepth:	return 1;
	case FreenectDepth11bit:	return 1;
	case FreenectDepth10bit:	return 1;	
		case Depth16mm:		return 1;
		
		case DepthFloatMetres:		return 1;
		case DepthHalfMetres:		return 1;
		case DepthDisparityFloat:		return 1;
		case DepthDisparityHalf:		return 1;

	case ChromaUV_8_8:	return 1;
	case ChromaUV_88:	return 2;
	case ChromaU_8:		return 1;
	case ChromaV_8:		return 1;

		//	treating these as all bytes at the moment
	case Uvy_8_88:
	case Yuv_8_88:
	case Yuv_8_8_8:
		return 1;

	case YYuv_8888:
	case uyvy_8888:
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

		default:
			return false;
	}
}


uint8_t SoyPixelsFormat::GetBytesPerChannel(SoyPixelsFormat::Type Format)
{
	switch (Format)
	{
	case Float1:
	case Float2:
	case Float3:
	case Float4:
	case DepthFloatMetres:
	case DepthDisparityFloat:
		return sizeof(float);

	//	maybe need to make a c++ type for this
	case DepthHalfMetres:
	case DepthDisparityHalf:
		return sizeof(uint16_t);

	case KinectDepth:
	case FreenectDepth11bit:
	case FreenectDepth10bit:
	case Depth16mm:
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
	const std::map<Type, BufferArray<Type,2>>&	GetMergedFormatMap2();
	const std::map<Type,BufferArray<Type,3>>&	GetMergedFormatMap3();
}

const std::map<SoyPixelsFormat::Type,BufferArray<SoyPixelsFormat::Type,2>>& SoyPixelsFormat::GetMergedFormatMap2()
{
	static std::map<Type,BufferArray<Type,2>> Map;

	if ( Map.empty() )
	{
		Map[Yuv_8_88].PushBackArray( { Luma, ChromaUV_88 } );
		Map[Yuv_8_8_8].PushBackArray( { Luma, ChromaUV_8_8 } );
		Map[ChromaUV_8_8].PushBackArray( { ChromaU_8, ChromaV_8 } );
	}

	return Map;
}

const std::map<SoyPixelsFormat::Type, BufferArray<SoyPixelsFormat::Type, 3>>& SoyPixelsFormat::GetMergedFormatMap3()
{
	static std::map<Type, BufferArray<Type, 3>> Map;

	if (Map.empty())
	{
		Map[Yuv_8_8_8].PushBackArray({ Luma, ChromaU_8,ChromaV_8 });
	}

	return Map;
}


void SoyPixelsFormat::GetFormatPlanes(Type Format,ArrayBridge<Type>&& PlaneFormats)
{
	//	prioritise 3 planes over 2 (this is mostly for yuv_8_8_8 as avf needs this to match 3 planes)
	{
		auto& Map3 = GetMergedFormatMap3();
		auto it3 = Map3.find(Format);
		
		if (it3 != Map3.end())
		{
			PlaneFormats.Copy(it3->second);
			return;
		}
	}

	{
		auto& Map2 = GetMergedFormatMap2();
		auto it2 = Map2.find(Format);
		if (it2 != Map2.end())
		{
			PlaneFormats.Copy(it2->second);
			return;
		}
	}

	PlaneFormats.PushBack(Format);
	return;
}



SoyPixelsFormat::Type SoyPixelsFormat::GetMergedFormat(SoyPixelsFormat::Type Formata,SoyPixelsFormat::Type Formatb)
{
	BufferArray<Type,2> Formatab( { Formata, Formatb } );

	auto& Map = GetMergedFormatMap2();
	for ( auto it=Map.begin();	it!=Map.end();	it++ )
	{
		auto& Matchab = it->second;
		auto& MergedFormat = it->first;
		if ( Matchab == Formatab )
			return MergedFormat;
	}

	std::stringstream Error;
	Error << "No merged format for " << Formata << " + " << Formatb;
	throw Soy::AssertException(Error);
}

SoyPixelsFormat::Type SoyPixelsFormat::GetMergedFormat(SoyPixelsFormat::Type Formata, SoyPixelsFormat::Type Formatb, SoyPixelsFormat::Type Formatc)
{
	BufferArray<Type, 3> Formatabc({ Formata, Formatb, Formatc });

	auto& Map = GetMergedFormatMap3();
	for (auto it = Map.begin(); it != Map.end(); it++)
	{
		auto& Matchab = it->second;
		auto& MergedFormat = it->first;
		if (Matchab == Formatabc)
			return MergedFormat;
	}

	std::stringstream Error;
	Error << "No merged format for " << Formata << " + " << Formatb << " + " << Formatc;
	throw Soy::AssertException(Error);
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



void ConvertFormat_Uvy844_To_Yuv_8_8_8(ArrayInterface<uint8_t>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	copy old one
	Array<uint8_t> UvyArray;
	UvyArray.Copy( PixelsArray );
	
	
	//	we just need to ADD another pair of planes, so luma plane stays the same
	SoyPixelsMeta YuvMeta(Meta.GetWidth(), Meta.GetHeight(), NewFormat);
	
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
		{
			auto pi = i*2;
			auto l = UvyArray[pi+1];
			LumaArray[i] = l;
		}
		
		//	testing writing chroma u/v
		if ( false )
		{
			//	gr: this is correct
			auto pi = (i/2)*4;
			uint8_t u = UvyArray[pi+2];
			LumaArray[i] = u;
		}
	}
	
	//	write chromas
	auto& ChromaU = *Planes[1];
	auto& ChromaUArray_Array = ChromaU.GetPixelsArray();
	auto* ChromaUArray = ChromaU.GetPixelsArray().GetArray();
	auto& ChromaV = *Planes[2];
	auto* ChromaVArray = ChromaV.GetPixelsArray().GetArray();
	for (auto i = 0; i < ChromaUArray_Array.GetDataSize(); i++)
	{
		//	this output is wrong (maybe uvy2 is half width, not half height)
		//	luma u luma v
		auto pi = i*4;
		uint8_t u = UvyArray[pi+0];
		uint8_t v = UvyArray[pi+2];
		//u = 128;
		//v = 128;
		ChromaUArray[i] = u;
		ChromaVArray[i] = v;
	}
	
	Meta = YuvMeta;
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


void ConvertFormat_Yuv_3Plane_To_2Plane(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	split the planes and then write the new data to them
	SoyPixelsRemote Yuv3_Pixels(PixelsArray.GetArray(), Meta.GetWidth(), Meta.GetHeight(), Meta.GetDataSize(), Meta.GetFormat());
	SoyPixelsRemote Yuv2_Pixels(PixelsArray.GetArray(), Meta.GetWidth(), Meta.GetHeight(), Meta.GetDataSize(), NewFormat);
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> OldPlanes;
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> NewPlanes;
	Yuv3_Pixels.SplitPlanes(GetArrayBridge(OldPlanes));
	Yuv2_Pixels.SplitPlanes(GetArrayBridge(NewPlanes));
	
	//	luma already in correct size & place

	//	chroma [u][v] turning into chroma[uv]
	auto ChromaPixelCount = OldPlanes[1]->GetWidth() * OldPlanes[1]->GetHeight();
	SoyPixels ChromaUCopy(*OldPlanes[1]);
	SoyPixels ChromaVCopy(*OldPlanes[2]);
	auto* ChromaUV = &NewPlanes[1]->GetPixelPtr(0,0,0);
	auto* ChromaU = &ChromaUCopy.GetPixelPtr(0,0,0);
	auto* ChromaV = &ChromaVCopy.GetPixelPtr(0,0,0);

	for ( int i=0;	i<ChromaPixelCount;	i++ )
	{
		auto u = ChromaU[i];
		auto v = ChromaV[i];
		auto uvindex = i*2;
		ChromaUV[uvindex+0] = u;
		ChromaUV[uvindex+1] = v;
	}
	
	Meta.DumbSetFormat(NewFormat);
}


void ConvertFormat_Yuv_2Plane_To_3Plane(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	split the planes and then write the new data to them
	SoyPixelsRemote Yuv2_Pixels(PixelsArray.GetArray(), Meta.GetWidth(), Meta.GetHeight(), Meta.GetDataSize(), Meta.GetFormat());
	SoyPixelsRemote Yuv3_Pixels(PixelsArray.GetArray(), Meta.GetWidth(), Meta.GetHeight(), Meta.GetDataSize(), NewFormat);
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> OldPlanes;
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> NewPlanes;
	Yuv2_Pixels.SplitPlanes(GetArrayBridge(OldPlanes));
	Yuv3_Pixels.SplitPlanes(GetArrayBridge(NewPlanes));
	
	//	luma already in correct size & place
	
	//	chroma [uv] turning into chroma[u][v]
	auto ChromaPixelCount = OldPlanes[1]->GetWidth() * OldPlanes[1]->GetHeight();
	SoyPixels ChromaUvCopy(*OldPlanes[1]);
	auto* ChromaUV = &ChromaUvCopy.GetPixelPtr(0,0,0);
	auto* ChromaU = &NewPlanes[1]->GetPixelPtr(0,0,0);
	auto* ChromaV = &NewPlanes[2]->GetPixelPtr(0,0,0);
	
	//	if we work backwards we can overwrite ourselves without losing data
	for ( int i=0;	i<ChromaPixelCount;	i++ )
	{
		auto uvindex = i*2;
		auto u = ChromaUV[uvindex+0];
		auto v = ChromaUV[uvindex+1];
		ChromaU[i] = u;
		ChromaV[i] = v;
	}
	
	Meta.DumbSetFormat(NewFormat);
}

void ConvertFormat_YuvPlane_To_Greyscale(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	crop to first plane
	SoyPixelsMeta NewMeta(Meta.GetWidth(), Meta.GetHeight(), NewFormat);
	PixelsArray.SetSize(NewMeta.GetDataSize());
	Meta = NewMeta;
}

void ConvertFormat_Greyscale_To_Yuv_8_88(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	//	we just need to ADD another pair of planes, so luma plane stays the same
	SoyPixelsMeta YuvMeta(Meta.GetWidth(), Meta.GetHeight(), NewFormat);

	//	split the planes and then write the new data to them
	PixelsArray.SetSize(YuvMeta.GetDataSize());
	SoyPixelsRemote YuvPixels(PixelsArray.GetArray(), YuvMeta.GetWidth(), YuvMeta.GetHeight(), YuvMeta.GetDataSize(), YuvMeta.GetFormat());
	BufferArray<std::shared_ptr<SoyPixelsImpl>, 3> Planes;
	YuvPixels.SplitPlanes(GetArrayBridge(Planes));

	//	luma already in correct size & place

	//	write chromas... all grey, so all 128
	auto& ChromaUV = *Planes[1];
	auto& ChromaUVArray_Array = ChromaUV.GetPixelsArray();
	auto* ChromaUVArray = ChromaUVArray_Array.GetArray();
	for (auto i = 0; i < ChromaUVArray_Array.GetDataSize(); i+=2)
	{
		uint8_t u = 128;
		uint8_t v = 128;
		ChromaUVArray[i+0] = u;
		ChromaUVArray[i+1] = v;
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
		auto x = i % ChromaU.GetWidth();
		auto y = i / ChromaU.GetWidth();
		auto rgbi = (x * 2) + (y * 2 * Luma.GetWidth());
		rgbi *= RgbStride;
		auto r = RgbPixels[rgbi + 0];
		auto g = RgbPixels[rgbi + 1];
		auto b = RgbPixels[rgbi + 2];

		//	convert to chroma uv
		uint8_t u = g;
		uint8_t v = b;
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
	//	gr: tiny optimisation (save ~2ms in debug)
	uint8_t* OldPos = &Pixels[0];
	uint8_t* NewPos = &Pixels[0];
	for ( int p=0;	p<PixelCount;	p++ )
	{
		//uint8_t* OldPos = &Pixels[p*YuvStride];
		//uint8_t* NewPos = &Pixels[p*LumaStride];
		
		//	invert
		//auto Luma = 255 - OldPos[1];
		auto Luma = OldPos[1];
		
		//	threshold
		//Luma = (Luma < 100) ? 0 : 255;

		NewPos[0] = Luma;

		OldPos += YuvStride;
		NewPos += LumaStride;
	}
	
	PixelsArray.SetSize( PixelCount );
	//PixelsArray.SetSize( LumaMeta.GetDataSize() );

	Meta = LumaMeta;
}


void ConvertFormat_YYuv8888_To_Luma(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	auto& YuvMeta = Meta;
	auto w = YuvMeta.GetWidth();
	auto h = YuvMeta.GetHeight();
	SoyPixelsMeta LumaMeta(w, h, NewFormat);
	auto PixelCount = size_cast<int>(w*h);
	
	auto YuvStride = 2;
	auto LumaStride = 1;
	auto* Pixels = PixelsArray.GetArray();

	//	when shrinking, we go forward through the array
	for (int p = 0; p < PixelCount; p++)
	{
		uint8_t* OldPos = &Pixels[p*YuvStride];
		uint8_t* NewPos = &Pixels[p*LumaStride];

		//	invert
		//auto Luma = 255 - OldPos[1];
		auto Luma = OldPos[0];

		//	threshold
		//Luma = (Luma < 100) ? 0 : 255;

		NewPos[0] = Luma;
	}

	PixelsArray.SetSize(LumaMeta.GetDataSize());

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

void Depth16_To_Plane(ArrayInterface<uint8>& PixelsArray, SoyPixelsMeta& Meta, SoyPixelsFormat::Type NewFormat)
{
	uint16* DepthPixels = reinterpret_cast<uint16*>(PixelsArray.GetArray());
	auto PixelCount = Meta.GetWidth() * Meta.GetHeight();

	//	get new format planes
	SoyPixelsMeta NewMeta( Meta.GetWidth(), Meta.GetHeight(), NewFormat );
	BufferArray<SoyPixelsMeta,5> NewPlaneMetas;
	NewMeta.GetPlanes(GetArrayBridge(NewPlaneMetas));

	//	if the first plane is exactly half (ie, 16bit to 8bit) then convert
	{
		auto NewPlaneSize = NewPlaneMetas[0].GetDataSize();
		auto OldPlaneSize = Meta.GetDataSize();
		if ( NewPlaneSize*2 != OldPlaneSize )
			throw Soy::AssertException("Cannot convert depth16 to plane8, not exactly half");
	}
	
	//	rewrite plane0
	{
		static auto DepthMax = 10000;
		auto* Plane0 = PixelsArray.GetArray();
		for (int p = 0; p < PixelCount; p++)
		{
			auto Depth16 = DepthPixels[p];
			float Depthf = Depth16 / static_cast<float>(DepthMax);
			auto Depth8 = Depthf * 255.0f;
			Plane0[p] = Depth8;
		}
	}
	
	//	resize data (do this AFTER writing otherwise we might clip the 16bit data)
	PixelsArray.SetSize(NewMeta.GetDataSize());
	Meta.DumbSetFormat(NewFormat);

	//	clear other planes
	{
		BufferArray<std::shared_ptr<SoyPixelsImpl>,5> Planes;
		SoyPixelsRemote Pixels( PixelsArray.GetArray(), PixelsArray.GetDataSize(), Meta );
		Pixels.SplitPlanes(GetArrayBridge(Planes));
		for ( auto p=1;	p<Planes.GetSize();	p++ )
		{
			auto& PlaneArray = Planes[p]->GetPixelsArray();
			auto* PlanePixels = PlaneArray.GetArray();
			for ( auto i=0;	i<PlaneArray.GetDataSize();	i++ )
				PlanePixels[i] = 0;
		}
	}
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
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Luma, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Luma, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::Luma, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Greyscale_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::ChromaUV_88, SoyPixelsFormat::RGBA, ConvertFormat_TwoChannelToFour ),
	TConvertFunc( SoyPixelsFormat::Uvy_8_88, SoyPixelsFormat::Greyscale, ConvertFormat_Uvy844_To_Luma),
	TConvertFunc( SoyPixelsFormat::Uvy_8_88, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Uvy844_To_Yuv_8_8_8),
	TConvertFunc( SoyPixelsFormat::YYuv_8888, SoyPixelsFormat::Greyscale, ConvertFormat_YYuv8888_To_Luma),
	TConvertFunc( SoyPixelsFormat::YYuv_8888, SoyPixelsFormat::Greyscale, ConvertFormat_YYuv8888_To_Luma),
	TConvertFunc( SoyPixelsFormat::YYuv_8888, SoyPixelsFormat::Greyscale, ConvertFormat_YYuv8888_To_Luma),
	TConvertFunc( SoyPixelsFormat::RGB, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_RGB_To_Yuv_8_8_8),
	TConvertFunc(SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_88, ConvertFormat_Greyscale_To_Yuv_8_88),
	TConvertFunc(SoyPixelsFormat::Greyscale, SoyPixelsFormat::Yuv_8_88, ConvertFormat_Greyscale_To_Yuv_8_88),
	TConvertFunc(SoyPixelsFormat::Luma, SoyPixelsFormat::Yuv_8_88, ConvertFormat_Greyscale_To_Yuv_8_88),
	TConvertFunc(SoyPixelsFormat::Luma, SoyPixelsFormat::Yuv_8_88, ConvertFormat_Greyscale_To_Yuv_8_88),

	TConvertFunc( SoyPixelsFormat::Yuv_8_8_8, SoyPixelsFormat::Greyscale, ConvertFormat_YuvPlane_To_Greyscale),
	TConvertFunc( SoyPixelsFormat::Yuv_8_8_8, SoyPixelsFormat::Greyscale, ConvertFormat_YuvPlane_To_Greyscale),
	TConvertFunc( SoyPixelsFormat::Yuv_8_88, SoyPixelsFormat::Greyscale, ConvertFormat_YuvPlane_To_Greyscale),
	TConvertFunc( SoyPixelsFormat::Yuv_8_88, SoyPixelsFormat::Greyscale, ConvertFormat_YuvPlane_To_Greyscale),
	TConvertFunc( SoyPixelsFormat::Yuv_8_8_8, SoyPixelsFormat::Yuv_8_88, ConvertFormat_Yuv_3Plane_To_2Plane),
	TConvertFunc( SoyPixelsFormat::Yuv_8_88, SoyPixelsFormat::Yuv_8_8_8, ConvertFormat_Yuv_2Plane_To_3Plane),

	TConvertFunc( SoyPixelsFormat::Depth16mm, SoyPixelsFormat::Greyscale, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Greyscale, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::Greyscale, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::Greyscale, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::Depth16mm, SoyPixelsFormat::Yuv_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Yuv_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::Yuv_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::Yuv_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::Depth16mm, SoyPixelsFormat::Yuv_8_8_8, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Yuv_8_8_8, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::Yuv_8_8_8, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::Yuv_8_8_8, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::Depth16mm, SoyPixelsFormat::Yvu_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::KinectDepth, SoyPixelsFormat::Yvu_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth10bit, SoyPixelsFormat::Yvu_8_88, Depth16_To_Plane),
	TConvertFunc( SoyPixelsFormat::FreenectDepth11bit, SoyPixelsFormat::Yvu_8_88, Depth16_To_Plane),

	
};


void SoyPixelsImpl::SetFormat(SoyPixelsFormat::Type Format)
{
	auto OldFormat = GetFormat();
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
	

	//	see if we can use of simple channel-count change
	bool UseOfPixels = false;
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::RGBA );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::RGB );
	//UseOfPixels |= ( GetFormat() == SoyPixelsFormat::BGRA && Format == SoyPixelsFormat::BGR );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::Greyscale );
	UseOfPixels |= ( GetFormat() == SoyPixelsFormat::RGB && Format == SoyPixelsFormat::Greyscale );
	
	//	report missing, but desired conversions
	std::stringstream Error;
	Error << "No soypixel conversion from " << SoyPixelsFormat::ToString(OldFormat) << " to " << SoyPixelsFormat::ToString(Format);
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

void SoyPixelsImpl::AppendPlane(const SoyPixelsImpl& Plane)
{
	auto NewFormat = GetMergedFormat(this->GetFormat(), Plane.GetFormat());
	auto& PixelsA = this->GetPixelsArray();
	auto& PixelsB = Plane.GetPixelsArray();

	//	verify this is valid first
	auto NewSize = PixelsA.GetDataSize() + PixelsB.GetDataSize();
	SoyPixelsRemote NewPixels( PixelsA.GetArray(), this->GetWidth(), this->GetHeight(), NewSize, NewFormat);

	//	add new data
	PixelsA.PushBackArray(PixelsB);

	//	set new meta
	this->GetMeta().DumbSetFormat(NewFormat);
}

void SoyPixelsImpl::AppendPlane(const SoyPixelsImpl& PlaneB, const SoyPixelsImpl& PlaneC)
{
	auto NewFormat = GetMergedFormat(this->GetFormat(), PlaneB.GetFormat(), PlaneC.GetFormat());
	auto& PixelsA = this->GetPixelsArray();
	auto& PixelsB = PlaneB.GetPixelsArray();
	auto& PixelsC = PlaneC.GetPixelsArray();

	//	verify this is valid first
	auto NewSize = PixelsA.GetDataSize() + PixelsB.GetDataSize() + PixelsC.GetDataSize();
	SoyPixelsRemote NewPixels(PixelsA.GetArray(), this->GetWidth(), this->GetHeight(), NewSize, NewFormat);

	//	add new data
	PixelsA.PushBackArray(PixelsB);
	PixelsA.PushBackArray(PixelsC);

	//	set new meta
	this->GetMeta().DumbSetFormat(NewFormat);
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
	Soy::TScopeTimerPrint Timer(__PRETTY_FUNCTION__, 2);
	//	copy old data
	SoyPixels Old;
	Old.Copy(*this);
	
	auto& New = *this;
	auto NewChannelCount = GetChannels();
	
	//	dumb resize buffer
	Init( NewWidth, NewHeight, GetFormat() );
	
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
	auto LineSize = GetRowPitchBytes();
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
		case SoyPixelsFormat::Yuv_8_88:
			Planes.PushBack(SoyPixelsMeta(GetWidth(), GetHeight(), SoyPixelsFormat::Luma));
			Planes.PushBack(SoyPixelsMeta(GetWidth() / 2, GetHeight() / 2, SoyPixelsFormat::ChromaUV_88));
			break;
		
		case SoyPixelsFormat::Yuv_8_8_8:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::Luma ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaU_8 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth()/2, GetHeight()/2, SoyPixelsFormat::ChromaV_8 ) );
			break;
		
		case SoyPixelsFormat::ChromaUV_8_8:
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaU_8 ) );
			Planes.PushBack( SoyPixelsMeta( GetWidth(), GetHeight(), SoyPixelsFormat::ChromaV_8 ) );
			break;

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
