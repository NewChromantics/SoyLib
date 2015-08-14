#pragma once

#include "SoyVector.h"

namespace Soy
{
	class TUniform;				//	bordering on Pop JobParam
	class TUniformContainer;
}

namespace Opengl
{
	class TTexture;
}


class Soy::TUniform
{
public:
	TUniform()	{}
	TUniform(const std::string& Name) :
		mName	( Name )
	{
	}
	
public:
	std::string		mName;
};

class Soy::TUniformContainer
{
public:
	virtual bool	SetUniform(const char* Name,const float& v)=0;
	virtual bool	SetUniform(const char* Name,const vec2f& v)=0;
	virtual bool	SetUniform(const char* Name,const vec4f& v)=0;
	
	//	gr: here I want to find a way that doesn't require this to be defined so we're not dependant on Opengl/OPencl/Cuda
	virtual bool	SetUniform(const char* Name,const Opengl::TTexture& v)=0;

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
