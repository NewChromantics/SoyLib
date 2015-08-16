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
#if defined(TARGET_WINDOWS)
	{	OpenglExtensions::VertexArrayObjects,	"GL_ARB_vertex_array_object"	},
#else
	{	OpenglExtensions::VertexArrayObjects,	"GL_APPLE_vertex_array_object"	},
#endif
};


//	gr: only expose extensions here
#if defined(TARGET_OSX)
#if defined(GL_GLEXT_FUNCTION_POINTERS)
#undef GL_GLEXT_FUNCTION_POINTERS
#endif
#include <Opengl/glext.h>
#endif


namespace Opengl
{
	std::function<void(GLuint)>					BindVertexArray;
	std::function<void(GLsizei,GLuint*)>		GenVertexArrays;
	std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	std::function<GLboolean(GLuint)>			IsVertexArray;
}


Opengl::TContext::TContext()
{
}

void Opengl::TContext::Init()
{
	//	windows needs Glew to initialise all it's extension stuff
//	#define glewInit() glewContextInit(glewGetContext())
//#define glewIsSupported(x) glewContextIsSupported(glewGetContext(), x)
//#define glewIsExtensionSupported(x) glewIsSupported(x)
#if defined(TARGET_WINDOWS)
	{
		auto Error = glewInit();
		if ( Error != GL_NO_ERROR )
		{
			std::stringstream ErrorStr;
			ErrorStr << "Error initialising opengl: " << GetEnumString( Error );
			throw Soy::AssertException( ErrorStr.str() );
		}
	}
#endif

	//	init version
	auto* VersionString = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
	Soy::Assert( VersionString!=nullptr, "Version string invalid. Context not valid? Not on opengl thread?" );
	
	//	iphone says: "OpenGL ES 3.0 Apple A7 GPU - 53.13"
	mVersion = Soy::TVersion( std::string( VersionString ), "OpenGL ES " );
	
	auto* DeviceString = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
	Soy::Assert( DeviceString!=nullptr, "device string invalid. Context not valid? Not on opengl thread?" );
	mDeviceName = std::string( DeviceString );
	
	//	trigger extensions init early
	IsSupported( OpenglExtensions::Invalid );
	
	std::Debug << "Opengl version: " << mVersion << " on " << mDeviceName << std::endl;
}



bool PushExtension(std::map<OpenglExtensions::Type,bool>& SupportedExtensions,const std::string& ExtensionName)
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
}

//	pre-opengl 3 we have a string to parse
void ParseExtensions(std::map<OpenglExtensions::Type,bool>& SupportedExtensions,const std::string& ExtensionsList)
{
	//	pump split's into map
	auto ParseExtension = [&SupportedExtensions](const std::string& ExtensionName)
	{
		return PushExtension( SupportedExtensions, ExtensionName );
	};
	
	//	parse extensions string
	std::string AllExtensions( ExtensionsList );
	Soy::StringSplitByString( ParseExtension, AllExtensions, " " );
	

}

template<class FUNCTION>
void SetUnsupportedFunction(FUNCTION& Function,const char* Name)
{
	Function = [Name](...)
	{
		std::stringstream Error;
		Error << "Calling unsupported extension " << Name;
		throw Soy::AssertException( Error.str() );

		//	gr: see if we can find a way to set an Opengl IsError for when exceptions aren't enabled
	};
}

//	gr: find a way to merge with above
template<typename RETURN,class FUNCTION>
void SetUnsupportedFunction(FUNCTION& Function,const char* Name,RETURN Return)
{
	Function = [Name,Return](...)
	{
		std::stringstream Error;
		Error << "Calling unsupported extension " << Name;
		throw Soy::AssertException( Error.str() );
		return Return;
	};
}

template<typename FUNCTYPE>
void SetFunction(std::function<FUNCTYPE>& f,void* x)
{
	FUNCTYPE* ff = (FUNCTYPE*)x;
	f = ff;
}

void Opengl::TContext::BindVertexArrayObjectsExtension()
{
	//	implicitly supported in opengl 3 & 4 (ES and CORE)
	bool ImplicitlySupported = (mVersion.mMajor >= 3);
	
#if !defined(TARGET_WINDOWS)
	if ( ImplicitlySupported )
	{
		SupportedExtensions[OpenglExtensions::VertexArrayObjects] = true;
		std::Debug << "Assumed implicit support for OpenglExtensions::VertexArrayObjects" << std::endl;

		BindVertexArray = glBindVertexArray;
		GenVertexArrays = glGenVertexArrays;
		DeleteVertexArrays = glDeleteVertexArrays;
		IsVertexArray = glIsVertexArray;
		return;
	}
#endif
	
	bool IsSupported = SupportedExtensions.find(OpenglExtensions::VertexArrayObjects) != SupportedExtensions.end();
	
	//	extension supported, set functions
	if ( IsSupported )
	{
#if defined(TARGET_OSX)
		BindVertexArray = glBindVertexArrayAPPLE;
		GenVertexArrays = glGenVertexArraysAPPLE;
		DeleteVertexArrays = glDeleteVertexArraysAPPLE;
		IsVertexArray = glIsVertexArrayAPPLE;
#elif defined(TARGET_WINDOWS)
		SetFunction( BindVertexArray, wglGetProcAddress("glBindVertexArray") );
		SetFunction( GenVertexArrays, wglGetProcAddress("glGenVertexArrays") );
		SetFunction( DeleteVertexArrays, wglGetProcAddress("glDeleteVertexArrays") );
		SetFunction( IsVertexArray, wglGetProcAddress("glIsVertexArray") );
 #else
		IsSupported = false;
#endif
	}

	//	not supported (or has been unset due to implementation)
	if ( !IsSupported )
	{
		SupportedExtensions[OpenglExtensions::VertexArrayObjects] = false;
		//	gr: set error/throw function wrappers
		SetUnsupportedFunction(BindVertexArray, "glBindVertexArray" );
		SetUnsupportedFunction(GenVertexArrays, "GenVertexArrays" );
		SetUnsupportedFunction(DeleteVertexArrays, "DeleteVertexArrays" );
		SetUnsupportedFunction(IsVertexArray, "IsVertexArray", (GLboolean)false );
		return;
	}

}


bool Opengl::TContext::IsSupported(OpenglExtensions::Type Extension,Opengl::TContext* pContext)
{
	//	first parse of extensions
	if ( SupportedExtensions.empty() && pContext )
	{
		//	deprecated in opengl 3 so this returns null in that version
		//	could do a version check, or just handle it nicely :)
		//	debug full string in lldb;
		//		set set target.max-string-summary-length 10000
		auto* pAllExtensions = glGetString( GL_EXTENSIONS );
		if ( pAllExtensions )
		{
			std::string AllExtensions( reinterpret_cast<const char*>(pAllExtensions) );
			ParseExtensions( SupportedExtensions, AllExtensions );
		}
		else
		{
			Opengl_IsOkayFlush();
			//	opengl 3 safe way
			GLint ExtensionCount = -1;
			glGetIntegerv( GL_NUM_EXTENSIONS, &ExtensionCount );
			Opengl::IsOkay( "GL_NUM_EXTENSIONS" );
			for ( int i=0;	i<ExtensionCount;	i++ )
			{
				auto* pExtensionName = glGetStringi( GL_EXTENSIONS, i );
				if ( pExtensionName == nullptr )
				{
					std::Debug << "GL_Extension #" << i << " returned null" << std::endl;
					continue;
				}
				
				std::string ExtensionName( reinterpret_cast<const char*>(pExtensionName) );
				PushExtension( SupportedExtensions, ExtensionName );
			}
		}

		pContext->BindVertexArrayObjectsExtension();
		
		//	force an entry so this won't be initialised again (for platforms with no extensions specified)
		SupportedExtensions[OpenglExtensions::Invalid] = false;
	}
	
	return SupportedExtensions[Extension];
}


void Opengl::TRenderTarget::SetViewportNormalised(Soy::Rectf Viewport)
{
	auto Rect = Soy::Rectf( GetSize() );

	Rect.x -= Viewport.x * Rect.w;
	Rect.y -= Viewport.y * Rect.h;
	Rect.w /= Viewport.w;
	Rect.h /= Viewport.h;
	Rect.x /= Viewport.w;
	Rect.y /= Viewport.h;
	
	Opengl::SetViewport( Rect );
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
			SoyPixelsMeta TextureMeta( size_cast<int>(Meta.mSize.x), size_cast<int>(Meta.mSize.y), SoyPixelsFormat::RGBA );
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
		SoyPixelsMeta TextureMeta( size_cast<int>(Meta.mSize.x), size_cast<int>(Meta.mSize.y), SoyPixelsFormat::RGBA );
		mTexture = TTexture( TextureMeta, GL_TEXTURE_2D );
	}

	mFbo.reset( new TFbo( mTexture ) );
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



Opengl::TSync::TSync() :
	mSyncObject	( nullptr )
{
	mSyncObject = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	Opengl::IsOkay("glFenceSync");
}

void Opengl::TSync::Wait(const char* TimerName)
{
	if ( !glIsSync( mSyncObject ) )
		return;

	//glWaitSync( mSyncObject, 0, GL_TIMEOUT_IGNORED );
	GLbitfield Flags = GL_SYNC_FLUSH_COMMANDS_BIT;
	GLuint64 TimeoutMs = 1000;
	GLuint64 TimeoutNs = TimeoutMs * 1000000;
	GLenum Result = GL_INVALID_ENUM;
	do
	{
		Result = glClientWaitSync( mSyncObject, Flags, TimeoutNs );
	}
	while ( Result == GL_TIMEOUT_EXPIRED );
	
	if ( Result == GL_WAIT_FAILED )
		Opengl::IsOkay("glClientWaitSync GL_WAIT_FAILED");

	glDeleteSync( mSyncObject );
	mSyncObject = nullptr;
}

