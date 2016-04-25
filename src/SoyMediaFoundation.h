#pragma once

#include <memory>
#include <string>
#include <SoyTypes.h>

#include <Mfapi.h>
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"Mfuuid.lib")

#include <Mfidl.h>
#pragma comment(lib,"Mf.lib")

#include <Mfreadwrite.h>
#pragma comment(lib,"Mfreadwrite.lib")


std::ostream& operator<<(std::ostream& os, REFGUID guid);

class MfExtractor;



namespace MediaFoundation
{
	class TContext;

	std::shared_ptr<TContext>	GetContext();	//	singleton
	void						Shutdown();		//	cleanup singleton context

	inline bool					IsOkay(HRESULT Error,const std::string& Context,bool ThrowException=true)	{	return Platform::IsOkay( Error, Context, ThrowException );	}


}


class MediaFoundation::TContext
{
public:
	TContext();
	~TContext();
};

