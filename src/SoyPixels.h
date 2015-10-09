#pragma once

#include "SoyTypes.h"
#include "Array.hpp"
#include "HeapArray.hpp"
#include "SoyEnum.h"
#include "RemoteArray.h"


class SoyPixelsMeta;


namespace SoyPixelsFormat
{
	enum Type
	{
		UnityUnknown	=-1,	//	gr: temp for this project

		Invalid			= 0,
		Greyscale		= 1,
		GreyscaleAlpha	= 2,	//	png has this for 2 channel, so why not us!
		RGB				= 3,
		RGBA			= 4,

		//	non integer-based channel counts
		BGRA			= 5,
		BGR				= 6,
		KinectDepth		= 7,	//	16 bit, so "two channels". 13 bits of depth, 3 bits of user-index
		FreenectDepth10bit	= 8,	//	16 bit
		FreenectDepth11bit	= 9,	//	16 bit
		FreenectDepthmm	= 10,	//	16 bit
	

		//	http://stackoverflow.com/a/6315159/355753
		//	bi planar is luma followed by chroma.
		//	Full range is 0..255
		//	video LUMA range 16-235 (chroma is still 0-255)	http://stackoverflow.com/a/10129300/355753
		//	Y=luma	uv=ChromaUv
		Yuv420_Biplanar_Full	= 16,
		Yuv420_Biplanar_Video	= 17,
		Yuv422_Biplanar_Full	= 18,
		Yuv444_Biplanar_Full	= 19,

		LumaFull		= Greyscale,	//	Luma plane of a YUV
		LumaVideo		= 20,			//	Video-range luma plane
		Chroma2			= GreyscaleAlpha,	//	16 bit chroma plane
		
		//	shorthand names for different platforms
		Nv12			= Yuv420_Biplanar_Video,
	};

	size_t		GetChannelCount(Type Format);
	Type		GetFormatFromChannelCount(size_t ChannelCount);
	void		GetFormatPlanes(Type Format,ArrayBridge<Type>&& PlaneFormats);
	void		GetFormatPlanes(SoyPixelsMeta Format,ArrayBridge<SoyPixelsMeta>&& PlaneFormats);
	
	int			GetMaxValue(SoyPixelsFormat::Type Format);
	int			GetMinValue(SoyPixelsFormat::Type Format);
	int			GetInvalidValue(SoyPixelsFormat::Type Format);
	int			GetPlayerIndexFirstBit(SoyPixelsFormat::Type Format);
	bool		GetIsFrontToBackDepth(SoyPixelsFormat::Type Format);
	
	DECLARE_SOYENUM( SoyPixelsFormat );
};

namespace Soy
{
	namespace Platform
	{
#if defined(__OBJC__)
		OSType					GetFormat(SoyPixelsFormat::Type Format);
		SoyPixelsFormat::Type 	GetFormat(OSType Format);
		SoyPixelsFormat::Type 	GetFormat(NSNumber* Format);
		std::string				PixelFormatToString(NSNumber* FormatCv);
		std::string				PixelFormatToString(id FormatCv);
		
#endif
	}
}


//	meta data for pixels (header when using raw data)
class SoyPixelsMeta
{
public:
	SoyPixelsMeta() :
		mFormat		( SoyPixelsFormat::Invalid ),
		mWidth		( 0 ),
		mHeight		( 0 )
	{
	}
	SoyPixelsMeta(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format) :
		mWidth	( Width ),
		mHeight	( Height ),
		mFormat	( Format )
	{
	}
	
	bool			IsValid() const					{	return (mWidth>0) && (mHeight>0) && SoyPixelsFormat::IsValid(mFormat);	}
	bool			IsValidDimensions() const		{	return (mWidth>0) && (mHeight>0);	}
	uint8			GetBitDepth() const				{	return 8;	}
	uint8			GetChannels() const				{	return size_cast<uint8>(SoyPixelsFormat::GetChannelCount(mFormat));	}
	uint16			GetWidth() const				{	return mWidth;	}
	uint16			GetHeight() const				{	return mHeight;	}
	size_t			GetDataSize() const				{	return GetWidth() * GetChannels() * GetHeight();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}
	size_t			GetRowDataSize() const			{	return GetChannels() * GetWidth();	}

	void			DumbSetFormat(SoyPixelsFormat::Type Format)	{	mFormat = Format;	}
	void			DumbSetChannels(size_t Channels)	{	mFormat = SoyPixelsFormat::GetFormatFromChannelCount(Channels);	}
	void			DumbSetWidth(uint16 Width)		{	mWidth = Width;	}
	void			DumbSetHeight(uint16 Height)	{	mHeight = Height;	}

	bool			operator==(const SoyPixelsMeta& that) const
	{
		return this->mWidth == that.mWidth &&
		this->mHeight == that.mHeight &&
		this->mFormat == that.mFormat;
	}
	bool			operator!=(const SoyPixelsMeta& that) const
	{
		return !(*this == that);
	}
	
protected:
	//	gr: assuming we will always have a length of data so we can determine height/stride
	SoyPixelsFormat::Type	mFormat;
	uint16					mWidth;
	uint16					mHeight;
};
DECLARE_NONCOMPLEX_TYPE( SoyPixelsMeta );



inline std::ostream& operator<< (std::ostream &out,const SoyPixelsMeta &in)
{
	out << in.GetWidth() << 'x' << in.GetHeight() << '^' << in.GetFormat();
	return out;
}


//	all the image-manipulation functionality, but data is somewhere else (any array you like, in-place image manipulation!)
class SoyPixelsImpl
{
public:
	virtual ~SoyPixelsImpl()	{}

	bool			Init(const SoyPixelsMeta& Meta);
	bool			Init(size_t Width,size_t Height,SoyPixelsFormat::Type Format);
	bool			Init(size_t Width,size_t Height,size_t Channels);
	void			Clear(bool Dealloc=false);

	virtual bool	Copy(const SoyPixelsImpl& that,bool AllowReallocation=true);
	
	bool			IsValid() const					{	return GetMeta().IsValid();	}
	uint8			GetBitDepth() const				{	return GetMeta().GetBitDepth();	}
	uint8			GetChannels() const				{	return size_cast<uint8>( GetMeta().GetChannels() );	}
	uint16			GetWidth() const				{	return GetMeta().GetWidth();	}
	uint16			GetHeight() const				{	return GetMeta().GetHeight();	}
	size_t			GetRowPitchBytes() const		{	return sizeof(uint8) * GetChannels() * GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return GetMeta().GetFormat();	}
	void			PrintPixels(const std::string& Prefix,std::ostream& Stream,bool Hex,const char* PixelSuffix) const;

	bool			GetPng(ArrayBridge<char>&& PngData) const	{	return GetPng( PngData );	}
	bool			GetPng(ArrayBridge<char>& PngData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>& RawData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>&& RawData) const	{	return GetRawSoyPixels( RawData );	}
	const uint8&	GetPixel(uint16 x,uint16 y,uint16 Channel) const;
	bool			SetPixel(uint16 x,uint16 y,uint16 Channel,const uint8& Component);
	void			SetColour(const ArrayBridge<uint8>& Components);
	vec2f			GetUv(size_t PixelIndex) const;
	vec2x<size_t>	GetXy(size_t PixelIndex) const;

	bool			SetFormat(SoyPixelsFormat::Type Format);
	bool			SetChannels(uint8 Channels);
	void			SetPng(const ArrayBridge<char>& PngData);
	bool			SetRawSoyPixels(const ArrayBridge<char>& RawData);
	bool			SetRawSoyPixels(const ArrayBridge<char>&& RawData)	{	return SetRawSoyPixels( RawData );	}

	void			ResizeClip(uint16 Width,uint16 Height);
	void			ResizeFastSample(uint16 Width,uint16 Height);
	
	void			RotateFlip();

	//	split these pixels into multiple pixels if there are multiple planes
	void			SplitPlanes(ArrayBridge<std::shared_ptr<SoyPixelsImpl>>&& Planes);
	
	virtual SoyPixelsMeta&					GetMeta()=0;
	virtual const SoyPixelsMeta&			GetMeta() const=0;
	virtual ArrayInterface<uint8>&			GetPixelsArray()=0;
	virtual const ArrayInterface<uint8>&	GetPixelsArray() const=0;
};

template<class TARRAY>
class SoyPixelsDef : public SoyPixelsImpl
{
public:
	explicit SoyPixelsDef(TARRAY& Array,SoyPixelsMeta& Meta) :
		mArray	( Array ),
		mArrayBridge	( Array ),
		mMeta	( Meta )
	{
	}

	virtual SoyPixelsMeta&				GetMeta() override					{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const override			{	return mMeta;	}
	virtual ArrayInterface<uint8>&		GetPixelsArray() override			{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8>&	GetPixelsArray() const override	{	return mArrayBridge;	}

public:
	ArrayBridgeDef<TARRAY>	mArrayBridge;
	TARRAY&			mArray;
	SoyPixelsMeta&	mMeta;
};


class SoyPixels : public SoyPixelsDef<Array<uint8>>
{
public:
	SoyPixels(prmem::Heap& Heap=prcore::Heap) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8>>	( mArray, mMeta )
	{
	}
	
	SoyPixels& operator=(const SoyPixels& that)	{	Copy( that );	return *this;	}

public:
	SoyPixelsMeta	mMeta;
	Array<uint8>	mArray;
};
DECLARE_TYPE_NAME( SoyPixels );



class SoyPixelsRemote : public SoyPixelsImpl
{
public:
	typedef FixedRemoteArray<uint8> TARRAY;
public:
	SoyPixelsRemote() :
		mArray			( nullptr, 0 ),
		mArrayBridge	( mArray )
	{
		Soy::Assert( !IsValid(), "should be invalid" );
	}
	SoyPixelsRemote(const SoyPixelsRemote& that) :
		mArray			( nullptr, 0 ),
		mArrayBridge	( mArray )
	{
		*this = that;
	}
	explicit SoyPixelsRemote(uint8* Array, size_t Width, size_t Height, size_t DataSize, SoyPixelsFormat::Type Format) :
		mArray(Array, DataSize),
		mMeta(Width, Height, Format),
		mArrayBridge(mArray)
	{
	}
	explicit SoyPixelsRemote(uint8* Array, size_t DataSize,const SoyPixelsMeta& Meta) :
		mArray(Array, DataSize),
		mMeta(Meta),
		mArrayBridge(mArray)
	{
	}
	explicit SoyPixelsRemote(SoyPixelsImpl& Pixels) :
		mArray			( Pixels.GetPixelsArray().GetArray(), Pixels.GetPixelsArray().GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( Pixels.GetWidth(), Pixels.GetHeight(), Pixels.GetFormat() )
	{
	}
	explicit SoyPixelsRemote(ArrayBridge<uint8>&& Pixels,const SoyPixelsMeta& Meta) :
		mArray			( Pixels.GetArray(), Pixels.GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( Meta )
	{
	}
	
	SoyPixelsRemote&	operator=(const SoyPixelsRemote& that)
	{
		this->mArray = that.mArray;
		this->mMeta = that.mMeta;
		return *this;
	}
	
	virtual SoyPixelsMeta&				GetMeta() override					{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const override			{	return mMeta;	}
	virtual ArrayInterface<uint8>&		GetPixelsArray() override			{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8>&	GetPixelsArray() const override	{	return mArrayBridge;	}
	
public:
	TARRAY					mArray;
	ArrayBridgeDef<TARRAY>	mArrayBridge;
	SoyPixelsMeta			mMeta;
};





//	gr; unsupported for now... 
#if defined(TARGET_WINDOWS)

inline std::ostream& operator<< ( std::ostream &out, const SoyPixels &in )
{
	out.setstate( std::ios::failbit );	
	return out; 
}

inline std::istream& operator>> ( std::istream &in, SoyPixels &out )
{
	in.setstate( std::ios::failbit );
	return in;
}



#endif

