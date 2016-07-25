#pragma once

/*
	Stubs, for NDA. But also so Soy classes can reference this graphics device.
*/

#include <memory>
#include <functional>

namespace Gnm
{
	class TContext;
	class TTexture;

	std::shared_ptr<TContext>	AllocContext(void* DevicePtr,std::function<void*(size_t)> GpuAlloc,std::function<bool(void*)> GpuFree);
	void						IterateContext(TContext& Context);
}
