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
	{	OpenglExtensions::VertexArrayObjects,	"GL_APPLE_vertex_array_object"	},
};


//	gr: only expose extensions here
#if defined(GL_GLEXT_FUNCTION_POINTERS)
#undef GL_GLEXT_FUNCTION_POINTERS
#endif
#include <Opengl/glext.h>

namespace Opengl
{
	std::function<void(GLuint)>					BindVertexArray;
	std::function<void(GLsizei,GLuint*)>		GenVertexArrays;
	std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	std::function<GLboolean(GLuint)>			IsVertexArray;
}


Opengl::TVersion::TVersion(std::string VersionStr) :
	mMajor	( 0 ),
	mMinor	( 0 )
{
	//	strip off prefix's
	//	iphone says: "OpenGL ES 3.0 Apple A7 GPU - 53.13"
	std::string OpenglEsPrefix = "OpenGL ES ";
	if ( Soy::StringBeginsWith(VersionStr,OpenglEsPrefix, false ) )
		VersionStr.erase(0, OpenglEsPrefix.length() );
	
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
	
	Soy::StringSplitByMatches( PushVersions, VersionStr, " .", false );
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

		//	if this is asupported as an EXTENSION, then it needs setting up
		if ( SupportedExtensions.find(OpenglExtensions::VertexArrayObjects) != SupportedExtensions.end() )
		{
			BindVertexArray = glBindVertexArrayAPPLE;
			GenVertexArrays = glGenVertexArraysAPPLE;
			DeleteVertexArrays = glDeleteVertexArraysAPPLE;
			IsVertexArray = glIsVertexArrayAPPLE;
		}
		else
		{
			//	gr: implicitly supported in IOS, version should be correct here
			//	might be supported implicitly in opengl 4 etc (test this!)
			bool ImplicitlySupported = (pContext->mVersion.mMajor >= 3);
			
			SupportedExtensions[OpenglExtensions::VertexArrayObjects] = ImplicitlySupported;
			std::Debug << "Assumed implicit support for OpenglExtensions::VertexArrayObjects is " << (ImplicitlySupported?"true":"false") << std::endl;

			if ( ImplicitlySupported )
			{
				BindVertexArray = glBindVertexArray;
				GenVertexArrays = glGenVertexArrays;
				DeleteVertexArrays = glDeleteVertexArrays;
				IsVertexArray = glIsVertexArray;
			}
		}
		
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

