#include "SoyDirectx.h"
#include "SoyPixels.h"


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


Directx::TContext::TContext(ID3D11Device& Device) :
	mDevice			( &Device ),
	mLockedContext	( nullptr )
{
}


bool Directx::TContext::Lock()
{
	//	already locked... throw here? increment ref count, if any?
	if ( mLockedContext )
		throw Soy::AssertException("Context already locked");

	mDevice->GetImmediateContext( &mLockedContext );
	if ( !Soy::Assert( mLockedContext!=nullptr, "Failed to get immediate context" ) )
		return false;

	return true;
}

void Directx::TContext::Unlock()
{
	//	already locked... throw here? increment ref count, if any?
	if ( !mLockedContext )
		throw Soy::AssertException("Context was not locked");
	mLockedContext = nullptr;
}


void Directx::TTexture::Write(TTexture& Texture,TContext& ContextDx)
{
	Soy::Assert( IsValid(), "Writing to invalid texture" );
	Soy::Assert( Texture.IsValid(), "Writing from invalid texture" );
	if ( !ContextDx.Lock() )
		throw Soy::AssertException("Failed to lock context");
	auto& Context = *ContextDx.mLockedContext;

	//	copy to real texture (gpu->gpu)
	//	gr: this will fail silently if dimensions/format different, detect this!
	{
		//Unity::TScopeTimerWarning MapTimer("DX::copy resource",2);
		Context.CopyResource( mTexture, Texture.mTexture );
	}

	ContextDx.Unlock();
}


void Directx::TTexture::Write(const SoyPixelsImpl& Pixels,TContext& ContextDx)
{
	Soy::Assert( IsValid(), "Writing to invalid texture" );
	if ( !ContextDx.Lock() )
		throw Soy::AssertException("Failed to lock context");
	auto& Context = *ContextDx.mLockedContext;

	bool Blocking = true;

	//	update our dynamic texture
	try
	{
		D3D11_TEXTURE2D_DESC SrcDesc;
		mTexture->GetDesc(&SrcDesc);

		D3D11_MAPPED_SUBRESOURCE resource;
		ZeroMemory( &resource, sizeof(resource) );
		int SubResource = 0;
		//bool IsDefferedContext = (ctx->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED);
		bool IsDefferedContext = true;

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
			MapMode = D3D11_MAP_WRITE;
		}

		HRESULT hr = Context.Map( mTexture, SubResource, MapMode, MapFlags, &resource);

		//	specified do not block, and GPU is using the texture
		if ( !Blocking && hr == DXGI_ERROR_WAS_STILL_DRAWING )
		{
			throw Soy::AssertException("GPU is still using texture during map() and we specified non-blocking");
		}

		//	other error
		{
			std::stringstream ErrorString;
			ErrorString << "Failed to get Map() for dynamic texture(" << SrcDesc.Width << "," << SrcDesc.Height << ")";
			Directx::IsOkay( hr, ErrorString.str() );
		}

		size_t ResourceDataSize = resource.RowPitch * SrcDesc.Height;//	width in bytes
		auto& PixelsArray = Pixels.GetPixelsArray();
		if ( PixelsArray.GetDataSize() != ResourceDataSize )
		{
			std::stringstream Error;
			Error << "Warning: resource/texture data size mismatch; " << PixelsArray.GetDataSize() << " (frame) vs " << ResourceDataSize << " (resource)";
			std::Debug << Error.str() << std::endl;

			ResourceDataSize = std::min( ResourceDataSize, PixelsArray.GetDataSize() );
		}

		//	update contents 
		memcpy( resource.pData, PixelsArray.GetArray(), ResourceDataSize );
		ContextDx.Unlock();
	}
	catch (std::exception& e)
	{
		//	unlock and re-throw
		ContextDx.Unlock();
		throw;
	}
}



