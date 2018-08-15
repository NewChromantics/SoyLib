#pragma once

#include "SoyEvent.h"
#include "SoyMath.h"
#include "SoyPixels.h"
#include "SoyGraphics.h"


#if defined(TARGET_ANDROID) || defined(TARGET_IOS)

//	use latest SDK, but helps narrow down what might need supporting if we use ES2 headers
#define OPENGL_ES	3
//#define OPENGL_ES	2

#elif defined(TARGET_OSX)
	#define OPENGL_CORE	3	//	need 3 for VBA's
#elif defined(TARGET_WINDOWS)
	#define OPENGL_CORE	3
#endif

//#define GL_NONE				GL_NO_ERROR	//	declared in GLES3

#if defined(TARGET_ANDROID) && (OPENGL_ES==3)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES/glext.h>
#endif

#if defined(TARGET_ANDROID) && (OPENGL_ES==2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES/glext.h>	//	need for EOS
#endif

#if defined(TARGET_OSX) && (OPENGL_CORE==3)
#include <Opengl/gl3.h>
#include <Opengl/gl3ext.h>
#endif

#if defined(TARGET_IOS) && (OPENGL_ES==3)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif

#if defined(TARGET_IOS) && (OPENGL_ES==2)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

#if defined(TARGET_WINDOWS)
#if !defined(GLEW_STATIC)
#error expected GLEW_STATIC to be defined
#endif
#pragma comment(lib,"opengl32.lib")
//#include <gl/GL.h>
//#include "gl/glext.h"
#include <GL/glew.h>
#endif

#define GL_ASSET_INVALID	0
#define GL_UNIFORM_INVALID	-1



namespace Opengl
{
	class TFboMeta;
	class TAsset;
	class TShader;
	class TShaderState;
	class TTexture;
	class TTextureMeta;
	class TFbo;
	class TPbo;
	class TGeoQuad;
	class TShaderEosBlit;
	class TGeometry;
	class TContext;
	class TSync;


	#define Opengl_IsOkay()			Opengl::IsOkay(__func__)
	//#define Opengl_IsOkayFlush()	Opengl::IsOkay( std::string(__func__)+ " flush", false )
	#define Opengl_IsOkayFlush()	Opengl::FlushError( __func__ )

	bool			IsOkay(const char* Context,std::function<void(const std::string&)> ExceptionContainer);	//	if container is null, we throw
	//inline bool		IsOkay(const std::string& Context,std::function<void(const std::string&)> ExceptionContainer)	{	return IsOkay( Context.c_str(), ExceptionContainer );	}
	bool			IsOkay(const char* Context,bool ThrowException=true);
	inline bool		IsOkay(const std::string& Context,bool ThrowException=true)	{	return IsOkay( Context.c_str(), ThrowException );	}
	void			FlushError(const char* Context);
	std::string		GetEnumString(GLenum Type);

	void					GetUploadPixelFormats(ArrayBridge<GLenum>&& Formats,const Opengl::TTexture& Texture,SoyPixelsFormat::Type Format,bool AllowConversion);
	void					GetNewTexturePixelFormats(ArrayBridge<GLenum>&& Formats,SoyPixelsFormat::Type Format);
	void					GetDownloadPixelFormats(ArrayBridge<GLenum>&& Formats,const TTexture& Texture,SoyPixelsFormat::Type PixelFormat);
	SoyPixelsFormat::Type	GetDownloadPixelFormat(GLenum Format);
	void					GetReadPixelsFormats(ArrayBridge<GLenum>&& Formats);	//	glReadPixels has a limited range of formats. Array[channelcount] = format

	//	helpers
	void	ClearColour(Soy::TRgb Colour,float Alpha=1);
	void	ClearDepth();
	void	ClearStencil();
	void	SetViewport(Soy::Rectf Viewport);
};


class Opengl::TTextureAndContext
{
public:
	TTextureAndContext(TTexture& t,TContext& c) :
		mTexture	( t ),
		mContext	( c )
	{
	}

	TTexture&	mTexture;
	TContext&	mContext;
};


class Opengl::TFboMeta
{
public:
	TFboMeta()	{}
	TFboMeta(const std::string& Name,size_t Width,size_t Height) :
		mName	( Name ),
		mSize	( Width, Height )
	{
	}
	
	std::string		mName;
	vec2x<size_t>	mSize;
};




//	gr: for now, POD, no virtuals...
class Opengl::TAsset
{
public:
	TAsset() :
	mName	( GL_ASSET_INVALID )
	{
	}
	explicit TAsset(GLuint Name) :
	mName	( Name )
	{
	}
	
	bool	IsValid() const		{	return mName != GL_ASSET_INVALID;	}
	bool	operator==(const TAsset& Asset) const	{	return mName == Asset.mName;	}
	bool	operator!=(const TAsset& Asset) const	{	return mName != Asset.mName;	}
	
	GLuint	mName;
};


class Opengl::TSync
{
public:
	TSync(bool Create=true);
	explicit TSync(TSync&& Move)	{	*this = std::move(Move);	}
	~TSync()						{	Delete();	}
	
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
	bool	IsValid() const			{ return mSyncObject != nullptr; }
#else
	bool	IsValid() const			{ return mSyncObject; }
#endif
	void	Delete();
	void	Wait(const char* TimerName=nullptr);
	
	TSync& operator=(TSync&& Move)
	{
		if ( this != &Move )
		{
			//	delete existing sync
			Delete();
			
			mSyncObject = Move.mSyncObject;
			
			//	stolen the resource
			Move.mSyncObject = nullptr;
		}
		return *this;
	}
	
public:
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
	GLsync				mSyncObject;
#else
	bool				mSyncObject;	//	dummy for cleaner code
#endif
};

//	clever class which does the binding, auto texture mapping, and unbinding
//	why? so we can use const TShaders and share them across threads
class Opengl::TShaderState : public Soy::TUniformContainer//, public NonCopyable
{
public:
	TShaderState(const TShader& Shader);
	~TShaderState();
	
	virtual bool	SetUniform(const char* Name,const int& v) override;
	virtual bool	SetUniform(const char* Name,const float& v) override;
	virtual bool	SetUniform(const char* Name,const vec2f& v) override;
	virtual bool	SetUniform(const char* Name,const vec3f& v) override;
	virtual bool	SetUniform(const char* Name,const vec4f& v) override;
	virtual bool	SetUniform(const char* Name,const Opengl::TTexture& Texture);	//	special case which tracks how many textures are bound
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& Texture) override	{	return SetUniform( Name, Texture.mTexture );	}
	bool			SetUniform(const char* Name,const float3x3& v);
	virtual bool	SetUniform(const char* Name,const SoyPixelsImpl& Texture) override	{	Soy_AssertTodo();	}
	bool			SetUniform(const char* Name,const Soy::TRgb& v)						{	return SetUniform( Name, v.mRgb );	}
	bool			SetUniform(const char* Name,const Soy::THsl& v)						{	return SetUniform( Name, v.mHsl );	}

	template<typename TYPE>
	bool	SetUniform(const std::string& Name,const TYPE& v)
	{
		return SetUniform( Name.c_str(), v );
	}
									   

	static void		BindTexture(size_t TextureIndex,TTexture Texture,size_t UniformIndex);	//	use to unbind too
	static void		BindTexture(size_t TextureIndex,TTexture Texture,std::function<void(GLuint)> SetUniform);
	
	
public:
	const TShader&	mShader;
	size_t			mTextureBindCount;
};

class Opengl::TShader
{
public:
	TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const std::string& ShaderName,Opengl::TContext& Context);
	~TShader();
	
	TShaderState	Bind();	//	let this go out of scope to unbind
	SoyGraphics::TUniform		GetUniform(const char* Name) const
	{
		auto* Uniform = mUniforms.Find( Name );
		return Uniform ? *Uniform : SoyGraphics::TUniform();
	}
	SoyGraphics::TUniform		GetAttribute(const char* Name) const
	{
		auto* Uniform = mAttributes.Find( Name );
		return Uniform ? *Uniform : SoyGraphics::TUniform();
	}
	
	void			SetUniform(const SoyGraphics::TUniform& Uniform,ArrayBridge<float>&& Floats);
	void			SetUniform(const SoyGraphics::TUniform& Uniform,const TTexture& Texture,size_t BindIndex);
	void			SetUniform(const SoyGraphics::TUniform& Uniform,bool Bool);

public:
	TAsset			mProgram;
	TAsset			mVertexShader;
	TAsset			mFragmentShader;
	
	Array<SoyGraphics::TUniform>	mUniforms;
	Array<SoyGraphics::TUniform>	mAttributes;
};



class Opengl::TGeometry
{
public:
	TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& Indexes,const SoyGraphics::TGeometryVertex& Vertex);
	~TGeometry();

	void	Draw();
	bool	IsValid() const;

	void	Bind();
	void	Unbind();
	
	static void	EnableAttribs(const SoyGraphics::TGeometryVertex& Descripton,bool Enable=true);

public:
	SoyGraphics::TGeometryVertex	mVertexDescription;	//	for attrib binding info
	GLuint			mVertexArrayObject;
	GLuint			mVertexBuffer;
	GLsizei			mVertexCount;
	GLuint			mIndexBuffer;
	GLsizei			mIndexCount;
	GLenum			mIndexType;		//	short/int etc data stored in index buffer
};



class Opengl::TTextureMeta
{
public:
	TTextureMeta(const SoyPixelsMeta& Meta, GLenum Type) :
		mMeta	( Meta ),
		mType	( Type )
	{
	}
	
public:
	SoyPixelsMeta		mMeta;
	GLenum				mType;
};
std::ostream& operator<<(std::ostream& out,const Opengl::TTextureMeta& Meta);


class Opengl::TTexture
{
public:
	//	invalid
	explicit TTexture() :
		mAutoRelease	( false ),
		mType			( GL_TEXTURE_2D )
	{
	}
	TTexture(TTexture&& Move)
	{
		*this = std::move(Move);
	}
	TTexture(const TTexture& Reference)
	{
		*this = Reference;
	}
	explicit TTexture(SoyPixelsMeta Meta,GLenum Type);	//	alloc
	
	//	reference from external
	TTexture(void* TexturePtr,const SoyPixelsMeta& Meta,GLenum Type) :
	mMeta			( Meta ),
	mType			( Type ),
	mAutoRelease	( false ),
#if defined(TARGET_ANDROID)
	mTexture		( reinterpret_cast<GLuint>(TexturePtr) )
#elif defined(TARGET_OSX)
	mTexture		( static_cast<GLuint>(reinterpret_cast<GLuint64>(TexturePtr)) )
#elif defined(TARGET_IOS)
	mTexture		( static_cast<GLuint>( reinterpret_cast<intptr_t>(TexturePtr) ) )
#elif defined(TARGET_WINDOWS)
	mTexture		( static_cast<GLuint>(reinterpret_cast<intptr_t>(TexturePtr)) )
#endif
	{
		Soy::Assert( IsValid(), "Not a valid opengl texture" );
	}
	
	TTexture(GLuint TextureName,const SoyPixelsMeta& Meta,GLenum Type) :
		mMeta			( Meta ),
		mType			( Type ),
		mAutoRelease	( false ),
		mTexture		( TextureName )
	{
	}
	
	~TTexture()
	{
		if ( mAutoRelease )
			Delete();
	}
	
	
	TTexture& operator=(const TTexture& Weak)
	{
		if ( this != &Weak )
		{
			mAutoRelease = false;
			mTexture = Weak.mTexture;
			mMeta = Weak.mMeta;
			mType = Weak.mType;
			mClientBuffer = Weak.mClientBuffer;
			mWriteSync = Weak.mWriteSync;
		}
		return *this;
	}

	TTexture& operator=(TTexture&& Move)
	{
		if ( this != &Move )
		{
			mAutoRelease = Move.mAutoRelease;
			mTexture = Move.mTexture;
			mMeta = Move.mMeta;
			mType = Move.mType;
			mClientBuffer = Move.mClientBuffer;
			mWriteSync = Move.mWriteSync;
			
			//	stolen the resource
			Move.mAutoRelease = false;
			Move.mWriteSync.reset();
		}
		return *this;
	}

	SoyPixelsMeta		GetInternalMeta(GLenum& Type) const;	//	read meta from opengl

	SoyPixelsMeta		GetMeta() const		{	return mMeta;	}
	size_t				GetWidth() const	{	return mMeta.GetWidth();	}
	size_t				GetHeight() const	{	return mMeta.GetHeight();	}
	SoyPixelsFormat::Type	GetFormat() const	{	return mMeta.GetFormat();	}

	bool				Bind() const;
	void				Unbind() const;
	bool				IsValid(bool InvasiveTest=true) const;	//	only use InvasiveTest on opengl threads
	void				Delete();
	void				Write(const SoyPixelsImpl& Pixels,SoyGraphics::TTextureUploadParams Params=SoyGraphics::TTextureUploadParams());
	void				Read(SoyPixelsImpl& Pixels,SoyPixelsFormat::Type ForceFormat=SoyPixelsFormat::Invalid,bool Flip=true) const;
	void				SetRepeat(bool Repeat=true);
	void				SetFilter(bool Linear);		//	as soon as we need it, implement min/mag options and mipmap levels rather than nearest/linear
	void				SetClamped()				{	SetRepeat(false);	}
	void				GenerateMipMaps();

	void				OnWrite();
	
	bool				operator==(const TTextureMeta& Meta) const	{	return mMeta == Meta.mMeta && mType == Meta.mType;	}
	bool				operator==(const TTexture& that) const	{	return mTexture == that.mTexture;	}
	bool				operator!=(const TTexture& that) const	{	return mTexture != that.mTexture;	}

public:
	bool				mAutoRelease;
	std::shared_ptr<SoyPixelsImpl>	mClientBuffer;	//	for CPU-buffered textures, it's kept here. ownership should go with mAutoRelease, but shared_ptr maybe takes care of that?
	TAsset				mTexture;
	SoyPixelsMeta		mMeta;
	GLenum				mType;			//	GL_TEXTURE_2D by default. gr: "type" may be the wrong nomenclature here
	std::shared_ptr<TSync>	mWriteSync;	//	a sync queued after a write so we can tell when a write has finished
};

class Opengl::TFbo
{
public:
	TFbo(TTexture Texture);
	~TFbo();
	
	bool		IsValid() const	{	return mFbo.IsValid() && mTarget.IsValid();	}
	bool		Bind();
	void		Unbind();
	void		InvalidateContent();
	void		CheckStatus();
	
	void		Delete(Opengl::TContext& Context,bool Blocking=false);	//	deffered delete
	void		Delete();
	
	size_t		GetAlphaBits() const;
	size_t		GetWidth() const	{	return mTarget.GetWidth();	}
	size_t		GetHeight() const	{	return mTarget.GetHeight();	}
	
public:
	TAsset		mFbo;
	TTexture	mTarget;
};


class Opengl::TPbo
{
public:
	TPbo(SoyPixelsMeta Meta);
	~TPbo();
	
	void		Bind();
	void		Unbind();
	
	void			ReadPixels(GLenum PixelType=GL_UNSIGNED_BYTE);
	const uint8*	LockBuffer();
	void			UnlockBuffer();
	size_t			GetDataSize();
	
public:
	GLuint			mPbo;
	SoyPixelsMeta	mMeta;
};


/*
class Opengl::TGeoQuad
{
public:
	TGeoQuad();
	~TGeoQuad();
	
	bool		IsValid() const	{	return mGeometry.IsValid();	}
	
	TGeometry	mGeometry;
	
	bool		Render();
};


class Opengl::TShaderEosBlit
{
public:
	TShaderEosBlit();
	~TShaderEosBlit();
	
	bool		IsValid() const	{	return mProgram.IsValid();	}
	
	Opengl::TShader	mProgram;
};
*/

