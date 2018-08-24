#pragma once

#include "SoyUniform.h"
#include "HeapArray.hpp"

//	better name maybe?
namespace SoyGraphics
{
	class TTextureUploadParams;
	class TGeometryVertex;
	class TUniform;

	namespace TElementType
	{
		enum Type
		{
			Invalid,
			Int32,
			Float,
			Float2,
			Float3,
			Float4,
			Float3x3,
			Float4x4,
			Texture2D,
			Bool,
		};

		size_t		GetDataSize(Type t);
		size_t		GetFloatCount(Type t);	//	throws if not a float type
		bool		IsFloat(Type t);
		bool		IsImage(Type t);
	}
}

std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in);

namespace SoyGraphics
{
	std::ostream& operator<<(std::ostream &out,const SoyGraphics::TElementType::Type& in);
}


class SoyGraphics::TUniform
{
public:
	TUniform() :
		mNormalised			( false ),
		mType				( TElementType::Invalid ),
		mArraySize			( 0 ),
		mIndex				( 0 )
	{
	}

	template<typename TYPE>
	void				SetType()	{	throw Soy::AssertException( std::string("Unhandled vertex uniform type ") + Soy::GetTypeName<TYPE>() );	}
	template<typename TYPE>
	void				SetType(const TYPE& t)	{	SetType<TYPE>();	}

	bool				IsValid() const	{	return mType != TElementType::Invalid;	}

	size_t				GetElementDataSize() const			{	return TElementType::GetDataSize(mType);	}
	size_t				GetDataSize() const					{	return GetElementDataSize() * GetArraySize();	}
	size_t				GetArraySize() const				{	return std::max<size_t>( 1, mArraySize );	}
	size_t				GetFloatCount() const				{	return TElementType::GetFloatCount(mType) * GetArraySize(); }
	size_t				GetElementFloatCount() const		{	return TElementType::GetFloatCount(mType); }
	bool				operator==(const char* Name) const	{	return mName == Name;	}

public:
	std::string			mName;
	bool				mNormalised;
	TElementType::Type	mType;
	size_t				mArraySize;	//	for arrays of mType
	size_t				mIndex;		//	semantic index
};

namespace SoyGraphics
{
	std::ostream& operator<<(std::ostream &out,const SoyGraphics::TUniform& in);
}

template<>
inline void SoyGraphics::TUniform::SetType<float>()	{	mType = TElementType::Float;	}

template<>
inline void SoyGraphics::TUniform::SetType<vec2f>()	{	mType = TElementType::Float2;	}

template<>
inline void SoyGraphics::TUniform::SetType<vec3f>()	{	mType = TElementType::Float3;	}

template<>
inline void SoyGraphics::TUniform::SetType<vec4f>()	{	mType = TElementType::Float4;	}


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
	Array<TUniform>	mElements;
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
