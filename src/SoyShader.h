#pragma once


#include "SoyEnum.h"
#include "Array.hpp"


//	gr: make this a TVersion later, once we encounter more glsl version specific stuff
namespace OpenglShaderVersion
{
	enum Type
	{
		Invalid,	//	non-specific
		glsl300,	//	es 3
		glsl150,	//	core 3
	};
	
	DECLARE_SOYENUM( OpenglShaderVersion );
};

//	gr: turn this into a more clever class for different systems
namespace SoyShader
{
	namespace Opengl
	{
		void	UpgradeVertShader(ArrayBridge<std::string>&& Shader,OpenglShaderVersion::Type Version);
		void	UpgradeFragShader(ArrayBridge<std::string>&& Shader,OpenglShaderVersion::Type Version);
	};
};

