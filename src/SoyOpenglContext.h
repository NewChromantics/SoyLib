//	high level opengl stuff
#pragma once

#include "SoyOpengl.h"
#include "SoyEnum.h"
#include "SoyThread.h"


typedef struct _CGLContextObject       *CGLContextObj;


namespace OpenglExtensions
{
	enum Type
	{
		Invalid,
		AppleClientStorage,
		VertexArrayObjects,
		VertexBuffers,
		DrawBuffers,
	};
	
	DECLARE_SOYENUM( OpenglExtensions );
}

namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)

	class TRenderTargetFbo;
	class TSync;			//	uses fences to sync
	
	
	//	extension bindings
	extern std::function<void(GLuint)>					BindVertexArray;
	extern std::function<void(GLsizei,GLuint*)>			GenVertexArrays;
	extern std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	extern std::function<GLboolean(GLuint)>				IsVertexArray;
	
	extern std::function<void(GLsizei,const GLenum *)>	DrawBuffers;
};



class Opengl::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext();
	virtual ~TContext()				{}

	void			Init();
	void			Iteration()			{	Flush(*this);	}
	virtual bool	Lock() override		{	return true;	}
	virtual void	Unlock() override	{	}
	virtual std::shared_ptr<Opengl::TContext>	CreateSharedContext()	{	return nullptr;	}

	bool			IsSupported(OpenglExtensions::Type Extension)	{	return IsSupported(Extension,this);	}
	static bool		IsSupported(OpenglExtensions::Type Extension,TContext* Context);
	
	void			BindVertexArrayObjectsExtension();
	void			BindVertexBuffersExtension();

#if defined(TARGET_OSX)
	virtual CGLContextObj	GetPlatformContext()	{	return nullptr;	}
#endif
	
public:
	Soy::TVersion	mVersion;
	Soy::TVersion	mShaderVersion;	//	max version supported
	std::string		mDeviceName;
};


//	render/frame buffer
class Opengl::TRenderTarget
{
public:
	TRenderTarget(const std::string& Name) :
		mName	( Name )
	{
	}
	virtual ~TRenderTarget()	{}
	
	virtual bool				Bind()=0;
	virtual void				Unbind()=0;
	virtual Soy::Rectx<size_t>	GetSize()=0;
	
	void			SetViewportNormalised(Soy::Rectf Viewport);
	
	std::string		mName;
};


class Opengl::TRenderTargetFbo : public Opengl::TRenderTarget
{
public:
	//	provide context for non-immediate construction
	TRenderTargetFbo(TFboMeta Meta,Opengl::TContext& Context,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	TRenderTargetFbo(TFboMeta Meta,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	~TRenderTargetFbo()
	{
		mFbo.reset();
	}
	
	virtual bool				Bind();
	virtual void				Unbind();
	virtual Soy::Rectx<size_t>	GetSize();
	TTexture					GetTexture();
	
	std::shared_ptr<TFbo>	mFbo;
	TTexture				mTexture;
};

class Opengl::TSync
{
public:
	TSync();
	
	void	Wait(const char* TimerName=nullptr);
	
private:
#if defined(OPENGL_ES_3) || defined(OPENGL_CORE_3)
	GLsync				mSyncObject;
#endif
};
