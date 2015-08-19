#pragma once

#include "SoyThread.h"

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
}

class Directx::TContext : public PopWorker::TContext
{
public:
	TContext(ID3D11Device& Device);

	virtual bool	Lock() override;
	virtual void	Unlock() override;

public:
	ID3D11DeviceContext*	mLockedContext;
	ID3D11Device*			mDevice;
};

class Directx::TTexture
{
public:
	TTexture() :
		mTexture	( nullptr )
	{
	}
    
	TTexture(ID3D11Texture2D* Texture) :
        mTexture    ( Texture )
    {
		//	validate and throw here
		Soy::Assert( mTexture != nullptr, "null directx texture" );
    }

	bool	IsValid() const		{	return mTexture!=nullptr;	}
	void	Write(TTexture& Texture,TContext& Context);
	void	Write(const SoyPixelsImpl& Pixels,TContext& Context);

public:
	ID3D11Texture2D*	mTexture;
};