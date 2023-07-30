#include "SoyTime.h"

//	requires corefoundation
#if defined(__OBJC__)
std::chrono::milliseconds Soy::Platform::GetTime(CFTimeInterval Time)
{
	auto TimeMs = size_cast<uint64>(Time*1000.0);
	return std::chrono::milliseconds(TimeMs);
}
#endif
