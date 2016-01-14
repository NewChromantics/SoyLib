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

void Stb::Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer,TReadFunction ReadFunction)
{
#if defined(USE_STB)
	
	//	store popped data in case we abort and need to put it back into the buffer
	Array<char> PoppedData;

	class TLambdaTemp
	{
	public:
		std::function<int(char*,int)>	mReadFunction;
		std::function<void(int)>		mSkipFunction;
		std::function<int()>			mEofFunction;
	};
	
	auto Read = [&PoppedData,&Buffer](char* Destination,int DestinationSize) -> int
	{
		auto PopSize = std::min( size_cast<size_t>(DestinationSize), Buffer.GetBufferedSize() );
		size_t DestinationCounter = 0;
		auto DestinationArray = GetRemoteArray( Destination, DestinationSize, DestinationCounter );
		Buffer.Pop( PopSize, GetArrayBridge(DestinationArray) );
		PoppedData.PushBackArray( DestinationArray );
		return size_cast<int>(DestinationArray.GetSize());
	};
	auto ReadWrapper = [](void* Context,char* Destination,int DestinationSize)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		return LambdaTemp.mReadFunction( Destination, DestinationSize );
	};
	
	auto Skip = [&PoppedData,&Buffer](int SkipSize)
	{
		//	gr: stb doesn't handle no-enough data...
		if ( SkipSize > Buffer.GetBufferedSize() )
		{
			std::stringstream Error;
			Error << "stb trying to skip " << SkipSize << " bytes but only " << Buffer.GetBufferedSize() << " remaining";
			throw Soy::AssertException( Error.str() );
		}
		auto PopSize = std::min( size_cast<size_t>(SkipSize), Buffer.GetBufferedSize() );
		Buffer.Pop( PopSize, GetArrayBridge(PoppedData) );
	};
	auto SkipWrapper = [](void* Context,int SkipSize)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		LambdaTemp.mSkipFunction( SkipSize );
	};
	
	
	auto Eof = [&PoppedData,&Buffer]() -> int
	{
		return Buffer.GetBufferedSize() == 0;
	};
	auto EofWrapper = [](void* Context)
	{
		auto& LambdaTemp = *reinterpret_cast<TLambdaTemp*>(Context);
		return LambdaTemp.mEofFunction();
	};
	
	
	TLambdaTemp LambdaTemp;
	LambdaTemp.mReadFunction = Read;
	LambdaTemp.mSkipFunction = Skip;
	LambdaTemp.mEofFunction = Eof;

	stbi_io_callbacks Callbacks;
	Callbacks.read = ReadWrapper;
	Callbacks.skip = SkipWrapper;
	Callbacks.eof = EofWrapper;
	
	stbi__context Context;
	stbi__start_callbacks( &Context, &Callbacks, &LambdaTemp );
	
	try
	{
		int Width = 0;
		int Height = 0;
		int Channels = 0;
		//	gr: use 0 for "defaults"
		int RequestedChannels = 0;
		auto* DecodedPixels = ReadFunction( &Context, &Width, &Height, &Channels, RequestedChannels );
		if ( !DecodedPixels )
		{
			//	gr: as this is not thread safe, it could come out mangled :( maybe add a lock around error popping before read (maybe have to lock around the whole thing)
			auto* StbError = stbi_failure_reason();
			if ( !StbError )
				StbError = "Unknown error";
			throw Soy::AssertException( StbError );
		}
		
		//	convert output into pixels
		//	gr: have to assume size
		auto Format = SoyPixelsFormat::GetFormatFromChannelCount( Channels );
		SoyPixelsMeta Meta( Width, Height, Format );
		SoyPixelsRemote OutputPixels( DecodedPixels, Meta.GetDataSize(), Meta );
		Pixels.Copy( OutputPixels );
	}
	catch(std::exception& e)
	{
		//	unpop data so we can try again
		Buffer.UnPop( GetArrayBridge(PoppedData) );
		throw;
	}

#else
	throw Soy::AssertException("No STB support, no image reading");
#endif
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
