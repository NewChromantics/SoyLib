#include "SoyGraphics.h"


std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in)
{
	out << "mAllowClientStorage=" << in.mAllowClientStorage << ",";
	out << "mAllowCpuConversion=" << in.mAllowCpuConversion << ",";
	out << "mAllowOpenglConversion=" << in.mAllowOpenglConversion << ",";
	out << "mStretch=" << in.mStretch << ",";
	return out;
}


std::ostream& SoyGraphics::operator<<(std::ostream &out,const SoyGraphics::TUniform& in)
{
	out << "#" << in.mIndex << " ";
	out << in.mType;
	if ( in.mArraySize != 1 )
		out << "[" << in.mArraySize << "]";
	out << " " << in.mName;
	return out;
}




size_t SoyGraphics::TGeometryVertex::GetDataSize() const
{
	size_t Size = 0;
	for ( int e=0;	e<mElements.GetSize();	e++ )
		Size += mElements[e].mElementDataSize;
	return Size;
}

size_t SoyGraphics::TGeometryVertex::GetOffset(size_t ElementIndex) const
{
	size_t Size = 0;
	for ( int e=0;	e<ElementIndex;	e++ )
		Size += mElements[e].mElementDataSize;
	return Size;
}

size_t SoyGraphics::TGeometryVertex::GetStride(size_t ElementIndex) const
{
	//	gr: this is for interleaved vertexes
	//	todo: handle serial elements AND mixed interleaved & serial
	//	serial elements would be 0
	size_t Stride = GetDataSize();
	Stride -= mElements[ElementIndex].mElementDataSize;
	return Stride;
}

size_t SoyGraphics::TGeometryVertex::GetVertexSize() const
{
	size_t Size = 0;
	for ( int e=0;	e<mElements.GetSize();	e++ )
		Size += mElements[e].mElementDataSize;

	return Size;
}
