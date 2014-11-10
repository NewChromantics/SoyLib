#pragma once

#include "SoyTypes.h"
#include "Array.hpp"
#include "HeapArray.hpp"


namespace SoyPixelsFormat
{
	enum Type
	{
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
	
		//HSL,
		//HSLA,
	};

	int			GetChannelCount(Type Format);
	Type		GetFormatFromChannelCount(int ChannelCount);
	inline bool	IsValid(Type Format)			{	return GetChannelCount( Format ) > 0;	}
	
	
	int			GetMaxValue(SoyPixelsFormat::Type Format);
	int			GetMinValue(SoyPixelsFormat::Type Format);
	int			GetInvalidValue(SoyPixelsFormat::Type Format);
	int			GetPlayerIndexFirstBit(SoyPixelsFormat::Type Format);
	bool		GetIsFrontToBackDepth(SoyPixelsFormat::Type Format);
};
std::ostream& operator<< (std::ostream &out,const SoyPixelsFormat::Type &in);


//	meta data for pixels (header when using raw data)
class SoyPixelsMeta
{
public:
	SoyPixelsMeta() :
		mFormat		( SoyPixelsFormat::Invalid ),
		mWidth		( 0 )
	{
		assert( sizeof(mFormat)==sizeof(uint32) );
	}

	bool			IsValid() const					{	return (mWidth>0) && SoyPixelsFormat::IsValid(mFormat);	}
	uint8			GetBitDepth() const				{	return 8;	}
	uint8			GetChannels() const				{	return SoyPixelsFormat::GetChannelCount(mFormat);	}
	uint16			GetWidth() const				{	return mWidth;	}
	uint16			GetHeight(int DataSize) const	{	return IsValid() && DataSize>0 ? DataSize / (GetChannels()*mWidth) : 0;	}
	bool			GetOpenglFormat(int& glFormat) const;
	bool			GetOpenclFormat(int& ChannelOrder) const;
	int				GetDataSize(int Height) const	{	return Height * GetChannels() * GetWidth();	}
	void			DumbSetFormat(SoyPixelsFormat::Type Format)	{	mFormat = Format;	}
	void			DumbSetChannels(int Channels)	{	mFormat = SoyPixelsFormat::GetFormatFromChannelCount(Channels);	}
	void			DumbSetWidth(uint16 Width)		{	mWidth = Width;	}
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}


protected:
	//	gr: assuming we will always have a length of data so we can determine height/stride
	SoyPixelsFormat::Type	mFormat;
	uint16					mWidth;
};
DECLARE_NONCOMPLEX_TYPE( SoyPixelsMeta );


//	all the image-manipulation functionality, but data is somewhere else (any array you like, in-place image manipulation!)
class SoyPixelsImpl
{
public:
	virtual ~SoyPixelsImpl()	{}

	bool			Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format);
	bool			Init(uint16 Width,uint16 Height,uint8 Channels);

	bool			Copy(const SoyPixelsImpl& that);
	
	uint16			GetHeight() const				{	return GetMeta().GetHeight( GetPixelsArray().GetSize() );	}
	bool			IsValid() const					{	return GetMeta().IsValid();	}
	uint8			GetBitDepth() const				{	return GetMeta().GetBitDepth();	}
	uint8			GetChannels() const				{	return GetMeta().GetChannels();	}
	uint16			GetWidth() const				{	return GetMeta().GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return GetMeta().GetFormat();	}

	bool			GetPng(ArrayBridge<char>& PngData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>& RawData) const;
	const uint8&	GetPixel(uint16 x,uint16 y,uint16 Channel) const;
	bool			SetPixel(uint16 x,uint16 y,uint16 Channel,const uint8& Component);

	bool			SetFormat(SoyPixelsFormat::Type Format);
	bool			SetChannels(uint8 Channels);
	bool			SetPng(const ArrayBridge<char>& PngData,std::stringstream& Error);
	bool			SetRawSoyPixels(const ArrayBridge<char>& RawData);

	void			ResizeClip(uint16 Width,uint16 Height);
	void			ResizeFastSample(uint16 Width,uint16 Height);

	virtual SoyPixelsMeta&				GetMeta()=0;
	virtual const SoyPixelsMeta&		GetMeta() const=0;
	virtual ArrayBridge<uint8>&			GetPixelsArray()=0;
	virtual const ArrayBridge<uint8>&	GetPixelsArray() const=0;
};

template<class TARRAY>
class SoyPixelsDef : public SoyPixelsImpl
{
public:
	explicit SoyPixelsDef(ArrayBridgeDef<TARRAY> Pixels,SoyPixelsMeta& Meta) :
		mPixels	( Pixels ),
		mMeta	( Meta )
	{
	}

	virtual SoyPixelsMeta&				GetMeta()			{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const		{	return mMeta;	}
	virtual ArrayBridge<uint8>&			GetPixelsArray() override	{	return mPixels;	}
	virtual const ArrayBridge<uint8>&	GetPixelsArray() const override	{	return mPixels;	}

public:
	SoyPixelsMeta&			mMeta;
	ArrayBridgeDef<TARRAY>	mPixels;
};


class TPixels
{
public:
	TPixels(prmem::Heap& Heap=prcore::Heap) :
		mPixels						( Heap )
	{
	}

#if defined(OPENFRAMEWORKS)
	bool		Init(uint16 Width,uint16 Height,uint8 Channels,const ofColour& DefaultColour);
	bool		Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format,const ofColour& DefaultColour);
#endif

#if defined(OPENFRAMEWORKS)
	bool		Get(ofImage& Pixels) const;
	bool		Get(ofPixels& Pixels) const;
	bool		Get(ofxCvImage& Pixels) const;
	bool		Get(ofxCvGrayscaleImage& Pixels,SoyOpenClManager& OpenClManager) const;
	bool		Get(ofTexture& Pixels) const;
#endif

#if defined(SOY_OPENCL)
	bool		Get(msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel,cl_int clMemMode=CL_MEM_READ_WRITE) const;
	bool		Get(msa::OpenCLImage& Pixels,cl_command_queue Queue,cl_int clMemMode=CL_MEM_READ_WRITE) const;
#endif

#if defined(OPENFRAMEWORKS)
	bool		Set(const ofPixels& Pixels);
	bool		Set(ofxCvImage& Pixels);
	bool		Set(const IplImage& Pixels);	//	opencv internal image
#endif
	bool		Set(const TPixels& Pixels);
	bool		Set(const SoyPixelsImpl& Pixels);
#if defined(OPENFRAMEWORKS)
	bool		Set(const msa::OpenCLImage& Pixels,cl_command_queue Queue);
	bool		Set(const msa::OpenCLImage& Pixels,SoyOpenClKernel& Kernel);
#endif
#if defined(SOY_OPENCL)
	bool		SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const ofColour& DefaultColour=ofColour(255,255,255,255));
	bool		SetChannels(uint8 Channels,SoyOpenClManager& OpenClManager,const clSetPixelParams& Params);
#endif
	void		Clear();						//	delete data

#if defined(SOY_OPENCL)
	bool		RgbaToHsla(SoyOpenClManager& OpenClManager);
	bool		RgbaToHsla(SoyOpenClManager& OpenClManager,ofPtr<SoyClDataBuffer>& Hsla) const;
	bool		RgbaToHsla(SoyOpenClManager& OpenClManager,ofPtr<msa::OpenCLImage>& Hsla) const;
#endif	

	uint16			GetHeight() const				{	return Def().GetHeight();	}
	bool			IsValid() const					{	return Def().IsValid();	}
	uint8			GetBitDepth() const				{	return Def().GetBitDepth();	}
	uint8			GetChannels() const				{	return Def().GetChannels();	}
	uint16			GetWidth() const				{	return Def().GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return Def().GetFormat();	}
	virtual SoyPixelsMeta&				GetMeta()			{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const		{	return mMeta;	}
	//virtual ArrayBridge<uint8>			GetPixelsArray() override		{	return GetArrayBridge(mPixels);	}
	//virtual const ArrayBridge<uint8>	GetPixelsArray() const override	{	return GetArrayBridge(mPixels);	}

	SoyPixelsDef<Array<uint8>>			Def()		{	return SoyPixelsDef<Array<uint8>>( GetArrayBridge( mPixels ), mMeta );	}
	const SoyPixelsDef<Array<uint8>>	Def() const	{	return const_cast<TPixels*>(this)->Def();	}

public:
	SoyPixelsMeta	mMeta;
	Array<uint8>	mPixels;
};
DECLARE_TYPE_NAME( TPixels );


class SoyPixels : public SoyPixelsDef<Array<uint8>>
{
public:
	SoyPixels(prmem::Heap& Heap=prcore::Heap) :
		mPixels						( Heap ),
		SoyPixelsDef<Array<uint8>>	( GetArrayBridge( mPixels.mPixels ), mPixels.mMeta )
	{
	}
	
	operator TPixels& ()					{	return mPixels;	}
	operator const TPixels& () const		{	return mPixels;	}

	SoyPixels& operator=(const SoyPixels& that)	{	Copy( that );	return *this;	}
	
public:
	TPixels		mPixels;
};
DECLARE_TYPE_NAME( SoyPixels );

