#include "SoyGraphics.h"


std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in)
{
	out << "mAllowClientStorage=" << in.mAllowClientStorage << ",";
	out << "mAllowCpuConversion=" << in.mAllowCpuConversion << ",";
	out << "mAllowOpenglConversion=" << in.mAllowOpenglConversion << ",";
	out << "mStretch=" << in.mStretch << ",";
	return out;
}