#pragma once

#include "SoyOpenglContext.h"


#if defined(ENABLE_CUDA)
#include <SoyCuda.h>
#endif

#if defined(TARGET_WINDOWS)
#include <SoyDirectx.h>
#endif

//#define ENABLE_METAL

namespace Metal
{
	class TContext;
};


namespace UnityDevice
{
	//	GfxDeviceRenderer
	//	http://docs.unity3d.com/Manual/NativePluginInterface.html
	enum Type
	{
		Invalid = -1,
		kGfxRendererOpenGL = 0, // desktop OpenGL
		kGfxRendererD3D9 = 1, // Direct3D 9
		kGfxRendererD3D11 = 2, // Direct3D 11
		kGfxRendererGCM = 3, // PlayStation 3
		kGfxRendererNull = 4, // "null" device (used in batch mode)
		kGfxRendererXenon = 6, // Xbox 360
		kGfxRendererOpenGLES20 = 8, // OpenGL ES 2.0
		kGfxRendererOpenGLES30 = 11, // OpenGL ES 3.0
		kGfxRendererGXM = 12, // PlayStation Vita
		kGfxRendererPS4 = 13, // PlayStation 4
		kGfxRendererXboxOne = 14, // Xbox One
		kGfxRendererMetal = 16, // iOS Metal,
	};
	DECLARE_SOYENUM(UnityDevice);
}

namespace UnityEvent
{
	//	GfxDeviceEventType
	//	http://docs.unity3d.com/Manual/NativePluginInterface.html
	enum Type
	{
		Invalid = -1,
		kGfxDeviceEventInitialize = 0,
		kGfxDeviceEventShutdown = 1,
		kGfxDeviceEventBeforeReset = 2,
		kGfxDeviceEventAfterReset = 3,
	};
	DECLARE_SOYENUM(UnityEvent);
}

namespace Unity
{
#if defined(TARGET_IOS)||defined(TARGET_ANDROID)
	typedef uint64	ulong;
	typedef double	Float;	//	floats are 64 bit... but number seems to come through mangled... maybe don't use this at all!
#elif defined(TARGET_OSX) || defined(TARGET_WINDOWS)
	typedef uint32	ulong;
	typedef float	Float;
#endif
	typedef uint32			uint;
	typedef uint32			boolean;
	typedef sint32			sint;
	typedef void*			NativeTexturePtr;
	
	typedef void (*LogCallback)(const char*);

	namespace RenderTexturePixelFormat
	{
		enum Type
		{
			ARGB32 = 0,
			Depth = 1,
			ARGBHalf = 2,
			Shadowmap = 3,
			RGB565 = 4,
			ARGB4444 = 5,
			ARGB1555 = 6,
			Default = 7,		//	gr: wtf is this?
			ARGB2101010 = 8,
			DefaultHDR = 9,
			//@TODO: kRTFormatARGB64 = 10
			ARGBFloat = 11,
			RGFloat = 12,
			RGHalf = 13,
			RFloat = 14,
			RHalf = 15,
			R8 = 16,
			ARGBInt = 17,
			RGInt = 18,
			RInt = 19
		};
	}
	
	namespace Texture2DPixelFormat
	{
		enum Type
		{
			Alpha8		= 1,
			ARGB4444	= 2,
			RGB24		= 3,
			RGBA32		= 4,
			ARGB32		= 5,
			RGB565		= 7,
			R16			= 9,
			DXT1		= 10,
			DXT5		= 12,
			RGBA4444	= 13,
			BGRA32		= 14,
			
			RHalf		= 15,
			RGHalf		= 16,
			RGBAHalf	= 17,
			RFloat		= 18,
			RGFloat		= 19,
			RGBAFloat	= 20,
			
			YUY2		= 21,	//	only for dx9, dx11 and xbone
			
			PVRTC_RGB2	= 30,
			PVRTC_RGBA2	= 31,
			PVRTC_RGB4	= 32,
			PVRTC_RGBA4	= 33,
			ETC_RGB4	= 34,
			ATC_RGB4	= 35,
			ATC_RGBA8	= 36,
			
			EAC_R = 41,
			EAC_R_SIGNED = 42,
			EAC_RG = 43,
			EAC_RG_SIGNED = 44,
			ETC2_RGB = 45,
			ETC2_RGBA1 = 46,
			ETC2_RGBA8 = 47,
			
			ASTC_RGB_4x4 = 48,
			ASTC_RGB_5x5 = 49,
			ASTC_RGB_6x6 = 50,
			ASTC_RGB_8x8 = 51,
			ASTC_RGB_10x10 = 52,
			ASTC_RGB_12x12 = 53,
			
			ASTC_RGBA_4x4 = 54,
			ASTC_RGBA_5x5 = 55,
			ASTC_RGBA_6x6 = 56,
			ASTC_RGBA_8x8 = 57,
			ASTC_RGBA_10x10 = 58,
			ASTC_RGBA_12x12 = 59,
		};
	}
	
	
	void				Init(UnityDevice::Type Device,void* DevicePtr);
	void				Shutdown(UnityDevice::Type Device);
	void				RenderEvent(Unity::sint EventId);
	
	//	these throw when accessed if wrong device
	Opengl::TContext&				GetOpenglContext();
	std::shared_ptr<Opengl::TContext>&	GetOpenglContextPtr();
	bool							HasOpenglContext();
#if defined(TARGET_WINDOWS)
	Directx::TContext&				GetDirectxContext();
	std::shared_ptr<Directx::TContext>&	GetDirectxContextPtr();
	bool							HasDirectxContext();
#endif
	Metal::TContext&				GetMetalContext();
#if defined(ENABLE_CUDA)
	std::shared_ptr<Cuda::TContext>	GetCudaContext();
#endif
	
	SoyPixelsFormat::Type		GetPixelFormat(RenderTexturePixelFormat::Type Format);
	SoyPixelsFormat::Type		GetPixelFormat(Texture2DPixelFormat::Type Format);
	
};


__export void	UnitySetGraphicsDevice(void* device,Unity::sint deviceType,Unity::sint eventType);
__export void	FlushDebug(Unity::LogCallback Callback);


class Metal::TContext : public PopWorker::TContext
{
public:
	virtual bool	Lock() override { return true; }
	virtual void	Unlock() override {	}
	void			Iteration()		{}
};

