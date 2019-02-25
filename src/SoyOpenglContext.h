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
		FrameBuffers,
		GenerateMipMap,
		ImageExternal,
		ImageExternalSS3,
		Sync,
	};
	
	DECLARE_SOYENUM( OpenglExtensions );
}

namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)

	class TRenderTargetFbo;
	
	
	//	extension bindings
	extern std::function<void(GLuint)>					BindVertexArray;
	extern std::function<void(GLsizei,GLuint*)>			GenVertexArrays;
	extern std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	extern std::function<GLboolean(GLuint)>				IsVertexArray;
	
	extern std::function<void(GLsizei,GLuint*)>			GenFramebuffers;
	extern std::function<void(GLenum,GLuint)>			BindFramebuffer;
	extern std::function<void(GLenum,GLenum,GLenum,GLuint,GLint)>	FramebufferTexture2D;
	extern std::function<void(GLsizei,const GLuint*)>	DeleteFramebuffers;
	extern std::function<GLboolean(GLuint)>				IsFramebuffer;
	extern std::function<GLenum(GLenum)>				CheckFramebufferStatus;
	extern std::function<void(GLenum,GLenum,GLenum,GLint*)>		GetFramebufferAttachmentParameteriv;

	extern std::function<void(GLsizei,const GLenum *)>	DrawBuffers;

	extern std::function<void(GLenum)>					GenerateMipmap;

	extern std::function<GLsync(GLenum,GLbitfield)>		FenceSync;
	extern std::function<void(GLsync)>					DeleteSync;
	extern std::function<GLboolean(GLsync)>				IsSync;
	extern std::function<GLenum(GLsync,GLbitfield,GLuint64)>	ClientWaitSync;
};



class Opengl::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext();
	virtual ~TContext()				{}

	void			Init();
    bool            IsInitialised() const   {   return mVersion != Soy::TVersion();  }
	void			Iteration()			{	Flush(*this);	}
	virtual void	Lock() override;
	virtual void	Unlock() override;
	virtual bool	IsLocked(std::thread::id Thread) override;
	virtual std::shared_ptr<Opengl::TContext>	CreateSharedContext()	{	return nullptr;	}
	bool			HasMultithreadAccess() const	{	return false;	}
	virtual void	WaitForThreadToFinish()	{};

	bool			IsSupported(OpenglExtensions::Type Extension)	{	return IsSupported(Extension,this);	}
	static bool		IsSupported(OpenglExtensions::Type Extension,TContext* Context);
	

#if defined(TARGET_OSX)
	virtual CGLContextObj	GetPlatformContext()	{	return nullptr;	}
#endif
	
private:
	void			BindVertexArrayObjectsExtension();
	void			BindDrawBuffersExtension();
	void			BindFramebuffersExtension();
	void			BindGenerateMipMapExtension();
	void			BindSyncExtension();

	void			BindExtension(OpenglExtensions::Type Extension,std::function<void(bool)> BindFunctions,std::function<void(void)> BindUnsupportedFunctions);

public:
	Soy::TVersion	mVersion;
	Soy::TVersion	mShaderVersion;	//	max version supported
	std::string		mDeviceName;
	
	size_t			mCurrentTextureSlot = 0;	//	this is a little high level, but is at at Context level
	
private:
	std::mutex			mLockLock;		//	we have our own now, the mLockedThread means we can tell if we have the lock. We should NOT be recursive, as we need locks to balance
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
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
	
	virtual void				Bind()=0;
	virtual void				Unbind()=0;
	virtual Soy::Rectx<size_t>	GetSize()=0;
	
	void			SetViewportNormalised(Soy::Rectf Viewport);
	
	std::string		mName;
};


class Opengl::TRenderTargetFbo : public Opengl::TRenderTarget
{
public:
	//	provide context for non-immediate construction
	TRenderTargetFbo(const std::string& Name,TTexture ExistingTexture);
	~TRenderTargetFbo()
	{
		mFbo.reset();
	}
	
	virtual void				Bind() override;
	virtual void				Unbind() override;
	virtual Soy::Rectx<size_t>	GetSize() override;
	TTexture					GetTexture();
	
public:
	bool					mGenerateMipMaps;
	std::shared_ptr<TFbo>	mFbo;
	TTexture				mTexture;
};

