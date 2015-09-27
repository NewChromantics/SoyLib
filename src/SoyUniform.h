#pragma once

#include "SoyVector.h"
#include "SoyString.h"

namespace Soy
{
	class TUniform;				//	bordering on Pop JobParam
	class TUniformContainer;
}

namespace Opengl
{
	class TTextureAndContext;
}


class Soy::TUniform
{
public:
	TUniform()	{}
	TUniform(const std::string& Name,const std::string& Type=std::string()) :
		mName	( Name ),
		mType	( Type )
	{
	}
	
public:
	std::string		mName;
	std::string		mType;
};

class Soy::TUniformContainer
{
public:
	virtual bool	SetUniform(const char* Name,const std::string& v)	{	return false;	}
	virtual bool	SetUniform(const char* Name,const int& v)=0;
	virtual bool	SetUniform(const char* Name,const float& v)=0;
	virtual bool	SetUniform(const char* Name,const vec2f& v)=0;
	virtual bool	SetUniform(const char* Name,const vec4f& v)=0;
	
	//	gr: here I want to find a way that doesn't require this to be defined so we're not dependant on Opengl/OPencl/Cuda
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v)=0;

	template<typename TYPE>
	bool			SetUniform_s(const std::string& Name,const TYPE& v)
	{
		return SetUniform( Name.c_str(), v );
	}
	template<typename TYPE>
	bool			SetUniform(const TUniform& Uniform,const TYPE& v)
	{
		return SetUniform( Uniform.mName.c_str(), v );
	}
};




//	uniform wrapper for a variable, it is both a uniform and a container (so external systems can set it to itself from its own meta data)
template<typename TYPE>
class TUniformWrapper : public Soy::TUniformContainer, public Soy::TUniform
{
public:
	TUniformWrapper(const std::string& Name,TYPE DefaultValue) :
	TUniform	( Name, Soy::GetTypeName<TYPE>() ),
	mValue		( DefaultValue )
	{
	}
	
	//	setters... this is getting close to the SoyData thing again
	virtual bool	SetUniform(const char* Name,const std::string& v) override;
	virtual bool	SetUniform(const char* Name,const int& v) override;
	virtual bool	SetUniform(const char* Name,const float& v) override;
	virtual bool	SetUniform(const char* Name,const vec2f& v) override;
	virtual bool	SetUniform(const char* Name,const vec4f& v) override;
	virtual bool	SetUniform(const char* Name,const Opengl::TTextureAndContext& v) override;
	
	operator TYPE()	{	return mValue;	}
	
public:
	TYPE			mValue;
};


template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const float& v)
{
	if ( mName == Name )
	{
		mValue = v;
		return true;
	}
	return false;
}

template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const int& v)
{
	if ( mName == Name )
	{
		mValue = v;
		return true;
	}
	return false;
}

template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const std::string& v)
{
	if ( mName == Name )
	{
		Soy::StringToType( mValue, v );
		return true;
	}
	return false;
}

template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const vec2f& v)
{
	return false;
}

template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const vec4f& v)
{
	return false;
}

template<typename TYPE>
bool TUniformWrapper<TYPE>::SetUniform(const char* Name,const Opengl::TTextureAndContext& v)
{
	return false;
}


