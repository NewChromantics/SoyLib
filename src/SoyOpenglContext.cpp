#include "SoyOpenglContext.h"



void Opengl::TJobQueue::Push(std::shared_ptr<TJob>& Job)
{
	mLock.lock();
	mJobs.push_back( Job );
	mLock.unlock();
}

void Opengl::TJobQueue::Flush(TContext& Context)
{
	while ( true )
	{
		//	pop task
		mLock.lock();
		std::shared_ptr<TJob> Job;
		auto NextJob = mJobs.begin();
		if ( NextJob != mJobs.end() )
		{
			Job = *NextJob;
			mJobs.erase( NextJob );
		}
		//bool MoreJobs = !mJobs.empty();
		mLock.unlock();
		
		if ( !Job )
			break;
		
		//	lock the context
		if ( !Context.Lock() )
		{
			mLock.lock();
			mJobs.insert( mJobs.begin(), Job );
			mLock.unlock();
			break;
		}
		auto ContextLock = SoyScope( []{}, [&Context]{Context.Unlock();} );
		
		//	execute task, if it returns false, we don't run any more this time and re-insert
		if ( !Job->Run( std::Debug ) )
		{
			mLock.lock();
			mJobs.insert( mJobs.begin(), Job );
			mLock.unlock();
			break;
		}
	}
}


void Opengl::TContext::PushJob(std::function<bool ()> Function)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJob( Job );
}



Opengl::TRenderTargetFbo::TRenderTargetFbo(TFboMeta Meta,Opengl::TContext& Context,Opengl::TTexture ExistingTexture) :
	TRenderTarget	( Meta.mName ),
	mTexture		( ExistingTexture )
{
	auto CreateFbo = [this,Meta]()
	{
		Opengl_IsOkay();

		//	create texture
		if ( !mTexture.IsValid() )
		{
			SoyPixelsMetaFull TextureMeta( Meta.mSize.x, Meta.mSize.y, SoyPixelsFormat::RGBA );
			mTexture = TTexture( TextureMeta, GL_TEXTURE_2D );
			/*
			 glGenTextures(1, &mTexture.mTexture.mName );
			 //	set mip-map levels to 0..0
			 mTexture.Bind();
			 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			 Opengl_IsOkay();
			 */
		}
		
		try
		{
			mFbo.reset( new TFbo( mTexture ) );
		}
		catch ( std::exception& e )
		{
			std::Debug << "Failed to create FBO: " << e.what() << std::endl;
			mFbo.reset();
		}
		return true;
	};
	
	//	create fbo object in the context, flush the commands and wait for the result. then throw if it fails!
	Context.PushJob( CreateFbo );
	Context.FlushJobs();
	
	//	check result of fbo
	Soy::Assert( mFbo!=nullptr, "Failed to create FBO" );
}
	
bool Opengl::TRenderTargetFbo::Bind()
{
	if ( !mFbo )
		return false;
	return mFbo->Bind();
}

void Opengl::TRenderTargetFbo::Unbind()
{
	if ( !mFbo )
		return;
	
	//	needed?
	glFlush();
	glFinish();
	
	mFbo->Unbind();
}

Soy::Rectx<size_t> Opengl::TRenderTargetFbo::GetSize()
{
	return Soy::Rectx<size_t>(0,0,mTexture.mMeta.GetWidth(),mTexture.mMeta.GetHeight());
}

Opengl::TTexture Opengl::TRenderTargetFbo::GetTexture()
{
	return mTexture;
}


