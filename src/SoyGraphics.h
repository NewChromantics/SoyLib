#pragma once

#include "SoyUniform.h"
#include "heaparray.hpp"

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
			Texture2D,
			Padding,		//	for directx structs
		};

		size_t		GetDataSize(Type t);
		Type		GetFromSize(size_t Size);
	}
}

std::ostream& operator<<(std::ostream &out,const SoyGraphics::TTextureUploadParams& in);



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
	TUniform(TElementType::Type ElementType,const std::string& Name,size_t ArraySize) :
		mNormalised			( false ),
		mType				( ElementType ),
		mArraySize			( ArraySize ),
		mIndex				( 0 ),
		mName				( Name )
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
	size_t				GetDataSize() const;	//	size of vertex struct
	size_t				GetOffset(size_t ElementIndex) const;
	size_t				GetStride(size_t ElementIndex) const;
	size_t				GetVertexSize() const;

	void				InsertElementAt(size_t DataOffset,const TUniform& Uniform);		//	add element and pad as neccessary

	template<typename TYPE>
	void				Write(ArrayBridge<uint8_t>&& Buffer,const char* Name,const TYPE& Value) const
	{
		//	find element index
		auto UniformIndex = mElements.FindIndex(Name);
		if ( UniformIndex == -1 )
		{
			std::stringstream Error;
			Error << "Unknown uniform " << Name;
			throw Soy::AssertException( Error.str() );
		}
		auto& Uniform = mElements[UniformIndex];

		//	do implicit type conversions

		//	memcpy
		throw Soy::AssertException("Todo writing to Uniform Buffer");
	}

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
