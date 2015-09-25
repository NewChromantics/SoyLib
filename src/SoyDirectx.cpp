#include "SoyDirectx.h"
#include "SoyPixels.h"


namespace Directx
{
	bool	CanCopyMeta(const SoyPixelsMeta& Source,const SoyPixelsMeta& Destination);
}


std::map<Directx::TTextureMode::Type,std::string> Directx::TTextureMode::EnumMap = 
{
	{	Directx::TTextureMode::Invalid,			"Invalid"	},
	{	Directx::TTextureMode::ReadOnly,		"ReadOnly"	},
	{	Directx::TTextureMode::Writable,		"Writable"	},
	{	Directx::TTextureMode::RenderTarget,	"RenderTarget"	},
};


DXGI_FORMAT Directx::GetFormat(SoyPixelsFormat::Type Format)
{
	switch ( Format )
	{
		//	gr: these are unsupported natively by directx, so force 32 bit and have to do conversion in write()
		case SoyPixelsFormat::RGB:	return DXGI_FORMAT_R8G8B8A8_UNORM;
		case SoyPixelsFormat::BGR:	return DXGI_FORMAT_R8G8B8A8_UNORM;

		case SoyPixelsFormat::RGBA:	return DXGI_FORMAT_R8G8B8A8_UNORM;
		case SoyPixelsFormat::BGRA:	return DXGI_FORMAT_B8G8R8A8_UNORM;

		default:
			//	gr: throw here so we can start handling more formats
			return DXGI_FORMAT_UNKNOWN;
	}
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


SoyPixelsFormat::Type Directx::GetFormat(DXGI_FORMAT Format)
{
	//	https://msdn.microsoft.com/en-gb/library/windows/desktop/bb173059(v=vs.85).aspx
	switch ( Format )
	{
		default:								return SoyPixelsFormat::Invalid;
		case DXGI_FORMAT_UNKNOWN:				return SoyPixelsFormat::UnityUnknown;

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:		return SoyPixelsFormat::RGBA;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return SoyPixelsFormat::RGBA;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:	return SoyPixelsFormat::RGBA;
		case DXGI_FORMAT_R8G8B8A8_UINT:			return SoyPixelsFormat::RGBA;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:		return SoyPixelsFormat::BGRA;
		case DXGI_FORMAT_B8G8R8A8_UNORM:		return SoyPixelsFormat::BGRA;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:	return SoyPixelsFormat::BGRA;
		case DXGI_FORMAT_R8G8_TYPELESS:			return SoyPixelsFormat::GreyscaleAlpha;
		case DXGI_FORMAT_R8G8_UNORM:			return SoyPixelsFormat::GreyscaleAlpha;
		case DXGI_FORMAT_R8G8_SNORM:			return SoyPixelsFormat::GreyscaleAlpha;
		case DXGI_FORMAT_R8G8_UINT:				return SoyPixelsFormat::GreyscaleAlpha;
		case DXGI_FORMAT_R8G8_SINT:				return SoyPixelsFormat::GreyscaleAlpha;
		case DXGI_FORMAT_R8_TYPELESS:			return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_R8_UNORM:				return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_R8_UINT:				return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_R8_SNORM:				return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_R8_SINT:				return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_A8_UNORM:				return SoyPixelsFormat::Greyscale;
		case DXGI_FORMAT_NV12:					return SoyPixelsFormat::Nv12;
	}
}

std::string Directx::GetEnumString(HRESULT Error)	
{
	return Soy::Platform::GetErrorString( Error );	
}

bool Directx::IsOkay(HRESULT Error,const std::string& Context,bool ThrowException)
{
	if ( Error == S_OK )
		return true;

	std::stringstream ErrorStr;
	ErrorStr << "Directx error in " << Context << ": " << GetEnumString(Error) << std::endl;
	
	if ( !ThrowException )
	{
		std::Debug << ErrorStr.str() << std::endl;
		return false;
	}
	
	return Soy::Assert( Error == S_OK, ErrorStr.str() );
}


std::ostream& Directx::operator<<(std::ostream &out,const Directx::TTexture& in)
{
	out << in.mMeta << "(" << in.GetMode() << ")";
	return out;
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



Directx::TTexture::TTexture(SoyPixelsMeta Meta,TContext& ContextDx,TTextureMode::Type Mode)
{
	static bool AutoMipMap = false;

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
		Desc.Format = GetFormat( Meta.GetFormat() );//DXGI_FORMAT_R32G32B32A32_FLOAT;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = 0;
	}
	else if ( Mode == TTextureMode::Writable )
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.Format = GetFormat( Meta.GetFormat() );
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else if ( Mode == TTextureMode::ReadOnly )
	{
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.Format = GetFormat( Meta.GetFormat() );
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = 0x0;
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
	}
	catch( std::exception& e)
	{
		ContextDx.Unlock();
		throw;
	}
	ContextDx.Unlock();
}

Directx::TTexture::TTexture(ID3D11Texture2D* Texture)
{
	//	validate and throw here
	Soy::Assert( Texture != nullptr, "null directx texture" );

	mTexture.Set( Texture, true );

	//	get meta
	D3D11_TEXTURE2D_DESC SrcDesc;
	mTexture->GetDesc( &SrcDesc );
	mMeta = SoyPixelsMeta( SrcDesc.Width, SrcDesc.Height, GetFormat(SrcDesc.Format) );
}

Directx::TTextureMode::Type Directx::TTexture::GetMode() const
{
	//	get meta
	D3D11_TEXTURE2D_DESC SrcDesc;
	mTexture.mObject->GetDesc( &SrcDesc );
	bool IsRenderTarget = (SrcDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0;
	bool IsWritable = (SrcDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0;

	if ( IsRenderTarget )
	{
		return TTextureMode::RenderTarget;
	}
	else if ( IsWritable )
	{
		return TTextureMode::Writable;
	}
	else
	{
		return TTextureMode::ReadOnly;
	}
}

void Directx::TTexture::Write(TTexture& Destination,TContext& ContextDx)
{
	Soy::Assert( IsValid(), "Writing to invalid texture" );
	Soy::Assert( Destination.IsValid(), "Writing from invalid texture" );

	//	try simple no-errors-reported-by-dx copy resource fast path
	if ( CanCopyMeta( mMeta, Destination.mMeta ) )
	{
		auto& Context = ContextDx.LockGetContext();
		Context.CopyResource( mTexture, Destination.mTexture );
		ContextDx.Unlock();
		return;
	}

	std::stringstream Error;
	Error << "No path to copy " << (*this) << " to " << Destination;
	throw Soy::AssertException( Error.str() );
}


void Directx::TTexture::Write(const SoyPixelsImpl& Pixels,TContext& ContextDx)
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
		Soy::Assert(CanWrite, "Texture does not have CPU mapping access");

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
				MapMode = D3D11_MAP_WRITE_DISCARD;
			}
			else
			{
				MapFlags = !Blocking ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0x0;
				//	gr: for immediate context, we ALSO want write_discard
				//MapMode = D3D11_MAP_WRITE;
				MapMode = D3D11_MAP_WRITE_DISCARD;
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

			size_t ResourceDataSize = resource.RowPitch * SrcDesc.Height;//	width in bytes
			auto& PixelsArray = Pixels.GetPixelsArray();
			if ( PixelsArray.GetDataSize() != ResourceDataSize )
			{
				std::stringstream Error;
				Error << "Warning: resource/texture data size mismatch; " << PixelsArray.GetDataSize() << " (frame) vs " << ResourceDataSize << " (resource)";
				std::Debug << Error.str() << std::endl;

				ResourceDataSize = std::min(ResourceDataSize, PixelsArray.GetDataSize());
			}

			//	update contents 
			memcpy(resource.pData, PixelsArray.GetArray(), ResourceDataSize);
			Context.Unmap( mTexture, SubResource);
		}
		ContextDx.Unlock();
	}
	catch (std::exception& e)
	{
		//	unlock and re-throw
		ContextDx.Unlock();
		throw;
	}
}



Directx::TRenderTarget::TRenderTarget(std::shared_ptr<TTexture>& Texture,TContext& ContextDx) :
	mTexture		( Texture )
{
	Soy::Assert( Texture && Texture->IsValid(), "Render target needs a valid texture target" );
	auto& Meta = Texture->GetMeta();
	
	// Create the render target view.
	auto& Device = ContextDx.LockGetDevice();
	try
	{
		auto* TextureDx = mTexture->mTexture.mObject;
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		renderTargetViewDesc.Format = GetFormat( Meta.GetFormat() );
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;
		{
			auto Result = Device.CreateRenderTargetView( TextureDx, &renderTargetViewDesc, &mRenderTargetView.mObject );
			std::stringstream Error;
			Error << "CreateRenderTargetView(" << Texture << ")";
			Directx::IsOkay( Result, Error.str() );
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.Format = GetFormat( Meta.GetFormat() );
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

	Context.OMSetRenderTargets( 1, &mRenderTargetView.mObject, nullptr );
	//deviceContext->OMSetRenderTargets(1, &m_renderTargetView, depthStencilView);

	ContextDx.Unlock();
}

void Directx::TRenderTarget::Unbind()
{
}

void Directx::TRenderTarget::Clear(TContext& ContextDx,Soy::TRgb Colour,float Alpha)
{
	float Colour4[4] = { Colour.r(), Colour.g(), Colour.b(), Alpha };

	auto& Context = ContextDx.LockGetContext();
	Context.ClearRenderTargetView( mRenderTargetView, Colour4 );
    //Context.ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	ContextDx.Unlock();
}

Directx::TGeometry::TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& _Indexes,const Opengl::TGeometryVertex& Vertex,TContext& ContextDx) :
	mVertexDescription	( Vertex ),
	mVertexCount		( 0 ),
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
		//mVertexCount = 

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



Directx::TShaderBlob::TShaderBlob(const std::string& Source,const std::string& Name) :
	mName	( Name )
{
	Array<char> SourceBuffer;
	Soy::StringToArray( Source, GetArrayBridge(SourceBuffer) );
	const D3D_SHADER_MACRO* Macros = nullptr;
	ID3DInclude* IncludeMode = nullptr;
	const char* EntryPoint = nullptr;
	const char* Target = "vs_5_0";
	UINT ShaderOptions = D3D10_SHADER_ENABLE_STRICTNESS;
	UINT EffectOptions = 0;

	AutoReleasePtr<ID3DBlob> ErrorBlob;

	auto Result = D3DCompile( SourceBuffer.GetArray(), SourceBuffer.GetDataSize(),
          Name.c_str(), Macros, IncludeMode, EntryPoint, Target, ShaderOptions,
         EffectOptions,
         &mBlob.mObject,
        &ErrorBlob.mObject );

	std::stringstream Error;
	Error << "D3DCompile()->";
	GetBlobString( ErrorBlob.mObject, Error );

	Directx::IsOkay( Result, Error.str() );
}


Directx::TShader::TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const Opengl::TGeometryVertex& Vertex,const std::string& ShaderName,Directx::TContext& ContextDx) :
	mBoundContext	( nullptr )
{
	auto& Device = ContextDx.LockGetDevice();

	try
	{
		TShaderBlob VertBlob( vertexSrc, ShaderName + " vert shader");
		TShaderBlob FragBlob( fragmentSrc, ShaderName + " frag shader");
	
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


DXGI_FORMAT GetType(GLenum Type)
{
	switch ( Type )
	{
		default:
			Soy::Assert( false, "Unhandled type");
	}
	//	gr: sort this 
	//	DXGI_FORMAT_R32G32B32_FLOAT
	//	DXGI_FORMAT_R32G32_FLOAT
	return DXGI_FORMAT_R32_FLOAT;
}

void Directx::TShader::MakeLayout(const Opengl::TGeometryVertex& Vertex,TShaderBlob& ShaderBlob,ID3D11Device& Device)
{
	Array<D3D11_INPUT_ELEMENT_DESC> Layouts;

	for ( int e=0;	e<Vertex.mElements.GetSize();	e++ )
	{
		auto& Element = Vertex.mElements[e];
		auto& Layout = Layouts.PushBack();
		memset( &Layout, 0, sizeof(Layout) );
		
		Layout.SemanticName = Element.mName.c_str();
		Layout.Format = GetType( Element.mType );
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


void Directx::TShader::Bind(TContext& ContextDx)
{
	Soy::Assert(mBoundContext==nullptr,"Shader already bound");
	mBoundContext = &ContextDx;
	auto& Context = ContextDx.LockGetContext();

	// Set the vertex input layout.
	Context.IASetInputLayout( mLayout.mObject );

    // Set the vertex and pixel shaders that will be used to render this triangle.
    Context.VSSetShader( mVertexShader.mObject, nullptr, 0);
    Context.PSSetShader( mPixelShader.mObject, nullptr, 0);

	// Set the sampler state in the pixel shader.
	//Context.PSSetSamplers(0, 1, &m_sampleState);

	ContextDx.Unlock();
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


