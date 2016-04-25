#include "SoyMediaFoundation.h"
#include <SoyDirectx.h>

#include <Mferror.h>
#include <Codecapi.h>


namespace MediaFoundation
{
	namespace Private
	{
		std::shared_ptr<TContext> Context;
	}
}



std::shared_ptr<MediaFoundation::TContext> MediaFoundation::GetContext()
{
	if ( !Private::Context )
	{
		Private::Context.reset(new TContext() );
	}
	return Private::Context;
}

void MediaFoundation::Shutdown()
{
	//	free last context
	if ( Private::Context.unique() )
		Private::Context.reset();
}

MediaFoundation::TContext::TContext()
{
	auto Result = MFStartup( MF_VERSION, MFSTARTUP_FULL );
	Directx::IsOkay(Result, "MFStartup");
}

MediaFoundation::TContext::~TContext()
{
	auto Result = MFShutdown();
	Directx::IsOkay(Result, "MFShutdown", false );
}
