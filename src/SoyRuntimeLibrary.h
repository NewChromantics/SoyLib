#pragma once
#include <string>
#include <functional>

/*
	helper class for managing dynamic library's at runtime
*/
namespace Soy
{
	class TRuntimeLibrary;

	std::string	GetCurrentWorkingDir();
}




class Soy::TRuntimeLibrary
{
public:
	TRuntimeLibrary(std::string Filename,std::function<bool(void)> LoadTest);
	~TRuntimeLibrary()
	{
		Close();
	}
	
	void		Close();
	void*		GetSymbol(const char* Name);
	
private:
#if defined(TARGET_OSX)
	void*		mHandle;
#endif
};


