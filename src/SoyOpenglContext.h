//	high level opengl stuff
#pragma once

#include "SoyOpengl.h"



namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)

	class TRenderTargetFbo;
	
	//	gr: renamed this to job to replace with SoyJob's later
	class TJobQueue;
	class TJob;
	class TJob_Function;
};




class Opengl::TJob
{
public:
	//	returns "im finished"
	//	return false to stop any more tasks being run and re-insert this in the queue
	virtual bool		Run(std::ostream& Error)=0;
	
	//	todo:
	//SoyEvent<bool>		mOnFinished;
};



class Opengl::TJob_Function : public TJob
{
public:
	TJob_Function(std::function<bool ()> Function) :
		mFunction	( Function )
	{
	}

	virtual bool		Run(std::ostream& Error)
	{
		return mFunction();
	}
	
	std::function<bool ()>	mFunction;
};



class Opengl::TJobQueue
{
public:
	void		Push(std::shared_ptr<TJob>& Job);
	void		Flush(TContext& Context);

private:
	//	gr: change this to a nice soy ringbuffer
	std::vector<std::shared_ptr<TJob>>	mJobs;
	std::recursive_mutex				mLock;
};

class Opengl::TContext
{
public:
	virtual ~TContext()				{}

	void			Iteration()		{	FlushJobs();	}
	virtual bool	Lock()			{	return true;	}
	virtual void	Unlock()		{	}
	void			PushJob(std::function<bool(void)> Lambda);
	void			PushJob(std::shared_ptr<TJob>& Job)			{	mJobQueue.Push( Job );	}
	void			FlushJobs()		{	mJobQueue.Flush( *this );	}
	
	TJobQueue		mJobQueue;
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
		mTexture.Delete();
	}
	
	virtual bool				Bind();
	virtual void				Unbind();
	virtual Soy::Rectx<size_t>	GetSize();
	TTexture					GetTexture();
	
	std::shared_ptr<TFbo>	mFbo;
	TTexture				mTexture;
};
