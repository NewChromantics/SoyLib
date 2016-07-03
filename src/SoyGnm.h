#pragma once

/*
	Stubs, for NDA. But also so Soy classes can reference this graphics device.
*/

#include <memory>

namespace Gnm
{
	class TContext;
	class TTexture;

	std::shared_ptr<TContext>	AllocContext(void* DevicePtr);
}
