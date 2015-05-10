#pragma once


class SoyPixelsImpl;


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
	
	bool		CheckMagic(TArrayReader& ArrayReader);
	bool		CheckMagic(ArrayBridge<char>&& PngData);

	TColour::Type			GetColourType(SoyPixelsFormat::Type Format);
	SoyPixelsFormat::Type	GetPixelFormatType(TColour::Type Format);
	
	bool		GetPngData(Array<char>& PngData,const SoyPixelsImpl& Image,TCompression::Type Compression);
	bool		GetDeflateData(Array<char>& ChunkData,const ArrayBridge<uint8>& PixelBlock,bool LastBlock,int WindowSize);
	
	bool		ReadHeader(SoyPixelsImpl& Pixels,THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error);
	bool		ReadData(SoyPixelsImpl& Pixels,const THeader& Header,ArrayBridge<char>& Data,std::stringstream& Error);
	bool		ReadTail(SoyPixelsImpl& Pixels,ArrayBridge<char>& Data,std::stringstream& Error);
};
std::ostream& operator<< (std::ostream &out,const TPng::TColour::Type &in);
std::ostream& operator<< (std::ostream &out,const TPng::TFilterNone_ScanlineFilter::Type &in);


