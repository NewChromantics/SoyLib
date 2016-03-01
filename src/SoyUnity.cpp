#include "SoyUnity.h"
#include "SoyDebug.h"
#include <sstream>
#include "SoyOpenglContext.h"

//	new interfaces in 5.2+
#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"
#if defined(TARGET_WINDOWS)
class IDirect3D9;
class IDirect3DDevice9;
#include "Unity/IUnityGraphicsD3D9.h"

#include "Unity/IUnityGraphicsD3D11.h"

class ID3D12Device;
class ID3D12CommandQueue;
class ID3D12Fence;
class ID3D12Resource;
class D3D12_RESOURCE_STATES;
#include "Unity/IUnityGraphicsD3D12.h"

#endif

//	unity 5.4
/*
#if defined(ENABLE_METAL)
#include "Unity/IUnityGraphicsMetal.h"
#endif
*/

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
	SoyListenerId	DebugListener;
	
	void			PushDebugString(const std::string& Message);
	bool			PopDebugString(std::string& Buffer);
	void			FlushDebugStrings();
	
	std::mutex					DebugBufferLock;
	std::vector<std::string>	DebugBuffer;

	std::shared_ptr<Opengl::TContext>	OpenglContext;
#if defined(TARGET_WINDOWS)
	std::shared_ptr<Directx::TContext>	DirectxContext;
#endif
#if defined(ENABLE_METAL)
	std::shared_ptr<Metal::TContext>	MetalContext;
#endif
#if defined(ENABLE_CUDA)
	std::shared_ptr<Cuda::TContext>		CudaContext;
#endif
	
	
	SoyEvent<bool>		mOnDeviceShutdown;
	
	
	void				AllocOpenglContext();
#if defined(ENABLE_METAL)
	void				AllocMetalContext();
	void				IosDetectGraphicsDevice();
#endif
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


//	from ios trampoline / UnityInterface.h
#if defined(TARGET_IOS)
typedef enum
UnityRenderingAPI
{
	apiOpenGLES2	= 2,
	apiOpenGLES3	= 3,
	apiMetal		= 4,
}
UnityRenderingAPI;
class EAGLContext;

extern "C" int					UnitySelectedRenderingAPI();
extern "C" MTLDeviceRef			UnityGetMetalDevice();
extern "C" EAGLContext*			UnityGetDataContextEAGL();
/*
extern "C" NSBundle*			UnityGetMetalBundle()		{ return _MetalBundle; }
extern "C" MTLDeviceRef			UnityGetMetalDevice()		{ return _MetalDevice; }
extern "C" MTLCommandQueueRef	UnityGetMetalCommandQueue()	{ return  ((UnityDisplaySurfaceMTL*)GetMainDisplaySurface())->commandQueue; }

extern "C" EAGLContext*			UnityGetDataContextEAGL()	{ return _GlesContext; }
extern "C" int					UnitySelectedRenderingAPI()	{ return _renderingAPI; }

extern "C" UnityRenderBuffer	UnityBackbufferColor()		{ return GetMainDisplaySurface()->unityColorBuffer; }
extern "C" UnityRenderBuffer	UnityBackbufferDepth()		{ return GetMainDisplaySurface()->unityDepthBuffer; }
*/
#endif

#if defined(ENABLE_METAL)
void Unity::IosDetectGraphicsDevice()
{
	//	already decided
	if ( OpenglContext || MetalContext )
		return;
	
	//	on ios UnitySetGraphicsDevice never gets called, so we do it ourselves depending on API
	//	gr: cannot find a more hard api (eg. active metal context, grabbing the system one seems like we'd just be using that, even if unity isnt)
	auto Api = UnitySelectedRenderingAPI();
	switch ( Api )
	{
		case apiOpenGLES2:
			UnitySetGraphicsDevice( UnityGetDataContextEAGL(), UnityDevice::kGfxRendererOpenGLES20, UnityEvent::kGfxDeviceEventInitialize );
			break;
			
		case apiOpenGLES3:
			UnitySetGraphicsDevice( UnityGetDataContextEAGL(), UnityDevice::kGfxRendererOpenGLES30, UnityEvent::kGfxDeviceEventInitialize );
			break;
			
		case apiMetal:
			UnitySetGraphicsDevice( UnityGetMetalDevice(), UnityDevice::kGfxRendererMetal, UnityEvent::kGfxDeviceEventInitialize );
			break;

		default:
			break;
	}
	
	std::stringstream Error;
	Error << "Unknown unity API selected: " << Api;
	throw Soy::AssertException( Error.str() );
}
#endif

bool Unity::HasOpenglContext()
{
#if defined(ENABLE_METAL)
	IosDetectGraphicsDevice();
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
#if defined(ENABLE_METAL)
	IosDetectGraphicsDevice();
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



#if defined(ENABLE_METAL)
std::shared_ptr<Metal::TContext>& Unity::GetMetalContextPtr()
{
	IosDetectGraphicsDevice();
	return MetalContext;
}
#endif

#if defined(ENABLE_METAL)
Metal::TContext& Unity::GetMetalContext()
{
	auto Ptr = GetMetalContextPtr();
	
	if (!Ptr)
		throw Soy::AssertException("Getting metal context on non-metal run");
	
	return *Ptr;
}
#endif

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
	//std::Debug << "DllMain(" << Reason << ")" << std::endl;
	return TRUE;
}
#endif

void Unity::RenderEvent(Unity::sint eventID)
{
	ofScopeTimerWarning Timer(__func__,4);

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
	
#if defined(ENABLE_METAL)
	if ( Unity::MetalContext )
	{
		Unity::MetalContext->Iteration();
	}
#endif
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
			auto InitContext = []
			{
				Unity::GetOpenglContext().Init();
			};
			
			OpenglContext.reset(new Opengl::TContext);
			Unity::GetOpenglContext().PushJob(InitContext);

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
		
#if defined(ENABLE_METAL)
		case UnityDevice::kGfxRendererMetal:
		{
			MetalContext.reset(new Metal::TContext( DevicePtr ) );
		}
		break;
#endif
			
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
#if defined(ENABLE_METAL)
	MetalContext.reset();
#endif
#if defined(TARGET_WINDOWS)
	DirectxContext.reset();
#endif
}


void Unity::PushDebugString(const std::string& Message)
{
	if ( Message.empty() )
		return;
	
	static int BufferLimit = 1000;
	std::lock_guard<std::mutex> Lock( DebugBufferLock );
	DebugBuffer.push_back( Message );
	while ( DebugBuffer.size() > BufferLimit )
	{
		DebugBuffer.erase( DebugBuffer.begin() );
	}
}

bool Unity::PopDebugString(std::string& Buffer)
{
	std::lock_guard<std::mutex> Lock( DebugBufferLock );
	if ( DebugBuffer.empty() )
		return false;
	
	Buffer = DebugBuffer[0];
	DebugBuffer.erase( DebugBuffer.begin() );
	return true;
}

void Unity::FlushDebugStrings()
{
	std::lock_guard<std::mutex> Lock( DebugBufferLock );
	DebugBuffer.clear();
}

__export void FlushDebug(Unity::LogCallback Callback)
{
	ofScopeTimerWarning UpdateTextureTimer(__func__,15);

	static bool ForceFlush = false;
	if ( ForceFlush )
		Callback = nullptr;
	
	if ( !Callback )
		Unity::FlushDebugStrings();
	
	//	we might get a thread that's pumping so much debug that we never get back to unity.
	//	lets break out.
	//	gr: smaybe flush messages if that happens?
	size_t PrintCount = 0;
	static size_t MaxPrintCount = 60;
	static bool FlushExcessMessages = false;

	std::string Buffer;
	while ( Unity::PopDebugString( Buffer ) )
	{
		if ( Callback )
		{
			Callback( Buffer.c_str() );
		}

		PrintCount++;
		if ( PrintCount >= MaxPrintCount )
		{
			std::stringstream BreakMessage;
			BreakMessage << "Printed " << PrintCount << " messages in one go. ";
			
			if ( FlushExcessMessages )
				BreakMessage << "Flushing the rest to avoid hang[s].";

			Callback( BreakMessage.str().c_str() );

			if ( FlushExcessMessages )
				Unity::FlushDebugStrings();
			break;
		}
	}
}



namespace Unity
{
	IUnityGraphics*		GraphicsDevice = nullptr;
	IUnityInterfaces*	Interfaces = nullptr;
}


template<typename INTERFACETYPE>
void* GetDeviceContext()
{
	if ( !Unity::Interfaces )
		return nullptr;

	auto* Interface = Unity::Interfaces->Get<INTERFACETYPE>();
	return Interface->GetDevice();
}

void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	auto Device = Unity::GraphicsDevice;
	if ( !Device )
	{
		std::Debug << __func__ << " missing graphics device" << std::endl;
		return;
	}

	void* DeviceContext = nullptr;
	auto DeviceType = static_cast<UnityDevice::Type>( Unity::GraphicsDevice->GetRenderer() );

	switch ( DeviceType )
	{
#if defined(TARGET_WINDOWS)
		case kUnityGfxRendererD3D9:		DeviceContext = GetDeviceContext<IUnityGraphicsD3D9>();	break;
		case kUnityGfxRendererD3D11:	DeviceContext = GetDeviceContext<IUnityGraphicsD3D11>();	break;
		case kUnityGfxRendererD3D12:	DeviceContext = GetDeviceContext<IUnityGraphicsD3D12>();	break;
#endif
			
		default:
			break;
	}

	UnitySetGraphicsDevice( DeviceContext, DeviceType, eventType );
}


// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	Unity::Interfaces = unityInterfaces;
	Unity::GraphicsDevice = Unity::Interfaces->Get<IUnityGraphics>();

    Unity::GraphicsDevice->RegisterDeviceEventCallback( OnGraphicsDeviceEvent );

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    // to not miss the event in case the graphics device is already initialized
    OnGraphicsDeviceEvent( kUnityGfxDeviceEventInitialize );
}

// Unity plugin unload event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	if ( Unity::GraphicsDevice )
	{
		Unity::GraphicsDevice->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
		//Unity::GraphicsDevice = nullptr;
	}
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
