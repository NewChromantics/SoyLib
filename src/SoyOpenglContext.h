//	high level opengl stuff
#pragma once

#include "SoyOpengl.h"
#include "SoyEnum.h"
#include "SoyThread.h"


namespace OpenglExtensions
{
	enum Type
	{
		Invalid,
		AppleClientStorage,
		VertexArrayObjects,
	};
	
	DECLARE_SOYENUM( OpenglExtensions );
}

namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)
	class TVersion;

	class TRenderTargetFbo;
	class TSync;			//	uses fences to sync
};


class Opengl::TVersion
{
public:
	TVersion() :
		mMajor	( 0 ),
		mMinor	( 0 )
	{
	}
	TVersion(size_t Major,size_t Minor) :
		mMajor	( Major ),
		mMinor	( Minor )
	{
	}
	explicit TVersion(const std::string& VersionStr);
	
public:
	size_t	mMajor;
	size_t	mMinor;
};

namespace Opengl
{
	inline std::ostream& operator<<(std::ostream &out,const Opengl::TVersion& in)
	{
		out << in.mMajor << '.' << in.mMinor;
		return out;
	}
}


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
	

public:
	TVersion		mVersion;
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
	TSync(Opengl::TContext& Context);
	
	void	Wait(const char* TimerName=nullptr);
	
private:
	GLsync				mSyncObject;
	Opengl::TContext&	mContext;
};
