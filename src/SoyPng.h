#pragma once


class SoyPixelsImpl;

namespace Soy
{
	class TFourcc;
}


namespace TPng
{
	extern const Soy::TFourcc	IHDR;
	extern const Soy::TFourcc	IEND;
	extern const Soy::TFourcc	IDAT;

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
			Invalid = -1,
			DEFLATE = 0,
		};};
	
	namespace TFilter { enum Type
		{
			Invalid = -1,
			None = 0,
		};};
	
	namespace TFilterNone_ScanlineFilter { enum Type
		{
			Invalid = -1,
			None	= 0,
			Sub		= 1,
			Up		= 2,
			Average	= 3,
			Paeth	= 4,
		};};
	
	namespace TInterlace { enum Type
		{
			Invalid = -1,
			None = 0,
		};};
	
	//	gr: integrate this into TSerialisation! no excuse not to. Export/ImportNative() or something
	//	gr: these are the bits that are needed outside of SoyPixels
	class THeader
	{
	public:
		THeader() :
			mCompression	( TCompression::Invalid ),
			mFilter			( TFilter::Invalid ),
			mInterlace		( TInterlace::Invalid )
		{
		}
		bool				IsValid() const;
		
		TCompression::Type	mCompression;
		TFilter::Type		mFilter;
		TInterlace::Type	mInterlace;
	};
	
	//	fast, non-throwing PNG header check
	bool		IsPngHeader(const ArrayBridge<uint8_t>&& Data);

	//	these are designed for reading, so they throw
	void		GetMagic(ArrayBridge<char>&& Magic);
	bool		CheckMagic(TArrayReader& ArrayReader);
	bool		CheckMagic(ArrayBridge<char>&& PngData);
	void		CheckMagic(const ArrayBridge<uint8_t>& PngData);

	TColour::Type			GetColourType(SoyPixelsFormat::Type Format);
	SoyPixelsFormat::Type	GetPixelFormatType(TColour::Type Format);
	
	void		GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression,float CompressionLevel);
	void		GetDeflateData(Array<char>& ChunkData,const ArrayBridge<uint8>& PixelBlock,bool LastBlock,int WindowSize);
	
	bool		ReadHeader(SoyPixelsImpl& Pixels,THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error);
	bool		ReadData(SoyPixelsImpl& Pixels,const THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error);
	bool		ReadTail(SoyPixelsImpl& Pixels,ArrayBridge<char>& Data,std::stringstream& Error);

	//	moved from soypixels
	//	todo: fix type from char
	void		GetPng(const SoyPixelsImpl& Pixels,ArrayBridge<char>& PngData,float CompressionLevel,ArrayBridge<uint8_t>* Exif=nullptr);
	inline void	GetPng(const SoyPixelsImpl& Pixels,ArrayBridge<char>& PngData,float CompressionLevel,ArrayBridge<uint8_t>&& Exif)
	{
		GetPng( Pixels, PngData, CompressionLevel, &Exif );		
	}
	
	void		EnumChunks(const ArrayBridge<uint8_t>&& PngData,std::function<void(Soy::TFourcc&,uint32_t,ArrayBridge<uint8_t>&&)> EnumChunk);
};
std::ostream& operator<< (std::ostream &out,const TPng::TColour::Type &in);
std::ostream& operator<< (std::ostream &out,const TPng::TFilterNone_ScanlineFilter::Type &in);

