#include "SoyDirectx.h"
#include "SoyPixels.h"
#include "SoyRuntimeLibrary.h"
#include "SoyPool.h"


//	we never use this, but for old DX11 support, we need the type name
class ID3DX11ThreadPump;


namespace Directx
{
	bool	CanCopyMeta(const SoyPixelsMeta& Source,const SoyPixelsMeta& Destination);

	//	gr: not sure why I can't do this (pD3DCompile is in the SDK header)
	//typedef pD3DCompile D3DCompileFunc;
	typedef HRESULT (__stdcall D3DCompileFunc)(LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);
	typedef void (__stdcall LPD3DX11COMPILEFROMMEMORY)(LPCSTR pSrcData, SIZE_T SrcDataLen, LPCSTR pFileName, const D3D10_SHADER_MACRO *pDefines, LPD3D10INCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, ID3DX11ThreadPump *pPump, ID3D10Blob **ppShader, ID3D10Blob **ppErrorMsgs, HRESULT *pHResult);
}

class Directx::TCompiler
{
public:
	TCompiler();

	//	use win8 sdk d3dcompiler if possible
	//	https://gist.github.com/rygorous/7936047
	std::function<D3DCompileFunc>				GetCompilerFunc()	{	return mD3dCompileFunc;	}
	
private:
	std::function<D3DCompileFunc>			mD3dCompileFunc;
	std::shared_ptr<Soy::TRuntimeLibrary>	mD3dCompileLib;
};

std::map<Directx::TTextureMode::Type,std::string> Directx::TTextureMode::EnumMap = 
{
	{	Directx::TTextureMode::Invalid,			"Invalid"	},
	{	Directx::TTextureMode::ReadOnly,		"ReadOnly"	},
	{	Directx::TTextureMode::WriteOnly,		"WriteOnly"	},
	{	Directx::TTextureMode::GpuOnly,			"GpuOnly"	},
	{	Directx::TTextureMode::RenderTarget,	"RenderTarget"	},
};






#define FORMAT_MAP(SoyFormat,PlatformFormat)	TPlatformFormatMap<DXGI_FORMAT>( PlatformFormat, #PlatformFormat, SoyFormat )
template<typename PLATFORMTYPE,PLATFORMTYPE InvalidValue=DXGI_FORMAT_UNKNOWN>
class TPlatformFormatMap
{
public:
	TPlatformFormatMap(PLATFORMTYPE PlatformFormat,const char* EnumName,SoyMediaFormat::Type SoyFormat) :
		mPlatformFormat	( PlatformFormat ),
		mName			( EnumName ),
		mSoyFormat		( SoyFormat )
	{
		Soy::Assert( IsValid(), "Expected valid enum - or invalid enum is bad" );
	}
	TPlatformFormatMap() :
		mPlatformFormat	( InvalidValue ),
		mName			( "Invalid enum" ),
		mSoyFormat		( SoyPixelsFormat::Invalid )
	{
	}

	bool		IsValid() const		{	return mPlatformFormat != InvalidValue;	}

	bool		operator==(const PLATFORMTYPE& Enum) const				{	return mPlatformFormat == Enum;	}
	bool		operator==(const SoyPixelsFormat::Type& Format) const	{	return *this == SoyMediaFormat::FromPixelFormat(Format);	}
	bool		operator==(const SoyMediaFormat::Type& Format) const	{	return mSoyFormat == Format;	}

public:
	PLATFORMTYPE			mPlatformFormat;
	SoyMediaFormat::Type	mSoyFormat;
	std::string				mName;
};



//	https://msdn.microsoft.com/en-gb/library/windows/desktop/bb173059(v=vs.85).aspx
static TPlatformFormatMap<DXGI_FORMAT> PlatformFormatMap[] =
{
	//	gr; very special case!
//	FORMAT_MAP( SoyPixelsFormat::UnityUnknown,	DXGI_FORMAT_UNKNOWN	),

	FORMAT_MAP( SoyMediaFormat::RGBA,	DXGI_FORMAT_R8G8B8A8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::RGBA,	DXGI_FORMAT_R8G8B8A8_TYPELESS	),
	FORMAT_MAP( SoyMediaFormat::RGBA,	DXGI_FORMAT_R8G8B8A8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::RGBA,	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB	),
	FORMAT_MAP( SoyMediaFormat::RGBA,	DXGI_FORMAT_R8G8B8A8_UINT	),

	FORMAT_MAP( SoyMediaFormat::BGRA,	DXGI_FORMAT_B8G8R8A8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::BGRA,	DXGI_FORMAT_B8G8R8A8_TYPELESS	),
	FORMAT_MAP( SoyMediaFormat::BGRA,	DXGI_FORMAT_B8G8R8A8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::BGRA,	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB	),

	//	gr: these are unsupported natively by directx, so force 32 bit and hav looking for DXGI_FORMAT_
	FORMAT_MAP( SoyMediaFormat::RGB,	DXGI_FORMAT_R8G8B8A8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::BGR,	DXGI_FORMAT_R8G8B8A8_UNORM	),

	FORMAT_MAP( SoyMediaFormat::Yuv_8_88_Full,		DXGI_FORMAT_NV12	),	
	FORMAT_MAP( SoyMediaFormat::Yuv_8_88_Ntsc,		DXGI_FORMAT_NV12	),
	FORMAT_MAP( SoyMediaFormat::Yuv_8_88_Smptec,	DXGI_FORMAT_NV12	),	

	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_R8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_R8_TYPELESS	),
	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_R8_UINT	),
	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_R8_SNORM	),
	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_R8_SINT	),
	FORMAT_MAP( SoyMediaFormat::Greyscale,			DXGI_FORMAT_A8_UNORM	),
			
	FORMAT_MAP( SoyMediaFormat::Luma_Ntsc,			DXGI_FORMAT_R8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::Luma_Smptec,		DXGI_FORMAT_R8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_TYPELESS	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_SNORM	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_UINT	),
	FORMAT_MAP( SoyMediaFormat::GreyscaleAlpha,		DXGI_FORMAT_R8G8_SINT	),
			
	FORMAT_MAP( SoyMediaFormat::ChromaUV_88,		DXGI_FORMAT_R8G8_UNORM	),

	//	_R8G8_B8G8 is a special format for YUY2... but I think it may not be supported on everything
	//	gr: using RG for now and ignoring chroma until we have variables etc... we'll just fix monochrome when someone complains
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Full,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Full,		DXGI_FORMAT_YUY2	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Full,		DXGI_FORMAT_R8G8_B8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Ntsc,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Ntsc,		DXGI_FORMAT_YUY2	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Ntsc,		DXGI_FORMAT_R8G8_B8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Smptec,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Smptec,		DXGI_FORMAT_YUY2	),
	FORMAT_MAP( SoyMediaFormat::YYuv_8888_Smptec,		DXGI_FORMAT_R8G8_B8G8_UNORM	),


//case SoyPixelsFormat::YYuv_8888_Full:		return DXGI_FORMAT_R8G8_B8G8_UNORM;
//case SoyPixelsFormat::YYuv_8888_Ntsc:		return DXGI_FORMAT_R8G8_B8G8_UNORM;
//case SoyPixelsFormat::YYuv_8888_Smptec:		return DXGI_FORMAT_R8G8_B8G8_UNORM;
//	DXGI_FORMAT_YUY2 is failing to bind to a resource...
//case SoyPixelsFormat::YYuv_8888_Full:		return Windows8Plus ? DXGI_FORMAT_YUY2 : DXGI_FORMAT_R8G8_UNORM;
//case SoyPixelsFormat::YYuv_8888_Ntsc:		return Windows8Plus ? DXGI_FORMAT_YUY2 : DXGI_FORMAT_R8G8_UNORM;
//case SoyPixelsFormat::YYuv_8888_Smptec:		return Windows8Plus ? DXGI_FORMAT_YUY2 : DXGI_FORMAT_R8G8_UNORM;

//	other dx formats:
//	https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx
//	DXGI_FORMAT_YUV_444 : DXGI_FORMAT_AYUV	win8+ only
//	DXGI_FORMAT_Y410 YUV_444 10 bit per channel
//	DXGI_FORMAT_Y416 YUV_444 16 bit per channel
//	DXGI_FORMAT_P010  (bi?)planar 4_2_0 10 bit per channel

//	DXGI_FORMAT_P016	yuv_4_2_0	16 bit per channel (bi?) planar
//	Width and height must be even. Direct3D 11 staging resources and initData parameters for this format use (rowPitch * (height + (height / 2))) bytes. The first (SysMemPitch * height) bytes are the Y plane, the remaining (SysMemPitch * (height / 2)) bytes are the UV plane.

//	Width and height must be even. Direct3D 11 staging resources and initData parameters for this format use (rowPitch * (height + (height / 2))) bytes.
//case SoyPixelsFormat::Yuv_844_Full:		return DXGI_FORMAT_420_OPAQUE;

	FORMAT_MAP( SoyMediaFormat::KinectDepth,			DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::FreenectDepth10bit,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::FreenectDepth11bit,		DXGI_FORMAT_R8G8_UNORM	),
	FORMAT_MAP( SoyMediaFormat::FreenectDepthmm,		DXGI_FORMAT_R8G8_UNORM	),
};


SoyMediaFormat::Type Directx::GetFormat(DXGI_FORMAT Format,bool Windows8Plus)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return SoyMediaFormat::Invalid;

	return Meta->mSoyFormat;
}

SoyPixelsFormat::Type Directx::GetPixelFormat(DXGI_FORMAT Format,bool Windows8Plus)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return SoyPixelsFormat::Invalid;

	return SoyMediaFormat::GetPixelFormat( Meta->mSoyFormat );
}

DXGI_FORMAT Directx::GetFormat(SoyPixelsFormat::Type Format,bool Windows8Plus)
{
	auto Table = GetRemoteArray( PlatformFormatMap );
	auto* Meta = GetArrayBridge(Table).Find( Format );

	if ( !Meta )
		return DXGI_FORMAT_UNKNOWN;

	return Meta->mPlatformFormat;
}



bool Directx::CanCopyMeta(const SoyPixelsMeta& Source,const SoyPixelsMeta& Destination)
{
	//	gr: this function could do with a tiny bit of changing for very specific format comparison
	/*
		from CopyResources
		https://msdn.microsoft.com/en-us/library/windows/desktop/ff476392(v=vs.85).aspx
		Must have compatible DXGI formats, which means the formats must be identical or at least 
		from the same type group. For example, a DXGI_FORMAT_R32G32B32_FLOAT texture can be copied 
		to an DXGI_FORMAT_R32G32B32_UINT texture since both of these formats are in the 
		DXGI_FORMAT_R32G32B32_TYPELESS group. CopyResource can copy between a few format types. 
		For more info, see Format Conversion using Direct3D 10.1.
		*/
	return Source == Destination;
}




std::ostream& Directx::operator<<(std::ostream &out,const Directx::TTexture& in)
{
	out << in.mMeta << "/" << in.GetMode();
	return out;
}

std::ostream& operator<<(std::ostream &out,const Directx::TTextureMeta& in)
{
	out << in.mMeta << "/" << in.mMode;
	return out;
}


Directx::TCompiler::TCompiler()
{
	//	dynamically link compiler functions
	//	https://github.com/pathscale/nvidia_sdk_samples/blob/master/matrixMul/common/inc/dynlink_d3d11.h
	//	https://gist.github.com/rygorous/7936047
	
	class DllReference
	{
	public:
		DllReference()	{}
		DllReference(const char* Dll,const char* Func,const char* Desc) :
			LibraryName		( Dll ),
			FunctionName	( Func ),
			Description		( Desc )
		{
		};

		std::string	LibraryName;
		std::string	FunctionName;
		std::string	Description;
	};

	std::function<LPD3DX11COMPILEFROMMEMORY> CompileFromMemoryFunc;
	Array<DllReference> DllFunctions;

	//	https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
	//	remember, 47 & 46 are MUCH FASTER!
	DllFunctions.PushBack( DllReference("d3dcompiler_47.dll","D3DCompile","Win10/Win8.1 sdk") );
	DllFunctions.PushBack( DllReference("d3dcompiler_46.dll","D3DCompile","Win8.0 sdk") );

	DllFunctions.PushBack( DllReference("D3DX11d_43.dll","D3DX11CompileFromMemory","DXSDK 2010 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_42.dll","D3DX11CompileFromMemory","DXSDK 2010 Feb") );
	DllFunctions.PushBack( DllReference("D3DX11d_41.dll","D3DX11CompileFromMemory","DXSDK 2009 March") );
	DllFunctions.PushBack( DllReference("D3DX11d_40.dll","D3DX11CompileFromMemory","DXSDK 2008 November") );
	DllFunctions.PushBack( DllReference("D3DX11d_39.dll","D3DX11CompileFromMemory","DXSDK 2008 August") );
	DllFunctions.PushBack( DllReference("D3DX11d_38.dll","D3DX11CompileFromMemory","DXSDK 2008 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_37.dll","D3DX11CompileFromMemory","DXSDK 2008 March") );
	DllFunctions.PushBack( DllReference("D3DX11d_36.dll","D3DX11CompileFromMemory","DXSDK 2007 November") );
	DllFunctions.PushBack( DllReference("D3DX11d_35.dll","D3DX11CompileFromMemory","DXSDK 2007 August") );
	DllFunctions.PushBack( DllReference("D3DX11d_34.dll","D3DX11CompileFromMemory","DXSDK 2007 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_33.dll","D3DX11CompileFromMemory","DXSDK 2007 Aprli") );

	for ( int d=0;	d<DllFunctions.GetSize();	d++ )
	{
		auto& Dll = DllFunctions[d];
		try
		{
			mD3dCompileLib.reset( new Soy::TRuntimeLibrary(Dll.LibraryName) );

			if ( Dll.FunctionName == "D3DCompile" )
				mD3dCompileLib->SetFunction( mD3dCompileFunc, Dll.FunctionName.c_str() );
			else
				mD3dCompileLib->SetFunction( CompileFromMemoryFunc, Dll.FunctionName.c_str() );
			std::Debug << "Using DX compiler from " << Dll.Description << " (" << Dll.LibraryName << ")" << std::endl;
			break;
		}
		catch(std::exception& e)
		{
			std::Debug << "Failed to load " << Dll.FunctionName << " from " << Dll.LibraryName << " (" << Dll.Description << ")" << std::endl;

			mD3dCompileLib.reset();
			mD3dCompileFunc = nullptr;
			CompileFromMemoryFunc = nullptr;
		}
	}

	//	assigned direct func
	if ( mD3dCompileFunc )
		return;

	//	need a wrapper to old func
	if ( !mD3dCompileFunc && CompileFromMemoryFunc )
	{
		std::function<D3DCompileFunc> WrapperFunc = [CompileFromMemoryFunc](LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
		{
			//	make wrapper!
			//	https://msdn.microsoft.com/en-us/library/windows/desktop/ff476262(v=vs.85).aspx
			ID3DX11ThreadPump* Pump = nullptr;
			const char* Source = reinterpret_cast<const char*>( pSrcData );
			auto* Profile = pTarget;
			HRESULT Result = S_FALSE;
			CompileFromMemoryFunc( Source, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, Profile, Flags1, Flags2, Pump, ppCode, ppErrorMsgs, &Result );
			return Result;
		};
		mD3dCompileFunc = WrapperFunc;
		return;
	}

	//	didn't find any func
	//	gr: should this throw here? or during compile...as current code does...
//	throw Soy::AssertException("Failed to load directx shader compiler");
	std::function<D3DCompileFunc> UnsupportedFunc = [](LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
	{
		throw Soy::AssertException( "D3D shader compiling unsupported." );
		return (HRESULT)S_FALSE;
	};
	mD3dCompileFunc = UnsupportedFunc;
}



void GetBlobString(ID3DBlob* Blob,std::ostream& String)
{
	if ( !Blob )
		return;

	Array<char> Buffer;
	auto BlobSize = Blob->GetBufferSize();
	auto* BlobData = reinterpret_cast<char*>( Blob->GetBufferPointer() );
	if ( !BlobData )
	{
		String << "<BlobBufferMissing[" << BlobSize << "]>";
		return;
	}

	//	clip size to something sensible
	if ( BlobSize > 1024*1024*1 )
		BlobSize = 1024*1024*1;

	auto BlobBuffer = GetRemoteArray( BlobData, BlobSize );
	String << BlobBuffer.GetArray();
}






Directx::TContext::TContext(ID3D11Device& Device) :
	mDevice			( &Device ),
	mLockCount		( 0 )
{
	//	gr: just pre-empting for testing, could be done on-demand
	mCompiler.reset( new TCompiler );
}

ID3D11DeviceContext& Directx::TContext::LockGetContext()
{
	if ( !Lock() )
		throw Soy::AssertException("Failed to lock context");

	return *mLockedContext.mObject;
}

ID3D11Device& Directx::TContext::LockGetDevice()
{
	Soy::Assert( mDevice!=nullptr, "Device expected" );

	if ( !Lock() )
		throw Soy::AssertException("Failed to lock context");

	return *mDevice;
}

bool Directx::TContext::Lock()
{
	if ( mLockedContext )
	{
		mLockCount++;
		return true;
	}

	mDevice->GetImmediateContext( &mLockedContext.mObject );
	if ( !Soy::Assert( mLockedContext!=nullptr, "Failed to get immediate context" ) )
		return false;
	Soy::Assert( mLockCount == 0, "Lock count is not zero");
	mLockCount++;

	return true;
}

void Directx::TContext::Unlock()
{
	Soy::Assert( mLockedContext!=nullptr, "Context not locked!" );
	Soy::Assert( mLockCount>0, "Context lock counter invalid!" );

	mLockCount--;

	if ( mLockCount == 0 )
	{
		mLockedContext.Release();
	}
}

Directx::TCompiler& Directx::TContext::GetCompiler()
{
	if ( !mCompiler )
		mCompiler.reset( new TCompiler );
	return *mCompiler;
}

Directx::TTexture::TTexture(SoyPixelsMeta Meta,TContext& ContextDx,TTextureMode::Type Mode)
{
	static bool AutoMipMap = false;
	bool IsWindows8 = Platform::GetWindowsVersion() >= 8;

	Soy::Assert( Meta.IsValid(), "Cannot create texture with invalid meta");
	
	//	create description
	D3D11_TEXTURE2D_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));
	Desc.Width = Meta.GetWidth();
	Desc.Height = Meta.GetHeight();
	Desc.MipLevels = AutoMipMap ? 0 : 1;
	Desc.ArraySize = 1;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;

	if ( Mode == TTextureMode::RenderTarget )
	{
		Desc.Format = GetFormat( Meta.GetFormat(), IsWindows8 );//DXGI_FORMAT_R32G32B32A32_FLOAT;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = 0;
	}
	else if ( Mode == TTextureMode::WriteOnly )
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.Format = GetFormat( Meta.GetFormat(), IsWindows8  );
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else if ( Mode == TTextureMode::ReadOnly )
	{
		Desc.Usage = D3D11_USAGE_STAGING;
		Desc.Format = GetFormat( Meta.GetFormat(), IsWindows8  );
		Desc.BindFlags = 0;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	}
	else if ( Mode == TTextureMode::GpuOnly )
	{
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.Format = GetFormat( Meta.GetFormat(), IsWindows8  );
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = 0;
	}
	else
	{
		throw Soy::AssertException("Dont know this texture mode");
	}

	ID3D11Texture2D* pTexture = nullptr;
	auto& Context = ContextDx.LockGetContext();
	Soy::Assert( ContextDx.mDevice, "Device expected");
	auto& Device = *ContextDx.mDevice;

	try
	{
		auto Result = Device.CreateTexture2D( &Desc, nullptr, &mTexture.mObject );
		std::stringstream Error;
		Error << "Creating texture2D(" << Meta << ")";
		Directx::IsOkay( Result, Error.str() );
		Soy::Assert( mTexture, "CreateTexture succeeded, but no texture");
		mTexture->AddRef();
		mMeta = Meta;
		mFormat = Desc.Format;
	}
	catch( std::exception& e)
	{
		ContextDx.Unlock();
		throw;
	}
	ContextDx.Unlock();
}

Directx::TTexture::TTexture(ID3D11Texture2D* Texture) :
	mFormat	( DXGI_FORMAT_UNKNOWN )
{
	//	validate and throw here
	Soy::Assert( Texture != nullptr, "null directx texture" );

	mTexture.Set( Texture, true );

	//	get meta
	D3D11_TEXTURE2D_DESC SrcDesc;
	mTexture->GetDesc( &SrcDesc );
	mMeta = SoyPixelsMeta( SrcDesc.Width, SrcDesc.Height, GetPixelFormat( SrcDesc.Format ) );
	mFormat = SrcDesc.Format;

	//	todo: copy sample params from Description
}

Directx::TTexture::TTexture(const TTexture& Texture) :
	TTexture	( Texture.mTexture.mObject )
{
}

bool Directx::TTexture::CanBindToShaderUniform() const
{
	auto Mode = GetMode();
	return Mode == TTextureMode::GpuOnly;
}


Directx::TTextureMode::Type Directx::TTexture::GetMode() const
{
	//	get meta
	D3D11_TEXTURE2D_DESC SrcDesc;
	mTexture.mObject->GetDesc( &SrcDesc );
	bool IsRenderTarget = (SrcDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0;
	bool IsWritable = (SrcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0;
	bool IsReadable = (SrcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0;

	//	gr: note, we're not covering IsWritable && IsReadable

	if ( IsRenderTarget )
	{
		return TTextureMode::RenderTarget;
	}
	else if ( IsWritable )
	{
		return TTextureMode::WriteOnly;
	}
	else if ( IsReadable )
	{
		return TTextureMode::ReadOnly;
	}
	else
	{
		return TTextureMode::GpuOnly;
	}
}

void Directx::TTexture::Write(const TTexture& Source,TContext& ContextDx)
{
	Soy::Assert( IsValid(), "Writing to invalid texture" );
	Soy::Assert( Source.IsValid(), "Writing from invalid texture" );

	//	try simple no-errors-reported-by-dx copy resource fast path
	if ( CanCopyMeta( mMeta, Source.mMeta ) )
	{
		auto& Context = ContextDx.LockGetContext();

		const ID3D11Texture2D* Resource = Source.mTexture;
		auto* ResourceMutable = const_cast<ID3D11Texture2D*>( Resource );

		Context.CopyResource( mTexture, ResourceMutable );
		ContextDx.Unlock();
		return;
	}

	std::stringstream Error;
	Error << "No path to copy " << (*this) << " to " << Source;
	throw Soy::AssertException( Error.str() );
}


Directx::TLockedTextureData Directx::TTexture::LockTextureData(TContext& ContextDx,bool WriteAccess)
{
	Soy::Assert( IsValid(), "Writing to invalid texture" );
	auto& Context = ContextDx.LockGetContext();

	bool Blocking = true;

	//	update our dynamic texture
	try
	{
		D3D11_TEXTURE2D_DESC SrcDesc;
		mTexture->GetDesc(&SrcDesc);

		D3D11_MAPPED_SUBRESOURCE resource;
		ZeroMemory( &resource, sizeof(resource) );
		int SubResource = 0;
		auto ContextType = Context.GetType();
		bool IsDefferedContext = ( ContextType == D3D11_DEVICE_CONTEXT_DEFERRED );
		
		bool CanWrite = SrcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE;
		bool CanRead = SrcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ;
		Soy::Assert( WriteAccess || CanRead, "Texture does not have CPU read mapping access");
		Soy::Assert( !WriteAccess || CanWrite, "Texture does not have CPU write mapping access");

		//	https://msdn.microsoft.com/en-us/library/windows/desktop/ff476457(v=vs.85).aspx
		//	particular case which doesn't map to a resource and needs to be written to in a different way 
		if ( SrcDesc.Usage == D3D11_USAGE_DEFAULT && ContextType == D3D11_DEVICE_CONTEXT_IMMEDIATE )
		{
			int MapFlags = 0x0;
			D3D11_MAP MapMode = D3D11_MAP_WRITE_DISCARD;
			HRESULT hr = Context.Map(mTexture, SubResource, MapMode, MapFlags, nullptr );

			std::stringstream ErrorString;
			ErrorString << "Failed to get Map() for immediate/usage_default dynamic texture(" << SrcDesc.Width << "," << SrcDesc.Height << ")";
			Directx::IsOkay(hr, ErrorString.str());

			Context.Unmap(nullptr, SubResource);
			throw Soy::AssertException("Cannot map non read/write(D3D11_USAGE_DEFAULT) texture");
		}
		else
		{
			static bool ForcedDefferedContext = false;
			if ( ForcedDefferedContext )
				IsDefferedContext = true;

			int MapFlags;
			D3D11_MAP MapMode;

			if ( IsDefferedContext )
			{
				MapFlags = 0x0;

				if ( WriteAccess )
					MapMode = D3D11_MAP_WRITE_DISCARD;
				else
					MapMode = D3D11_MAP_READ;
			}
			else
			{
				MapFlags = !Blocking ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0x0;
				if ( WriteAccess )
				{
					//	gr: for immediate context, we ALSO want write_discard
					//MapMode = D3D11_MAP_WRITE;
					MapMode = D3D11_MAP_WRITE_DISCARD;
				}
				else
				{
					MapMode = D3D11_MAP_READ;
				}
			}

			HRESULT hr = Context.Map(mTexture, SubResource, MapMode, MapFlags, &resource);

			//	specified do not block, and GPU is using the texture
			if ( !Blocking && hr == DXGI_ERROR_WAS_STILL_DRAWING )
			{
				throw Soy::AssertException("GPU is still using texture during map() and we specified non-blocking");
			}

			//	check for other error
			{
				std::stringstream ErrorString;
				ErrorString << "Failed to get Map() for dynamic texture(" << SrcDesc.Width << "," << SrcDesc.Height << ")";
				Directx::IsOkay(hr, ErrorString.str());
			}

			//	depth pitch is one slice, so contains the resource's full data. 
			//	double check in case its zero though...
			size_t ResourceDataSize = resource.DepthPitch;
			if ( ResourceDataSize == 0 )
				ResourceDataSize = resource.RowPitch * SrcDesc.Height;

			auto Unlock = [this, SubResource, &Context,&ContextDx]
			{
				Context.Unmap( mTexture, SubResource);
				ContextDx.Unlock();
			};

			SoyPixelsMeta ResourceMeta( SrcDesc.Width, SrcDesc.Height, GetPixelFormat(SrcDesc.Format) );

			return TLockedTextureData( resource.pData, ResourceDataSize, ResourceMeta, resource.RowPitch, Unlock );
		}
	}
	catch (std::exception& e)
	{
		//	unlock and re-throw
		ContextDx.Unlock();
		throw;
	}
}


void Directx::TTexture::Write(const SoyPixelsImpl& SourcePixels,TContext& ContextDx)
{
	auto Lock = LockTextureData( ContextDx, true );

	//	copy row by row to handle misalignment
	SoyPixelsRemote DestPixels( reinterpret_cast<uint8*>(Lock.mData), Lock.GetPaddedWidth(), Lock.mMeta.GetHeight(), Lock.mSize, Lock.mMeta.GetFormat() );

	DestPixels.Copy( SourcePixels, TSoyPixelsCopyParams(true,true,true) );
}


void Directx::TTexture::Read(SoyPixelsImpl& DestPixels,TContext& ContextDx,TPool<TTexture>* pTexturePool)
{
	//	not readable, so copy to a temp texture first and then read
	//	gr: use try/catch on the lock to cover more cases?
	if ( GetMode() != TTextureMode::ReadOnly && pTexturePool )
	{
		auto& TexturePool = *pTexturePool;
		auto Meta = TTextureMeta(this->GetMeta(), TTextureMode::ReadOnly);

		auto Alloc = [this,&ContextDx]()
		{
			return std::make_shared<TTexture>(this->GetMeta(), ContextDx, TTextureMode::ReadOnly);
		};

		auto& TempTexture = TexturePool.Alloc(Meta, Alloc);
		TempTexture.Write(*this, ContextDx);
		//	no pool!
		TempTexture.Read(DestPixels, ContextDx);
		TexturePool.Release(TempTexture);
		return;
	}

	auto Lock = LockTextureData( ContextDx, false );

	DestPixels.Init( Lock.mMeta );

	//	copy row by row to handle misalignment
	SoyPixelsRemote SourcePixels( reinterpret_cast<uint8*>(Lock.mData), Lock.GetPaddedWidth(), Lock.mMeta.GetHeight(), Lock.mSize, Lock.mMeta.GetFormat() );
	
	DestPixels.Copy( SourcePixels, TSoyPixelsCopyParams(true,true,true) );
}

Directx::TRenderTarget::TRenderTarget(TTexture& Texture,TContext& ContextDx) :
	mTexture		( Texture )
{
	Soy::Assert(mTexture.IsValid(), "Render target needs a valid texture target" );
	auto& Meta = mTexture.GetMeta();
	
	bool IsWindows8 = Platform::GetWindowsVersion() >= 8;

	// Create the render target view.
	auto& Device = ContextDx.LockGetDevice();
	try
	{
		auto* TextureDx = mTexture.mTexture.mObject;
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format = GetFormat( Meta.GetFormat(), IsWindows8 );
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;
		{
			auto Result = Device.CreateRenderTargetView( TextureDx, &renderTargetViewDesc, &mRenderTargetView.mObject );
			std::stringstream Error;
			Error << "CreateRenderTargetView(" << Texture << ")";
			Directx::IsOkay( Result, Error.str() );
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.Format = GetFormat( Meta.GetFormat(), IsWindows8 );
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		{
			auto Result = Device.CreateShaderResourceView( TextureDx, &shaderResourceViewDesc, &mShaderResourceView.mObject );
			std::stringstream Error;
			Error << "CreateShaderResourceView(" << Texture << ")";
			Directx::IsOkay( Result, Error.str() );
		}

		ContextDx.Unlock();
	}
	catch(std::exception& e)
	{
		ContextDx.Unlock();
		throw;
	}
}

void Directx::TRenderTarget::Bind(TContext& ContextDx)
{
	auto& Context = ContextDx.LockGetContext();

	//	save rendertargets to restore
	mRestoreRenderTarget.Release();
	mRestoreStencilTarget.Release();
	Context.OMGetRenderTargets( 1, &mRestoreRenderTarget.mObject, &mRestoreStencilTarget.mObject );

	//	set new render target
	Context.OMSetRenderTargets( 1, &mRenderTargetView.mObject, nullptr );
	
	//	set viewport
	D3D11_VIEWPORT Viewport;
	ZeroMemory(&Viewport, sizeof(D3D11_VIEWPORT));

	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = GetMeta().GetWidth();
	Viewport.Height = GetMeta().GetHeight();

	Context.RSSetViewports( 1, &Viewport );
	
	ContextDx.Unlock();
}

void Directx::TRenderTarget::Unbind(TContext& ContextDx)
{
	//	restore pre-bind render targets
	auto& Context = ContextDx.LockGetContext();

	Context.OMSetRenderTargets( 1, &mRestoreRenderTarget.mObject, mRestoreStencilTarget.mObject );

	mRestoreRenderTarget.Release();
	mRestoreStencilTarget.Release();

	ContextDx.Unlock();
}

void Directx::TRenderTarget::ClearColour(TContext& ContextDx,Soy::TRgb Colour,float Alpha)
{
	float Colour4[4] = { Colour.r(), Colour.g(), Colour.b(), Alpha };

	auto& Context = ContextDx.LockGetContext();
	Context.ClearRenderTargetView( mRenderTargetView, Colour4 );
    //Context.ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	ContextDx.Unlock();
}

void Directx::TRenderTarget::ClearDepth(TContext& ContextDx)
{
	std::Debug << "Render target depth not currently implemented" << std::endl;
}

void Directx::TRenderTarget::ClearStencil(TContext& ContextDx)
{
	std::Debug << "Render target stencil not currently implemented" << std::endl;
}

Directx::TGeometry::TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& _Indexes,const SoyGraphics::TGeometryVertex& Vertex,TContext& ContextDx) :
	mVertexDescription	( Vertex ),
	mIndexCount			( 0 )
{
	Array<uint32> Indexes;
	for ( int i=0;	i<_Indexes.GetSize();	i++ )
		Indexes.PushBack( size_cast<uint32>( _Indexes[i] ) );


	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = Data.GetDataSize();//Vertex.GetDataSize();
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = Vertex.GetStride(0);	//	should be 0

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = Data.GetArray();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Set up the description of the static index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = Indexes.GetDataSize();
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = Indexes.GetArray();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;


	auto& Device = ContextDx.LockGetDevice();
	try
	{
		auto Result = Device.CreateBuffer(&vertexBufferDesc, &vertexData, &mVertexBuffer.mObject );
		Directx::IsOkay( Result, "Create vertex buffer");

		Result = Device.CreateBuffer(&indexBufferDesc, &indexData, &mIndexBuffer.mObject );
		Directx::IsOkay( Result, "Create index buffer");
		mIndexCount = Indexes.GetSize();
		mIndexFormat = DXGI_FORMAT_R32_UINT;
	}
	catch( std::exception& e)
	{
		ContextDx.Unlock();
		throw;
	}
}

Directx::TGeometry::~TGeometry()
{
}


void Directx::TGeometry::Draw(TContext& ContextDx)
{
	auto& Context = ContextDx.LockGetContext();

	// Set vertex buffer stride and offset.
	unsigned int stride = mVertexDescription.GetVertexSize();	//sizeof(VertexType); 
	unsigned int offset = 0;
    
	// Set the vertex buffer to active in the input assembler so it can be rendered.
	Context.IASetVertexBuffers( 0, 1, &mVertexBuffer.mObject, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	Context.IASetIndexBuffer( mIndexBuffer, mIndexFormat, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	Context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	render
	Context.DrawIndexed( mIndexCount, 0, 0);

	ContextDx.Unlock();
}




Directx::TShaderState::TShaderState(const Directx::TShader& Shader) :
	mTextureBindCount	( 0 ),
	mShader				( Shader ),
	mBaked				( false )
{
	//	opengl unbinds here rather than in TShader
	/*
	//	setup constants buffer for shader[s]
	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.ByteWidth = Data.GetDataSize();//Vertex.GetDataSize();
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = Vertex.GetStride(0);	//	should be 0


	nstant buffer can only use a single bind flag (D3D11_BIND_CONSTANT_BUFFER), which cannot be combined with any other bind flag. To bind a shader-constant buffer to the pipeline, call one of the following methods: ID3D11DeviceContext::GSSetConstantBuffers, ID3D11DeviceContext::PSSetConstantBuffers, or ID3D11DeviceContext::VSSetConstantBuffers.
	*/
}

Directx::TShaderState::~TShaderState()
{
	Soy::Assert( mBaked, "ShaderState was never baked before being destroyed. Code maybe missing a .Bake() (needed for directx)");
	/*
	//	unbind textures
	TTexture NullTexture;
	while ( mTextureBindCount > 0 )
	{
		BindTexture( mTextureBindCount-1, NullTexture );
		mTextureBindCount--;
	}
	*/
	
	//	opengl unbinds here rather than in TShader
	const_cast<TShader&>(mShader).Unbind();
}


ID3D11DeviceContext& Directx::TShaderState::GetContext()
{
	Soy::Assert( mShader.mBoundContext, "Shader not bound");
	Soy::Assert( mShader.mBoundContext->mLockedContext, "Shader not bound with locked context");
	return *mShader.mBoundContext->mLockedContext.mObject;
}

ID3D11Device& Directx::TShaderState::GetDevice()
{
	Soy::Assert( mShader.mBoundContext, "Shader not bound");
	Soy::Assert( mShader.mBoundContext->mDevice, "Shader bound missing device");
	return *mShader.mBoundContext->mDevice;
}


bool Directx::TShaderState::SetUniform(const char* Name,const float3x3& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const float& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const int& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const vec4f& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const vec3f& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const vec2f& v)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const TTexture& Texture)
{
	//	find uniform to put texture in the right slot
	BindTexture( mTextureBindCount, Texture );
	mTextureBindCount++;
	return true;
}

bool Directx::TShaderState::SetUniform(const char* Name,const Opengl::TTexture& Texture)
{
	Soy::Assert(false, "Opengl->Directx without context Not supported");
	return false;
}

bool Directx::TShaderState::SetUniform(const char* Name,const Opengl::TTextureAndContext& Texture)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}

bool Directx::TShaderState::SetUniform(const char* Name,const SoyPixelsImpl& Texture)
{
	auto* VertUniform = mShader.mVertexShaderUniforms.Find(Name);
	auto* PixelUniform = mShader.mPixelShaderUniforms.Find(Name);
	if ( !VertUniform && !PixelUniform )
		return false;
	
	Soy_ThrowTodo;
}


void Directx::TShaderState::BindTexture(size_t TextureIndex,const TTexture& Texture)
{
	//	allocate sampler
	{
		std::shared_ptr<AutoReleasePtr<ID3D11SamplerState>> pSampler( new AutoReleasePtr<ID3D11SamplerState> );
		auto& Sampler = pSampler->mObject;
		auto& TextureParams = Texture.mSamplingParams;

		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = TextureParams.mLinearFilter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = TextureParams.mWrapU ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = TextureParams.mWrapV ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = TextureParams.mWrapW ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = TextureParams.mMinLod;
		samplerDesc.MaxLOD = TextureParams.mMaxLod == -1 ? D3D11_FLOAT32_MAX : TextureParams.mMaxLod;

		// Create the texture sampler state.
		auto& Device = GetDevice();
		auto Result = Device.CreateSamplerState( &samplerDesc, &Sampler );
		Directx::IsOkay( Result, "Creating sampler" );
		mSamplers.PushBack( pSampler );
	}

	//	create resource view (resource->shader binding)
	{
		std::shared_ptr<AutoReleasePtr<ID3D11ShaderResourceView>> pResourceView( new AutoReleasePtr<ID3D11ShaderResourceView> );
		auto& ResourceView = *pResourceView;

		ID3D11Resource* Resource = Texture.mTexture.mObject;
		//	no description means it uses original params
		const D3D11_SHADER_RESOURCE_VIEW_DESC* ResourceDesc = nullptr;
		auto& Device = GetDevice();
		auto Result = Device.CreateShaderResourceView( Resource, ResourceDesc, &ResourceView.mObject );
		Directx::IsOkay( Result, "Createing resource view");
		mResources.PushBack( pResourceView );
	}
}

void Directx::TShaderState::Bake()
{
	//	Set the sampler state in the pixel shader.
	auto& Context = GetContext();

	{
		int SamplerFirstSlot = 0;
		Soy::Assert( SamplerFirstSlot+mSamplers.GetSize() < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, "binding too many samplers in shader" );
		BufferArray<ID3D11SamplerState*,100> Samplers;
		for ( int i=0;	i<mSamplers.GetSize();	i++ )
		{
			auto* Sampler = mSamplers[i]->mObject;
			Samplers.PushBack( Sampler );
		}
		//	gr: can I use a temporary here?
		Context.PSSetSamplers( SamplerFirstSlot, Samplers.GetSize(), Samplers.GetArray() );
	}

	{
		int ResourceFirstSlot = 0;
		//Soy::Assert( ResourceFirstSlot+mSamplers.GetSize() < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, "binding too many resources in shader" );
		BufferArray<ID3D11ShaderResourceView*,100> Resources;
		for ( int i=0;	i<mResources.GetSize();	i++ )
		{
			auto* Resource = mResources[i]->mObject;
			Resources.PushBack( Resource );
		}
		//	gr: can I use a temporary here?
		Context.PSSetShaderResources( ResourceFirstSlot, Resources.GetSize(), Resources.GetArray() );
	}

	mBaked = true;
}

Directx::TShaderBlob::TShaderBlob(const std::string& Source,const std::string& Function,const std::string& Target,const std::string& Name,TCompiler& Compiler) :
	mName	( Name )
{
	Array<char> SourceBuffer;
	Soy::StringToArray( Source, GetArrayBridge(SourceBuffer) );
	const D3D_SHADER_MACRO* Macros = nullptr;
	ID3DInclude* IncludeMode = nullptr;
	UINT ShaderOptions = D3D10_SHADER_ENABLE_STRICTNESS;
	UINT EffectOptions = 0;

	AutoReleasePtr<ID3DBlob> ErrorBlob;

	auto Compile = Compiler.GetCompilerFunc();
	Soy::Assert( Compile!=nullptr, "Compile func missing" );

	auto Result = Compile( SourceBuffer.GetArray(), SourceBuffer.GetDataSize(),
          Name.c_str(), Macros, IncludeMode, Function.c_str(), Target.c_str(), ShaderOptions,
         EffectOptions,
         &mBlob.mObject,
        &ErrorBlob.mObject );

	std::stringstream Error;
	Error << "D3DCompile()->";
	GetBlobString( ErrorBlob.mObject, Error );

	Directx::IsOkay( Result, Error.str() );
}


Directx::TShader::TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const SoyGraphics::TGeometryVertex& Vertex,const std::string& ShaderName,Directx::TContext& ContextDx) :
	mBoundContext	( nullptr )
{
	auto& Device = ContextDx.LockGetDevice();

	try
	{
		TShaderBlob VertBlob( vertexSrc, "Vert", "vs_5_0", ShaderName + " vert shader", ContextDx.GetCompiler() );
		TShaderBlob FragBlob( fragmentSrc, "Frag", "ps_5_0", ShaderName + " frag shader", ContextDx.GetCompiler() );
	
		auto Result = Device.CreateVertexShader( VertBlob.GetBuffer(), VertBlob.GetBufferSize(), nullptr, &mVertexShader.mObject );
		Directx::IsOkay( Result, "CreateVertexShader" );

		Result = Device.CreatePixelShader( FragBlob.GetBuffer(), FragBlob.GetBufferSize(), nullptr, &mPixelShader.mObject );
		Directx::IsOkay( Result, "CreatePixelShader" );

		MakeLayout( Vertex, VertBlob, Device );
	}
	catch(std::exception& e)
	{
		ContextDx.Unlock();
		throw;
	}
}


DXGI_FORMAT GetType(const SoyGraphics::TElementType::Type& Type,size_t Length)
{
	if ( Type == SoyGraphics::TElementType::Float )
	{
		if ( Length == 1 )	return DXGI_FORMAT_R32_FLOAT;
		if ( Length == 2 )	return DXGI_FORMAT_R32G32_FLOAT;
		if ( Length == 3 )	return DXGI_FORMAT_R32G32B32_FLOAT;
		if ( Length == 4 )	return DXGI_FORMAT_R32G32B32A32_FLOAT;
	}

	throw Soy::AssertException("Unhandled graphics uniform type -> DXGI_FORMAT");
}

void Directx::TShader::MakeLayout(const SoyGraphics::TGeometryVertex& Vertex,TShaderBlob& ShaderBlob,ID3D11Device& Device)
{
	Array<D3D11_INPUT_ELEMENT_DESC> Layouts;

	for ( int e=0;	e<Vertex.mElements.GetSize();	e++ )
	{
		auto& Element = Vertex.mElements[e];
		auto& Layout = Layouts.PushBack();
		memset( &Layout, 0, sizeof(Layout) );
		
		Layout.SemanticName = Element.mName.c_str();
		Layout.Format = GetType( Element.mType, Element.mArraySize );
		//Layout.SemanticIndex = Element.mIndex;
		Layout.SemanticIndex = 0;
		Layout.InputSlot = 0;
		Layout.AlignedByteOffset = 0;	//	D3D11_APPEND_ALIGNED_ELEMENT
		Layout.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		Layout.InstanceDataStepRate = 0;
	}

	auto Result = Device.CreateInputLayout( Layouts.GetArray(), Layouts.GetSize(), ShaderBlob.GetBuffer(), ShaderBlob.GetBufferSize(), &mLayout.mObject );
	std::stringstream Error;
	Error << "CreateInputLayout(" << ShaderBlob.mName << ")";
	Directx::IsOkay( Result, Error.str() );
}


Directx::TShaderState Directx::TShader::Bind(TContext& ContextDx)
{
	Soy::Assert(mBoundContext==nullptr,"Shader already bound");
	mBoundContext = &ContextDx;
	auto& Context = ContextDx.LockGetContext();

	// Set the vertex input layout.
	Context.IASetInputLayout( mLayout.mObject );

    // Set the vertex and pixel shaders that will be used to render this triangle.
    Context.VSSetShader( mVertexShader.mObject, nullptr, 0);
    Context.PSSetShader( mPixelShader.mObject, nullptr, 0);

	return TShaderState(*this);
}

void Directx::TShader::Unbind()
{
	Soy::Assert(mBoundContext!=nullptr,"Shader was not bound");

	mBoundContext->Unlock();
	mBoundContext = nullptr;
}


void Directx::TShader::SetPixelUniform(Soy::TUniform& Uniform,const float& v)
{
	//	gr: you can only set one buffer at time, so this function may need to update a buffer and mark it as dirty...
//	deviceContext->PSSetShaderResources(0, 1, &texture);
}

void Directx::TShader::SetVertexUniform(Soy::TUniform& Uniform,const float& v)
{
	//	gr: you can only set one buffer at time, so this function may need to update a buffer and mark it as dirty...
	//deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
}

ID3D11DeviceContext& Directx::TShader::GetContext()
{
	Soy::Assert( mBoundContext!=nullptr, "Shader is not bound");
	return *mBoundContext->mLockedContext;
}


size_t Directx::TLockedTextureData::GetPaddedWidth()
{
	auto ChannelCount = mMeta.GetChannels();
	Soy::Assert( ChannelCount > 0, "Locked data channel count zero, cannot work out padded width");
	auto Overflow = mRowPitch % ChannelCount;
	Soy::Assert( Overflow == 0, "Locked data pitch doesn't align to channel count");
	auto PaddedWidth = mRowPitch / ChannelCount;
	return PaddedWidth;
}


