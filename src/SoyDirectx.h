#pragma once

#include "SoyThread.h"
#include "SoyPixels.h"
#include "SoyUniform.h"

#include "SoyOpengl.h"	//	re-using opengl's vertex description atm


class SoyPixelsImpl;

//	something is including d3d10.h (from dx sdk) and some errors have different export types from winerror.h (winsdk)
#pragma warning( push )
//#pragma warning( disable : 4005 )
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")
#pragma warning( pop )

namespace Directx
{
	class TContext;
	class TTexture;
	class TRenderTarget;
	class TGeometry;
	class TShader;
	class TShaderBlob;

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
	size_t					mLockCount;		//	for recursive locking
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

	void		Bind(TContext& Context);
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

	void	Draw(TContext& Context);

public:
	Opengl::TGeometryVertex			mVertexDescription;	//	for attrib binding info
	AutoReleasePtr<ID3D11Buffer>	mVertexBuffer;
	AutoReleasePtr<ID3D11Buffer>	mIndexBuffer;
	size_t							mIndexCount;
	DXGI_FORMAT						mIndexFormat;
};


//	compiled shader
class Directx::TShaderBlob
{
public:
	TShaderBlob(const std::string& Source,const std::string& Function,const std::string& Target,const std::string& Name);

	void*		GetBuffer()			{	return mBlob ? mBlob->GetBufferPointer() : nullptr;	}
	size_t		GetBufferSize()		{	return mBlob ? mBlob->GetBufferSize() : 0;	}

public:
	std::string					mName;
	AutoReleasePtr<ID3D10Blob>	mBlob;
};


class Directx::TShader : public Soy::TUniformContainer
{
public:
	TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const Opengl::TGeometryVertex& Vertex,const std::string& ShaderName,Directx::TContext& Context);

	void			Bind(TContext& Context);
	void			Unbind();

	bool			SetUniform(const char* Name,const TTexture& v)	{	return SetUniformImpl( Name, v );	}

	virtual bool	SetUniform(const char* Name,const float& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const vec2f& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const vec4f& v) override	{	return SetUniformImpl( Name, v );	}
	
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v)	{	return SetUniformImpl( Name, v );	}

private:
	template<typename TYPE>
	bool			SetUniformImpl(const char* Name,const TYPE& v);
	void			SetPixelUniform(Soy::TUniform& Uniform,const float& v);
	void			SetVertexUniform(Soy::TUniform& Uniform,const float& v);

	ID3D11DeviceContext&		GetContext();

	void			MakeLayout(const Opengl::TGeometryVertex& Vertex,TShaderBlob& Shader,ID3D11Device& Device);

public:
	TContext*							mBoundContext;

	Array<Soy::TUniform>				mVertexShaderUniforms;
	Array<Soy::TUniform>				mPixelShaderUniforms;
	AutoReleasePtr<ID3D11VertexShader>	mVertexShader;
	AutoReleasePtr<ID3D11PixelShader>	mPixelShader;
	AutoReleasePtr<ID3D11InputLayout>	mLayout;	//	maybe for geometry?
};




template<typename TYPE>
bool Directx::TShader::SetUniformImpl(const char* Name,const TYPE& v)
{
	auto* VertUniform = mVertexShaderUniforms.Find(Name);
	//if ( VertUniform )
	//	SetVertexUniform( *VertUniform, v );

	auto* PixelUniform = mPixelShaderUniforms.Find(Name);
	//if ( PixelUniform )
	//	SetPixelUniform( *PixelUniform, v );

	return (PixelUniform || VertUniform);
}




