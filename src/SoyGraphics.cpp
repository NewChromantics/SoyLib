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
		Size += mElements[e].GetDataSize();
	return Size;
}

size_t SoyGraphics::TGeometryVertex::GetOffset(size_t ElementIndex) const
{
	size_t Size = 0;
	for ( int e=0;	e<ElementIndex;	e++ )
		Size += mElements[e].GetDataSize();
	return Size;
}

size_t SoyGraphics::TGeometryVertex::GetStride(size_t ElementIndex) const
{
	//	gr: this is for interleaved vertexes
	//	todo: handle serial elements AND mixed interleaved & serial
	//	serial elements would be 0
	size_t Stride = GetDataSize();
	Stride -= mElements[ElementIndex].GetDataSize();
	return Stride;
}

size_t SoyGraphics::TGeometryVertex::GetVertexSize() const
{
	size_t Size = 0;
	for ( int e=0;	e<mElements.GetSize();	e++ )
		Size += mElements[e].GetDataSize();

	return Size;
}

size_t SoyGraphics::TElementType::GetDataSize(Type t)
{
	switch (t)
	{
		case Int32:			return sizeof(uint32_t);
		case Float:			return sizeof(float);
		case Float2:		return sizeof(vec2f);
		case Float3:		return sizeof(vec3f);
		case Float4:		return sizeof(vec4f);
		case Float3x3:		return sizeof(float3x3);

		default:break;
	}

	std::stringstream Error;
	Error << __func__ << "not implemented for " << t;
	throw Soy::AssertException( Error.str() );
}


size_t SoyGraphics::TElementType::GetFloatCount(Type t)
{
	switch (t)
	{
		case Float:			return 1;
		case Float2:		return 2;
		case Float3:		return 3;
		case Float4:		return 4;
		case Float3x3:		return 3*3;
		
		default:break;
	}
	
	std::stringstream Error;
	Error << __func__ << " not float type " << t;
	throw Soy::AssertException( Error.str() );
}

