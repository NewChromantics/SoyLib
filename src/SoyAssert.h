#pragma once

#include "SoyTypes.h"
#include <functional>


//	windows complains that I don't need to specify throw exception type?
#if !defined(TARGET_PS4)&& !defined(TARGET_LINUX)
#pragma warning(disable:4290)	//	C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#endif

#if defined(__OBJC__)
@class NSException;
#endif

namespace Soy
{
	//	forward declarations
	class AssertException;
	
	
#if defined(__OBJC__)
	std::string		NSErrorToString(NSException* e);
	std::string		NSErrorToString(NSError* e);
#endif
};


//	helper which inserts the function name
#define Soy_AssertException(Message)	Soy::AssertException( __PRETTY_FUNCTION__, (Message) )

class Soy::AssertException : public std::exception
{
public:
	//	2 param version for easy  AssertException(__PRETTY_FUNCTION__,"Message")
	AssertException(const std::string& Message) :
		mError	( Message )
	{
	}
	AssertException(const std::stringstream& Message) :
		mError	( Message.str() )
	{
	}
	AssertException(const char* Function,const std::stringstream& Message) :
		mError	( std::string(Function) + std::string(" ") + Message.str() )
	{
	}
	AssertException(const char* Message) :
		mErrorConst	( Message )
	{
	}
	AssertException(const char* Function,const char* Message) :
		mError	( std::string(Function) + std::string(" ") + std::string(Message) )
	{
	}
#if defined(__OBJC__)
	AssertException(NSException* e) :
		mError	( Soy::NSErrorToString(e) )
	{
	}
#endif
#if defined(__OBJC__)
	AssertException(NSError* e) :
		mError	( Soy::NSErrorToString(e) )
	{
	}
#endif
	
	virtual const char* what() const __noexcept {	return mErrorConst ? mErrorConst : mError.c_str();	}

public:
	const char*			mErrorConst = nullptr;
	std::string			mError;
};



#define Soy_AssertTodo()	throw Soy::AssertException( std::string("todo: ").append(__func__) )



