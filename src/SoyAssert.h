#pragma once

#include "SoyTypes.h"
#include <functional>


//	windows complains that I don't need to specify throw exception type?
#pragma warning(disable:4290)	//	C++ exception specification ignored except to indicate a function is not __declspec(nothrow)


namespace Soy
{
	//	forward declarations
	class AssertException;
};


class Soy::AssertException : public std::exception
{
public:
	AssertException(std::string Message) :
	mError	( Message )
	{
	}
	virtual const char* what() const __noexcept	{	return mError.c_str();	}

public:
	std::string			mError;
};



namespace Soy
{
	typedef std::string(*TErrorMessageFunc)();

	namespace Private
	{
		void		Assert_Impl(TErrorMessageFunc ErrorMessageFunc) throw(AssertException);
	};
	
	//	replace asserts with exception. If condition fails false is returned to save code

	//	allow? inline by having the condition here and branching and doing the other stuff on failure
	inline bool		Assert_Impl(bool Condition,TErrorMessageFunc ErrorMessageFunc) throw(AssertException)
	{
		if ( Condition )
			return true;
		Private::Assert_Impl( ErrorMessageFunc );
		return false;
	}
	
	inline bool	Assert(bool Condition,std::function<std::string()>&& ErrorMessageFunc) throw(AssertException)
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
	inline bool	Assert(bool Condition,const std::string& ErrorMessage) throw(AssertException)
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
	
	inline bool	Assert(bool Condition, std::stringstream&& ErrorMessage ) throw( AssertException )
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
	
	bool	Assert(bool Condition, std::ostream& ErrorMessage) throw(AssertException);
	inline bool	Assert(bool Condition, std::stringstream& ErrorMessage) throw(AssertException)
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
	inline bool	Assert(bool Condition,const char* ErrorMessage) throw(AssertException)
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
#define Soy_AssertTodo()	Soy::Assert(false, std::stringstream()<<"todo: "<<__func__ )



