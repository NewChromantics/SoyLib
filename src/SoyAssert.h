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
	
	virtual const char* what() const __noexcept	{	return mErrorConst ? mErrorConst : mError.c_str();	}

public:
	const char*			mErrorConst = nullptr;
	std::string			mError;
};



namespace Soy
{
	typedef std::string(*TErrorMessageFunc)();

	namespace Private
	{
		void		Assert_Impl(TErrorMessageFunc ErrorMessageFunc);
	};
	
	//	replace asserts with exception. If condition fails false is returned to save code

	//	allow? inline by having the condition here and branching and doing the other stuff on failure
	inline bool		Assert_Impl(bool Condition,TErrorMessageFunc ErrorMessageFunc)
	{
		if ( Condition )
			return true;
		Private::Assert_Impl( ErrorMessageFunc );
		return false;
	}
	
	inline bool	Assert(bool Condition,std::function<std::string()>&& ErrorMessageFunc)
	{
		__thread static std::function<std::string()>* LastFunc = nullptr;
		__thread static TErrorMessageFunc ErrorFunc = nullptr;
		if ( !ErrorFunc )
		{
			ErrorFunc = []
			{
				return (*LastFunc)();
			};
		}
		LastFunc = &ErrorMessageFunc;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	//	helpful wrappers for common string types
	inline bool	Assert(bool Condition,const std::string& ErrorMessage)
	{
		__thread static const std::string* LastErrorMessage = nullptr;
		__thread static TErrorMessageFunc ErrorFunc = nullptr;
		if ( !ErrorFunc )
		{
			ErrorFunc = []()->std::string
			{
				return std::string(*LastErrorMessage);
			};
		}
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	inline bool	Assert(bool Condition, std::stringstream&& ErrorMessage )
	{
		__thread static std::stringstream* LastErrorMessage = nullptr;
		__thread static TErrorMessageFunc ErrorFunc = nullptr;
		if ( !ErrorFunc )
		{
			ErrorFunc = []
			{
				return LastErrorMessage->str();
			};
		}
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	bool	Assert(bool Condition, std::ostream& ErrorMessage);
	inline bool	Assert(bool Condition, std::stringstream& ErrorMessage)
	{
		__thread static std::stringstream* LastErrorMessage = nullptr;
		__thread static TErrorMessageFunc ErrorFunc = nullptr;
		if ( !ErrorFunc )
		{
			ErrorFunc = []
			{
				return LastErrorMessage->str();
			};
		}
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	//	const char* version to avoid unncessary allocation to std::string
	inline bool	Assert(bool Condition,const char* ErrorMessage)
	{
		//	lambdas with capture are expensive to construct and destruct
		__thread static const char* LastErrorMessage = nullptr;
		__thread static TErrorMessageFunc ErrorFunc = nullptr;
		if ( !ErrorFunc )
		{
			ErrorFunc = []
			{
				return std::string( LastErrorMessage );
			};
		}
		LastErrorMessage = ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
};

#define Soy_AssertTodo()	throw Soy::AssertException( std::string("todo: ").append(__func__) )



