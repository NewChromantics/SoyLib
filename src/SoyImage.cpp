#include "SoyImage.h"
#include "SoyStream.h"


#define USE_STB
#if defined(USE_STB)
//#define STBI_FAILURE_USERMSG		//	friendly messages (if not always accurate...)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO				//	all soy file access is abstracted so don't allow it in stb

//	gr: on windows we currently get a whole load of extra stb warnings
#pragma warning(push)
#pragma warning(disable: 4312)
#include "stb/stb_image.h"
#pragma warning(pop)

#endif


namespace Stb
{
	typedef std::function<stbi_uc*(stbi__context* s,int* x,int* y,int* comp,int req_comp)> TReadFunction;
	void	Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer,TReadFunction ReadFunction);
	void	Read(SoyPixelsImpl& Pixels,const ArrayBridge<char>& Buffer,TReadFunction ReadFunction);
}


class TLambdaTemp
{
public:
	std::function<int(char*,int)>	mReadFunction;
	std::function<void(int)>		mSkipFunction;
	std::function<int()>			mEofFunction;
};



class StbContext
{
public:
	StbContext(TStreamBuffer& Buffer,bool PopData);
	~StbContext();
	
	std::string		GetError();
	void			Flush();		//	when finished, flush data so on destruction we won't unpop it
	
public:
	TStreamBuffer&	mBuffer;
#if defined(USE_STB)
	TLambdaTemp		mLambdaTemp;
	stbi_io_callbacks	mCallbacks;
	stbi__context	mContext;
#endif
	bool			mPopData;
	Array<char>		mPoppedData;		//	data to restore on error, or unpop if we want to save it
};




StbContext::StbContext(TStreamBuffer& Buffer,bool PopData) :
	mPopData	( PopData ),
	mBuffer		( Buffer )
{
#if defined(USE_STB)
	
	auto Read = [this,&Buffer](char* Destination,int DestinationSize) -> int
	{
		auto PopSize = std::min( size_cast<size_t>(DestinationSize), Buffer.GetBufferedSize() );
		size_t DestinationCounter = 0;
		auto DestinationArray = GetRemoteArray( Destination, DestinationSize, DestinationCounter );
		Buffer.Pop( PopSize, GetArrayBridge(DestinationArray) );
		this->mPoppedData.PushBackArray( DestinationArray );
		return size_cast<int>(DestinationArray.GetSize());
	};
	auto ReadWrapper = [](void* Context,char* Destination,int DestinationSize)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		return LambdaTemp.mReadFunction( Destination, DestinationSize );
	};
	
	auto Skip = [this,&Buffer](int SkipSize)
	{
		//	gr: stb doesn't handle no-enough data...
		if ( SkipSize > Buffer.GetBufferedSize() )
		{
			std::stringstream Error;
			Error << "stb trying to skip " << SkipSize << " bytes but only " << Buffer.GetBufferedSize() << " remaining";
			throw Soy::AssertException( Error.str() );
		}
		auto PopSize = std::min( size_cast<size_t>(SkipSize), Buffer.GetBufferedSize() );
		Buffer.Pop( PopSize, GetArrayBridge(this->mPoppedData) );
	};
	auto SkipWrapper = [](void* Context,int SkipSize)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		LambdaTemp.mSkipFunction( SkipSize );
	};
	
	
	auto Eof = [&Buffer]() -> int
	{
		return Buffer.GetBufferedSize() == 0;
	};
	auto EofWrapper = [](void* Context)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		return LambdaTemp.mEofFunction();
	};
	
	
	mLambdaTemp.mReadFunction = Read;
	mLambdaTemp.mSkipFunction = Skip;
	mLambdaTemp.mEofFunction = Eof;
	
	mCallbacks.read = ReadWrapper;
	mCallbacks.skip = SkipWrapper;
	mCallbacks.eof = EofWrapper;
	
	stbi__start_callbacks( &mContext, &mCallbacks, &mLambdaTemp );
#else
	throw Soy::AssertException("No STB support, no image reading");
#endif
}

StbContext::~StbContext()
{
	//	if we reach here and there's popped data, we have probably error'd and need to restore the data back to the buffer
	//	use Flush() on success
	mBuffer.UnPop( GetArrayBridge(mPoppedData) );
}

void StbContext::Flush()
{
	if ( mPopData )
		mPoppedData.Clear();
}


std::string StbContext::GetError()
{
	//	gr: as this is not thread safe, it could come out mangled :( maybe add a lock around error popping before read (maybe have to lock around the whole thing)
	auto* StbError = stbi_failure_reason();
	if ( !StbError )
		return "Unknown error";

	return StbError;
}


void Stb::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer,TReadFunction ReadFunction)
{
	StbContext Context( Buffer, true );
	int Width = 0;
	int Height = 0;
	int Channels = 0;
	//	gr: use 0 for "defaults"
	int RequestedChannels = 0;
	auto* DecodedPixels = ReadFunction( &Context.mContext, &Width, &Height, &Channels, RequestedChannels );
	if ( !DecodedPixels )
	{
		throw Soy::AssertException( Context.GetError() );
	}
		
	//	convert output into pixels
	//	gr: have to assume size
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	SoyPixelsMeta Meta( Width, Height, Format );
	SoyPixelsRemote OutputPixels( DecodedPixels, Meta.GetDataSize(), Meta );
	Pixels.Copy( OutputPixels );
	
	//	success, don't let context unpop data
	Context.Flush();

}



void Stb::Read(SoyPixelsImpl& Pixels,const ArrayBridge<char>& ArrayBuffer,TReadFunction ReadFunction)
{
#if defined(USE_STB)
	const stbi_uc* Buffer = reinterpret_cast<const stbi_uc*>( ArrayBuffer.GetArray() );
	auto BufferSize = size_cast<int>( ArrayBuffer.GetDataSize() );
	int Width = 0;
	int Height = 0;
	int Channels = 0;
	int RequestedChannels = 4;
	auto* DecodedPixels = stbi_load_from_memory( Buffer, BufferSize, &Width, &Height, &Channels, RequestedChannels );
	if ( !DecodedPixels )
	{
		//	gr: as this is not thread safe, it could come out mangled :( maybe add a lock around error popping before read (maybe have to lock around the whole thing)
		auto* StbError = stbi_failure_reason();
		if ( !StbError )
			StbError = "Unknown error";
		std::stringstream Error;
		Error << "Failed to read image pixels; " << StbError;
		
		throw Soy::AssertException( Error.str() );
	}
	
	//	convert output into pixels
	//	gr: have to assume size
	auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
	SoyPixelsMeta Meta( Width, Height, Format );
	SoyPixelsRemote OutputPixels( DecodedPixels, Meta.GetDataSize(), Meta );
	Pixels.Copy( OutputPixels );
#else
	throw Soy::AssertException("No STB support, no image reading");
#endif
}


void Png::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__png_load );
}

void Png::Read(SoyPixelsImpl& Pixels,const ArrayBridge<char>& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__png_load );
}

/*static */int stbi__process_marker(stbi__jpeg *z, int m);

static int stbi__process_marker_withmeta(stbi__jpeg *z, int m,std::function<void(const stbi_uc*,int,stbi_uc)> EnumMeta)
{
	// check for comment block or APP blocks
	if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
		
		int length = stbi__get16be(z->s)-2;
		if ( EnumMeta )
		{
			auto* Data = (unsigned char*)stbi__malloc( length );
			if (!Data)
			{
				return stbi__err("outofmem", "Out of memory");
			}
			if (!stbi__getn(z->s, Data, length ))
			{
				STBI_FREE(Data);
				return stbi__err("outofmem", "Out of memory");
			}
			EnumMeta( Data, length, m );
			STBI_FREE(Data);
		}
		else
		{
			stbi__skip(z->s, length );
		}
		return 1;
	}
	
	return stbi__process_marker( z, m );
}

static int stbi__decode_jpeg_header_with_meta(stbi__jpeg *z, int scan,std::function<void(const stbi_uc*,int,stbi_uc)> EnumMeta)
{
	int m;
	z->marker = STBI__MARKER_none; // initialize cached marker to empty
	m = stbi__get_marker(z);
	if (!stbi__SOI(m)) return stbi__err("no SOI","Corrupt JPEG");
	if (scan == STBI__SCAN_type) return 1;
	m = stbi__get_marker(z);
	while (!stbi__SOF(m)) {
		if (!stbi__process_marker_withmeta(z,m,EnumMeta)) return 0;
		m = stbi__get_marker(z);
		while (m == STBI__MARKER_none) {
			// some files have extra padding after their blocks, so ok, we'll scan
			if (stbi__at_eof(z->s)) return stbi__err("no SOF", "Corrupt JPEG");
			m = stbi__get_marker(z);
		}
	}
	z->progressive = stbi__SOF_progressive(m);
	if (!stbi__process_frame_header(z, scan)) return 0;
	return 1;
}

void Jpeg::ReadMeta(ArrayBridge<Jpeg::Exif>&& Metas,TStreamBuffer& Buffer)
{
	auto EnumMeta = [&Metas](const stbi_uc* Data,int DataLength,stbi_uc MarkerCode)
	{
		//	EXIF is split by a zero byte
		stbi_uc ExifDelim = 0;
		auto& Meta = Metas.PushBack();
		
		for ( int i=0;	i<DataLength;	i++ )
		{
			if ( Data[i] == 0 )
				break;
			Meta.mKey += static_cast<char>( Data[i] );
		}
		
		//	copy the rest of the data
		auto ExifData = GetRemoteArray( Data, DataLength - Meta.mKey.length() );
		Meta.mData.Copy( ExifData );
	};
	
	StbContext Context( Buffer, false );
	
	auto* s = &Context.mContext;
	stbi__jpeg j;
	j.s = s;
	stbi__setup_jpeg(&j);
	auto r = stbi__decode_jpeg_header_with_meta(&j, STBI__SCAN_load, EnumMeta );
	stbi__rewind(s);
}

void Jpeg::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__jpeg_load );
}


void Gif::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__gif_load );
}

void Tga::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__tga_load );
}

void Bmp::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__bmp_load );
}

void Psd::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer)
{
	Stb::Read( Pixels, Buffer, stbi__psd_load );
}
