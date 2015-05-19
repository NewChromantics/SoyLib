#include "SoyAssert.h"
#include "SoyDebug.h"



namespace Soy
{
	//	defualt on, but allow build options to change default (or call Soy::EnableAssertThrow)
#if !defined(SOY_ENABLE_ASSERT_THROW)
#define SOY_ENABLE_ASSERT_THROW true
#endif
	bool	gEnableThrowInAssert = SOY_ENABLE_ASSERT_THROW;
}


void Soy::EnableThrowInAssert(bool Enable)
{
	gEnableThrowInAssert = Enable;
}

bool Soy::Assert(bool Condition, std::ostream& ErrorMessage ) throw( AssertException )
{
	__thread static std::ostream* LastErrorMessage = nullptr;
	__thread static Soy::TErrorMessageFunc ErrorFunc = nullptr;
	
	if ( !ErrorFunc )
	{
		ErrorFunc = []
		{
			return Soy::StreamToString( *LastErrorMessage );
		};
	}
	LastErrorMessage = &ErrorMessage;

	return Assert( Condition, ErrorFunc );
}


void Soy::Private::Assert_Impl(TErrorMessageFunc ErrorMessageFunc) throw(AssertException)
{
	std::string ErrorMessage = ErrorMessageFunc();
	
	std::Debug << "Assert: " << ErrorMessage << std::endl;
	
	//	if the debugger is attached, try and break the debugger without an exception so we can continue
	if ( Platform::IsDebuggerAttached() )
		if ( Soy::Platform::DebugBreak() )
			return;
	
	//	sometimes we disable throwing an exception to stop hosting being taken down
	if ( !gEnableThrowInAssert )
		return;
	
	//	throw exception
	throw Soy::AssertException( ErrorMessage );
}

