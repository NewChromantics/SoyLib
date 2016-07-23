#pragma once

#include "SoyThread.h"
#include "SoyPixels.h"
#include "SoyUniform.h"
#include <functional>

#include "SoyOpengl.h"	//	re-using opengl's vertex description atm
#include "SoyMediaFormat.h"


template<typename TYPE>
class TPool;
class SoyPixelsImpl;

//	something is including d3d10.h (from dx sdk) and some errors have different export types from winerror.h (winsdk)
#pragma warning( push )
//#pragma warning( disable : 4005 )
#include <d3d11.h>
#pragma warning( pop )


//	if compiling against win8 lib/runtime, then we can include the new d3d compiler lib directly
#if WINDOWS_TARGET_SDK >= 8
#include <d3dcompiler.h>
//	gr: do not link to d3dcompiler. This means it will not load the DLL and we load it manually (InitCompilerExtension)
//#pragma comment(lib, "D3DCompiler.lib")
#endif

//	rename this when we swap Directx for Directx11
namespace DirectxCompiler
{
	class TCompiler;			//	wrapper to hold the compile func and a reference to the runtime library. Defined in source for cleaner code
	class TCompilerImpl;
}

namespace Soy
{
	class TRuntimeLibrary;
}

class DirectxCompiler::TCompiler
{
public:
	TCompiler();
	void		Compile(ArrayBridge<uint8>&& Compiled,const std::string& Source,const std::string& Function,const std::string& Target,const std::string& Name,const std::map<std::string,std::string>& Macros);

public:
	std::shared_ptr<Soy::TRuntimeLibrary>	mCompileLib;
	std::shared_ptr<TCompilerImpl>			mImpl;
};

