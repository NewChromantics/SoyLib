#include "SoyUnity.h"
#include "SoyDebug.h"
#include <sstream>
#include "SoyOpenglContext.h"
#include "SoyExportManager.h"
#include "SoyFilesystem.h"

#if defined(TARGET_ANDROID)
#include "SoyJava.h"
#endif


#if defined(TARGET_IOS)
extern "C" {
typedef	void	(*UnityPluginSetGraphicsDeviceFunc)(void* device, int deviceType, int eventType);
typedef	void	(*UnityPluginRenderMarkerFunc)(int marker);
void	UnityRegisterRenderingPlugin(UnityPluginSetGraphicsDeviceFunc setDevice, UnityPluginRenderMarkerFunc renderMarker);
}
#endif

namespace Platform
{
	std::string		GetBundleIdentifier();
}


namespace Unity
{
	static int		gRenderEventTimerMs = 8;

	
	//	attach to std::debug
	SoyListenerId	DebugListener;
	void			PushDebugString(const std::string& Message);
	
	//	actual storage of debug strings
	static size_t	DefaultDebugStringExportLimit = 1000;
	std::shared_ptr<TExportManager<std::string,char>>	gDebugStringManager;
	TExportManager<std::string,char>&	GetDebugStringManager();
	
	//	c/# interface of strings we've exported and pending to be read by c/#
	std::mutex							gDebugExportedStringsLock;
	Array<const char*>					gDebugExportedStrings;
	bool								gDebugExportedStringsEnabled = true;
	
	
	std::shared_ptr<Opengl::TContext>	OpenglContext;
#if defined(TARGET_WINDOWS)
	std::shared_ptr<Directx::TContext>	DirectxContext;
#endif
	std::shared_ptr<Metal::TContext>	MetalContext;

#if defined(ENABLE_CUDA)
	std::shared_ptr<Cuda::TContext>		CudaContext;
#endif
	
	
	SoyEvent<bool>		mOnDeviceShutdown;	
}



std::map<UnityDevice::Type, std::string> UnityDevice::EnumMap =
{
	{ UnityDevice::Invalid,				"Invalid" },
	{ UnityDevice::kGfxRendererOpenGL,	"Opengl" },
	{ UnityDevice::kGfxRendererD3D9,	"Directx9"	},
	{ UnityDevice::kGfxRendererD3D11,	"Directx11"	},
	{ UnityDevice::kGfxRendererGCM,		"PS3"	},
	{ UnityDevice::kGfxRendererNull,	"NullBatchMode"	},
	{ UnityDevice::kGfxRendererXenon,	"Xbox360"	},
	{ UnityDevice::kGfxRendererOpenGLES20,	"OpenglES2"	},
	{ UnityDevice::kGfxRendererOpenGLES30,	"OpenglES3"	},
	{ UnityDevice::kGfxRendererGXM,		"PSVita"	},
	{ UnityDevice::kGfxRendererPS4,		"PS4"	},
	{ UnityDevice::kGfxRendererXboxOne,	"XboxOne"	},
	{ UnityDevice::kGfxRendererMetal,	"Metal"	},
	{ UnityDevice::kGfxRendererOpenGLCore,		"Opengl Core"	},
	{ UnityDevice::kGfxRendererD3D12,		"Directx12"	},
};

std::map<UnityEvent::Type, std::string> UnityEvent::EnumMap =
{
	{ UnityEvent::Invalid,						"Invalid" },
	{ UnityEvent::kGfxDeviceEventInitialize,	"kGfxDeviceEventInitialize" },
	{ UnityEvent::kGfxDeviceEventShutdown,		"kGfxDeviceEventShutdown" },
	{ UnityEvent::kGfxDeviceEventBeforeReset,	"kGfxDeviceEventBeforeReset" },
	{ UnityEvent::kGfxDeviceEventAfterReset,	"kGfxDeviceEventAfterReset" },
};

bool Unity::HasOpenglContext()
{
	//	auto-init for ios, see GetOpenglContext
#if defined(TARGET_IOS)
	GetOpenglContext();
#endif
	return OpenglContext != nullptr;
}

Opengl::TContext& Unity::GetOpenglContext()
{
	auto Ptr = GetOpenglContextPtr();

	if (!Ptr)
		throw Soy::AssertException("Getting opengl context on non-opengl run");

	return *Ptr;
}


std::shared_ptr<Opengl::TContext>& Unity::GetOpenglContextPtr()
{
	//	on ios UnitySetGraphicsDevice never gets called, so init... right now!
#if defined(TARGET_IOS)
	if ( !OpenglContext )
		UnitySetGraphicsDevice( nullptr, UnityDevice::kGfxRendererOpenGLES30, UnityEvent::kGfxDeviceEventInitialize );
#endif
	
	return OpenglContext;
}

#if defined(TARGET_WINDOWS)
bool Unity::HasDirectxContext()
{
	return DirectxContext != nullptr;
}
#endif

#if defined(TARGET_WINDOWS)
Directx::TContext& Unity::GetDirectxContext()
{
	if (!DirectxContext)
		throw Soy::AssertException("Getting directx context on non-directx run");

	return *DirectxContext;
}
#endif

#if defined(TARGET_WINDOWS)
std::shared_ptr<Directx::TContext>& Unity::GetDirectxContextPtr()
{
	return DirectxContext;
}
#endif

Metal::TContext& Unity::GetMetalContext()
{
	if (!MetalContext)
		throw Soy::AssertException("Getting metal context on non-metal run");

	return *MetalContext;
}

#if defined(ENABLE_CUDA)
std::shared_ptr<Cuda::TContext> Unity::GetCudaContext()
{
	if (!CudaContext)
		CudaContext.reset(new Cuda::TContext);

	return CudaContext;
}
#endif

SoyPixelsFormat::Type Unity::GetPixelFormat(RenderTexturePixelFormat::Type Format)
{
	switch ( Format )
	{
		//	gr: this appears to be RGBA (opengl texture format is RGBA8 on osx)
		//	gr: possibly it IS ARGB but opengl reinterprets it?
		case RenderTexturePixelFormat::ARGB32:	return SoyPixelsFormat::RGBA;
	
		//	todo: test this
		case RenderTexturePixelFormat::R8:		return SoyPixelsFormat::Greyscale;

		default:
			return SoyPixelsFormat::Invalid;
	}
}


SoyPixelsFormat::Type Unity::GetPixelFormat(Texture2DPixelFormat::Type Format)
{
	switch ( Format )
	{
		case Texture2DPixelFormat::RGBA32:	return SoyPixelsFormat::RGBA;
		case Texture2DPixelFormat::RGB24:	return SoyPixelsFormat::RGB;
		case Texture2DPixelFormat::BGRA32:	return SoyPixelsFormat::BGRA;
		case Texture2DPixelFormat::Alpha8:	return SoyPixelsFormat::Greyscale;

		//	gr: this was commented out... but it comes up...
		case Texture2DPixelFormat::ARGB32:	return SoyPixelsFormat::ARGB;
			
		default:
			return SoyPixelsFormat::Invalid;
	}
}


//	try and help debug DLL loading in windows
#if defined(TARGET_WINDOWS)
BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID Reserved)
{
	std::string Path;

	char PathBuffer[MAX_PATH];
	auto PathBufferLength = GetModuleFileNameA( Module, PathBuffer, sizeof(PathBuffer) );
	if ( Platform::IsOkay( "GetModuleFileName in DLLMain", false ) )
	{
		//	just in case 
		PathBufferLength = std::min<size_t>( PathBufferLength, sizeof(PathBuffer)-1 );
		PathBuffer[PathBufferLength] = '\0';
		Path = PathBuffer;
	}

	/*
	char path[MAX_PARAM];
	HMODULE hm = NULL;

	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&DllMain, 
		&hm))
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle returned %d\n", ret);
	}
	GetModuleFileNameA(hm, path, sizeof(path));
	*/
	Platform::SetDllPath( Path );


	//std::Debug << "DllMain(" << Reason << ")" << std::endl;
	return TRUE;
}
#endif

void Unity::RenderEvent(Unity::sint eventID)
{
	ofScopeTimerWarning Timer(__func__,gRenderEventTimerMs);

	//	iterate current context
	if (Unity::OpenglContext)
	{
		Unity::OpenglContext->Iteration();
	}

#if defined(TARGET_WINDOWS)
	if ( Unity::DirectxContext )
	{
		Unity::DirectxContext->Iteration();
	}
#endif
	
	if ( Unity::MetalContext )
	{
		Unity::MetalContext->Iteration();
	}
}


__export void UnitySetGraphicsDevice(void* device,Unity::sint deviceType,Unity::sint eventType)
{
	auto DeviceType = UnityDevice::Validate(deviceType);
	auto DeviceEvent = UnityEvent::Validate(eventType);

	std::Debug << __func__ << "(" << UnityDevice::ToString(DeviceType) << "," << UnityEvent::ToString(DeviceEvent) << ")" << std::endl;

	switch (DeviceEvent)
	{
	case UnityEvent::kGfxDeviceEventShutdown:
		Unity::Shutdown(DeviceType);
		break;

	case UnityEvent::kGfxDeviceEventInitialize:
		Unity::Init(DeviceType, device);
		break;

	default:
		break;
	};
}



//	gr: check this is okay with multiple plugins linking, which was the original reason for a unique function name...
#if defined(TARGET_IOS)
__export void UnityRenderEvent_Ios(Unity::sint eventID)
#else
__export void UnityRenderEvent(Unity::sint eventID)
#endif
{
	//	event triggered by other plugin
	if ( eventID != Unity::GetPluginEventId() )
	{
	//	if ( Unity::mDebugPluginEvent )
		std::Debug << "UnityRenderEvent(" << eventID << ") Not ours " << Unity::GetPluginEventId() << std::endl;
		return;
	}
	
	Unity::RenderEvent( eventID );
}


void Unity::Init(UnityDevice::Type Device,void* DevicePtr)
{
	if (!Platform::Init())
		throw Soy::AssertException("Soy Failed to init platform");

	if ( !DebugListener.IsValid() )
	{
		auto& Event = std::Debug.GetOnFlushEvent();
		
		auto PushDebug = [](const std::string& Debug)
		{
			PushDebugString( Debug );
		};
		
		DebugListener = Event.AddListener( PushDebug );
	}
	
	//	ios needs to manually register callbacks
	//	http://gamedev.stackexchange.com/questions/100485/how-do-i-get-gl-issuerenderevent-to-work-on-ios-with-unity-5
#if defined(TARGET_IOS)
	UnityRegisterRenderingPlugin( nullptr, &UnityRenderEvent_Ios );
#endif

	//	allocate appropriate context and init
	switch ( Device )
	{
		case UnityDevice::kGfxRendererOpenGL:
		case UnityDevice::kGfxRendererOpenGLES20:
		case UnityDevice::kGfxRendererOpenGLES30:
		case UnityDevice::kGfxRendererOpenGLCore:
		{
			//	init context on first run
			auto InitOpenglContext = []
			{
				Unity::GetOpenglContext().Init();
			};

			OpenglContext.reset(new Opengl::TContext);
			Unity::GetOpenglContext().PushJob(InitOpenglContext);
		}
		break;

#if defined(TARGET_WINDOWS)
		case UnityDevice::kGfxRendererD3D11:
		case UnityDevice::kGfxRendererD3D12:
		{
			auto* DeviceDx = static_cast<ID3D11Device*>( DevicePtr );
			Soy::Assert( DeviceDx != nullptr, "Missing device pointer to create directx context");
			DirectxContext.reset(new Directx::TContext( *DeviceDx ) );
		}
		break;
#endif
		
		case UnityDevice::kGfxRendererMetal:
		{
			MetalContext.reset(new Metal::TContext);
		}
		break;
			
		default:
		{
			std::string DeviceName;
			try
			{
				DeviceName = UnityDevice::ToString(Device);
			}
			catch(...)
			{
				std::stringstream Error;
				Error << "(Unknown device id " << static_cast<int>(Device) << ")";
				DeviceName = Error.str();
			}
			
			std::Debug << "Unsupported device type " << DeviceName << std::endl;
		}
		break;
	};

	std::Debug << "Unity::Init(" << UnityDevice::ToString(Device) << ") finished" << std::endl;
}

void Unity::Shutdown(UnityDevice::Type Device)
{
	std::Debug.GetOnFlushEvent().RemoveListener( DebugListener );

	{
		bool Dummy;
		mOnDeviceShutdown.OnTriggered(Dummy);
	}
	
	//	free all contexts
	//	gr: may need to defer some of these!
	OpenglContext.reset();
	MetalContext.reset();
#if defined(TARGET_WINDOWS)
	DirectxContext.reset();
#endif
}

void Unity::EnableDebugStrings(bool Enable)
{
	gDebugExportedStringsEnabled = Enable;
}

TExportManager<std::string,char>& Unity::GetDebugStringManager()
{
	if ( !gDebugStringManager )
		gDebugStringManager.reset( new TExportManager<std::string,char>(DefaultDebugStringExportLimit) );
	
	return *gDebugStringManager;
}



void Unity::PushDebugString(const std::string& Message)
{
	if ( Message.empty() )
		return;
	
	if ( !gDebugExportedStringsEnabled )
		return;

	//	we now immediately export the string and keep a list of strings for c/# interface to work through
	try
	{
		auto& Manager = GetDebugStringManager();
		auto* LockedString = Manager.Lock( Message );
		if ( !LockedString )
			return;
		std::lock_guard<std::mutex> Lock( gDebugExportedStringsLock );
		gDebugExportedStrings.PushBack( LockedString );
	}
	catch(std::exception& e)
	{
		int x;
		//	recursion?
		//std::Debug << __func__ << " caught exception: " << e.what() << std::endl;
	}
}

const char* Unity::PopDebugString()
{
	std::lock_guard<std::mutex> Lock( gDebugExportedStringsLock );
	if ( gDebugExportedStrings.IsEmpty() )
		return nullptr;
	auto* PoppedString = gDebugExportedStrings.PopAt(0);
	return PoppedString;
}

void Unity::ReleaseDebugString(const char* ExportedString)
{
	auto& Manager = GetDebugStringManager();
	Manager.Unlock( ExportedString );
}

void Unity::ReleaseDebugStringAll()
{
	{
		std::lock_guard<std::mutex> Lock( gDebugExportedStringsLock );
		gDebugExportedStrings.Clear();
	}
	
	auto& Manager = GetDebugStringManager();
	Manager.UnlockAll();
}



void Unity::GetSystemFileExtensions(ArrayBridge<std::string>&& Extensions)
{
	Extensions.PushBack(".meta");
}


#if defined(TARGET_ANDROID)
std::string Platform::GetBundleIdentifier()
{
	return Java::GetBundleIdentifier();
}
#endif


#if defined(TARGET_WINDOWS)
std::string Platform::GetBundleIdentifier()
{
	return "Todo:BundleIdentifierForWindows";
}
#endif

const std::string& Unity::GetBundleIdentifier()
{
	//	cache this
	static std::string CachedIdentifier = Soy::StringToLower( Platform::GetBundleIdentifier() );
	return CachedIdentifier;
}

