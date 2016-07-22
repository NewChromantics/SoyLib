#pragma once

#include "SoyThread.h"
#include "SoyPixels.h"
#include "SoyUniform.h"
#include <functional>

#include "SoyOpengl.h"	//	re-using opengl's vertex description atm
#include "SoyMediaFormat.h"


template<typename TYPE>
class TPool;
class SoyPixelsImpl;

#include <d3d9.h>

namespace DirectxCompiler
{
	class TCompiler;			//	wrapper to hold the compile func and a reference to the runtime library. Defined in source for cleaner code
}



namespace Directx9
{
	class TContext;
	class TTexture;
	class TTextureMeta;
	class TLockedTextureData;
	class TRenderTarget;
	class TGeometry;
	class TShader;
	class TShaderState;
	class TTextureSamplingParams;


	inline std::string		GetEnumString(HRESULT Error)												{	return Platform::GetErrorString( Error );	}
	inline bool				IsOkay(HRESULT Error,const std::string& Context,bool ThrowException=true)	{	return Platform::IsOkay( Error, Context, ThrowException );	}
	SoyMediaFormat::Type	GetFormat(D3DFORMAT Format);
	SoyPixelsFormat::Type	GetPixelFormat(D3DFORMAT Format);
	D3DFORMAT				GetFormat(SoyPixelsFormat::Type Format);
	
	namespace TTextureMode
	{
		enum Type
		{
			Invalid,
			Dynamic,
			RenderTarget,
			DepthStencil,
		};

		DECLARE_SOYENUM(Directx9::TTextureMode);
	}

}

class Directx9::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext(IDirect3DDevice9& Device);

	virtual bool	Lock() override;
	virtual void	Unlock() override;

	void			Iteration()			{	Flush(*this);	}
	bool			HasMultithreadAccess() const	{	return false;	}	//	gr: do real probe for... deffered, not immediate, context type?

//	ID3D11DeviceContext&	LockGetContext();
//	ID3D11Device&			LockGetDevice();
	DirectxCompiler::TCompiler&	GetCompiler();
	IDirect3DDevice9&		GetDevice()		{	return *mDevice;	}

public:
	//AutoReleasePtr<ID3D11DeviceContext>	mLockedContext;
	size_t						mLockCount;		//	for recursive locking
	IDirect3DDevice9*			mDevice;
	std::shared_ptr<DirectxCompiler::TCompiler>	mCompiler;
};

/*
//	note: these are set in opengl at creation time. in dx they can be seperate from the texture...
class Directx9::TTextureSamplingParams
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
*/

/*
class Directx::TLockedTextureData
{
	//	gr: inheriting from noncopyable means we cannot use this scoped class in lambdas, not sure why it works if we specify the =delete here
	TLockedTextureData& operator=(const TLockedTextureData&) = delete;
	//TShaderState(const TShaderState&) = delete;
public:
	TLockedTextureData(void* Data,size_t Size,const SoyPixelsMeta& Meta,size_t RowPitch,const std::function<void()>& Unlock) :
		mMeta		( Meta ),
		mRowPitch	( RowPitch ),
		mUnlock		( Unlock ),
		mData		( Data ),
		mSize		( Size )
	{
	}
	~TLockedTextureData()
	{
		if ( mUnlock )
			mUnlock();
	}

	RemoteArray<uint8>			GetArray();
	size_t						GetPaddedWidth();

public:
	SoyPixelsMeta				mMeta;
	size_t						mRowPitch;

	void*						mData;
	size_t						mSize;

private:
	std::function<void()>			mUnlock;
};
*/


class Directx9::TTextureMeta
{
public:
	TTextureMeta(const SoyPixelsMeta& Meta, TTextureMode::Type Mode) :
		mMeta	( Meta ),
		mMode	( Mode )
	{
	}

public:
	SoyPixelsMeta		mMeta;
	TTextureMode::Type	mMode;
};
std::ostream& operator<<(std::ostream &out,const Directx9::TTextureMeta& in);



class Directx9::TTexture
{
public:
	TTexture()		{}
    explicit TTexture(SoyPixelsMeta Meta,TContext& ContextDx,TTextureMode::Type Mode);	//	allocate
	TTexture(IDirect3DTexture9* Texture);
	TTexture(const TTexture& Texture);

	bool				IsValid() const		{	return mTexture;	}
	void				Write(const TTexture& Source,TContext& Context);
	void				Write(const SoyPixelsImpl& Source,TContext& Context);

	//	with directx, we can't always read from a texture, but if you give a pool, we can copy to a temp one and read from that
	void				Read(SoyPixelsImpl& Pixels, TContext& Context)								{	Read(Pixels, Context, nullptr );	}
	void				Read(SoyPixelsImpl& Pixels,TContext& Context,TPool<TTexture>& TexturePool)	{	Read(Pixels, Context, &TexturePool );	}	
	void				Read(SoyPixelsImpl& Pixels,TContext& Context,TPool<TTexture>* TexturePool);

//	bool				CanBindToShaderUniform() const;
	TTextureMode::Type	GetMode() const;
	SoyPixelsMeta		GetMeta() const		{	return mMeta;	}
//	DXGI_FORMAT			GetDirectxFormat() const	{	return mFormat;	}
	D3DPOOL				GetPool() const;

	bool				operator==(const TTextureMeta& Meta) const	{	return mMeta == Meta.mMeta && GetMode() == Meta.mMode;	}
	bool				operator==(const TTexture& that) const	{	return mTexture.mObject == that.mTexture.mObject;	}
	bool				operator!=(const TTexture& that) const	{	return !(*this == that);	}

private:
	TLockedTextureData	LockTextureData(TContext& Context,bool WriteAccess);
	void				CacheMeta(SoyPixelsFormat::Type OverrideFormat=SoyPixelsFormat::Invalid);

public:
//	TTextureSamplingParams			mSamplingParams;
	SoyPixelsMeta					mMeta;			//	cache
	AutoReleasePtr<IDirect3DTexture9>	mTexture;
	D3DFORMAT						mFormat;		//	dx format
};
namespace Directx9
{
std::ostream& operator<<(std::ostream &out,const Directx9::TTexture& in);
}



class Directx9::TRenderTarget
{
public:
	TRenderTarget(TTexture& Texture,TContext& ContextDx);

	void			Bind(TContext& Context);
	void			Unbind(TContext& ContextDx);

	void			ClearColour(TContext& ContextDx,Soy::TRgb Colour,float Alpha=1.f);
	//void			ClearDepth(TContext& ContextDx);
	//void			ClearStencil(TContext& ContextDx);
	SoyPixelsMeta	GetMeta() const									{	return mTexture.GetMeta();	}

	bool			operator==(const TTexture& Texture) const		{	return mTexture == Texture;	}
	bool			operator!=(const TTexture& Texture) const		{	return !(*this == Texture);	}

private:
	AutoReleasePtr<IDirect3DSurface9>			mSurface;
//	AutoReleasePtr<ID3D11ShaderResourceView>	mShaderResourceView;
//	AutoReleasePtr<ID3D11RenderTargetView>		mRenderTargetView;
	TTexture									mTexture;

//	AutoReleasePtr<ID3D11RenderTargetView>		mRestoreRenderTarget;
//	AutoReleasePtr<ID3D11DepthStencilView>		mRestoreStencilTarget;
};



class Directx9::TGeometry
{
public:
	TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& Indexes,const SoyGraphics::TGeometryVertex& Vertex,TContext& ContextDx);
	~TGeometry();

	void	Draw(TContext& Context);

public:
	SoyGraphics::TGeometryVertex	mVertexDescription;	//	for attrib binding info
	AutoReleasePtr<IDirect3DVertexDeclaration9>	mVertexDeclaration;
	//AutoReleasePtr<IDirect3DVertexBuffer9>	mVertexBuffer;
	Array<uint8>					mVertexData;		//	this should probably be in a buffer
	Array<uint16_t>					mIndexes;			//	this should probably be in a buffer
	/*
	AutoReleasePtr<ID3D11Buffer>	mVertexBuffer;
	AutoReleasePtr<ID3D11Buffer>	mIndexBuffer;
	size_t							mIndexCount;
	DXGI_FORMAT						mIndexFormat;
	*/
	size_t							mVertexCount;
	size_t							mTriangleCount;
};




//	clever class which does the binding, auto texture mapping, and unbinding
//	why? so we can use const TShaders and share them across threads
class Directx9::TShaderState : public Soy::TUniformContainer
{
private:
	//	gr: inheriting from noncopyable means we cannot use this scoped class in lambdas, not sure why it works if we specify the =delete here
	TShaderState& operator=(const TShaderState&) = delete;
	//TShaderState(const TShaderState&) = delete;
public:
	TShaderState(const TShader& Shader,TContext& Context);
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
	bool			SetUniform(const char* Name,const Directx9::TTexture& v);
	virtual bool	SetUniform(const char* Name,const SoyPixelsImpl& v) override;

	template<typename TYPE>
	bool	SetUniform(const std::string& Name,const TYPE& v)
	{
		return SetUniform( Name.c_str(), v );
	}
									   

	void	BindTexture(size_t TextureIndex,const TTexture& Texture);	//	use to unbind too
	
private:
	TContext&			GetContext()		{	return *mBoundContext;	}
	//ID3D11DeviceContext&		GetContext();
	//ID3D11Device&				GetDevice();
	bool						mBaked;			//	warning for code; if we never baked the shader on destruction, we may have never sent data pre-geo. DirectX needs a bake but others dont...

public:
	const TShader&		mShader;
	size_t				mTextureBindCount;
	TContext*			mBoundContext;
	//Array<std::shared_ptr<AutoReleasePtr<ID3D11SamplerState>>>			mSamplers;
	//Array<std::shared_ptr<AutoReleasePtr<ID3D11ShaderResourceView>>>	mResources;	//	textures
};


class Directx9::TShader
{
public:
	TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const SoyGraphics::TGeometryVertex& Vertex,const std::string& ShaderName,TContext& Context);

	TShaderState	Bind(TContext& Context);	//	let this go out of scope to unbind
	void			Unbind();

private:

	//ID3D11DeviceContext&		GetContext();

	//void			MakeLayout(const SoyGraphics::TGeometryVertex& Vertex,TShaderBlob& Shader,ID3D11Device& Device);

public:
	//TContext*							mBoundContext;	//	this binding should be moved to TShaderState
	Array<Soy::TUniform>				mVertexShaderUniforms;
	Array<Soy::TUniform>				mPixelShaderUniforms;
	AutoReleasePtr<IDirect3DVertexShader9>	mVertexShader;
	AutoReleasePtr<IDirect3DPixelShader9>	mPixelShader;
//	AutoReleasePtr<ID3D11InputLayout>	mLayout;	//	maybe for geometry?
};

