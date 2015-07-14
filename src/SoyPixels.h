#pragma once

#include "SoyTypes.h"
#include "Array.hpp"
#include "HeapArray.hpp"


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
	

		//	special cases for AVF decoder handling
		YCBCR8_Full		= 11,
		YCBCR8_Video	= 12,

		//HSL,
		//HSLA,
	};

	size_t		GetChannelCount(Type Format);
	Type		GetFormatFromChannelCount(size_t ChannelCount);
	inline bool	IsValid(Type Format)			{	return GetChannelCount( Format ) > 0;	}
	
	bool		GetOpenclFormat(int& ChannelOrder,Type Format);
	
	int			GetMaxValue(SoyPixelsFormat::Type Format);
	int			GetMinValue(SoyPixelsFormat::Type Format);
	int			GetInvalidValue(SoyPixelsFormat::Type Format);
	int			GetPlayerIndexFirstBit(SoyPixelsFormat::Type Format);
	bool		GetIsFrontToBackDepth(SoyPixelsFormat::Type Format);
};
std::ostream& operator<< ( std::ostream &out, const SoyPixelsFormat::Type &in );


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
	uint16			GetHeight(size_t DataSize) const	{	return IsValid() && DataSize>0 ? size_cast<uint16>(DataSize / (GetChannels()*mWidth)) : 0;	}
	bool			GetOpenclFormat(int& clFormat) const	{	return SoyPixelsFormat::GetOpenclFormat( clFormat, GetFormat() );	}
	size_t			GetDataSize(size_t Height) const	{	return Height * GetChannels() * GetWidth();	}
	void			DumbSetFormat(SoyPixelsFormat::Type Format)	{	mFormat = Format;	}
	void			DumbSetChannels(size_t Channels)	{	mFormat = SoyPixelsFormat::GetFormatFromChannelCount(Channels);	}
	void			DumbSetWidth(uint16 Width)		{	mWidth = Width;	}
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}


protected:
	//	gr: assuming we will always have a length of data so we can determine height/stride
	SoyPixelsFormat::Type	mFormat;
	uint16					mWidth;
};
DECLARE_NONCOMPLEX_TYPE( SoyPixelsMeta );




class SoyPixelsMetaFull : public SoyPixelsMeta
{
public:
	SoyPixelsMetaFull() :
	mDataSize	( 0 )
	{
	}
	SoyPixelsMetaFull(size_t Width,size_t Height,SoyPixelsFormat::Type Format)
	{
		mWidth = Width;
		mFormat = Format;
		mDataSize = SoyPixelsMeta::GetDataSize(Height);
	}
	
	uint16		GetHeight() const		{	return SoyPixelsMeta::GetHeight( GetDataSize() );	}
	size_t		GetDataSize() const		{	return mDataSize;	}
	
	inline bool	operator==(const SoyPixelsMetaFull& that) const
	{
		return	this->GetHeight() == that.GetHeight() &&
				this->GetWidth() == that.GetWidth() &&
		this->GetFormat() == that.GetFormat();
		
	}
	
public:
	size_t		mDataSize;
};

/*
//	meta data for pixels (header when using raw data)
class SoyPixelsMetaFull
{
public:
	SoyPixelsMeta() :
		mFormat		( SoyPixelsFormat::Invalid ),
		mWidth		( 0 ),
		mHeight		( 0 )
	{
	}
	SoyPixelsMeta(size_t w,size_t h,SoyPixelsFormat::Type Format) :
		mWidth		( w ),
		mHeight		( h ),
		mFormat		( Format )
	{
	}
	
	SoyPixelsFormat::Type	GetFormat() const		{	return mFormat;	}
	size_t			GetWidth() const		{	return mWidth;	}
	size_t			GetHeight() const		{	return mHeight;	}
	
	bool			IsValid() const					{	return mWidth && mHeight && SoyPixelsFormat::IsValid(mFormat);	}
	size_t			GetBitDepth() const				{	return 8;	}
	size_t			GetChannels() const				{	return SoyPixelsFormat::GetChannelCount(mFormat);	}
	size_t			GetDataSize() const				{	return GetHeight() * GetChannels() * GetWidth();	}
	void			DumbSetFormat(SoyPixelsFormat::Type Format)	{	mFormat = Format;	}
	void			DumbSetChannels(int Channels)	{	mFormat = SoyPixelsFormat::GetFormatFromChannelCount(Channels);	}
	void			DumbSetWidth(size_t Width)		{	mWidth = Width;	}
	
	inline bool		operator==(const SoyPixelsMeta& that) const
	{
		return this->mWidth == that.mWidth && this->mHeight == that.mHeight && this->mFormat == that.mFormat;
	}
	
protected:
	//	gr: assuming we will always have a length of data so we can determine height/stride
	SoyPixelsFormat::Type	mFormat;
	size_t					mWidth;
	size_t					mHeight;
};
*/


inline std::ostream& operator<< (std::ostream &out,const SoyPixelsMetaFull &in)
{
	out << in.GetWidth() << 'x' << in.GetHeight() << '^' << in.GetFormat();
	return out;
}


//	all the image-manipulation functionality, but data is somewhere else (any array you like, in-place image manipulation!)
class SoyPixelsImpl
{
public:
	virtual ~SoyPixelsImpl()	{}

	bool			Init(uint16 Width,uint16 Height,SoyPixelsFormat::Type Format);
	bool			Init(uint16 Width,uint16 Height,uint8 Channels);
	void			Clear(bool Dealloc=false);

	virtual bool	Copy(const SoyPixelsImpl& that);
	
	uint16			GetHeight() const				{	return GetMeta().GetHeight( GetPixelsArray().GetSize() );	}
	bool			IsValid() const					{	return GetMeta().IsValid();	}
	uint8			GetBitDepth() const				{	return GetMeta().GetBitDepth();	}
	uint8			GetChannels() const				{	return GetMeta().GetChannels();	}
	uint16			GetWidth() const				{	return GetMeta().GetWidth();	}
	SoyPixelsFormat::Type	GetFormat() const		{	return GetMeta().GetFormat();	}

	bool			GetPng(ArrayBridge<char>& PngData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>& RawData) const;
	bool			GetRawSoyPixels(ArrayBridge<char>&& RawData) const	{	return GetRawSoyPixels( RawData );	}
	const uint8&	GetPixel(uint16 x,uint16 y,uint16 Channel) const;
	bool			SetPixel(uint16 x,uint16 y,uint16 Channel,const uint8& Component);
	void			SetColour(const ArrayBridge<char>& Components);

	bool			SetFormat(SoyPixelsFormat::Type Format);
	bool			SetChannels(uint8 Channels);
	bool			SetPng(const ArrayBridge<char>& PngData,std::stringstream& Error);
	bool			SetRawSoyPixels(const ArrayBridge<char>& RawData);
	bool			SetRawSoyPixels(const ArrayBridge<char>&& RawData)	{	return SetRawSoyPixels( RawData );	}

	void			ResizeClip(uint16 Width,uint16 Height);
	void			ResizeFastSample(uint16 Width,uint16 Height);
	
	void			RotateFlip();
	

	virtual SoyPixelsMeta&				GetMeta()=0;
	virtual const SoyPixelsMeta&		GetMeta() const=0;
	virtual ArrayInterface<char>&		GetPixelsArray()=0;
	virtual const ArrayInterface<char>&	GetPixelsArray() const=0;
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

	virtual SoyPixelsMeta&				GetMeta()			{	return mMeta;	}
	virtual const SoyPixelsMeta&		GetMeta() const		{	return mMeta;	}
	virtual ArrayInterface<char>&		GetPixelsArray()	{	return mArrayBridge;	}
	virtual const ArrayInterface<char>&	GetPixelsArray() const	{	return mArrayBridge;	}

public:
	ArrayBridgeDef<TARRAY>	mArrayBridge;
	TARRAY&			mArray;
	SoyPixelsMeta&	mMeta;
};


class SoyPixels : public SoyPixelsDef<Array<char>>
{
public:
	SoyPixels(prmem::Heap& Heap=prcore::Heap) :
		mArray						( Heap ),
		SoyPixelsDef<Array<char>>	( mArray, mMeta )
	{
	}
	
	SoyPixels& operator=(const SoyPixels& that)	{	Copy( that );	return *this;	}

public:
	SoyPixelsMeta	mMeta;
	Array<char>		mArray;
};
DECLARE_TYPE_NAME( SoyPixels );

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

