#include "SoyMetal.h"
#include <SoyDebug.h>
#include <SoyString.h>
#include <SoyOpengl.h>

template<typename ID_TYPE,typename PROTOCOL_BASE_TYPE=NSObject>
class ObjcWrapper
{
public:
	ObjcWrapper(void* Object);
	ObjcWrapper(ID_TYPE Id_Object);
	~ObjcWrapper();

	//	yet this converts without hassle??
	void*		AsPtr() const	{	return mObject;	}
	operator	ID_TYPE()		{	return mObject;	}
	
	ID_TYPE		mObject;
};


class MTLDevice_impl : public ObjcWrapper<id<MTLDevice>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};

class MTLTexture_impl : public ObjcWrapper<id<MTLTexture>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};

class MTLBuffer_impl : public ObjcWrapper<id<MTLBuffer>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};

class MTLCommandBuffer_impl: public ObjcWrapper<id<MTLCommandBuffer>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};

class MTLCommandQueue_impl : public ObjcWrapper<id<MTLCommandQueue>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};

class MTLCommandEncoder_impl : public ObjcWrapper<id<MTLCommandEncoder>>
{
public:
	using ObjcWrapper::ObjcWrapper;	//	inherit constructors
};


template<typename TYPE_impl>
Objc<TYPE_impl> make_objc(void* TexturePtr)
{
	return Objc<TYPE_impl>( std::make_shared<TYPE_impl>( TexturePtr ) );
}



template<typename ID_TYPE,typename PROTOCOL_BASE_TYPE>
ObjcWrapper<ID_TYPE,PROTOCOL_BASE_TYPE>::ObjcWrapper(void* Object)
{
	//	gr: we throw to stop the shared_ptr of the wrapper being set like a failed alloc
	if ( !Object )
		throw Soy::AssertException("null objc pointer");
		
	//	bridge
	PROTOCOL_BASE_TYPE* ns = (PROTOCOL_BASE_TYPE*)Object;
		
	//	id is anything(void*) but won't take a void*
	id x = ns;
	
	//	can cast anything to a specific thing
	mObject = x;
	[mObject retain];
	std::Debug << Soy::GetTypeName(*this) << " alloc retain count: " << [mObject retainCount] << std::endl;
	
	auto* TestPtr = Object;
	Soy::Assert( TestPtr == Object, "Pointer conversion failed");
	//mObject = (__bridge*)Object;
}


template<typename ID_TYPE,typename PROTOCOL_BASE_TYPE>
ObjcWrapper<ID_TYPE,PROTOCOL_BASE_TYPE>::ObjcWrapper(ID_TYPE Id_Object) :
	mObject	( Id_Object )
{
	if ( !mObject )
		throw Soy::AssertException("null objc object");
}
	

template<typename ID_TYPE,typename PROTOCOL_BASE_TYPE>
ObjcWrapper<ID_TYPE,PROTOCOL_BASE_TYPE>::~ObjcWrapper()
{
	if ( mObject )
	{
		std::Debug << Soy::GetTypeName(*this) << " releasing retain count now: " << ([mObject retainCount])-1 << std::endl;
		[mObject release];
		mObject = nil;
	}
}


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
	
	std::shared_ptr<TDevice>		GetAnonymousDevice(void* DevicePtr);\
	

	//	when using invalid descriptors, metal aborts() rather than throws an exception so catch bad descriptors early
	void							IsOkay(MTLTextureDescriptor* Descriptor,const SoyPixelsMeta& OriginalMeta);
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




static TCvVideoTypeMeta<MTLPixelFormat> Metal_PixelFormatMap[] =
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
	auto Table = GetRemoteArray( Metal_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
		return CV_VIDEO_INVALID_ENUM;
	
	return Meta->mPlatformFormat;
}

SoyPixelsFormat::Type Metal::GetPixelFormat(MTLPixelFormat Format)
{
	auto Table = GetRemoteArray( Metal_PixelFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );
	
	if ( !Meta )
		return SoyPixelsFormat::Invalid;
	
	return Meta->mSoyFormat;
}

void Metal::IsOkay(MTLTextureDescriptor* Descriptor,const SoyPixelsMeta& OriginalMeta)
{
	if ( !Descriptor )
	{
		std::stringstream Error;
		Error << "Null MTLTextureDescriptor from " << OriginalMeta;
		throw Soy::AssertException( Error.str() );
	}
	
	auto MakeError = [&](const char* Error)
	{
		std::stringstream ErrorStr;
		ErrorStr << Error << " from " << OriginalMeta;
		return ErrorStr.str();
	};
	
	//	bad dimensions/pixel format abort()'\s in metal driver/layer
	Soy::Assert( [Descriptor pixelFormat] != MTLPixelFormatInvalid, MakeError("Descriptor has invalid pixelformat") );
	Soy::Assert( [Descriptor width] > 0, MakeError("Descriptor has width=0") );
	Soy::Assert( [Descriptor height] > 0, MakeError("Descriptor has height=0") );
	Soy::Assert( [Descriptor depth] > 0, MakeError("Descriptor has depth=0") );
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
	mTexture		( nullptr ),
	mAutoRelease	( false )
{
	mTexture = make_objc<MTLTexture_impl>( TexturePtr );
	Soy::Assert( mTexture, "Expected texture?");

	std::Debug << "Made external metal texture from void*: " << GetMeta() << std::endl;
}


Metal::TTexture::TTexture(id<MTLTexture> TexturePtr) :
	mTexture		( nullptr ),
	mAutoRelease	( false )
{
	mTexture = make_objc<MTLTexture_impl>( TexturePtr );
	Soy::Assert( mTexture, "Expected texture?");
	
	std::Debug << "Made external metal texture from id<>: " << GetMeta() << std::endl;
}


Metal::TTexture::TTexture(const SoyPixelsMeta& Meta,TContext& Context) :
	mTexture		( nullptr ),
	mAutoRelease	( false )
{
	auto Device = Context.GetDevice();
	auto PixelFormat = GetPixelFormat( Meta.GetFormat() );
	NSUInteger Width = Meta.GetWidth();
	NSUInteger Height = Meta.GetHeight();
	bool MipMapped = false;
	auto* Descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:PixelFormat width:Width height:Height mipmapped:MipMapped];
	Metal::IsOkay(Descriptor,Meta);
	mTexture = make_objc<MTLTexture_impl>( [Device newTextureWithDescriptor:Descriptor] );
	
	if ( !mTexture )
	{
		std::stringstream Error;
		Error << "Failed to create MTLTexture from " << Meta;
		throw Soy::AssertException( Error.str() );
	}	
}

void Metal::TTexture::Delete()
{
	//	gr:this doesn't apply as objects are just refcounted?
	mTexture.reset();
}

SoyPixelsMeta Metal::TTexture::GetMeta() const
{
	if ( !mTexture )
		return SoyPixelsMeta();

	@try
	{
		auto& Texture = mTexture->mObject;
		auto Format = Metal::GetPixelFormat( [Texture pixelFormat] );
		auto Width = [Texture width];
		auto Height = [Texture height];
		return SoyPixelsMeta( Width, Height, Format );
	}
	@catch (NSException *exception)
	{
		throw Soy::AssertException( exception );
	}
}


void Metal::TTexture::Write(const SoyPixelsImpl& Pixels,const Opengl::TTextureUploadParams& Params,TContext& Context)
{
	//	gr: cannot blit pixels that don't align to 64 bytes. Tried to bodge around it, but cannot start a row-copy that's not aligned either. so HAVE to realign :/
	{
		auto RowDataSize = Pixels.GetMeta().GetRowDataSize();
		auto Alignment64 = RowDataSize % 64;
		if ( Alignment64 != 0 )
		{
			auto ChannelCount = Pixels.GetMeta().GetChannels();
			//	how many pixels off are we?
			auto PadRowBytes = 64 - Alignment64;
			auto PadRowPixels = PadRowBytes / ChannelCount;
			auto PadRowAlignment = PadRowBytes % ChannelCount;
			if ( PadRowAlignment != 0 )
			{
				std::stringstream Error;
				Error << "Need to pad blit-to-metal pixels by " << PadRowBytes << " bytes, but doesn't align to channel count (" << ChannelCount << ") " << Pixels.GetMeta();
				throw Soy::AssertException( Error.str() );
			}
			
			std::Debug << "Warning: Metal blit row doesn't align to 64 bytes +" << Alignment64 << " (" << Pixels.GetMeta() << ") Having to pad pixels" << std::endl;
			SoyPixels PixelsPadded;
			PixelsPadded.Copy( Pixels );
			auto NewWidth = Pixels.GetMeta().GetWidth() + PadRowPixels;
			auto NewHeight = Pixels.GetMeta().GetHeight();
			PixelsPadded.ResizeClip( NewWidth, NewHeight );
			Write( PixelsPadded, Params, Context );
			return;
		}
	}


	
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
	//	simple copy
	//	if there is misalignment here, it will blindly put RGB into BGRA etc
	//	gr; need to check dimensions?
	if ( Pixels.GetMeta() == this->GetMeta() )
	{
		NSUInteger Offset = 0;
		NSUInteger BytesPerRow = Pixels.GetMeta().GetRowDataSize();
		NSUInteger BytesPerImage = Pixels.GetMeta().GetDataSize();
		auto Depth = 1;
		MTLSize SourceSize = MTLSizeMake( Pixels.GetWidth(), Pixels.GetHeight(), Depth );
		id<MTLTexture> TargetTexture = mTexture->mObject;
		NSUInteger DestinationSlice = 0;
		NSUInteger DestinationLevel = 0;
		MTLOrigin DestinationOrigin = MTLOriginMake(0,0,0);
		auto Buffer = pBuffer->GetCommandBuffer();

		static bool DoCommand = true;
		
		if ( DoCommand )
		{
			//	generate command
			id<MTLBlitCommandEncoder> Command = [Job.GetCommandBuffer() blitCommandEncoder];
			[Command copyFromBuffer:Buffer sourceOffset:Offset sourceBytesPerRow:BytesPerRow sourceBytesPerImage:BytesPerImage sourceSize:SourceSize toTexture:TargetTexture destinationSlice:DestinationSlice destinationLevel:DestinationLevel destinationOrigin:DestinationOrigin];
			
			if ( Params.mGenerateMipMaps )
				[Command generateMipmapsForTexture:TargetTexture];
			[Command endEncoding];
		}
	}
	else
	{
		//	create other image
		std::stringstream Error;
		Error << "Cannot push pixels(" << Pixels.GetMeta() << ") into other-format texture(" << this->GetMeta() << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	
	Soy::TSemaphore Semaphore;
	Job.Execute( Semaphore );
	Semaphore.Wait();
}


void Metal::TTexture::Write(const TTexture& That,const Opengl::TTextureUploadParams& Params,TContext& Context)
{
	auto pJob = Context.AllocJob();
	Soy::Assert( pJob!=nullptr, "Failed to allocate metal job");
	auto& Job = *pJob;

	auto ThatMeta = That.GetMeta();
	id<MTLTexture> SourceTexture = That.mTexture->mObject;
	NSUInteger SourceSlice = 0;
	NSUInteger SourceLevel = 0;
	MTLOrigin SourceOrigin = MTLOriginMake( 0, 0, 0 );
	auto SourceDepth = 1;
	MTLSize SourceSize = MTLSizeMake( ThatMeta.GetWidth(), ThatMeta.GetHeight(), SourceDepth );

	auto ThisMeta = GetMeta();
	id<MTLTexture> TargetTexture = mTexture->mObject;
	NSUInteger TargetSlice = 0;
	NSUInteger TargetLevel = 0;
	MTLOrigin TargetOrigin = MTLOriginMake( 0, 0, 0 );
	auto TargetDepth = 1;
	MTLSize TargetSize = MTLSizeMake( ThisMeta.GetWidth(), ThisMeta.GetHeight(), TargetDepth );

	//	catch various metal abort's
	Soy::Assert( [SourceTexture pixelFormat] == [TargetTexture pixelFormat], "Source PixelFormat must equal target PixelFormat" );

	//	rect copy
	id<MTLBlitCommandEncoder> Command = [Job.GetCommandBuffer() blitCommandEncoder];
	[Command copyFromTexture:SourceTexture sourceSlice:SourceSlice sourceLevel:SourceLevel sourceOrigin:SourceOrigin sourceSize:SourceSize toTexture:TargetTexture destinationSlice:TargetSlice destinationLevel:TargetLevel destinationOrigin:TargetOrigin];

	if ( Params.mGenerateMipMaps )
		[Command generateMipmapsForTexture:TargetTexture];
	[Command endEncoding];
	
	Soy::TSemaphore Semaphore;
	Job.Execute( Semaphore );
	Semaphore.Wait();
}

SoyPixelsMeta Metal::TTexture::CorrectPixelsForUpload(const SoyPixelsMeta& Meta)
{
	SoyPixelsMeta NewMeta = Meta;
	
	//	gr: cannot blit pixels that don't align to 64 bytes. Tried to bodge around it, but cannot start a row-copy that's not aligned either. so HAVE to realign :/
	{
		auto RowDataSize = NewMeta.GetRowDataSize();
		auto Alignment64 = RowDataSize % 64;
		if ( Alignment64 != 0 )
		{
			auto ChannelCount = NewMeta.GetChannels();
			//	how many pixels off are we?
			auto PadRowBytes = 64 - Alignment64;
			auto PadRowPixels = PadRowBytes / ChannelCount;
			auto PadRowAlignment = PadRowBytes % ChannelCount;
			
			
			if ( PadRowAlignment == 0 )
			{
				SoyPixels PixelsPadded;
				auto NewWidth = NewMeta.GetWidth() + PadRowPixels;
				NewMeta.DumbSetWidth( NewWidth );
			}
			else
			{
				//	cannot realign this format...
				std::stringstream Error;
				Error << "Need to pad blit-to-metal pixels by " << PadRowBytes << " bytes, but doesn't align to channel count (" << ChannelCount << ") " << Meta;
				//throw Soy::AssertException( Error.str() );
				std::Debug << Error.str() << std::endl;
			}
		}
	}
	
	return NewMeta;
}

Metal::TDevice::TDevice(void* DevicePtr) :
	mDevice	( nullptr )
{
	mDevice = make_objc<MTLDevice_impl>(DevicePtr);
}

id<MTLDevice> Metal::TDevice::GetDevice()
{
	return mDevice->mObject;
}

bool Metal::TDevice::operator==(void* DevicePtr) const
{
	return mDevice->AsPtr() == DevicePtr;
}


Metal::TContext::TContext(void* DevicePtr)
{
	mDevice = GetAnonymousDevice( DevicePtr );
	Soy::Assert( mDevice != nullptr, "Couldnt get anonymous metal device");
	
	//	allocate a new command queue for this thread
	auto Device = GetDevice();
	mQueue = make_objc<MTLCommandQueue_impl>( [Device newCommandQueue] );
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

id<MTLDevice> Metal::TContext::GetDevice()
{
	Soy::Assert( mDevice != nullptr, "Device expected");
	auto Device = mDevice->GetDevice();
	Soy::Assert( Device != nullptr, "Device->Device expected");
	return Device;
}

id<MTLCommandQueue> Metal::TContext::GetQueue()
{
	return mQueue->mObject;
}

Metal::TBuffer::TBuffer(const uint8* Data,size_t DataSize,TDevice& Device) :
	mBuffer		( nullptr )
{
	MTLResourceOptions Options = MTLResourceCPUCacheModeDefaultCache;
	auto DeviceMtl = Device.GetDevice();
	Soy::Assert( DeviceMtl!=nullptr, "Expected metal device");
	
	mBuffer = make_objc<MTLBuffer_impl>( [DeviceMtl newBufferWithBytes:Data length:DataSize options:Options] );
	if ( !mBuffer )
	{
		std::stringstream Error;
		Error << "Failed to create metal buffer of " << DataSize << " bytes";
		throw Soy::AssertException( Error.str() );
	}
	
	//	gr: is this immediate??
}

id<MTLBuffer> Metal::TBuffer::GetCommandBuffer()
{
	return mBuffer->mObject;
}


Metal::TJob::TJob(TContext& Context) :
	mCommandBuffer	( nullptr )
{
	mCommandBuffer = make_objc<MTLCommandBuffer_impl>( [Context.GetQueue() commandBuffer] );
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
	static bool SkipSemaphore = false;
	
	if ( SkipSemaphore && Semaphore )
	{
		auto& CommandBuffer = mCommandBuffer->mObject;
		[CommandBuffer commit];
		Semaphore->OnCompleted();
		return;
	}
	
	//	local copy for block (shouldn't need __block, but being explicit)
	__block Soy::TSemaphore* SemaphoreCopy = Semaphore;
	auto OnCompleted = ^(id<MTLCommandBuffer> Buffer)
	{
		//	gr: for some reason, in c# block or lambda, calling OnFailed eventually leads to a crash in debug print (with no stack)
		auto Error = GetBufferError(Buffer);
		
		if ( SemaphoreCopy )
		{
			if ( Error.empty() )
				SemaphoreCopy->OnCompleted();
			else
				SemaphoreCopy->OnFailed( Error.c_str() );
		}
		else
		{
			if ( Error.empty() )
				std::Debug << "Metal job success" << std::endl;
			else
				std::Debug << "Metal job error " << Error << std::endl;
		}
	};

	auto& CommandBuffer = mCommandBuffer->mObject;
	
	if ( Semaphore )
	{
		[CommandBuffer addCompletedHandler:OnCompleted];
	}

	[CommandBuffer commit];
}


id<MTLCommandBuffer> Metal::TJob::GetCommandBuffer()
{
	return mCommandBuffer->mObject;
}
