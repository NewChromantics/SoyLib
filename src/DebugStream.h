#pragma once
#include "SoyNet.h"



namespace std
{
	class DebugStreamBuf : public streambuf 
	{
	public:
		DebugStreamBuf()  { };
		~DebugStreamBuf()	{	flush();	}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);                // disallow copy construction
		void operator= (DebugStreamBuf const &);          // disallow copy assignment

	private:
		string	mBuffer;	///< buffer for current log message
	};

	class DebugStream : public basic_ostream<char,std::char_traits<char> >
	{
	public:
		explicit DebugStream() : 
			std::basic_ostream<char,std::char_traits<char> > (&mBuffer) 
		{
		}

	private:
		DebugStreamBuf	mBuffer;
	};

};


namespace std
{
	extern DebugStream	Debug;
}

