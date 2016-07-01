#pragma once
#include "IUnityInterface.h"

// Should only be used on the rendering thread unless noted otherwise.
UNITY_DECLARE_INTERFACE(IUnityGraphicsPS4)
{
	void* (UNITY_INTERFACE_API * GetGfxContext)();

	void *(UNITY_INTERFACE_API * AllocateGPUMemory)(size_t size, int alignment);
	void (UNITY_INTERFACE_API * ReleaseGPUMemory)(void *data);

	void *(UNITY_INTERFACE_API * GetCurrentRenderTarget)(int index);
	void *(UNITY_INTERFACE_API * GetCurrentDepthRenderTarget)();


};
UNITY_REGISTER_INTERFACE_GUID(0xada62b5d78f14e7ULL, 0xa60947365e5561cfULL, IUnityGraphicsPS4)
