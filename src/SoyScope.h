#pragma once

#include <functional>

namespace Soy
{
	
	//	run a function at enter and exit of a scope
	class TScopeCall
	{
	public:
		typedef std::function<void()> ENTER_FUNCTION;
		typedef std::function<void()> EXIT_FUNCTION;
	public:
		TScopeCall(ENTER_FUNCTION EnterFunc,EXIT_FUNCTION ExitFunc) :
		mExitFunc		( ExitFunc )
		{
			if ( EnterFunc )
				EnterFunc();
		}
		~TScopeCall()
		{
			if ( mExitFunc )
				mExitFunc();
		}
		
		EXIT_FUNCTION	mExitFunc;
	};
}


//	make a TScopeCall with a function pointer (can be empty lambda; []{ int++ };)
//	auto Enter = []{ GlobalVar++ };
//	auto Scope = SoyScope( Enter )
template<class ENTER_FUNCTION,class EXIT_FUNCTION>
Soy::TScopeCall SoyScope(ENTER_FUNCTION EnterFunc,EXIT_FUNCTION ExitFunc)
{
	return Soy::TScopeCall( EnterFunc, ExitFunc );
}

//	make a TScopeCall with an existing complex lambda variable (without the need to wrap it yourself)
//	auto Enter = [&LocalVar]{ LocalVar++ };
//	auto Scope = SoyScopeSimple( Enter )
template<class ENTER_FUNCTION,class EXIT_FUNCTION>
Soy::TScopeCall SoyScopeSimple(ENTER_FUNCTION EnterFunc,EXIT_FUNCTION ExitFunc)
{
	return Soy::TScopeCall( [EnterFunc]{EnterFunc();}, [ExitFunc]{ExitFunc();} );
}

//	make a TScopeCall that calls a member of an object (via a std::function)
//	TClass Object;
//	auto Scope = SoyScopeMember( Object, &TClass::Enter, &TClass::Exit )
template<class OBJECT>
Soy::TScopeCall SoyScopeMember(OBJECT& Object,void(OBJECT::*EnterMemberFunc)(),void(OBJECT::*ExitMemberFunc)())
{
	std::function<void(void)> EnterFunc = std::bind( EnterMemberFunc, &Object );
	std::function<void(void)> ExitFunc = std::bind( ExitMemberFunc, &Object );
	return SoyScopeSimple( EnterFunc, ExitFunc );
}
