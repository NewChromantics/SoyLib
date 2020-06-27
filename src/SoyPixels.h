#pragma once

#include "SoyTypes.h"
#include "Array.hpp"
#include "HeapArray.hpp"
#include "SoyEnum.h"
#include "RemoteArray.h"
#include "SoyVector.h"


class SoyPixelsMeta;
class SoyPixelsImpl;
class TStreamBuffer;



namespace SoyPixelsFormat
{
	enum Type
	{
		Invalid			= 0,
		Greyscale,
		GreyscaleAlpha,		//	png has this for 2 channel, so why not us!
		RGB,
		RGBA,
		ARGB,
		BGRA,
		BGR,

		//	non integer-based channel counts
		KinectDepth,		//	16 bit. 13 bits of depth, 3 bits of user-index
		FreenectDepth10bit,	//	16 bit, 1 channel
		FreenectDepth11bit,	//	16 bit, 1 channel
		Depth16mm,			//	16 bit, 1 channel
		DepthFloatMetres,	//	32bit float
		DepthHalfMetres,	//	16bit float
		DepthDisparityFloat,	//	32bit float - apple true depth camera raw format
		DepthDisparityHalf,		//	16bit float - apple true depth camera raw format

		//	YUV formats;
		//	ditched colourspace, this is now a multi-plane descriptor
		//	note: apple still has multiple colourspace!
		//	Planes seperated by underscore.
		//	Plane dictates bpp (todo: fix 4 so this indicates half resolution)
		
		//	biplane, uv is interleaved byte-by-byte
		//	https://stackoverflow.com/a/22793325/355753
		//	4:2:2, apple call this yuvs
		//	watch out for 2uvy where luma and chroma are backwards to normal!
		//	these are vertically interlaced. need to fix this in the plane splitting code
		//	gr: some times this is NOT vertically interlaced
		//		need to split this into Yuv_844 and Yuv_8_44
		Yuv_8_88,	//	NV12
		Yvu_8_88,	//	NV21
		Uvy_8_88,	//	yuvs	//	gr: should this be Uvy_88_8?

		//	this format has a flaw in that the data is striped, (splits correctly)
		//	but the data is WxH + WxH/2 + WxH/2 so data is 1.5x components so we can't make a buffer size correctly for opengl etc
		//	figure out a way to express virtual w/h, data w/h, non integer components and data size
		//	todo: distinguish striped format!
		Yuv_8_8_8,		//	luma, u, v seperate planes (uv is half size... reflect this somehow in the name!)

		//	gr: YUY2: LumaX,ChromaU,LumaX+1,ChromaV (4:2:2 ratio, 8 bit)
		//		we still treat it like a 2 component format so dimensions match original
		//		(maybe should be renamed YYuv_88 for this reason)
		YYuv_8888,
		//	https://github.com/ofTheo/ofxKinect/blob/ebb9075bcb5ab2543220b4dec598fd73cec40904/libs/libfreenect/src/cameras.c
		//	kinect (16bit?) yuv. See if its the same as a standard one
		uyvy_8888,

		//YuvAlpha_8888_8,	//	a2vy

		ChromaUV_8_8,		//	8 bit plane, 8 bit plane
		ChromaUV_88,		//	16 bit interleaved plane
		ChromaU_8,			//	single plane
		ChromaV_8,			//	single plane

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

		//	2 planes, RGB (palette+length8) Greyscale (indexes)
		//	warning, palette's first byte is the size of the palette! need to work out how to auto skip over this when extracting the plane...
		Palettised_RGB_8,
		Palettised_RGBA_8,
		
		//	to distinguish from RGBA etc
		Float1,
		Float2,
		Float3,
		Float4,
		
		Yuv_8_88_Depth16,	//	Luma plane, chroma plane, depth plane. For a single colour&depth kinect image
		BGRA_Depth16,
		
		Count,
	};
	
	//	magic_enum doesn't work with aliases, so here's so extra aliases
	const static auto Nv12			= Yuv_8_88;
	const static auto Nv21			= Yvu_8_88;
	const static auto I420			= Yuv_8_8_8;
	const static auto Luma			= Greyscale;

	//	gr: consider changing this to either Type, Bytes per channel or bits per channel to handle 16 bit better
	bool				IsFloatChannel(Type Format);
	uint8_t				GetBytesPerChannel(Type Format);
	
	size_t				GetChannelCount(Type Format);
	Type				GetFormatFromChannelCount(size_t ChannelCount);
	void				GetFormatPlanes(Type Format,ArrayBridge<Type>&& PlaneFormats);
	SoyPixelsFormat::Type	GetMergedFormat(SoyPixelsFormat::Type Formata, SoyPixelsFormat::Type Formatb);
	SoyPixelsFormat::Type	GetMergedFormat(SoyPixelsFormat::Type Formata, SoyPixelsFormat::Type Formatb, SoyPixelsFormat::Type FormatC);

	size_t			GetHeaderSize(SoyPixelsFormat::Type Format);
	void			GetHeaderPalettised(const ArrayBridge<uint8_t>&& Data,size_t& PaletteSize,size_t& TransparentIndex);

	//	merge index & palette into Paletteised_8_8
	void			MakePaletteised(SoyPixelsImpl& PalettisedImage,const SoyPixelsImpl& IndexedImage,const SoyPixelsImpl& Palette,uint8_t TransparentIndex);

	Type			GetFloatFormat(Type Format);
	Type			GetByteFormat(Type Format);
	
	std::string		ToString(Type Format);
	Type			ToFormat(const std::string& FormatString);
	inline Type		ToType(const std::string& FormatString)		{	return ToFormat(FormatString);	}
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
	
	bool			IsValid() const					{	return (mWidth>0) && (mHeight>0) && mFormat!=SoyPixelsFormat::Invalid;	}
	bool			IsValidDimensions() const		{	return (mWidth>0) && (mHeight>0);	}
	uint8_t			GetBitDepth() const				{	return 8;	}
	
	//	gr: deprecate this! shouldn't ever use it raw
	uint8_t			GetChannels() const				{	return size_cast<uint8_t>(SoyPixelsFormat::GetChannelCount(mFormat));	}
	uint8_t			GetBytesPerChannel() const		{	return SoyPixelsFormat::GetBytesPerChannel(mFormat);	}
	bool			IsFloatChannel() const			{	return SoyPixelsFormat::IsFloatChannel(mFormat);	}
	size_t			GetWidth() const				{	return mWidth;	}
	size_t			GetHeight() const				{	return mHeight;	}
	size_t			GetDataSize() const;			//	probes multiple planes to get full data size
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}
	uint8_t			GetPixelDataSize() const		{	return GetChannels() * GetBytesPerChannel();	}
	size_t			GetRowDataSize() const			{	return GetPixelDataSize() * GetWidth();	}
	void			GetPlanes(ArrayBridge<SoyPixelsMeta>&& PlaneFormats,const ArrayInterface<uint8_t>* Data=nullptr) const;	//	extract multiple plane formats where applicable (returns self if one plane)
	void			SplitPlanes(size_t PixelDataSize,ArrayBridge<std::tuple<size_t,size_t,SoyPixelsMeta>>&& PlaneOffsetSizeAndMetas,const ArrayInterface<uint8>* Data=nullptr) const;	//	get all the plane split info, asserts if data doesn't align

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
	uint8_t			GetBitDepth() const				{	return GetMeta().GetBitDepth();	}
	uint8_t			GetChannels() const				{	return size_cast<uint8_t>( GetMeta().GetChannels() );	}
	size_t			GetWidth() const				{	return GetMeta().GetWidth();	}
	size_t			GetHeight() const				{	return GetMeta().GetHeight();	}
	size_t			GetRowPitchBytes() const		{	return sizeof(uint8_t) * GetChannels() * GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return GetMeta().GetFormat();	}
	void			PrintPixels(const std::string& Prefix,std::ostream& Stream,bool Hex,const char* PixelSuffix) const;

	bool			GetRawSoyPixels(ArrayBridge<char>& RawData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>&& RawData) const	{	return GetRawSoyPixels( RawData );	}

	uint8_t&			GetPixelPtr(size_t x,size_t y,size_t ChannelOffset);		//	throws if OOB
	const uint8_t&	GetPixelPtr(size_t x,size_t y,size_t ChannelOffset) const;	//	throws if OOB
	uint8_t			GetPixel(size_t x,size_t y,size_t Channel) const;
	vec2x<uint8_t>	GetPixel2(size_t x,size_t y) const;
	vec3x<uint8_t>	GetPixel3(size_t x,size_t y) const;
	vec4x<uint8_t>	GetPixel4(size_t x,size_t y) const;
	
	void			SetPixel(size_t x,size_t y,size_t Channel,uint8_t Component);
	void			SetPixel(size_t x,size_t y,const vec2x<uint8_t>& Colour);
	void			SetPixel(size_t x,size_t y,const vec3x<uint8_t>& Colour);
	void			SetPixel(size_t x,size_t y,const vec4x<uint8_t>& Colour);
	void			SetPixels(const ArrayBridge<uint8_t>& Components);
							 
	vec2f			GetUv(size_t PixelIndex) const;
	vec2x<size_t>	GetXy(size_t PixelIndex) const;
	size_t			GetIndex(size_t x,size_t y,size_t ChannelOffset=0) const;	//	throws if OOB

	void			SetFormat(SoyPixelsFormat::Type Format);
	void			SetChannels(uint8_t Channels);
	bool			SetRawSoyPixels(const ArrayBridge<char>& RawData);
	bool			SetRawSoyPixels(const ArrayBridge<char>&& RawData)	{	return SetRawSoyPixels( RawData );	}

	void			ResizeClip(size_t Width,size_t Height);
	void			ResizeFastSample(size_t Width,size_t Height);
	void			Clip(size_t Left,size_t Top,size_t Width,size_t Height);

	void			Flip();

	//	split these pixels into multiple pixels if there are multiple planes
	void			SplitPlanes(ArrayBridge<std::shared_ptr<SoyPixelsImpl>>&& Planes) const;
	void			AppendPlane(const SoyPixelsImpl& Plane);
	void			AppendPlane(const SoyPixelsImpl& PlaneA,const SoyPixelsImpl& PlaneB);

	inline bool		operator==(const SoyPixelsImpl& that) const		{	return this == &that;	}
	inline bool		operator==(const SoyPixelsMeta& Meta) const		{	return GetMeta() == Meta;	}
	
	virtual SoyPixelsMeta&					GetMeta()=0;
	virtual const SoyPixelsMeta&			GetMeta() const=0;
	virtual ArrayInterface<uint8_t>&			GetPixelsArray()=0;
	virtual const ArrayInterface<uint8_t>&	GetPixelsArray() const=0;
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
	virtual ArrayInterface<uint8_t>&		GetPixelsArray() override			{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8_t>&	GetPixelsArray() const override	{	return mArrayBridge;	}

public:
	ArrayBridgeDef<TARRAY>	mArrayBridge;
	TARRAY&			mArray;
	SoyPixelsMeta&	mMeta;
};


class SoyPixels : public SoyPixelsDef<Array<uint8_t>>
{
public:
	static prmem::Heap&				GetDefaultHeap();
	
public:
	SoyPixels(const SoyPixelsImpl& that,prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8_t>>	( mArray, mMeta )
	{
		Copy( that );
	}
	SoyPixels(prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8_t>>	( mArray, mMeta )
	{
	}
	SoyPixels(const SoyPixelsMeta& Meta,prmem::Heap& Heap=GetDefaultHeap()) :
		mArray						( Heap ),
		SoyPixelsDef<Array<uint8_t>>	( mArray, mMeta )
	{
		Init( Meta );
	}
	
	SoyPixels& operator=(const SoyPixels& that)	{	Copy( that );	return *this;	}

public:
	SoyPixelsMeta	mMeta;
	Array<uint8_t>	mArray;
};
DECLARE_TYPE_NAME( SoyPixels );



//	gr: this currently won't modify an ARRAY properly, so those mutable constructors have been removed
class SoyPixelsRemote : public SoyPixelsImpl
{
public:
	typedef FixedRemoteArray<uint8_t> TARRAY;
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
		CheckDataSize();
	}
	explicit SoyPixelsRemote(uint8_t* Array, size_t Width, size_t Height, size_t DataSize, SoyPixelsFormat::Type Format) :
		mArray			(Array, DataSize),
		mMutableMeta	(Width, Height, Format),
		mMeta			( mMutableMeta ),
		mArrayBridge	(mArray)
	{
		CheckDataSize();
	}
	explicit SoyPixelsRemote(uint8_t* Array, size_t DataSize,const SoyPixelsMeta& Meta) :
		mArray			(Array, DataSize),
		mMutableMeta	( Meta ),
		mMeta			(mMutableMeta),
		mArrayBridge	(mArray)
	{
		CheckDataSize();
	}
	//	const cast so we can USE the pointer, but constructor makes it clear the array/source won't be modified correctly
	explicit SoyPixelsRemote(const SoyPixelsImpl& Pixels) :
		mArray			( const_cast<uint8_t*>(Pixels.GetPixelsArray().GetArray()), Pixels.GetPixelsArray().GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( mMutableMeta ),
		mMutableMeta	( Pixels.GetMeta() )
	{
		CheckDataSize();
	}
	/*	this array use won't modify the array properly, jsut the data & meta
	 explicit SoyPixelsRemote(ArrayBridge<uint8_t>&& Pixels,const SoyPixelsMeta& Meta) :
		mArray			( Pixels.GetArray(), Pixels.GetDataSize() ),
		mArrayBridge	( mArray ),
		mMeta			( Meta )
	 {
	 }
	 */
	explicit SoyPixelsRemote(const ArrayBridge<uint8_t>&& Pixels,const SoyPixelsMeta& Meta) :
		mArray			( const_cast<uint8_t*>(Pixels.GetArray()), Pixels.GetDataSize() ),
		mArrayBridge	( mArray ),
		mMutableMeta	( Meta ),
		mMeta			( mMutableMeta )
	{
		CheckDataSize();
	}

	SoyPixelsRemote&	operator=(const SoyPixelsRemote& that)
	{
		this->mArray = that.mArray;
		this->mMeta = that.mMeta;
		return *this;
	}
	
	virtual SoyPixelsMeta&				GetMeta() override					{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const override			{	return mMeta;	}
	virtual ArrayInterface<uint8_t>&		GetPixelsArray() override			{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8_t>&	GetPixelsArray() const override	{	return mArrayBridge;	}
	
protected:
	void					CheckDataSize();

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
	virtual ArrayInterface<uint8_t>&			GetPixelsArray() override		{	return mArrayBridge;	}
	virtual const ArrayInterface<uint8_t>&	GetPixelsArray() const override	{	return mArrayBridge;	}
	
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

