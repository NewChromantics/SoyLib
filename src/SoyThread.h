#pragma once

#include "ofxSoylent.h"



class SoyThread : public ofThread
{
public:
	SoyThread(const char* threadName)
	{
		if ( threadName )
			setThreadName( threadName );
	}

	void	startThread(bool blocking, bool verbose)
	{
		ofThread::startThread( blocking, verbose );
		setThreadName( getPocoThread().getName() );
	}


	void	setThreadName(const string& name)
	{
		//	set the Poco thread name
		getPocoThread().setName( name );

		//	set the OS thread name
		//	http://msdn.microsoft.com/en-gb/library/xcb2z8hs.aspx
	#if defined(TARGET_WIN32)
		const DWORD MS_VC_EXCEPTION=0x406D1388;
		#pragma pack(push,8)
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} THREADNAME_INFO;
		#pragma pack(pop)

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = name.c_str();
		info.dwThreadID = getPocoThread().tid();
		info.dwFlags = 0;

		//	this will fail if the OS thread hasn't started yet
		if ( info.dwThreadID != 0 )
		{
			__try
			{
				RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	#endif
	}
};

