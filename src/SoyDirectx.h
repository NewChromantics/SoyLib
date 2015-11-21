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
#pragma warning( pop )


//	if compiling against win8 lib/runtime, then we can include the new d3d compiler lib directly
#if WINDOWS_TARGET_SDK >= 8
#include <d3dcompiler.h>
//	gr: do not link to d3dcompiler. This means it will not load the DLL and we load it manually (InitCompilerExtension)
//#pragma comment(lib, "D3DCompiler.lib")
#endif

namespace Directx
{
	class TContext;
	class TTexture;
	class TRenderTarget;
	class TGeometry;
	class TShader;
	class TShaderState;
	class TShaderBlob;
	class TTextureSamplingParams;

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

class Directx::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext(ID3D11Device& Device);

	virtual bool	Lock() override;
	virtual void	Unlock() override;

	void			Iteration()			{	Flush(*this);	}
	bool			HasMultithreadAccess() const	{	return false;	}	//	gr: do real probe for... deffered, not immediate, context type?

	ID3D11DeviceContext&	LockGetContext();
	ID3D11Device&			LockGetDevice();

public:
	AutoReleasePtr<ID3D11DeviceContext>	mLockedContext;
	size_t					mLockCount;		//	for recursive locking
	ID3D11Device*			mDevice;
};


//	note: these are set in opengl at creation time. in dx they can be seperate from the texture...
class Directx::TTextureSamplingParams
{
public:
	TTextureSamplingParams() :
		mWrapU			( true ),
		mWrapV			( true ),
		mWrapW			( true ),
		mMinLod			( 0 ),
		mMaxLod			( 0 ),
		mLinearFilter	( false )
	{
	}

	bool	mWrapU;
	bool	mWrapV;
	bool	mWrapW;
	size_t	mMinLod;
	ssize_t	mMaxLod;		//	-1 = hardware max
	bool	mLinearFilter;	//	else nearest
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
	bool				operator!=(const TTexture& that) const	{	return !(*this == that);	}

public:
	TTextureSamplingParams			mSamplingParams;
	SoyPixelsMeta					mMeta;			//	cache
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

	void			Bind(TContext& Context);
	void			Unbind(TContext& ContextDx);

	void			Clear(TContext& ContextDx,Soy::TRgb Colour,float Alpha=1.f);
	SoyPixelsMeta	GetMeta() const									{	return mTexture ? mTexture->GetMeta() : SoyPixelsMeta();	}

	bool			operator==(const TTexture& Texture) const		{	return mTexture ? (*mTexture == Texture) : false;	}
	bool			operator!=(const TTexture& Texture) const		{	return !(*this == Texture);	}

private:
	AutoReleasePtr<ID3D11ShaderResourceView>	mShaderResourceView;
	AutoReleasePtr<ID3D11RenderTargetView>		mRenderTargetView;
	std::shared_ptr<TTexture>					mTexture;

	AutoReleasePtr<ID3D11RenderTargetView>		mRestoreRenderTarget;
	AutoReleasePtr<ID3D11DepthStencilView>		mRestoreStencilTarget;
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


//	clever class which does the binding, auto texture mapping, and unbinding
//	why? so we can use const TShaders and share them across threads
class Directx::TShaderState : public Soy::TUniformContainer, public NonCopyable
{
public:
	TShaderState(const TShader& Shader);
	~TShaderState();
	
	void			Bake();		//	upload final setup to GPU before drawing

	virtual bool	SetUniform(const char* Name,const int& v) override;
	virtual bool	SetUniform(const char* Name,const float& v) override;
	virtual bool	SetUniform(const char* Name,const vec2f& v) override;
	virtual bool	SetUniform(const char* Name,const vec3f& v) override;
	virtual bool	SetUniform(const char* Name,const vec4f& v) override;
	virtual bool	SetUniform(const char* Name,const Opengl::TTexture& Texture);	//	special case which tracks how many textures are bound
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& Texture) override;
	bool			SetUniform(const char* Name,const float3x3& v);
	bool			SetUniform(const char* Name,const Directx::TTexture& v);
	virtual bool	SetUniform(const char* Name,const SoyPixelsImpl& v) override;

	template<typename TYPE>
	bool	SetUniform(const std::string& Name,const TYPE& v)
	{
		return SetUniform( Name.c_str(), v );
	}
									   

	void	BindTexture(size_t TextureIndex,const TTexture& Texture);	//	use to unbind too
	
private:
	ID3D11DeviceContext&		GetContext();
	ID3D11Device&				GetDevice();

public:
	const TShader&		mShader;
	size_t				mTextureBindCount;
	
	Array<std::shared_ptr<AutoReleasePtr<ID3D11SamplerState>>>			mSamplers;
	Array<std::shared_ptr<AutoReleasePtr<ID3D11ShaderResourceView>>>	mResources;	//	textures
};


class Directx::TShader : public Soy::TUniformContainer
{
public:
	TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const Opengl::TGeometryVertex& Vertex,const std::string& ShaderName,Directx::TContext& Context);

	TShaderState	Bind(TContext& Context);	//	let this go out of scope to unbind
	void			Unbind();

	bool			SetUniform(const char* Name,const TTexture& v)	{	return SetUniformImpl( Name, v );	}

	virtual bool	SetUniform(const char* Name,const float& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const vec2f& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const vec3f& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const vec4f& v) override	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const int& v) override		{	return SetUniformImpl( Name, v );	}
	
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v)	{	return SetUniformImpl( Name, v );	}
	virtual bool	SetUniform(const char* Name,const SoyPixelsImpl& v) override		{	return SetUniformImpl( Name, v );	}

private:
	template<typename TYPE>
	bool			SetUniformImpl(const char* Name,const TYPE& v);
	void			SetPixelUniform(Soy::TUniform& Uniform,const float& v);
	void			SetVertexUniform(Soy::TUniform& Uniform,const float& v);

	ID3D11DeviceContext&		GetContext();

	void			MakeLayout(const Opengl::TGeometryVertex& Vertex,TShaderBlob& Shader,ID3D11Device& Device);

public:
	TContext*							mBoundContext;	//	this binding should be moved to TShaderState

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




