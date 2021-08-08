#include "SoyTime.h"

//	requires corefoundation
#if defined(__OBJC__)
SoyTime Soy::Platform::GetTime(CFTimeInterval Time)
{
	auto TimeMs = size_cast<uint64>(Time*1000.0);
	return SoyTime( std::chrono::milliseconds(TimeMs) );
}
#endif
