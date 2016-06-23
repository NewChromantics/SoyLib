#pragma once

#include "SoyUniform.h"
#include "heaparray.hpp"

//	better name maybe?
namespace SoyGraphics
{
	class TTextureUploadParams;
	class TGeometryVertex;
	class TGeometryVertexElement;
}

std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in);


class SoyGraphics::TGeometryVertexElement : public Soy::TUniform
{
public:
	size_t		mElementDataSize;
	bool		mNormalised;
};

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
