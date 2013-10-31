#include "SoyTypes.h"
#include <sstream>


#if defined(NO_OPENFRAMEWORKS)
void ofLogNotice(const std::string& Message)
{
	OutputDebugStringA( Message.c_str() );
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofLogError(const std::string& Message)
{
	OutputDebugStringA( Message.c_str() );
}
#endif

#if defined(NO_OPENFRAMEWORKS)
std::string ofToString(int Integer)
{
	std::ostringstream s;
	s << Integer;
	//return std::string( s.str() );
	return s.str();
}
#endif
