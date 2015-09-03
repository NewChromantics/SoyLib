#pragma once

#include "Array.hpp"


//	gr: turn this into a more clever class for different systems
namespace SoyShader
{
	namespace Opengl
	{
		void	UpgradeVertShader(ArrayBridge<std::string>&& Shader,Soy::TVersion Version);
		void	UpgradeFragShader(ArrayBridge<std::string>&& Shader,Soy::TVersion Version);
	};
};

