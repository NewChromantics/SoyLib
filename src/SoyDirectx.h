#pragma once

#include "SoyThread.h"
#include "SoyPixels.h"

#include "SoyOpengl.h"	//	re-using opengl's vertex description atm


class SoyPixelsImpl;

//	something is including d3d10.h (from dx sdk) and some errors have different export types from winerror.h (winsdk)
#pragma warning( push )
//#pragma warning( disable : 4005 )
#include <d3d11.h>
#pragma warning( pop )

namespace Directx
{
	class TContext;
	class TTexture;
	class TRenderTarget;
	class TGeometry;

	std::string		GetEnumString(HRESULT Error);
	bool			IsOkay(HRESULT Error,const std::string& Context,bool ThrowException=true);
	SoyPixelsFormat::Type	GetFormat(DXGI_FORMAT Format);
	DXGI_FORMAT				GetFormat(SoyPixelsFormat::Type Format);

	namespace TTextureMode
	{
		enum Type
		{
			Invalid,		//	only for soyenum!
			ReadOnly,
			Writable,		//	dynamic/mappable
			RenderTarget,
		};

		DECLARE_SOYENUM(Directx::TTextureMode);
	}
}

class Directx::TContext : public PopWorker::TContext
{
public:
	TContext(ID3D11Device& Device);

	virtual bool	Lock() override;
	virtual void	Unlock() override;

	ID3D11DeviceContext&	LockGetContext();
	ID3D11Device&			LockGetDevice();

public:
	AutoReleasePtr<ID3D11DeviceContext>	mLockedContext;
	ID3D11Device*			mDevice;
};

class Directx::TTexture
{
public:
	TTexture()		{}
    explicit TTexture(SoyPixelsMeta Meta,TContext& ContextDx,TTextureMode::Type Mode);	//	allocate
	TTexture(ID3D11Texture2D* Texture);

	bool				IsValid() const		{	return mTexture;	}
	void				Write(TTexture& Texture,TContext& Context);
	void				Write(const SoyPixelsImpl& Pixels,TContext& Context);
	TTextureMode::Type	GetMode() const;
	SoyPixelsMeta		GetMeta() const		{	return mMeta;	}

	bool				operator==(const TTexture& that) const	{	return mTexture.mObject == that.mTexture.mObject;	}

public:
	SoyPixelsMeta		mMeta;			//	cache
	AutoReleasePtr<ID3D11Texture2D>	mTexture;
};
namespace Directx
{
std::ostream& operator<<(std::ostream &out,const Directx::TTexture& in);
}



class Directx::TRenderTarget
{
public:
	TRenderTarget(std::shared_ptr<TTexture>& Texture,TContext& ContextDx);

	void		Bind(TContext& ContextDx);
	void		Unbind();

	void		Clear(TContext& ContextDx,Soy::TRgb Colour,float Alpha=1.f);

	bool		operator==(const TTexture& Texture) const		{	return mTexture ? (*mTexture == Texture) : false;	}
	bool		operator!=(const TTexture& Texture) const		{	return !(*this == Texture);	}

private:
	AutoReleasePtr<ID3D11ShaderResourceView>	mShaderResourceView;
	AutoReleasePtr<ID3D11RenderTargetView>		mRenderTargetView;
	std::shared_ptr<TTexture>					mTexture;
};


class Directx::TGeometry
{
public:
	TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& Indexes,const Opengl::TGeometryVertex& Vertex,TContext& ContextDx);
	~TGeometry();

	void	Draw(TContext& ContextDx);

public:
	Opengl::TGeometryVertex			mVertexDescription;	//	for attrib binding info
	AutoReleasePtr<ID3D11Buffer>	mVertexBuffer;
	size_t							mVertexCount;
	AutoReleasePtr<ID3D11Buffer>	mIndexBuffer;
	size_t							mIndexCount;
	DXGI_FORMAT						mIndexFormat;
};
