#pragma once

#include "SoyTypes.h"
#include "Array.hpp"
#include "HeapArray.hpp"
#include "SoyEnum.h"
#include "RemoteArray.h"


class SoyPixelsMeta;
class SoyPixelsImpl;
class TStreamBuffer;



namespace SoyPixelsFormat
{
	enum Type
	{
	//	UnityUnknown	=-1,	//	gr: temp for this project

		Invalid			= 0,
		Greyscale,
		GreyscaleAlpha,		//	png has this for 2 channel, so why not us!
		RGB,
		RGBA,
		ARGB,
		BGRA,
		BGR,

		//	non integer-based channel counts
		KinectDepth,		//	16 bit, so "two channels". 13 bits of depth, 3 bits of user-index
		FreenectDepth10bit,	//	16 bit
		FreenectDepth11bit,	//	16 bit
		FreenectDepthmm,	//	16 bit
	

		//	http://stackoverflow.com/a/6315159/355753
		//	bi planar is luma followed by chroma.
		//	Full range is 0..255
		//	video LUMA range 16-235 (chroma is still 0-255)	http://stackoverflow.com/a/10129300/355753
		//	Y=luma	uv=ChromaUv
		
		//	gr: I want to remove video/full/ntsc etc and have that explicit in a media format
		//	http://www.chromapure.com/colorscience-decoding.asp
		//	the range counts towards luma & chroma
		//	video = NTSC
		//	full = Rec. 709
		//	SMPTE-C = SMPTE 170M 

		//	gr: naming convention; planes seperated by underscore
		Yuv_8_88_Full,		//	8 bit Luma, interleaved Chroma uv plane (uv is half size... reflect this somehow in the name!)
		Yuv_8_88_Ntsc,		//	8 bit Luma, interleaved Chroma uv plane (uv is half size... reflect this somehow in the name!)
		Yuv_8_88_Smptec,		//	8 bit Luma, interleaved Chroma uv plane (uv is half size... reflect this somehow in the name!)
		Yuv_8_8_8_Full,		//	luma, u, v seperate planes (uv is half size... reflect this somehow in the name!)
		Yuv_8_8_8_Ntsc,	//	luma, u, v seperate planes (uv is half size... reflect this somehow in the name!)
		Yuv_8_8_8_Smptec,	//	luma, u, v seperate planes (uv is half size... reflect this somehow in the name!)

		//	gr: YUY2: LumaX,ChromaU,LumaX+1,ChromaV (4:2:2 ratio, 8 bit)
		//		we still treat it like a 2 component format so dimensions match original
		//		(maybe should be renamed YYuv_88 for this reason)
		YYuv_8888_Full,
		YYuv_8888_Ntsc,
		YYuv_8888_Smptec,
		
		//	https://stackoverflow.com/a/22793325/355753
		//	4:2:2, apple call this yuvs
		Yuv_844_Full,
		Yuv_844_Ntsc,
		Yuv_844_Smptec,

		ChromaUV_8_8,		//	8 bit plane, 8 bit plane
		ChromaUV_88,		//	16 bit interleaved plane
		ChromaU_8,			//	single plane
		ChromaV_8,			//	single plane
		

		//	https://github.com/ofTheo/ofxKinect/blob/ebb9075bcb5ab2543220b4dec598fd73cec40904/libs/libfreenect/src/cameras.c
		//	kinect (16bit?) yuv. See if its the same as a standard one 
		uyvy,
		/*
		 int u  = raw_buf[2*i];
			int y1 = raw_buf[2*i+1];
			int v  = raw_buf[2*i+2];
			int y2 = raw_buf[2*i+3];
			int r1 = (y1-16)*1164/1000 + (v-128)*1596/1000;
			int g1 = (y1-16)*1164/1000 - (v-128)*813/1000 - (u-128)*391/1000;
			int b1 = (y1-16)*1164/1000 + (u-128)*2018/1000;
			int r2 = (y2-16)*1164/1000 + (v-128)*1596/1000;
			int g2 = (y2-16)*1164/1000 - (v-128)*813/1000 - (u-128)*391/1000;
			int b2 = (y2-16)*1164/1000 + (u-128)*2018/1000;
			CLAMP(r1)
			CLAMP(g1)
			CLAMP(b1)
			CLAMP(r2)
			CLAMP(g2)
			CLAMP(b2)
			proc_buf[3*i]  =r1;
			proc_buf[3*i+1]=g1;
			proc_buf[3*i+2]=b1;
			proc_buf[3*i+3]=r2;
			proc_buf[3*i+4]=g2;
			proc_buf[3*i+5]=b2;		 */

		Luma_Ntsc,			//	ntsc-range luma plane
		Luma_Smptec,		//	Smptec-range luma plane

		//	2 planes, RGB (palette+length8) Greyscale (indexes)
		//	warning, palette's first byte is the size of the palette! need to work out how to auto skip over this when extracting the plane...
		Palettised_RGB_8,
		Palettised_RGBA_8,
		
		//	to distinguish from RGBA etc
		Float1,
		Float2,
		Float3,
		Float4,
		
		
		//	shorthand names
		//	http://www.fourcc.org/yuv.php
		Luma_Full		= Greyscale,	//	Luma plane of a YUV
		Nv12			= Yuv_8_88_Full,
		I420			= Yuv_8_8_8_Full,
		
		
		Count=99,
	};

	//	gr: consider changing this to either Type, Bytes per channel or bits per channel to handle 16 bit better
	bool			IsFloatChannel(Type Format);
	inline size_t	GetBytesPerChannel(Type Format)		{	return IsFloatChannel(Format) ? 4 : 1;	}
	
	size_t			GetChannelCount(Type Format);
	Type			GetFormatFromChannelCount(size_t ChannelCount);
	void			GetFormatPlanes(Type Format,ArrayBridge<Type>&& PlaneFormats);
	SoyPixelsFormat::Type	GetMergedFormat(SoyPixelsFormat::Type Formata,SoyPixelsFormat::Type Formatb);

	int				GetMaxValue(SoyPixelsFormat::Type Format);
	int				GetMinValue(SoyPixelsFormat::Type Format);
	int				GetInvalidValue(SoyPixelsFormat::Type Format);
	int				GetPlayerIndexFirstBit(SoyPixelsFormat::Type Format);
	bool			GetIsFrontToBackDepth(SoyPixelsFormat::Type Format);
	size_t			GetHeaderSize(SoyPixelsFormat::Type Format);
	void			GetHeaderPalettised(ArrayBridge<uint8>&& Data,size_t& PaletteSize,size_t& TransparentIndex);

	//	merge index & palette into Paletteised_8_8
	void			MakePaletteised(SoyPixelsImpl& PalettisedImage,const SoyPixelsImpl& IndexedImage,const SoyPixelsImpl& Palette,uint8 TransparentIndex);

	//	get alternatives of formats
	Type			GetYuvFull(Type Format);
	Type			GetYuvNtsc(Type Format);
	Type			GetYuvSmptec(Type Format);
	Type			ChangeYuvColourRange(Type Format,Type YuvColourRange);
	Type			GetFloatFormat(Type Format);
	Type			GetByteFormat(Type Format);
	
	DECLARE_SOYENUM( SoyPixelsFormat );
};


//	gr: move all the pixels stuff into a namespace!
class TSoyPixelsCopyParams
{
public:
	//	dumb realloc copy
	TSoyPixelsCopyParams() :
		mAllowRealloc		( true ),
		mAllowWidthClip		( true ),
		mAllowHeightClip	( true ),
		mAllowComponentSwizzle	( true ),
		mFlipSource			( false ),
		mFlipDestination	( false )
	{
	}
	//	no-realloc copy with params
	TSoyPixelsCopyParams(bool AllowWidthClip,bool AllowHeightClip,bool AllowComponentSwizzle,bool FlipSource,bool FlipDestination) :
		mAllowRealloc		( false ),
		mAllowWidthClip		( AllowWidthClip ),
		mAllowHeightClip	( AllowHeightClip ),
		mAllowComponentSwizzle	( AllowComponentSwizzle ),
		mFlipSource			( FlipSource ),
		mFlipDestination	( FlipDestination )
	{
	}
	
	bool	mAllowRealloc;		//	allow change meta & data
	bool	mAllowWidthClip;	//	allow a slow line-by-line copy where widths dont align
	bool	mAllowHeightClip;	//	allow fast memcpy, if height is clipped
	bool	mAllowComponentSwizzle;	//	allow BGRA to go into RGBA
	bool	mFlipSource;
	bool	mFlipDestination;
};



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
	SoyPixelsMeta(size_t Width,size_t Height,SoyPixelsFormat::Type Format) :
		mWidth	( Width ),
		mHeight	( Height ),
		mFormat	( Format )
	{
	}
	
	bool			IsValid() const					{	return (mWidth>0) && (mHeight>0) && SoyPixelsFormat::IsValid(mFormat);	}
	bool			IsValidDimensions() const		{	return (mWidth>0) && (mHeight>0);	}
	uint8			GetBitDepth() const				{	return 8;	}
	
	//	gr: deprecate this! shouldn't ever use it raw
	uint8			GetChannels() const				{	return size_cast<uint8>(SoyPixelsFormat::GetChannelCount(mFormat));	}
	uint8_t			GetBytesPerChannel() const		{	return SoyPixelsFormat::GetBytesPerChannel(mFormat);	}
	bool			IsFloatChannel() const			{	return SoyPixelsFormat::IsFloatChannel(mFormat);	}
	size_t			GetWidth() const				{	return mWidth;	}
	size_t			GetHeight() const				{	return mHeight;	}
	size_t			GetDataSize() const;			//	probes multiple planes to get full data size
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}
	uint8_t			GetPixelDataSize() const		{	return GetChannels() * GetBytesPerChannel();	}
	size_t			GetRowDataSize() const			{	return GetPixelDataSize() * GetWidth();	}
	void			GetPlanes(ArrayBridge<SoyPixelsMeta>&& PlaneFormats,ArrayInterface<uint8>* Data=nullptr) const;	//	extract multiple plane formats where applicable (returns self if one plane)
	void			SplitPlanes(size_t PixelDataSize,ArrayBridge<std::tuple<size_t,size_t,SoyPixelsMeta>>&& PlaneOffsetSizeAndMetas,ArrayInterface<uint8>* Data=nullptr) const;	//	get all the plane split info, asserts if data doesn't align

	//	unsafe funcs. (note: they WERE unsafe...)
	void			DumbSetFormat(SoyPixelsFormat::Type Format)	{	mFormat = Format;	}
	void			DumbSetChannels(size_t Channels)	{	mFormat = SoyPixelsFormat::GetFormatFromChannelCount(Channels);	}
	void			DumbSetWidth(size_t Width)		{	mWidth = Width;	}
	void			DumbSetHeight(size_t Height)	{	mHeight = Height;	}

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
	
private:
	size_t			GetSelfDataSize() const			{	return GetHeight() * GetWidth() * GetChannels() * GetBytesPerChannel();	}

protected:
	//	gr: assuming we will always have a length of data so we can determine height/stride
	SoyPixelsFormat::Type	mFormat;
	size_t					mWidth;
	size_t					mHeight;
};
DECLARE_NONCOMPLEX_TYPE( SoyPixelsMeta );

std::ostream& operator<< (std::ostream &out,const SoyPixelsMeta &in);
std::istream& operator>>( std::istream &in,SoyPixelsMeta &out);

 

//	all the image-manipulation functionality, but data is somewhere else (any array you like, in-place image manipulation!)
class SoyPixelsImpl
{
public:
	virtual ~SoyPixelsImpl()	{}

	void			Init(const SoyPixelsMeta& Meta);
	void			Init(size_t Width,size_t Height,SoyPixelsFormat::Type Format);
	void			Init(size_t Width,size_t Height,size_t Channels);
	void			Clear(bool Dealloc=false);

	virtual void	Copy(const SoyPixelsImpl& that,const TSoyPixelsCopyParams& Params=TSoyPixelsCopyParams());
	
	bool			IsValid() const					{	return GetMeta().IsValid();	}
	uint8			GetBitDepth() const				{	return GetMeta().GetBitDepth();	}
	uint8			GetChannels() const				{	return size_cast<uint8>( GetMeta().GetChannels() );	}
	size_t			GetWidth() const				{	return GetMeta().GetWidth();	}
	size_t			GetHeight() const				{	return GetMeta().GetHeight();	}
	size_t			GetRowPitchBytes() const		{	return sizeof(uint8) * GetChannels() * GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return GetMeta().GetFormat();	}
	void			PrintPixels(const std::string& Prefix,std::ostream& Stream,bool Hex,const char* PixelSuffix) const;

	bool			GetPng(ArrayBridge<char>&& PngData) const	{	return GetPng( PngData );	}
	bool			GetPng(ArrayBridge<char>& PngData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>& RawData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>&& RawData) const	{	return GetRawSoyPixels( RawData );	}

	uint8&			GetPixelPtr(size_t x,size_t y,size_t ChannelOffset);		//	throws if OOB
	const uint8&	GetPixelPtr(size_t x,size_t y,size_t ChannelOffset) const;	//	throws if OOB
	uint8			GetPixel(size_t x,size_t y,size_t Channel) const;
	vec2x<uint8>	GetPixel2(size_t x,size_t y) const;
	vec3x<uint8>	GetPixel3(size_t x,size_t y) const;
	vec4x<uint8>	GetPixel4(size_t x,size_t y) const;
	
	void			SetPixel(size_t x,size_t y,size_t Channel,uint8 Component);
	void			SetPixel(size_t x,size_t y,const vec2x<uint8>& Colour);
	void			SetPixel(size_t x,size_t y,const vec3x<uint8>& Colour);
	void			SetPixel(size_t x,size_t y,const vec4x<uint8>& Colour);
	void			SetPixels(const ArrayBridge<uint8>& Components);
							 
	vec2f			GetUv(size_t PixelIndex) const;
	vec2x<size_t>	GetXy(size_t PixelIndex) const;
	size_t			GetIndex(size_t x,size_t y,size_t ChannelOffset=0) const;	//	throws if OOB

	bool			SetFormat(SoyPixelsFormat::Type Format);
	bool			SetChannels(uint8 Channels);
	bool			SetRawSoyPixels(const ArrayBridge<char>& RawData);
	bool			SetRawSoyPixels(const ArrayBridge<char>&& RawData)	{	return SetRawSoyPixels( RawData );	}

	void			ResizeClip(size_t Width,size_t Height);
	void			ResizeFastSample(size_t Width,size_t Height);
	
	void			Flip();

	//	split these pixels into multiple pixels if there are multiple planes
	void			SplitPlanes(ArrayBridge<std::shared_ptr<SoyPixelsImpl>>&& Planes);
	
	inline bool		operator==(const SoyPixelsImpl& that) const		{	return this == &that;	}
	inline bool		operator==(const SoyPixelsMeta& Meta) const		{	return GetMeta() == Meta;	}
	
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
	static prmem::Heap&				GetDefaultHeap();
	
public:
	SoyPixels(const SoyPixelsImpl& that,prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8>>	( mArray, mMeta )
	{
		Copy( that );
	}
	SoyPixels(prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8>>	( mArray, mMeta )
	{
	}
	SoyPixels(const SoyPixelsMeta& Meta,prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8>>	( mArray, mMeta )
	{
		Init( Meta );
	}
	
	SoyPixels& operator=(const SoyPixels& that)	{	Copy( that );	return *this;	}

public:
	SoyPixelsMeta	mMeta;
	Array<uint8>	mArray;
};
DECLARE_TYPE_NAME( SoyPixels );



//	gr: this currently won't modify an ARRAY properly, so those mutable constructors have been removed
class SoyPixelsRemote : public SoyPixelsImpl
{
public:
	typedef FixedRemoteArray<uint8> TARRAY;
public:
	SoyPixelsRemote() :
		mArray			( nullptr, 0 ),
		mArrayBridge	( mArray ),
		mMeta			( mMutableMeta )
	{
		Soy::Assert( !IsValid(), "should be invalid" );
	}
	SoyPixelsRemote(const SoyPixelsRemote& that) :
		mArray			( nullptr, 0 ),
		mArrayBridge	( mArray ),
		mMeta			( mMutableMeta )
	{
		*this = that;
	}
	explicit SoyPixelsRemote(uint8* Array, size_t Width, size_t Height, size_t DataSize, SoyPixelsFormat::Type Format) :
		mArray			(Array, DataSize),
		mMutableMeta	(Width, Height, Format),
		mMeta			( mMutableMeta ),
		mArrayBridge	(mArray)
	{
	}
	explicit SoyPixelsRemote(uint8* Array, size_t DataSize,const SoyPixelsMeta& Meta) :
		mArray			(Array, DataSize),
		mMutableMeta	( Meta ),
		mMeta			(mMutableMeta),
		mArrayBridge	(mArray)
	{
	}
	//	const cast so we can USE the pointer, but constructor makes it clear the array/source won't be modified correctly
	explicit SoyPixelsRemote(const SoyPixelsImpl& Pixels) :
		mArray			( const_cast<uint8*>(Pixels.GetPixelsArray().GetArray()), Pixels.GetPixelsArray().GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( mMutableMeta ),
		mMutableMeta	( Pixels.GetMeta() )
	{
	}
	/*	this array use won't modify the array properly, jsut the data & meta
	 explicit SoyPixelsRemote(ArrayBridge<uint8>&& Pixels,const SoyPixelsMeta& Meta) :
		mArray			( Pixels.GetArray(), Pixels.GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( Meta )
	 {
	 }
	 */
	explicit SoyPixelsRemote(const ArrayBridge<uint8>&& Pixels,const SoyPixelsMeta& Meta) :
		mArray			( const_cast<uint8*>(Pixels.GetArray()), Pixels.GetDataSize() ),
		mArrayBridge	( mArray ),
		mMutableMeta	( Meta ),
		mMeta			( mMutableMeta )
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
	SoyPixelsMeta&			mMeta;

private:
	SoyPixelsMeta			mMutableMeta;
};



//	like SoyPixelsRemote but modifies underlying array
template<typename TARRAY>
class SoyPixelsBridge : public SoyPixelsImpl
{
public:
	//	const cast so we can USE the pointer, but constructor makes it clear the array/source won't be modified correctly
	explicit SoyPixelsBridge(TARRAY& Array,SoyPixelsMeta& Meta) :
		mArrayBridge	( Array ),
		mMeta			( Meta )
	{
	}
	
	virtual SoyPixelsMeta&					GetMeta() override				{	return mMeta;	}
	virtual const SoyPixelsMeta&			GetMeta() const override		{	return mMeta;	}
	virtual ArrayInterface<uint8>&			GetPixelsArray() override		{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8>&	GetPixelsArray() const override	{	return mArrayBridge;	}
	
public:
	ArrayBridgeDef<TARRAY>	mArrayBridge;
	SoyPixelsMeta&			mMeta;
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

