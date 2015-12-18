#include "SoyOpenglContext.h"
#include "SoyEnum.h"



//	gr: only expose extensions in this file
#if defined(TARGET_OSX)
#if defined(GL_GLEXT_FUNCTION_POINTERS)
#undef GL_GLEXT_FUNCTION_POINTERS
#endif
#include <Opengl/glext.h>
#endif

#if defined(TARGET_ANDROID)
#include <EGL/egl.h>
#endif

#if defined(TARGET_IOS) && (OPENGL_ES==2)
//#include <OpenGLES/EGL/egl.h>
#endif


//	todo: move this to context only
namespace Opengl
{
	//	static method is slow, cache on a context!
	std::map<OpenglExtensions::Type,bool>			SupportedExtensions;
	extern std::map<OpenglExtensions::Type,Soy::TVersion>	ImplicitExtensions;
};


std::map<OpenglExtensions::Type,std::string> OpenglExtensions::EnumMap =
{
	{	OpenglExtensions::Invalid,				"Invalid"	},
	{	OpenglExtensions::AppleClientStorage,	"GL_APPLE_client_storage"	},
#if defined(TARGET_WINDOWS)
	{	OpenglExtensions::VertexArrayObjects,	"GL_ARB_vertex_array_object"	},
#elif defined(TARGET_OSX)
	{	OpenglExtensions::VertexArrayObjects,	"GL_APPLE_vertex_array_object"	},
#elif defined(TARGET_ANDROID) || defined(TARGET_IOS)
	{	OpenglExtensions::VertexArrayObjects,	"GL_OES_vertex_array_object"	},
#endif
	{	OpenglExtensions::DrawBuffers,			"glDrawBuffer"	},
	{	OpenglExtensions::FrameBuffers,			"glGenFramebuffers"	},
	{	OpenglExtensions::ImageExternal,		"GL_OES_EGL_image_external"	},
	{	OpenglExtensions::ImageExternalSS3,		"GL_OES_EGL_image_external_essl3"	},
};

std::map<OpenglExtensions::Type,Soy::TVersion> Opengl::ImplicitExtensions =
{
	{	OpenglExtensions::VertexArrayObjects,	Soy::TVersion(3,0)	},
	{	OpenglExtensions::DrawBuffers,			Soy::TVersion(3,0)	},
	{	OpenglExtensions::FrameBuffers,			Soy::TVersion(3,0)	},
};



namespace Opengl
{
	std::function<void(GLuint)>					BindVertexArray;
	std::function<void(GLsizei,GLuint*)>		GenVertexArrays;
	std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	std::function<GLboolean(GLuint)>			IsVertexArray;
	
	std::function<void(GLsizei,GLuint*)>			GenFramebuffers;
	std::function<void(GLenum,GLuint)>				BindFramebuffer;
	std::function<void(GLenum,GLenum,GLenum,GLuint,GLint)>	FramebufferTexture2D;
	std::function<void(GLsizei,const GLuint*)>		DeleteFramebuffers;
	std::function<GLboolean(GLuint)>				IsFramebuffer;
	std::function<GLenum(GLenum)>					CheckFramebufferStatus;
	std::function<void(GLenum,GLenum,GLenum,GLint*)>	GetFramebufferAttachmentParameteriv;

	std::function<void(GLsizei,const GLenum *)>	DrawBuffers;

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
	
	//	get shader version
	//	they SHOULD align... but just in case http://stackoverflow.com/a/19022416/355753
	auto* ShaderVersionString = reinterpret_cast<const char*>( glGetString( GL_SHADING_LANGUAGE_VERSION ) );
	if ( ShaderVersionString )
	{
		std::string Preamble;
#if defined(OPENGL_ES)
		Preamble = "OpenGL ES GLSL ES";
#endif
		mShaderVersion = Soy::TVersion( std::string(ShaderVersionString), Preamble );
		std::Debug << "Set shader version " << mShaderVersion << " from " << std::string(ShaderVersionString) << std::endl;
		
		//	gr: on android, we cannot use external_OES and glsl3, force downgrade - make this an option somewhere
		//	note4:	ES3.1 and GLSL 3.10	on samsung note4 allows external OES in shader
		//			ES3.1 and GLSL 3.10 on samsung s6 does NOT allow external OES in shader
		//	todo: make a quick test-shader, compile it NOW, and if it fails, fallback
#if defined(TARGET_ANDROID)
		//	gr: non-edge s6 didn't drop down versions, but not sure what shader version it is, so just forcing this
		//	gr: s6 has 3.1, and this wasn't triggered??
		//if ( mShaderVersion >= Soy::TVersion(3,0) )
		{
			Soy::TVersion NewVersion(1,00);
			std::Debug << "Downgrading opengl shader version from " << mShaderVersion << " to " << NewVersion << " as 3.00 doesn't support GL_OES_EGL_image_external on some phones" << std::endl;
			mShaderVersion = NewVersion;
		}
#endif
	}
	else
	{
#if defined(OPENGL_ES)
		Soy::TVersion NewVersion(1,00);
#else
		Soy::TVersion NewVersion(1,10);
#endif
		std::Debug << "Warning: GL_SHADING_LANGUAGE_VERSION missing, assuming shader version " << NewVersion << std::endl;
		mShaderVersion = NewVersion;
	}

	
	auto* DeviceString = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
	Soy::Assert( DeviceString!=nullptr, "device string invalid. Context not valid? Not on opengl thread?" );
	mDeviceName = std::string( DeviceString );
	
	//	trigger extensions init early
	IsSupported( OpenglExtensions::Invalid );
	
	std::Debug << "Opengl version: " << mVersion << " on " << mDeviceName << std::endl;
}


bool Opengl::TContext::IsLocked(std::thread::id Thread)
{
	//	check for any thread
	if ( Thread == std::thread::id() )
		return mLockedThread!=std::thread::id();
	else
		return mLockedThread == Thread;
}

bool Opengl::TContext::Lock()
{
	Soy::Assert( mLockedThread == std::thread::id(), "context already locked" );
	
	mLockedThread = std::this_thread::get_id();
	return true;
}

void Opengl::TContext::Unlock()
{
	auto ThisThread = std::this_thread::get_id();
	Soy::Assert( mLockedThread != std::thread::id(), "context not locked to wrong thread" );
	Soy::Assert( mLockedThread == ThisThread, "context not unlocked from wrong thread" );
	mLockedThread = std::thread::id();
}


bool PushExtension(std::map<OpenglExtensions::Type,bool>& SupportedExtensions,const std::string& ExtensionName,ArrayBridge<std::string>& UnhandledExtensions)
{
	auto ExtensionType = OpenglExtensions::ToType( ExtensionName );
	
	//	not one we explicitly support
	if ( ExtensionType == OpenglExtensions::Invalid )
	{
		UnhandledExtensions.PushBack( ExtensionName );
		return true;
	}
	
	//	set support in the map
	SupportedExtensions[ExtensionType] = true;
	std::Debug << "Extension supported: " << OpenglExtensions::ToString( ExtensionType ) << std::endl;
	
	return true;
}

bool PushExtension(std::map<OpenglExtensions::Type,bool>& SupportedExtensions,const std::string& ExtensionName,ArrayBridge<std::string>&& UnhandledExtensions)
{
	return PushExtension( SupportedExtensions, ExtensionName, UnhandledExtensions );
}

//	pre-opengl 3 we have a string to parse
void ParseExtensions(std::map<OpenglExtensions::Type,bool>& SupportedExtensions,const std::string& ExtensionsList,ArrayBridge<std::string>&& UnhandledExtensions)
{
	//	pump split's into map
	auto ParseExtension = [&SupportedExtensions,&UnhandledExtensions](const std::string& ExtensionName)
	{
		return PushExtension( SupportedExtensions, ExtensionName, UnhandledExtensions );
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


#if defined(TARGET_ANDROID)

/*
void SetFunction(void (* xxxx)())
{
	//FUNCTYPE* ff = (FUNCTYPE*)x;
	//f = ff;
}
*/
template<typename FUNCTYPE>
void SetFunction(std::function<FUNCTYPE>& f,void (* xxxx)() )
{
	if ( !xxxx )
		throw Soy::AssertException("Function not found");
	
	FUNCTYPE* ff = (FUNCTYPE*)xxxx;
	f = ff;
}

#else
template<typename FUNCTYPE>
void SetFunction(std::function<FUNCTYPE>& f,void* x)
{
	if ( !x )
		throw Soy::AssertException("Function not found");
	
	FUNCTYPE* ff = (FUNCTYPE*)x;
	f = ff;
}
#endif




template<typename BINDFUNCTIONTYPE,typename REALFUNCTIONTYPE>
void BindFunction(BINDFUNCTIONTYPE& BindFunction,const std::initializer_list<const char*>& FunctionNames,REALFUNCTIONTYPE RealFunction,bool ForceUnsupported=false)
{
	if ( ForceUnsupported )
	{
		BindFunction = RealFunction;
		return;
	}

	bool Success = false;	//	gr; can replace with BindFunction != null?
	std::string LastError;
	//	try each name until we get a non-throwing case
	for ( auto FunctionName : FunctionNames )
	{
		try
		{
#if defined(TARGET_WINDOWS)
			SetFunction( BindFunction, wglGetProcAddress(FunctionName) );
#elif defined(TARGET_ANDROID)
			SetFunction( BindFunction, eglGetProcAddress(FunctionName) );
#else
			if ( RealFunction == nullptr )
				throw Soy::AssertException("No function on this platform");
			BindFunction = RealFunction;
#endif
			Success = true;
			break;
		}
		catch(std::exception& e)
		{
			LastError = e.what();
		}
	}
	if ( !Success )
		throw Soy::AssertException( LastError );
}


void BindExtension(bool& Supported,bool ImplicitSupport,std::function<void(bool)> BindFunctions,std::function<void(void)> BindUnsupportedFunctions)
{
	try
	{
		if ( !Supported )
			throw Soy::AssertException("Not supported");

		BindFunctions( ImplicitSupport );
	}
	catch(std::exception& e)
	{
		Supported = false;
		BindUnsupportedFunctions();
	}
}


void Opengl::TContext::BindExtension(OpenglExtensions::Type Extension,std::function<void(bool)> BindFunctions,std::function<void(void)> BindUnsupportedFunctions)
{
	bool IsSupported = SupportedExtensions.find(Extension) != SupportedExtensions.end();
	bool ImplicitSupport = mVersion >= ImplicitExtensions[Extension];

	//	if we have the extension, or implicit support, we expect it to be supported...
	SupportedExtensions[Extension] = IsSupported || ImplicitSupport;

	//	now bind (which will unset supported extensions)
	::BindExtension( SupportedExtensions[Extension], ImplicitSupport, BindFunctions, BindUnsupportedFunctions );
}



void Opengl::TContext::BindVertexArrayObjectsExtension()
{
	auto BindFunctions = [&](bool ImplicitSupport)
	{
#if defined(TARGET_OSX)
		auto Real_BindVertexArray = ( ImplicitSupport ) ? glBindVertexArray : glBindVertexArrayAPPLE;
		auto Real_GenVertexArraysy = ( ImplicitSupport ) ? glGenVertexArrays : glGenVertexArraysAPPLE;
		auto Real_DeleteVertexArrays = ( ImplicitSupport ) ? glDeleteVertexArrays : glDeleteVertexArraysAPPLE;
		auto Real_IsVertexArray = ( ImplicitSupport ) ? glIsVertexArray : glIsVertexArrayAPPLE;
#elif defined(TARGET_WINDOWS) || defined(TARGET_ANDROID) || (defined(TARGET_IOS) && (OPENGL_ES==3))
		auto Real_BindVertexArray = glBindVertexArray;
		auto Real_GenVertexArraysy = glGenVertexArrays;
		auto Real_DeleteVertexArrays = glDeleteVertexArrays;
		auto Real_IsVertexArray = glIsVertexArray;
#elif defined(TARGET_IOS) && (OPENGL_ES==2)
		auto Real_BindVertexArray = glBindVertexArrayOES;
		auto Real_GenVertexArraysy = glGenVertexArraysOES;
		auto Real_DeleteVertexArrays = glDeleteVertexArraysOES;
		auto Real_IsVertexArray = glIsVertexArrayOES;
#endif
		BindFunction( BindVertexArray, {"glBindVertexArray","glBindVertexArrayOES"}, Real_BindVertexArray );
		BindFunction( GenVertexArrays, {"glGenVertexArrays","glGenVertexArraysOES"}, Real_GenVertexArraysy );
		BindFunction( DeleteVertexArrays, {"glDeleteVertexArrays","glDeleteVertexArraysOES"}, Real_DeleteVertexArrays );
		BindFunction( IsVertexArray, {"glIsVertexArray","glIsVertexArrayOES"}, Real_IsVertexArray );
	};

	auto BindUnsupportedFunctions = []
	{
		//	gr: set error/throw function wrappers
		SetUnsupportedFunction(BindVertexArray, "glBindVertexArray" );
		SetUnsupportedFunction(GenVertexArrays, "GenVertexArrays" );
		SetUnsupportedFunction(DeleteVertexArrays, "DeleteVertexArrays" );
		SetUnsupportedFunction(IsVertexArray, "IsVertexArray", (GLboolean)false );
	};

	BindExtension( OpenglExtensions::VertexArrayObjects, BindFunctions, BindUnsupportedFunctions );
}



void Opengl::TContext::BindDrawBuffersExtension()
{
	auto BindFunctions = [&](bool ImplcitSupport)
	{
		bool ForceUnsupported = false;
	#if defined(TARGET_OSX)
		auto RealFunction = glDrawBuffersARB;
	#elif defined(TARGET_IOS) && (OPENGL_ES==3)
		auto RealFunction = glDrawBuffers;
		//	does not exist on ES2, but dont need to replace it
	#elif defined(TARGET_IOS) || defined(TARGET_ANDROID)
		auto RealFunction = [](GLsizei,const GLenum *){}
		if ( mVersion.mMajor <= 2 )
		{
			ForceUnsupported = true;
		}
	#else
		auto RealFunction = nullptr;
	#endif
		BindFunction( DrawBuffers, {"glDrawBuffers"}, RealFunction, ForceUnsupported );
	};

	auto BindUnsupportedFunctions = []
	{
		SetUnsupportedFunction(DrawBuffers, "glDrawBuffers" );
	};

	BindExtension( OpenglExtensions::DrawBuffers, BindFunctions, BindUnsupportedFunctions );
}




void Opengl::TContext::BindFramebuffersExtension()
{
	auto BindFunctions = [&](bool ImplicitSupport)
	{
		BindFunction( GenFramebuffers, {"glGenFramebuffers","glGenFramebuffersEXT"}, glGenFramebuffers );
		BindFunction( BindFramebuffer,  {"glBindFramebuffer"}, glBindFramebuffer );
		BindFunction( FramebufferTexture2D, {"glFramebufferTexture2D"}, glFramebufferTexture2D );
		BindFunction( DeleteFramebuffers, {"glDeleteFramebuffers"}, glDeleteFramebuffers );
		BindFunction( IsFramebuffer, {"glIsFramebuffer"}, glIsFramebuffer );
		BindFunction( CheckFramebufferStatus, {"glCheckFramebufferStatus"}, glCheckFramebufferStatus );
		BindFunction( GetFramebufferAttachmentParameteriv, {"glGetFramebufferAttachmentParameteriv"}, glGetFramebufferAttachmentParameteriv );
	};

	auto BindUnsupportedFunctions = []
	{
		//	gr: set error/throw function wrappers
		SetUnsupportedFunction( GenFramebuffers, "GenFrameBuffers" );
		SetUnsupportedFunction( BindFramebuffer, "BindFrameBuffer" );
		SetUnsupportedFunction( FramebufferTexture2D, "FrameBufferTexture2D" );
		SetUnsupportedFunction( DeleteFramebuffers, "DeleteFrameBuffers" );
		SetUnsupportedFunction( IsFramebuffer, "IsFramebuffer", (GLboolean)0 );
		SetUnsupportedFunction( CheckFramebufferStatus, "CheckFramebufferStatus", (GLenum)0 );
		SetUnsupportedFunction( GetFramebufferAttachmentParameteriv, "GetFramebufferAttachmentParameteriv" );
	};

	BindExtension( OpenglExtensions::VertexArrayObjects, BindFunctions, BindUnsupportedFunctions );
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
		Array<std::string> UnhandledExtensions;
		
		auto* pAllExtensions = glGetString( GL_EXTENSIONS );
		if ( pAllExtensions )
		{
			std::string AllExtensions( reinterpret_cast<const char*>(pAllExtensions) );
			ParseExtensions( SupportedExtensions, AllExtensions, GetArrayBridge(UnhandledExtensions) );
		}
		else
		{
			Opengl_IsOkayFlush();
			
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
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
				PushExtension( SupportedExtensions, ExtensionName, GetArrayBridge(UnhandledExtensions) );
			}
#else
			std::Debug << "Cannot determine opengl extensions" << std::endl;
#endif
		}

		//	list other extensions (do this first to aid debugging)
		std::stringstream UnhandledExtensionsStr;
		UnhandledExtensionsStr << "Unhandled extensions(x" << UnhandledExtensions.GetSize() << ") ";
		for ( int i=0;	i<UnhandledExtensions.GetSize();	i++ )
			UnhandledExtensionsStr << UnhandledExtensions[i] << " ";
		std::Debug << UnhandledExtensionsStr.str() << std::endl;

		//	check explicitly supported extensions
		auto& Impl = Opengl::ImplicitExtensions;
		for ( auto it=Impl.begin();	it!=Impl.end();	it++ )
		{
			auto& Version = it->second;
			auto& Extension = it->first;
			if ( pContext->mVersion < Version )
				continue;
			
			std::Debug << "Implicitly supporting " << Extension << " (>= version " << Version << ")" << std::endl;
			SupportedExtensions[Extension] = true;
		}
		
		//	setup extensions
		pContext->BindVertexArrayObjectsExtension();
		pContext->BindDrawBuffersExtension();
		pContext->BindFramebuffersExtension();
	
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
	TRenderTarget		( Meta.mName ),
	mTexture			( ExistingTexture ),
	mGenerateMipMaps	( true )
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
		
		mFbo.reset( new TFbo( mTexture ) );
	};
	
	std::Debug << "Deffering FBO rendertarget creation to " << ExistingTexture.mMeta << std::endl;
	Context.PushJob( CreateFbo );
}

Opengl::TRenderTargetFbo::TRenderTargetFbo(Opengl::TTexture ExistingTexture) :
	TRenderTargetFbo	( TFboMeta("Texture",ExistingTexture.GetWidth(),ExistingTexture.GetHeight()), ExistingTexture )
{
}

Opengl::TRenderTargetFbo::TRenderTargetFbo(TFboMeta Meta,Opengl::TTexture ExistingTexture) :
	TRenderTarget		( Meta.mName ),
	mTexture			( ExistingTexture ),
	mGenerateMipMaps	( true )
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


void Opengl::TRenderTargetFbo::Bind()
{
	Soy::Assert( mFbo!=nullptr, "FBO expected in render target");
	mFbo->Bind();
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
	
	//	generate mipmaps after we've drawn to the image
	if ( mGenerateMipMaps )
		mFbo->mTarget.GenerateMipMaps();
	
	//	mark as written to
	mFbo->mTarget.OnWrite();
}

Soy::Rectx<size_t> Opengl::TRenderTargetFbo::GetSize()
{
	return Soy::Rectx<size_t>(0,0,mTexture.mMeta.GetWidth(),mTexture.mMeta.GetHeight());
}

Opengl::TTexture Opengl::TRenderTargetFbo::GetTexture()
{
	return mTexture;
}


