#pragma once

#include "SoyTypes.h"
#include "string.hpp"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyEvent.h"
#include "SoyScope.h"


namespace Soy
{
	namespace Platform
	{
		bool	DebugBreak();
		bool	IsDebuggerAttached();
		void	DebugPrint(const std::string& String);
	}

	//	exception for Soy::Assert
	class AssertException;
};


namespace std
{
	class DebugStream;
	
	//	cross platform debug output stream
	//	std::Debug << XXX << std::endl;
	extern DebugStream	Debug;
}


namespace std
{
	class DebugStreamBuf : public streambuf
	{
	public:
		DebugStreamBuf() :
			mEnableStdOut	( true )
		{
		};
		~DebugStreamBuf()	{	flush();	}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);		// disallow copy construction
		void operator= (DebugStreamBuf const &);	// disallow copy assignment

		std::string&	GetBuffer();
		
	public:
		bool			mEnableStdOut;
		SoyEvent<const std::string>	mOnFlush;		//	catch debug output
	};

	class DebugStream : public std::ostream
	{
	public:
		explicit DebugStream() : 
			std::basic_ostream<char,std::char_traits<char> > (&mBuffer) 
		{
		}

		SoyEvent<const std::string>&		GetOnFlushEvent()	{	return mBuffer.mOnFlush;	}
		
		//	toggle std output for this std debug stream
		void			EnableStdOut(bool Enable)	{	mBuffer.mEnableStdOut = Enable;	}
		
	private:
		DebugStreamBuf	mBuffer;
	};

};



//	gr: move this to... string?
namespace Soy
{
	BufferString<20>	FormatSizeBytes(uint64 bytes);
	
}


class ofScopeTimerWarning
{
public:
	ofScopeTimerWarning(const char* Name,uint64 WarningTimeMs,bool AutoStart=true,std::ostream& Output=std::Debug) :
		mName				( Name ),
		mWarningTimeMs		( WarningTimeMs ),
		mStopped			( true ),
		mReportedOnLastStop	( false ),
		mAccumulatedTime	( 0 ),
		mOutputStream		( Output )
	{
		if ( AutoStart )
			Start( true );
	}
	~ofScopeTimerWarning()
	{
		if ( mStopped && !mReportedOnLastStop )
			Report();
		else
			Stop();
	}
	//	returns if a report was output
	bool				Stop(bool DoReport=true)
	{
		if ( mStopped )
		{
			mReportedOnLastStop = false;
			return false;
		}

		SoyTime Now(true);
		uint64 Delta = Now.GetTime() - mStartTime.GetTime();
		mAccumulatedTime += Delta;

		mReportedOnLastStop = DoReport;
		bool DidReport = false;
		if ( DoReport )
			DidReport = Report();
		
		mStopped = true;
		return DidReport;
	}
	bool				Report(bool Force=false)
	{
		if ( mAccumulatedTime >= mWarningTimeMs || Force )
		{
			mOutputStream << mName << " took " << mAccumulatedTime << "ms to execute" << std::endl;
			return true;
		}
		return false;
	}

	void				Start(bool Reset=false)
	{
		assert( mStopped );

		if ( Reset )
			mAccumulatedTime = 0;
			
		SoyTime Now(true);
		mStartTime = Now;
		mStopped = false;
	}

	SoyTime				mStartTime;
	uint64				mWarningTimeMs;
	BufferString<200>	mName;
	bool				mStopped;
	bool				mReportedOnLastStop;
	uint64				mAccumulatedTime;
	std::ostream&		mOutputStream;
};





class Soy::AssertException : public std::exception
{
public:
	AssertException(std::string Message) :
	mError	( Message )
	{
	}
	virtual const char* what() const _NOEXCEPT	{	return mError.c_str();	}
std::string			mError;
};

namespace Soy
{
	void		EnableThrowInAssert(bool Enable);		//	use to turn of throwing exceptions (good for plugins so they don't take down the host application)
	typedef std::string(*TErrorMessageFunc)();

	namespace Private
	{
		void		Assert_Impl(TErrorMessageFunc ErrorMessageFunc) throw(AssertException);
	};
	
#pragma warning(disable:4290)
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
		__thread static std::function<std::string()>* LastFunc = &ErrorMessageFunc;
		__thread static auto ErrorFunc = []
		{
			return (*LastFunc)();
		};
		LastFunc = &ErrorMessageFunc;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	//	helpful wrappers for common string types
	inline bool	Assert(bool Condition,const std::string& ErrorMessage) throw(AssertException)
	{
		__thread static auto* LastErrorMessage = &ErrorMessage;
		__thread static auto ErrorFunc = []()->std::string
		{
			return std::string(*LastErrorMessage);
		};
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	inline bool	Assert(bool Condition, std::stringstream&& ErrorMessage ) throw( AssertException )
	{
		__thread static auto* LastErrorMessage = &ErrorMessage;
		__thread static auto ErrorFunc = []
		{
			return LastErrorMessage->str();
		};
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	inline bool	Assert(bool Condition, std::stringstream& ErrorMessage ) throw( AssertException )
	{
		__thread static auto* LastErrorMessage = &ErrorMessage;
		__thread static auto ErrorFunc = []
		{
			return LastErrorMessage->str();
		};
		LastErrorMessage = &ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
	
	bool		Assert(bool Condition, std::ostream& ErrorMessage ) throw( AssertException );
	
	//	const char* version to avoid unncessary allocation to std::string
	inline bool	Assert(bool Condition,const char* ErrorMessage) throw(AssertException)
	{
		//	lambdas with capture are expensive to construct and destruct
		__thread static auto* LastErrorMessage = ErrorMessage;
		__thread static auto ErrorFunc = []
		{
			return std::string( LastErrorMessage );
		};
		LastErrorMessage = ErrorMessage;
		return Assert_Impl( Condition, ErrorFunc );
	}
};
#define Soy_AssertTodo()	Soy::Assert(false, std::stringstream()<<"todo: "<<__func__ )



