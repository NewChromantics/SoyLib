#include "SoyPixels.h"
#include "miniz/miniz.h"
#include "SoyDebug.h"
#include "SoyPng.h"
#include "RemoteArray.h"



TPng::TColour::Type TPng::GetColourType(SoyPixelsFormat::Type Format)
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
SoyPixelsFormat::Type TPng::GetPixelFormatType(TPng::TColour::Type Format)
{
	switch ( Format )
	{
		case TColour::Greyscale:		return SoyPixelsFormat::Greyscale;
		case TColour::GreyscaleAlpha:	return SoyPixelsFormat::GreyscaleAlpha;
		case TColour::RGB:				return SoyPixelsFormat::RGB;
		case TColour::RGBA:				return SoyPixelsFormat::RGBA;
		default:
			return SoyPixelsFormat::Invalid;
	}
}

std::ostream& operator<< (std::ostream &out,const TPng::TColour::Type &in)
{
	switch ( in )
	{
		case TPng::TColour::Greyscale:		return out << "Greyscale";
		case TPng::TColour::GreyscaleAlpha:	return out << "GreyscaleAlpha";
		case TPng::TColour::RGB:			return out << "RGB";
		case TPng::TColour::RGBA:			return out << "RGBA";
		default:
			return out << "<unknown TPng::TColour::" << static_cast<int>(in) << ">";
	}
}

std::ostream& operator<< (std::ostream &out,const TPng::TFilterNone_ScanlineFilter::Type &in)
{
	switch ( in )
	{
		case TPng::TFilterNone_ScanlineFilter::None:	return out << "None";
		case TPng::TFilterNone_ScanlineFilter::Sub:		return out << "Sub";
		case TPng::TFilterNone_ScanlineFilter::Up:		return out << "Up";
		case TPng::TFilterNone_ScanlineFilter::Average:	return out << "Average";
		case TPng::TFilterNone_ScanlineFilter::Paeth:	return out << "Paeth";
		default:
			return out << "<unknown TPng::TFilterNone_ScanlineFilter::" << static_cast<int>(in) << ">";
	}
}

void TPng::GetMagic(ArrayBridge<char>&& Magic)
{
	//const unsigned char _Magic[] = { 0xD3, 'P', 'N', 'G', 13, 10, 26, 10 };
	const unsigned char _Magic[] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
	auto MagicSigned = GetRemoteArray( reinterpret_cast<const char*>(_Magic), 8 );
	Magic.Copy( MagicSigned );
}


bool TPng::CheckMagic(ArrayBridge<char>&& PngData)
{
	TArrayReader ArrayReader( PngData );
	return CheckMagic( ArrayReader );
}

bool TPng::CheckMagic(TArrayReader& ArrayReader)
{
	BufferArray<char,8> Magic;
	GetMagic( GetArrayBridge(Magic) );
	
	if ( !ArrayReader.ReadCompare( GetArrayBridge(Magic) ) )
	{
		//	if we failed here and not at the start of the data, that might be the problem
		Soy::Assert( ArrayReader.mOffset == 0, "TPng::CheckMagic failed, and data is not at start... could be the issue" );
		return false;
	}
	
	return true;
}

bool TPng::THeader::IsValid() const
{
	if ( mCompression != TCompression::DEFLATE )
		return false;
	if ( mFilter != TFilter::None )
		return false;
	if ( mInterlace != TInterlace::None )
		return false;
	return true;
}

bool TPng::ReadHeader(SoyPixelsImpl& Pixels,THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error)
{
	uint32 Width,Height;
	uint8 BitDepth,ColourType8,Compression,Filter,Interlace;
	TArrayReader Reader( Data );

	int HeaderError = 0;
	HeaderError |= (1<<0) * !Reader.ReadReinterpretReverse<BufferArray<char,20>>( Width );
	HeaderError |= (1<<1) * !Reader.ReadReinterpretReverse<BufferArray<char,20>>( Height );
	HeaderError |= (1<<2) * !Reader.Read( BitDepth );
	HeaderError |= (1<<3) * !Reader.Read( ColourType8 );
	HeaderError |= (1<<4) * !Reader.Read( Compression );
	HeaderError |= (1<<5) * !Reader.Read( Filter );
	HeaderError |= (1<<6) * !Reader.Read( Interlace );
	
	if ( HeaderError )
	{
		Error << "Error reading header (" << HeaderError << ")";
		return false;
	}

	auto ColourType = static_cast<TPng::TColour::Type>(ColourType8);
	auto PixelFormat = TPng::GetPixelFormatType( ColourType );
	if ( PixelFormat == SoyPixelsFormat::Invalid )
	{
		Error << "Invalid pixel format (" << ColourType << ")";
		return false;
	}
	
	//	check bitdepth...
	if ( BitDepth != 8 )
	{
		Error << "Unsupported bit depth " << BitDepth;
		return false;
	}
	
	//	allocate
	Pixels.Init( Width, Height, PixelFormat );
	
	//	grab the other bits
	Header.mCompression = static_cast<TCompression::Type>(Compression);
	Header.mFilter = static_cast<TFilter::Type>(Filter);
	Header.mInterlace = static_cast<TInterlace::Type>(Interlace);
	if ( !Header.IsValid() )
	{
		Error << "Compression/Filter/Interlace is invalid/unsupported";
		return false;
	}
	
	return true;
}

/*
 //	from http://lpi.googlecode.com/svn/trunk/lodepng.cpp
static void filterScanline(unsigned char* out, const unsigned char* scanline, const unsigned char* prevline,
                           size_t length, size_t bytewidth, unsigned char filterType)
{
	size_t i;
	switch(filterType)
	{
	
		case 2: //Up
			if(prevline)
			{
				for(i = 0; i < length; i++) out[i] = scanline[i] - prevline[i];
			}
			else
			{
				for(i = 0; i < length; i++) out[i] = scanline[i];
			}
			break;
		case 3: //Average
			if(prevline)
			{
				for(i = 0; i < bytewidth; i++) out[i] = scanline[i] - prevline[i] / 2;
				for(i = bytewidth; i < length; i++) out[i] = scanline[i] - ((scanline[i - bytewidth] + prevline[i]) / 2);
			}
			else
			{
				for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
				for(i = bytewidth; i < length; i++) out[i] = scanline[i] - scanline[i - bytewidth] / 2;
			}
			break
		case 4: //Paeth
			if(prevline)
			{
				//paethPredictor(0, prevline[i], 0) is always prevline[i]
				for(i = 0; i < bytewidth; i++) out[i] = (scanline[i] - prevline[i]);
				for(i = bytewidth; i < length; i++)
				{
					out[i] = (scanline[i] - paethPredictor(scanline[i - bytewidth], prevline[i], prevline[i - bytewidth]));
				}
			}
			else
			{
				for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
				//paethPredictor(scanline[i - bytewidth], 0, 0) is always scanline[i - bytewidth]
				for(i = bytewidth; i < length; i++) out[i] = (scanline[i] - scanline[i - bytewidth]);
			}
			break;
		default: return; //unexisting filter type given
	}
}
 */

bool DeFilterScanline(TPng::TFilterNone_ScanlineFilter::Type Filter,const ArrayBridge<uint8>&& Scanline,int ByteWidth,ArrayBridge<uint8>& DeFilteredData,std::stringstream& Error)
{
	if ( Filter == TPng::TFilterNone_ScanlineFilter::None )
	{
		DeFilteredData.PushBackArray( Scanline );
		return true;
	}
	else if ( Filter == TPng::TFilterNone_ScanlineFilter::Sub )
	{
		///	bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise
		DeFilteredData.Reserve( Scanline.GetSize() );
		int i;
		for(i = 0; i < ByteWidth; i++)
			DeFilteredData.PushBack( Scanline[i] );
		for(i = ByteWidth; i < Scanline.GetSize(); i++)
			DeFilteredData.PushBack( Scanline[i] - Scanline[i - ByteWidth] );
		return true;
	}
	
	Error << "defiltering PNG data came across unhandled filter value (" << Filter << ")";
	return false;
}

bool TPng::ReadData(SoyPixelsImpl& Pixels,const THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error)
{
	if ( Header.mCompression == TCompression::DEFLATE )
	{
		//	decompress straight into image. But the decompressed data will have extra filter values per row!
		auto&& DecompressedData = Pixels.GetPixelsArray();
		DecompressedData.PushBlock( 1 * Pixels.GetHeight() );

		mz_ulong CompressedLength = size_cast<mz_ulong>(DecompressedData.GetDataSize());
		mz_ulong DecompressedLength = CompressedLength;
		auto Result = mz_uncompress( reinterpret_cast<Byte*>(DecompressedData.GetArray()), &DecompressedLength, reinterpret_cast<Byte*>(Data.GetArray()), CompressedLength);
		if ( Result != MZ_OK )
		{
			Error << "Error decompressing PNG data (" << mz_error(Result) << ")";
			return false;
		}
		if ( DecompressedLength != DecompressedData.GetDataSize() )
		{
			Error << "Decompressed to " << DecompressedLength << " bytes, expecting " << DecompressedData.GetDataSize();
			return false;
		}

		Array<uint8> _DeFilteredData;
		auto DeFilteredData = GetArrayBridge( _DeFilteredData );
		
		//	de-filter the pixels
		auto Stride = Pixels.GetChannels()*Pixels.GetWidth();
		//	decompressed data stride includes filter
		Stride += 1;
		for ( int i=0;	i<DecompressedData.GetSize();	i+=Stride )
		{
			//	get filter code
			auto Filter = static_cast<TFilterNone_ScanlineFilter::Type>( DecompressedData[i] );
			size_t ScanlineLength = Stride-1;
			auto Scanline = GetRemoteArray( &DecompressedData[i+1], ScanlineLength );
			int bytewidth = (Pixels.GetBitDepth() + 7) / 8;
			if ( !DeFilterScanline( Filter, GetArrayBridge( Scanline ), bytewidth, DeFilteredData, Error ) )
				return false;
		}
		//	overwrite data with defiltered data
		DecompressedData.Copy( DeFilteredData );
		
		//	validate image data length here
		return true;
	}
	else
	{
		Error << "Unsupported PNG data compression";
		return false;
	}
}

bool TPng::ReadTail(SoyPixelsImpl& Pixels,ArrayBridge<char>& Data,std::stringstream& Error)
{
	return true;
}


bool TPng::GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression)
{
	if ( Compression == TCompression::DEFLATE )
	{
		//	we need to add a filter value at the start of each row, so calculate all the byte indexes
		auto& OrigPixels = Image.GetPixelsArray();
		Array<uint8> FilteredPixels;
		FilteredPixels.Reserve( OrigPixels.GetDataSize() + Image.GetHeight() );
		FilteredPixels.PushBackArray( OrigPixels );
		auto Stride = Image.GetChannels()*Image.GetWidth();
		for ( ssize_t i=OrigPixels.GetSize()-Stride;	i>=0;	i-=Stride )
		{
			//	insert filter value/code
			static char FilterValue = TFilterNone_ScanlineFilter::None;
			auto* RowStart = FilteredPixels.InsertBlock(i,1);
			RowStart[0] = FilterValue;
		}

		//	use miniz to compress with deflate
		auto CompressionLevel = MZ_NO_COMPRESSION;
		std::stringstream Debug_TimerName;
		Debug_TimerName << "Deflate compression; " << Soy::FormatSizeBytes(FilteredPixels.GetDataSize()) << ". Compression level: " << CompressionLevel;
		ofScopeTimerWarning DeflateCompressTimer( Debug_TimerName.str().c_str(), 3 );
	
		auto DefAllocated = static_cast<mz_ulong>( 1.2f * FilteredPixels.GetDataSize() );
		mz_ulong DefUsed = DefAllocated;
		auto* DefData = PngData.PushBlock(DefAllocated);
		auto DecompressedSize = size_cast<mz_ulong>(FilteredPixels.GetDataSize());
		auto Result = mz_compress2( reinterpret_cast<Byte*>(DefData), &DefUsed, FilteredPixels.GetArray(), DecompressedSize, CompressionLevel );
		if ( !Soy::Assert( Result == MZ_OK, "mz compression failed" ) )
			return false;
		if ( !Soy::Assert( DefUsed <= DefAllocated, "miniz compressed reported that it used more memory than we had allocated" ) )
			return false;
		//	trim data
		auto Overflow = DefAllocated - size_cast<int>(DefUsed);
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
	uint16 Len = size_cast<uint16>(PixelBlock.GetDataSize());
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

