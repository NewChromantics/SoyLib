#pragma once

#include "SoyThread.h"
#include "SoyPixels.h"

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

public:
	ID3D11DeviceContext*	mLockedContext;
	ID3D11Device*			mDevice;
};

class Directx::TTexture
{
public:
	TTexture();
    explicit TTexture(SoyPixelsMeta Meta,TContext& ContextDx,TTextureMode::Type Mode);	//	allocate
	TTexture(ID3D11Texture2D* Texture);
	~TTexture();

	bool				IsValid() const		{	return mTexture!=nullptr;	}
	void				Write(TTexture& Texture,TContext& Context);
	void				Write(const SoyPixelsImpl& Pixels,TContext& Context);
	TTextureMode::Type	GetMode() const;

public:
	SoyPixelsMeta		mMeta;			//	cache
	ID3D11Texture2D*	mTexture;
};
namespace Directx
{
std::ostream& operator<<(std::ostream &out,const Directx::TTexture& in);
}

