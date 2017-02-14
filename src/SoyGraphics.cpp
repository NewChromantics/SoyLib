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

void SoyGraphics::TGeometryVertex::InsertElementAt(size_t UniformOffset,const TUniform& Uniform)
{
	//	walk to this data offset
	size_t Offset = 0;
	for ( int e=0;	e<mElements.GetSize();	e++ )
	{
		auto& Element = mElements[e];
		auto ElementSize = Element.GetDataSize();

		//	trying to insert into the middle of this element
		if ( UniformOffset >= Offset + ElementSize )
		{
			Offset += ElementSize;
			continue;
		}

		//	break up padding
		if ( Element.mType != TElementType::Padding )
			throw Soy::AssertException("Trying to insert uniform into another (non-padding) uniform");

		auto NewPrePadSize = UniformOffset - Offset;
		auto NewPostPadSize = Element.mArraySize - NewPrePadSize - Uniform.GetDataSize();

		//	remove element and insert 1/2/3 elements
		mElements.RemoveBlock( e, 1 );
		int InsertIndex = e;
		
		if ( NewPrePadSize > 0 )
		{
			GetArrayBridge( mElements ).InsertAt( InsertIndex, TUniform( TElementType::Padding, "Padding", NewPrePadSize ) );
			InsertIndex++;
		}

		{
			GetArrayBridge( mElements ).InsertAt( InsertIndex, Uniform );
			InsertIndex++;
		}

		if ( NewPostPadSize > 0 )
		{
			GetArrayBridge( mElements ).InsertAt( InsertIndex, TUniform( TElementType::Padding, "Padding", NewPostPadSize ) );
			InsertIndex++;
		}

	}

	//	add padding at end
	if ( UniformOffset > Offset )
	{
		mElements.PushBack( TUniform( TElementType::Padding, "Padding", UniformOffset - Offset ) );
	}

	mElements.PushBack( Uniform );
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
		case Padding:		return 1;

		default:break;
	}

	std::stringstream Error;
	Error << __func__ << "not implemented for " << t;
	throw Soy::AssertException( Error.str() );
}



SoyGraphics::TElementType::Type SoyGraphics::TElementType::GetFromSize(size_t Size)
{
	if ( Size == GetDataSize(Float) )		return Float;
	if ( Size == GetDataSize(Float2) )		return Float2;
	if ( Size == GetDataSize(Float3) )		return Float3;
	if ( Size == GetDataSize(Float4) )		return Float4;
	if ( Size == GetDataSize(Float3x3) )	return Float3x3;

	return Invalid;
}
