#pragma once

#include "SoyUniform.h"
#include "heaparray.hpp"

//	better name maybe?
namespace SoyGraphics
{
	class TTextureUploadParams;
	class TGeometryVertex;
	class TGeometryVertexElement;

	namespace TElementType
	{
		enum Type
		{
			Invalid,
			Float,
		};
	}
}

std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in);



class SoyGraphics::TGeometryVertexElement
{
public:
	TGeometryVertexElement() :
		mElementDataSize	( 0 ),
		mNormalised			( false ),
		mType				( TElementType::Invalid ),
		mArraySize			( 0 ),
		mIndex				( 0 )
	{
	}

	template<typename TYPE>
	void				SetType()	{	throw Soy::AssertException( std::string("Unhandled vertex uniform type ") + Soy::GetTypeName<TYPE>() );	}

public:
	std::string			mName;
	size_t				mElementDataSize;
	bool				mNormalised;
	TElementType::Type	mType;
	size_t				mArraySize;	//	for arrays of mType
	size_t				mIndex;		//	semantic index
};

template<>
inline void SoyGraphics::TGeometryVertexElement::SetType<float>()	{	mType = TElementType::Float;	}


class SoyGraphics::TGeometryVertex
{
public:
	size_t							GetDataSize() const;	//	size of vertex struct
	size_t							GetOffset(size_t ElementIndex) const;
	size_t							GetStride(size_t ElementIndex) const;
	size_t							GetVertexSize() const;

	void							EnableAttribs(bool Enable=true) const;
	void							DisableAttribs() const				{	EnableAttribs(false);	}

public:
	Array<TGeometryVertexElement>	mElements;
};



class SoyGraphics::TTextureUploadParams
{
public:
	TTextureUploadParams() :
		mStretch				( false ),
		mAllowCpuConversion		( true ),
		mAllowOpenglConversion	( true ),
		mAllowClientStorage		( false ),		//	currently unstable on texture release?
		mGenerateMipMaps		( false )
	{
	};

	bool	mStretch;				//	if smaller, should we stretch (subimage vs teximage)
	bool	mAllowCpuConversion;
	bool	mAllowOpenglConversion;
	bool	mAllowClientStorage;
	bool	mGenerateMipMaps;		//	should we generate mip maps afterwards
};
