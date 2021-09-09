#include "SoyPixels.h"
#include "SoyDebug.h"
#include "SoyPng.h"
#include "RemoteArray.h"
#include "SoyFourcc.h"

//	to avoid various symbol conflicts with other libs 
//	that use miniz (but different versions)
//	we put all the miniZ stuff in it's own namespace
namespace MiniZ
{
#pragma clang diagnostic ignored "-Wcomma"
#include "miniz/miniz.h"

	void	IsOkay(int MiniZResult, const char* Context);
}

void MiniZ::IsOkay(int MiniZResult, const char* Context)
{
	if (MiniZResult == MZ_OK)
		return;

	std::stringstream Error;
	Error << "Miniz error; " << mz_error(MiniZResult) << "(" << MiniZResult << ") in " << Context;
	throw Soy::AssertException(Error);
}


const Soy::TFourcc TPng::IHDR("IHDR");
const Soy::TFourcc TPng::IEND("IEND");
const Soy::TFourcc TPng::IDAT("IDAT");
	


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

void TPng::CheckMagic(const ArrayBridge<uint8_t>& PngData)
{
	BufferArray<char,8> Magic;
	GetMagic( GetArrayBridge(Magic) );
	
	auto Diff = memcmp( PngData.GetArray(), Magic.GetArray(), Magic.GetDataSize() );
	if ( Diff == 0 )
		return;
	
	throw Soy::AssertException("Png magic header mismatch");
}

bool TPng::IsPngHeader(const ArrayBridge<uint8_t>&& Data)
{
	if (Data.GetSize() < 8)
		return false;

	BufferArray<char, 8> Magic;
	GetMagic(GetArrayBridge(Magic));

	auto Diff = memcmp(Data.GetArray(), Magic.GetArray(), Magic.GetDataSize());
	if (Diff != 0)
		return false;

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

void DeFilterScanline(TPng::TFilterNone_ScanlineFilter::Type Filter,const ArrayBridge<uint8>&& Scanline,int ByteWidth,ArrayBridge<uint8>& DeFilteredData)
{
	if ( Filter == TPng::TFilterNone_ScanlineFilter::None )
	{
		DeFilteredData.PushBackArray( Scanline );
		return;
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
		return;
	}
	
	std::stringstream Error;
	Error << "defiltering PNG data came across unhandled filter value (" << Filter << ")";
	throw Soy::AssertException(Error);
}

void TPng::ReadData(SoyPixelsImpl& Pixels,const THeader& Header,ArrayBridge<char>& Data)
{
	using namespace MiniZ;

	if ( Header.mCompression == TCompression::DEFLATE )
	{
		//	decompress straight into image. But the decompressed data will have extra filter values per row!
		auto&& DecompressedData = Pixels.GetPixelsArray();
		DecompressedData.PushBlock( 1 * Pixels.GetHeight() );

		mz_ulong CompressedLength = size_cast<mz_ulong>(DecompressedData.GetDataSize());
		mz_ulong DecompressedLength = CompressedLength;
		auto Result = mz_uncompress( reinterpret_cast<Byte*>(DecompressedData.GetArray()), &DecompressedLength, reinterpret_cast<Byte*>(Data.GetArray()), CompressedLength);
		IsOkay(Result, "TPng::ReadData uncompressing PNG mz_uncompress()");
		if ( DecompressedLength != DecompressedData.GetDataSize() )
		{
			std::stringstream Error;
			Error << "Decompressed to " << DecompressedLength << " bytes, expecting " << DecompressedData.GetDataSize();
			throw Soy::AssertException(Error);
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
			DeFilterScanline(Filter, GetArrayBridge(Scanline), bytewidth, DeFilteredData);
		}
		//	overwrite data with defiltered data
		DecompressedData.Copy( DeFilteredData );
		
		//	validate image data length here
		return;
	}
	else
	{
		throw Soy::AssertException("Unsupported PNG data compression");
	}
}

bool TPng::ReadTail(SoyPixelsImpl& Pixels,ArrayBridge<char>& Data,std::stringstream& Error)
{
	return true;
}

int GetMinizCompressionLevel(float Levelf)
{
	using namespace MiniZ;
	//	0-10
	auto Level = static_cast<int>( Levelf * MZ_UBER_COMPRESSION );
	if ( Level < MZ_NO_COMPRESSION || Level > MZ_UBER_COMPRESSION )
	{
		std::stringstream Error;
		Error << "Png compression level " << Levelf << "/" << Level << " out of mini-z range(0-1)";
		throw Soy::AssertException(Error);
	}

	return Level;
}

void TPng::Private::GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression,float CompressionLevel)
{
	using namespace MiniZ;
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
		auto CompressionLevelEnum = GetMinizCompressionLevel( CompressionLevel );
		std::stringstream Debug_TimerName;
		Debug_TimerName << "Deflate compression; " << Soy::FormatSizeBytes(FilteredPixels.GetDataSize()) << ". Compression level: " << CompressionLevelEnum;
		ofScopeTimerWarning DeflateCompressTimer( Debug_TimerName.str().c_str(), 3 );
	
		//	for tiny data, this isn't enough
		auto DefAllocated = static_cast<mz_ulong>( 1.2f * FilteredPixels.GetDataSize() );
		DefAllocated = std::max<mz_ulong>(DefAllocated, 1024);
		mz_ulong DefUsed = DefAllocated;
		auto* DefData = PngData.PushBlock(DefAllocated);
		auto DecompressedSize = size_cast<mz_ulong>(FilteredPixels.GetDataSize());
		auto Result = mz_compress2( reinterpret_cast<Byte*>(DefData), &DefUsed, FilteredPixels.GetArray(), DecompressedSize, CompressionLevelEnum );
		IsOkay(Result, "mz_compress2");
	
		if ( DefUsed > DefAllocated )
			throw Soy_AssertException("miniz compressed reported that it used more memory than we had allocated" );
	
		//	trim data
		auto Overflow = DefAllocated - size_cast<int>(DefUsed);
		PngData.SetSize( PngData.GetSize() - Overflow );
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
		throw Soy_AssertException("Currently only supporting Deflate in png");
	}
}

void TPng::Private::GetDeflateData(Array<char>& DeflateData,const ArrayBridge<uint8>& PixelBlock,bool LastBlock,int WindowSize)
{
	//	pixel block should have already been split
	if (PixelBlock.GetSize() > WindowSize)
		throw Soy::AssertException("Pixel block should have already been split");

	//	if it's NOT the last block, it MUST be full-size
	if ( !LastBlock )
	{
		if (PixelBlock.GetSize() != WindowSize)
			throw Soy::AssertException("Expecting pixel block size to be window size");
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
}



void TPng::GetPng(const SoyPixelsImpl& Pixels,ArrayBridge<uint8_t>& PngData,float CompressionLevel,ArrayBridge<uint8_t>* Exif)
{
	//	remove need for Png. Isn't this deprecated anyway
#if defined(TARGET_PS4)
	throw Soy::Soy_AssertException("Not supported on PS4");
#else
	//	if non-supported PNG colour format, then convert to one that is
	auto PngColourType = TPng::GetColourType( Pixels.GetFormat() );
	if ( PngColourType == TPng::TColour::Invalid )
	{
		//	do conversion here
		SoyPixels OtherFormat;
		OtherFormat.Copy( Pixels );
		auto NewFormat = SoyPixelsFormat::RGB;
		
		//	gr: this conversion should be done in soypixels, with a list of compatible output formats
		switch ( Pixels.GetFormat() )
		{
				//case SoyPixelsFormat::KinectDepth:	NewFormat = SoyPixelsFormat::Greyscale;	break;
			case SoyPixelsFormat::BGRA:			NewFormat = SoyPixelsFormat::RGBA;	break;
			default:break;
		}
		
		//	new format MUST be compatible or we'll get stuck in a loop
		if ( TPng::GetColourType( NewFormat ) == TPng::TColour::Invalid )
		{
			throw Soy_AssertException("Converted to format that we can't process");
		}
		
		//	attempt conversion
		OtherFormat.SetFormat( NewFormat );
		
		GetPng( OtherFormat, PngData, CompressionLevel );
		return;
	}
	
	//	http://stackoverflow.com/questions/7942635/write-png-quickly
	
	//\211 P N G \r \n \032 \n (89 50 4E 47 0D 0A 1A 0A
	//uint8 Magic[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	BufferArray<char,8> Magic;
	TPng::GetMagic( GetArrayBridge(Magic) );
	char IHDR[] = { 73, 72, 68, 82 };	//'I', 'H', 'D', 'R'
	const char IEND[] = { 73, 69, 78, 68 }; //("IEND")
	const char IDAT[] = { 73, 68, 65, 84 };// ("IDAT")
	//	http://ftp-osl.osuosl.org/pub/libpng/documents/pngext-1.5.0.html#C.eXIf
	const char EXIF[] = { 101, 88, 73, 102 };// ("eXIf")
	
	//	write header chunk
	uint32 Width = size_cast<uint32>(Pixels.GetWidth());
	uint32 Height = size_cast<uint32>(Pixels.GetHeight());
	uint8 BitDepth = Pixels.GetBitDepth();
	uint8 ColourType = PngColourType;
	if (PngColourType == TPng::TColour::Invalid)
		throw Soy::AssertException("Png colour type is invalid");
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
	
	//	make exif data
	Array<char> ExifData;
	if ( Exif )
	{
		ExifData.PushBackArray( EXIF );
		ExifData.PushBackArray( *Exif );
	}
	
	//	write data chunks
	Array<char> PixelData;
	PixelData.PushBackArray( IDAT );
	TPng::Private::GetPngData( PixelData, Pixels, static_cast<TPng::TCompression::Type>(Compression), CompressionLevel );
	
	//	write Tail chunks
	Array<char> Tail;
	Tail.PushBackArray( IEND );
	
	//	put it all together!
	PngData.Reserve( Header.GetDataSize() + 12 );
	if ( !ExifData.IsEmpty() )
		PngData.Reserve( ExifData.GetDataSize() + 12 );
	PngData.Reserve( PixelData.GetDataSize() + 12 );
	PngData.Reserve( Tail.GetDataSize() + 12 );
	
	uint32 HeaderLength = size_cast<uint32>( Header.GetDataSize() - std::size(IHDR) );
	if ( HeaderLength != 13 )
		throw Soy_AssertException("HeaderLength not 13");
	
	uint32 HeaderCrc = Soy::GetCrc32( GetArrayBridge(Header) );
	PngData.PushBackArray( Magic );
	PngData.PushBackReinterpretReverse( HeaderLength );
	PngData.PushBackArray( Header );
	PngData.PushBackReinterpretReverse( HeaderCrc );
	
	//	exif can go between header and end, but not inbetween idats
	if ( !ExifData.IsEmpty() )
	{
		uint32 ExifDataLength = size_cast<uint32>( ExifData.GetDataSize() - std::size(EXIF) );
		uint32 ExifDataCrc = Soy::GetCrc32( GetArrayBridge(ExifData) );
		PngData.PushBackReinterpretReverse( ExifDataLength );
		PngData.PushBackArray( ExifData );
		PngData.PushBackReinterpretReverse( ExifDataCrc );
	}
	
	uint32 PixelDataLength = size_cast<uint32>( PixelData.GetDataSize() - std::size(IDAT) );
	uint32 PixelDataCrc = Soy::GetCrc32( GetArrayBridge(PixelData) );
	PngData.PushBackReinterpretReverse( PixelDataLength );
	PngData.PushBackArray( PixelData );
	PngData.PushBackReinterpretReverse( PixelDataCrc );
	
	uint32 TailLength = size_cast<uint32>( Tail.GetDataSize() - std::size(IEND) );
	uint32 TailCrc = Soy::GetCrc32( GetArrayBridge(Tail) );
	if ( TailCrc != 0xAE426082 )
		throw Soy_AssertException("Tail CRC not 0xAE426082");

	PngData.PushBackReinterpretReverse( TailLength );
	PngData.PushBackArray( Tail );
	PngData.PushBackReinterpretReverse( TailCrc );
#endif
}


void TPng::EnumChunks(const ArrayBridge<uint8_t>&& PngData,std::function<void(Soy::TFourcc&,uint32_t,ArrayBridge<uint8_t>&&)> EnumBlock)
{
	//	skip magic
	auto DataPosition = 0;
	
	CheckMagic(PngData);
	DataPosition += 8;

	while ( DataPosition < PngData.GetSize() )
	{
		//	pop fourcc
		auto* LengthPtr = &PngData[DataPosition+0];
		auto* FourccPtr = &PngData[DataPosition+4];
		auto* DataPtr = &PngData[DataPosition+4+4];
		Soy::TFourcc Fourcc( FourccPtr );
		
		auto Length = *reinterpret_cast<const uint32_t*>( LengthPtr );
		Soy::EndianSwap(Length);
		if ( DataPosition+Length > PngData.GetSize() )
		{
			std::stringstream Error;
			Error << "Png chunk (" << Fourcc << ") length (x" << Length << ") too large for data (x" << PngData.GetSize() << ")";
			throw Soy::AssertException( Error );
		}
		
		auto* CrcPtr = &PngData[DataPosition+4+4+Length];
		auto Crc = *reinterpret_cast<const uint32_t*>( CrcPtr );
		Soy::EndianSwap(Crc);
		
		auto Data = GetRemoteArray( DataPtr, Length );
		EnumBlock( Fourcc, Crc, GetArrayBridge(Data) );
		
		DataPosition = DataPosition+4+4+Length+4;
	}
}
