#pragma once
#include <string>
#include <functional>
#include "SoyDebug.h"

#if defined(TARGET_WINDOWS)
#include "SoyTypes.h"
#endif

/*
	helper class for managing dynamic library's at runtime
*/
namespace Soy
{
	class TRuntimeLibrary;

	std::string	GetCurrentWorkingDir();
	std::string	GetEnvVar(const char* Key);	//	throws on non-existance
}




template<typename FUNCTYPE>
void SetFunction(std::function<FUNCTYPE>& f,void* x)
{
	if ( !x )
		throw Soy::AssertException("Function not found");
	
	FUNCTYPE* ff = (FUNCTYPE*)x;
	f = ff;
}

class Soy::TRuntimeLibrary
{
public:
	TRuntimeLibrary(std::string Filename,std::function<bool(void)> LoadTest=nullptr);
	~TRuntimeLibrary()
	{
		Close();
	}
	
	void		Close();
	void*		GetSymbol(const char* Name);

	//	assign & cast function ptr to a symbol
	template<typename FUNCTYPE>
	void		SetFunction(std::function<FUNCTYPE>& FunctionPtr,const char* FunctionName)
	{
		auto* Symbol = GetSymbol( FunctionName );
		if ( !Symbol )
		{
			std::stringstream Error;
			Error << "Function " << FunctionName << " not found in " << mLibraryName;
			throw Soy::AssertException( Error.str() );
			return;
		}
	
		//	cast & assign
		FUNCTYPE* ff = reinterpret_cast<FUNCTYPE*>(Symbol);
		FunctionPtr = ff;
	}

private:
#if defined(TARGET_OSX)
	void*		mHandle;
#elif defined(TARGET_WINDOWS)
	HMODULE		mHandle;
#endif
	std::string	mLibraryName;	//	filename for debugging
};


