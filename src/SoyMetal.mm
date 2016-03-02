#include "SoyMetal.h"
#include <SoyDebug.h>
#include <SoyString.h>
#include <SoyOpengl.h>

#if defined(TARGET_IOS)
#define ENABLE_METAL
#elif defined(TARGET_OSX) && defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
//	need 10.11 sdk for metal on osx
#define ENABLE_METAL
#endif

#if defined(ENABLE_METAL)
#import <Metal/Metal.h>
#else

typedef NSUInteger MTLPixelFormat;
enum
{
	MTLPixelFormatBGRA8Unorm,
	MTLPixelFormatBGRA8Unorm_sRGB,
};

@protocol MTLDevice
- (id <MTLCommandQueue>)newCommandQueue;
@end

@protocol MTLCommandBuffer
- (void)presentDrawable:(id <MTLDrawable>)drawable;
@end

#endif

namespace Metal
{
	std::mutex						DevicesLock;
	Array<std::shared_ptr<TDevice>>	Devices;
	

	SoyPixelsFormat::Type			GetPixelFormat(MTLPixelFormat Format);
	MTLPixelFormat					GetPixelFormat(SoyPixelsFormat::Type Format);
	
	std::shared_ptr<TDevice>		GetAnonymousDevice(void* DevicePtr);
}




//	todo: abstract MediaFoundation templated version of this

#define CV_VIDEO_TYPE_META(Enum,SoyFormat)	TCvVideoTypeMeta<MTLPixelFormat>( Enum, #Enum, SoyFormat )
#define CV_VIDEO_INVALID_ENUM		MTLPixelFormatInvalid
template<typename PLATFORMFORMAT>
class TCvVideoTypeMeta
{
public:
	TCvVideoTypeMeta(PLATFORMFORMAT Enum,const char* EnumName,SoyPixelsFormat::Type SoyFormat) :
		mPlatformFormat		( Enum ),
		mName				( EnumName ),
		mSoyFormat			( SoyFormat )
	{
		Soy::Assert( IsValid(), "Expected valid enum - or invalid enum is bad" );
	}
	TCvVideoTypeMeta() :
		mPlatformFormat		( CV_VIDEO_INVALID_ENUM ),
		mName				( "Invalid enum" ),
		mSoyFormat			( SoyPixelsFormat::Invalid )
	{
	}
	
	bool		IsValid() const		{	return mPlatformFormat != CV_VIDEO_INVALID_ENUM;	}
	
	bool		operator==(const PLATFORMFORMAT& Enum) const			{	return mPlatformFormat == Enum;	}
	bool		operator==(const SoyPixelsFormat::Type& Format) const	{	return mSoyFormat == Format;	}
	
public:
	PLATFORMFORMAT			mPlatformFormat;
	SoyPixelsFormat::Type	mSoyFormat;
	std::string				mName;
};




TCvVideoTypeMeta<MTLPixelFormat> PixelFormatMap[] =
{
	CV_VIDEO_TYPE_META( MTLPixelFormatA8Unorm,		SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatR8Unorm,		SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatR8Unorm_sRGB,	SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatR8Snorm,		SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatR8Uint,		SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatR8Sint,		SoyPixelsFormat::Greyscale ),
	CV_VIDEO_TYPE_META( MTLPixelFormatStencil8,		SoyPixelsFormat::Greyscale ),
	
	CV_VIDEO_TYPE_META( MTLPixelFormatRG8Unorm,		SoyPixelsFormat::GreyscaleAlpha ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRG8Unorm_sRGB,	SoyPixelsFormat::GreyscaleAlpha ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRG8Uint,		SoyPixelsFormat::GreyscaleAlpha ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRG8Sint,		SoyPixelsFormat::GreyscaleAlpha ),
	
	CV_VIDEO_TYPE_META( MTLPixelFormatRGBA8Unorm,		SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRGBA8Unorm_sRGB,	SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRGBA8Snorm,		SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRGBA8Uint,		SoyPixelsFormat::RGBA ),
	CV_VIDEO_TYPE_META( MTLPixelFormatRGBA8Sint,		SoyPixelsFormat::RGBA ),
	
	CV_VIDEO_TYPE_META( MTLPixelFormatBGRA8Unorm,		SoyPixelsFormat::BGRA ),
	CV_VIDEO_TYPE_META( MTLPixelFormatBGRA8Unorm_sRGB,	SoyPixelsFormat::BGRA ),

	/*
			 @constant MTLPixelFormatGBGR422
			 @abstract A pixel format where the red and green channels are subsampled horizontally.  Two pixels are stored in 32 bits, with shared red and blue values, and unique green values.
			 @discussion This format is equivelent to YUY2, YUYV, yuvs, or GL_RGB_422_APPLE/GL_UNSIGNED_SHORT_8_8_REV_APPLE.   The component order, from lowest addressed byte to highest, is Y0, Cb, Y1, Cr.  There is no implicit colorspace conversion from YUV to RGB, the shader will receive (Cr, Y, Cb, 1).  422 textures must have a width that is a multiple of 2, and can only be used for 2D non-mipmap textures.  When sampling, ClampToEdge is the only usable wrap mode.
			MTLPixelFormatGBGR422 = 240,
			
			 @constant MTLPixelFormatBGRG422
			 @abstract A pixel format where the red and green channels are subsampled horizontally.  Two pixels are stored in 32 bits, with shared red and blue values, and unique green values.
			 @discussion This format is equivelent to UYVY, 2vuy, or GL_RGB_422_APPLE/GL_UNSIGNED_SHORT_8_8_APPLE. The component order, from lowest addressed byte to highest, is Cb, Y0, Cr, Y1.  There is no implicit colorspace conversion from YUV to RGB, the shader will receive (Cr, Y, Cb, 1).  422 textures must have a width that is a multiple of 2, and can only be used for 2D non-mipmap textures.  When sampling, ClampToEdge is the only usable wrap mode.
			MTLPixelFormatBGRG422 = 241,
*/
};


MTLPixelFormat Metal::GetPixelFormat(SoyPixelsFormat::Type Format)
{
	auto Table = GetRemoteArray( PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
		return CV_VIDEO_INVALID_ENUM;
	
	return Meta->mPlatformFormat;
}

SoyPixelsFormat::Type Metal::GetPixelFormat(MTLPixelFormat Format)
{
	auto Table = GetRemoteArray( PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
		return SoyPixelsFormat::Invalid;
	
	return Meta->mSoyFormat;
}






void Metal::EnumDevices(ArrayBridge<std::shared_ptr<TDevice>>&& Devices)
{
	//	refresh list
	std::lock_guard<std::mutex> Lock( DevicesLock );
	
#if defined(TARGET_OSX)
	NSArray<id<MTLDevice>>* MtlDevices = MTLCopyAllDevices();
	NSArray<id<MTLDevice>>* MtlDevices = MTLCopyAllDevices();
#else
	NSMutableArray<id<MTLDevice>>* MtlDevices = [[NSMutableArray alloc] init];

	auto DefaultDevice = MTLCreateSystemDefaultDevice();
	if ( DefaultDevice )
		[MtlDevices addObject:DefaultDevice];
#endif

	for ( int d=0;	MtlDevices && d<[MtlDevices count];	d++ )
	{
		id<MTLDevice> MtlDevice = [MtlDevices objectAtIndex:d];
		if ( !MtlDevice )
		{
			std::Debug << "Warning: Metal device " << d << "/" << ([MtlDevices count]) << " null " << std::endl;
			continue;
		}
	}

}

std::shared_ptr<Metal::TDevice> Metal::GetAnonymousDevice(void* DevicePtr)
{
	static std::shared_ptr<TDevice> AnonDevice;
	
	if ( AnonDevice && !(*AnonDevice == DevicePtr) )
	{
		throw Soy::AssertException("New anonymous metal device. Handle this");
	}
	
	if ( !AnonDevice )
	{
		AnonDevice.reset( new TDevice(DevicePtr) );
	}
	
	return AnonDevice;
}


Metal::TTexture::TTexture(void* TexturePtr) :
	mTexture	( nullptr )
{
	mTexture = (__bridge MTLTextureRef)TexturePtr;
	Soy::Assert( mTexture != nil, "Expected texture?");

	std::Debug << "Made external metal texture: " << GetMeta() << std::endl;
}
	
SoyPixelsMeta Metal::TTexture::GetMeta() const
{
	if ( !mTexture )
		return SoyPixelsMeta();

	auto Format = Metal::GetPixelFormat( [mTexture pixelFormat] );
	auto Width = [mTexture width];
	auto Height = [mTexture height];
	return SoyPixelsMeta( Width, Height, Format );
}


void Metal::TTexture::Write(SoyPixelsImpl& Pixels,Opengl::TTextureUploadParams& Params,TContext& Context)
{
	auto pJob = Context.AllocJob();
	Soy::Assert( pJob!=nullptr, "Failed to allocate metal job");
	auto& Job = *pJob;

	
	//	gr: copy buffer, note; use newBufferWithBytesNoCopy for no-copy, but need to explicitly alloc with mmap or vm_alloc (malloc will not work!)
	auto& PixelsArray = Pixels.GetPixelsArray();
	auto pBuffer = Context.AllocBuffer( GetArrayBridge(PixelsArray) );
	
	/*
	auto Format = GetPixelFormat( Pixels.GetFormat() );
	NSUInteger Width = Pixels.GetWidth();
	NSUInteger Height = Pixels.GetHeight();
	BOOL MipMaps = false;
	auto* Descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:Format width:Width height:Height mipmapped:MipMaps];
*/
	
	NSUInteger Offset = 0;
	NSUInteger BytesPerRow = Pixels.GetMeta().GetRowDataSize();
	NSUInteger BytesPerImage = Pixels.GetMeta().GetDataSize();
	MTLSize SourceSize = MTLSizeMake( Pixels.GetWidth(), Pixels.GetHeight(), 0 );
	id<MTLTexture> TargetTexture = mTexture;
	NSUInteger DestinationSlice = 0;
	NSUInteger DestinationLevel = 0;
	MTLOrigin DestinationOrigin = MTLOriginMake(0,0,0);
	auto Buffer = pBuffer->GetBuffer();

	//	generate command
	id<MTLBlitCommandEncoder> Command = [Job.GetCommandBuffer() blitCommandEncoder];
 

	[Command copyFromBuffer:Buffer sourceOffset:Offset sourceBytesPerRow:BytesPerRow sourceBytesPerImage:BytesPerImage sourceSize:SourceSize toTexture:TargetTexture destinationSlice:DestinationSlice destinationLevel:DestinationLevel destinationOrigin:DestinationOrigin];
	
	if ( Params.mGenerateMipMaps )
		[Command generateMipmapsForTexture:TargetTexture];
	[Command endEncoding];
	
	Soy::TSemaphore Semaphore;
	Job.Execute( Semaphore );
	Semaphore.Wait();
}



Metal::TDevice::TDevice(void* DevicePtr) :
	mDevice	( nullptr )
{
	mDevice = (MTLDeviceRef)DevicePtr;
	Soy::Assert( mDevice != nullptr, "Null device");
}


Metal::TContext::TContext(void* DevicePtr)
{
	mDevice = GetAnonymousDevice( DevicePtr );
	Soy::Assert( mDevice != nullptr, "Couldnt get anonymous metal device");
	
	//	allocate a new command queue for this thread
	auto Device = mDevice->GetDevice();
	mQueue = [Device newCommandQueue];
	Soy::Assert( mQueue != nullptr, "Failed to allocate queue");
}


bool Metal::TContext::IsLocked(std::thread::id Thread)
{
	//	check for any thread
	if ( Thread == std::thread::id() )
		return mLockedThread!=std::thread::id();
	else
		return mLockedThread == Thread;
}

bool Metal::TContext::Lock()
{
	Soy::Assert( mLockedThread == std::thread::id(), "context already locked" );
	
	mLockedThread = std::this_thread::get_id();
	return true;
}

void Metal::TContext::Unlock()
{
	auto ThisThread = std::this_thread::get_id();
	Soy::Assert( mLockedThread != std::thread::id(), "context not locked to wrong thread" );
	Soy::Assert( mLockedThread == ThisThread, "context not unlocked from wrong thread" );
	mLockedThread = std::thread::id();
}



std::shared_ptr<Metal::TBuffer> Metal::TContext::AllocBuffer(const uint8* Data,size_t DataSize)
{
	Soy::Assert( mDevice != nullptr, "AllocBuffer: Context missing device");

	return std::make_shared<TBuffer>( Data, DataSize, *mDevice );
}

std::shared_ptr<Metal::TJob> Metal::TContext::AllocJob()
{
	return std::make_shared<TJob>( *this );
}



Metal::TBuffer::TBuffer(const uint8* Data,size_t DataSize,TDevice& Device) :
	mBuffer		( nullptr )
{
	MTLResourceOptions Options = MTLResourceCPUCacheModeDefaultCache;
	auto DeviceMtl = Device.GetDevice();
	Soy::Assert( DeviceMtl!=nullptr, "Expected metal device");
	
	mBuffer = [DeviceMtl newBufferWithBytes:Data length:DataSize options:Options];
	if ( !mBuffer )
	{
		std::stringstream Error;
		Error << "Failed to create metal buffer of " << DataSize << " bytes";
		throw Soy::AssertException( Error.str() );
	}
	
	//	gr: is this immediate??
}


Metal::TJob::TJob(TContext& Context) :
	mCommandBuffer	( nullptr )
{
	mCommandBuffer = [Context.GetQueue() commandBuffer];
}


Metal::TJob::~TJob()
{
}

std::string GetBufferError(id<MTLCommandBuffer> Buffer)
{
	return std::string();

	if ( !Buffer )
		return "Buffer is null";
	
	//	look for an error
	NSError* ErrorNs = [Buffer error];
	/*MTLCommandBufferError*/auto Error = ErrorNs ? [ErrorNs code] : MTLCommandBufferErrorNone;
	
	if ( Error != MTLCommandBufferErrorNone )
	{
		std::stringstream ErrorStr;
		ErrorStr << "Error #" << (int)Error;
		return ErrorStr.str();
	}
	
	return std::string();
}


void Metal::TJob::Execute(Soy::TSemaphore* Semaphore)
{
	auto RealOnCompleted = [=](id<MTLCommandBuffer> Buffer)
	{
		//	gr: for some reason, in c# block or lambda, calling OnFailed eventually leads to a crash in debug print (with no stack)
		auto Error = GetBufferError(Buffer);

		if ( Semaphore )
		{
			if ( Error.empty() )
				Semaphore->OnCompleted();
			else
				Semaphore->OnFailed( Error.c_str() );
		}
		else
		{
			if ( Error.empty() )
				std::Debug << "Metal job success" << std::endl;
			else
				std::Debug << "Metal job error " << Error << std::endl;
		}
		
	};
	
	auto OnCompleted = ^(id<MTLCommandBuffer> Buffer)
	{
		RealOnCompleted( Buffer );
	};

	if ( Semaphore )
	{
		[mCommandBuffer addCompletedHandler:OnCompleted];
	}

	[mCommandBuffer commit];
}

