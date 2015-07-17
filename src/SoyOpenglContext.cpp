#include "SoyOpenglContext.h"
#include "SoyEnum.h"


//	todo: move this to context only
namespace Opengl
{
	//	static method is slow, cache on a context!
	std::map<OpenglExtensions::Type,bool>	SupportedExtensions;
};


std::map<OpenglExtensions::Type,std::string> OpenglExtensions::EnumMap =
{
	{	OpenglExtensions::Invalid,				"Invalid"	},
	{	OpenglExtensions::AppleClientStorage,	"GL_APPLE_client_storage"	},
};


Opengl::TVersion::TVersion(const std::string& VersionStr) :
	mMajor	( 0 ),
	mMinor	( 0 )
{
	int PartCounter = 0;
	auto PushVersions = [&PartCounter,this](const std::string& PartStr)
	{
		//	got all we need
		if ( PartCounter >= 2 )
			return false;
		
		auto& PartInt = (PartCounter==0) ? mMajor : mMinor;
		Soy::StringToType( PartInt, PartStr );
		
		PartCounter++;
		return true;
	};
	
	Soy::StringSplitByMatches( PushVersions, VersionStr, " ." );
}


void Opengl::TJobQueue::Push(std::shared_ptr<TJob>& Job)
{
	mLock.lock();
	mJobs.push_back( Job );
	mLock.unlock();
}

void Opengl::TJobQueue::Flush(TContext& Context)
{
	//	flush errors
	Opengl::IsOkay("JobQueue flush flush",false);
	
	auto AutoUnlockContext = [&Context]
	{
		Opengl::IsOkay("Post job flush",false);
		Context.Unlock();
	};

	
	ofScopeTimerWarning LockTimer("Waiting for job lock",4,false);
	while ( true )
	{
		LockTimer.Start();
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
		LockTimer.Stop();
		
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
		
		auto ContextLock = SoyScope( nullptr, AutoUnlockContext );
		
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


Opengl::TContext::TContext()
{
}

void Opengl::TContext::Init()
{
	//	init version
	auto* VersionString = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
	Soy::Assert( VersionString!=nullptr, "Version string invalid. Context not valid? Not on opengl thread?" );

	mVersion = Opengl::TVersion( std::string( VersionString ) );
	
	std::Debug << "Opengl version: " << mVersion << std::endl;
}


void Opengl::TContext::PushJob(std::function<bool ()> Function)
{
	std::shared_ptr<TJob> Job( new TJob_Function( Function ) );
	PushJob( Job );
}

bool Opengl::TContext::IsSupported(OpenglExtensions::Type Extension)
{
	//	first parse of extensions
	if ( SupportedExtensions.empty() )
	{
		auto* pAllExtensions = glGetString( GL_EXTENSIONS );
		if ( !Soy::Assert( pAllExtensions != nullptr, "GL_EXTENSIONS not returned... outside opengl thread?" ) )
			return false;
		
		//	pump split's into map
		auto ParseExtension = [](const std::string& ExtensionName)
		{
			auto ExtensionType = OpenglExtensions::ToType( ExtensionName );

			//	not one we explicitly support
			if ( ExtensionType == OpenglExtensions::Invalid )
			{
				std::Debug << "Unhandled/ignored extension: " << ExtensionName << std::endl;
				return true;
			}
			
			//	set support in the map
			SupportedExtensions[ExtensionType] = true;
			std::Debug << "Extension supported: " << OpenglExtensions::ToString( ExtensionType ) << std::endl;
			
			return true;
		};

		//	parse extensions string
		std::string AllExtensions( reinterpret_cast<const char*>(pAllExtensions) );
		Soy::StringSplitByString( ParseExtension, AllExtensions, " " );
		
		//	force an entry so this won't be initialised again (for platforms with no extensions specified)
		SupportedExtensions[OpenglExtensions::Invalid] = false;
	}
	
	return SupportedExtensions[Extension];
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
			SoyPixelsMetaFull TextureMeta( size_cast<int>(Meta.mSize.x), size_cast<int>(Meta.mSize.y), SoyPixelsFormat::RGBA );
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
	
	std::Debug << "Deffering FBO rendertarget creation to " << ExistingTexture.mMeta << std::endl;
	Context.PushJob( CreateFbo );
}


Opengl::TRenderTargetFbo::TRenderTargetFbo(TFboMeta Meta,Opengl::TTexture ExistingTexture) :
	TRenderTarget	( Meta.mName ),
	mTexture		( ExistingTexture )
{
	Opengl_IsOkay();
		
	//	create texture
	if ( !mTexture.IsValid() )
	{
		SoyPixelsMetaFull TextureMeta( size_cast<int>(Meta.mSize.x), size_cast<int>(Meta.mSize.y), SoyPixelsFormat::RGBA );
		mTexture = TTexture( TextureMeta, GL_TEXTURE_2D );
	}

	//	re-throw errors
	try
	{
		mFbo.reset( new TFbo( mTexture ) );
	}
	catch ( std::exception& e )
	{
		throw;
	};
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
	
	static bool Flush = false;
	if ( Flush )
	{
		//	needed?
		glFlush();
		glFinish();
	}
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


