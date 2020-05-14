#include "SoyShellExecute.h"



#if defined(TARGET_WINDOWS)
class TReadWritePipe
{
public:
	TReadWritePipe();
	~TReadWritePipe();

	void	StartReadThread(std::function<void(const std::string&)>& OnRead);

protected:
	void	CloseHandles();

public:
	HANDLE	mReadPipe = nullptr;
	HANDLE	mWritePipe = nullptr;
	std::shared_ptr<SoyThreadLambda>	mReadThread;
};
#endif



#if defined(TARGET_WINDOWS)
class Platform::TProcessInfo : public Soy::TProcessInfo
{
public:
	TProcessInfo(const std::string& Command,const ArrayBridge<std::string>& Arguments, std::function<void(const std::string&)>& OnStdOut, std::function<void(const std::string&)>& OnStdErr);

	virtual int32_t		WaitForProcessHandle() override;

	PROCESS_INFORMATION				mProcessInfo;
	std::shared_ptr<TReadWritePipe>	mStdOutPipe;
	std::shared_ptr<TReadWritePipe>	mStdErrPipe;

	std::function<void(const std::string&)>	mOnStdOut;
	std::function<void(const std::string&)>	mOnStdErr;

};
#endif

#if defined(TARGET_WINDOWS)
std::shared_ptr<Soy::TProcessInfo> Platform::AllocProcessInfo(const std::string& RunCommand, const ArrayBridge<std::string>& Arguments, std::function<void(const std::string&)>& OnStdOut, std::function<void(const std::string&)>& OnStdErr)
{
	return std::shared_ptr<Soy::TProcessInfo>(new Platform::TProcessInfo(RunCommand, Arguments, OnStdOut, OnStdErr ));
}
#endif

#if defined(TARGET_LINUX)
std::shared_ptr<Soy::TProcessInfo> Platform::AllocProcessInfo(const std::string& RunCommand, const ArrayBridge<std::string>& Arguments, std::function<void(const std::string&)>& OnStdOut, std::function<void(const std::string&)>& OnStdErr)
{
	Soy_AssertTodo();
}
#endif





#if defined(TARGET_WINDOWS)
TReadWritePipe::TReadWritePipe()
{
	// Create a pipe for the redirection of the STDOUT
	// of a child process.
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = nullptr;
	
	auto bSuccess = CreatePipe(&mReadPipe, &mWritePipe, &saAttr, 0);
	if (!bSuccess)
		Platform::ThrowLastError("CreatePipe(StdOut");
	
	bSuccess = SetHandleInformation(mReadPipe, HANDLE_FLAG_INHERIT, 0);
	if (!bSuccess)
		Platform::ThrowLastError("SetHandleInformation(StdOut");
	
}
#endif

#if defined(TARGET_WINDOWS)
TReadWritePipe::~TReadWritePipe()
{
	if (mReadThread)
	{
		mReadThread->Stop(false);
		
		//	can't call CloseHandle from a different thread as ReadFile is blocked,
		//	all I/O block (expects single threaded)
		//	so CloseHandle will hang whilst ReadFile hangs
		//	instead kill anything on that thread (the read) and we should be able to interrupt
		if (!CancelSynchronousIo(mReadThread->GetThreadNativeHandle()))
		{
			//	gr: this will fail if nothing is currently blocking
			auto Error = Platform::GetLastErrorString();
			std::Debug << "CancelSynchronousIo TReadWritePipe ReadThread error " << Error << std::endl;
		}
		//	gr: we can get a race condition here, where the loop has looped around and ReadFile is blocking again
		CloseHandles();
		
		//	wait for thread to end (OnOutput callback might still be executing)
		mReadThread->WaitToFinish();
		mReadThread.reset();
	}
	
	CloseHandles();
}
#endif

#if defined(TARGET_WINDOWS)
void TReadWritePipe::CloseHandles()
{
	//	CloseHandle throws an exception if closed twice, so we have to be safe
	if (mReadPipe)
	{
		CloseHandle(mReadPipe);
		mReadPipe = nullptr;
	}
	
	if (mWritePipe)
	{
		CloseHandle(mWritePipe);
		mWritePipe = nullptr;
	}
}
#endif

#if defined(TARGET_WINDOWS)
void TReadWritePipe::StartReadThread(std::function<void(const std::string&)>& OnRead)
{
	auto ReadThread = [this, OnRead]()
	{
		DWORD BytesAvailible = 0;
		DWORD BytesLeft = 0;
		if (!PeekNamedPipe(mReadPipe, nullptr, 0, nullptr, &BytesAvailible, &BytesLeft))
			Platform::ThrowLastError("ReadWritePipe PeekNamedPipe");
		
		CHAR Buffer[1024];
		DWORD BytesRead = 0;
		auto Success = ::ReadFile(this->mReadPipe, Buffer, std::size(Buffer), &BytesRead, NULL);
		//	https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
		//	Note  The GetLastError code ERROR_IO_PENDING is not a failure; it designates the read operation is pending completion asynchronously. For more information, see Remarks.
		if (!Success)
			Platform::ThrowLastError("ReadWritePipe ReadFile");
		
		if (BytesRead == 0)
		{
			std::Debug << "ReadWritePipe ReadFile returned 0 bytes read" << std::endl;
			return true;
		}
		//std::Debug << "Pipe read " << BytesRead << "/" << BytesAvailible << "/" << BytesLeft << " bytes" << std::endl;
		
		//	split by line?
		std::string Line(Buffer, BytesRead);
		OnRead(Line);
		return true;
	};
	this->mReadThread.reset(new SoyThreadLambda("ReadPipe",ReadThread));
}
#endif



Soy::TShellExecute::TShellExecute(const std::string& Command,const ArrayBridge<std::string>&& Arguments,std::function<void(int)> OnExit, std::function<void(const std::string&)> OnStdOut, std::function<void(const std::string&)> OnStdErr) :
	SoyThread	(std::string("ShellExecute ") + Command ),
	mOnExit		(OnExit),
	mOnStdOut	(OnStdOut),
	mOnStdErr	(OnStdErr)
{
	if ( !mOnExit )
	{
		mOnExit = [](int32_t ExitCode)
		{
			std::Debug << "Process exiteed with " << ExitCode << std::endl;
		};
	}

	//	throw here if we can't create the process
	mProcessInfo = ::Platform::AllocProcessInfo(Command,Arguments, mOnStdOut, mOnStdErr );
	//CreateProcessHandle( RunCommand );
	Start();
}

Soy::TShellExecute::~TShellExecute()
{
	//	need to add Kill() here to kill any process as we're currently waiting with Infinite timeout
	Stop(true);
	mProcessInfo.reset();
}

bool Soy::TShellExecute::ThreadIteration()
{
	//	we're not expecting this to throw, if we let it fall through, should get an error when thread is released
	auto ExitCode = mProcessInfo->WaitForProcessHandle();
	mOnExit(ExitCode);
	return false;
}


#if defined(TARGET_WINDOWS)
Platform::TProcessInfo::TProcessInfo(const std::string& RunCommand,const ArrayBridge<std::string>& Arguments, std::function<void(const std::string&)>& OnStdOut, std::function<void(const std::string&)>& OnStdErr) :
	mOnStdOut	( OnStdOut ),
	mOnStdErr	( OnStdErr )
{
	//	https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
	mStdOutPipe.reset(new TReadWritePipe);
	mStdErrPipe.reset(new TReadWritePipe);
	
	// Setup the child process to use the STDOUT redirection
	PROCESS_INFORMATION& piProcInfo = mProcessInfo;
	STARTUPINFO siStartInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	
	siStartInfo.hStdError = mStdErrPipe->mWritePipe;
	siStartInfo.hStdOutput = mStdOutPipe->mWritePipe;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	
	std::stringstream FullCommand;
	FullCommand << RunCommand;
	for (auto i = 0; i < Arguments.GetSize(); i++)
	{
		FullCommand << ' ' << Arguments[i];
	}
	auto FullCommandStr = FullCommand.str();
	auto* RunCommandStr = const_cast<char*>(FullCommandStr.c_str());
	
	// Execute a synchronous child process & get exit code
	auto Success = CreateProcessA(nullptr,
								  RunCommandStr,  // command line
								  nullptr,                 // process security attributes
								  nullptr,                 // primary thread security attributes
								  true,                 // handles are inherited
								  0,                    // creation flags
								  nullptr,                 // use parent's environment
								  nullptr,                 // use parent's current directory
								  &siStartInfo,         // STARTUPINFO pointer
								  &piProcInfo);        // receives PROCESS_INFORMATION
	
	std::Debug << "Started process " << RunCommand << "..." << std::endl;
	if (Success)
		return;
	
	//	get last error
	Platform::IsOkay("CreateProcess");
	
	//	if that didn't throw like it should, just throw anyway
	throw Soy::AssertException("CreateProcess failed (No LastError?)");
}
#endif


#if defined(TARGET_WINDOWS)
int32_t Platform::TProcessInfo::WaitForProcessHandle()
{
	//	start async read-pipe/file threads
	mStdOutPipe->StartReadThread(this->mOnStdOut);
	mStdErrPipe->StartReadThread(this->mOnStdErr);
	
	auto Timeout = INFINITE;	//	 (DWORD)(-1L)
	DWORD ExitCode = 0;
	WaitForSingleObject(mProcessInfo.hProcess, Timeout);
	GetExitCodeProcess(mProcessInfo.hProcess, &ExitCode);
	CloseHandle(mProcessInfo.hProcess);
	CloseHandle(mProcessInfo.hThread);
	
	//	these should have auto ended by now
	mStdOutPipe.reset();
	mStdErrPipe.reset();
	
	return ExitCode;
	/*
	 // Read the data written to the pipe
	 DWORD bytesInPipe = 0;
	 while (bytesInPipe == 0) {
	 bSuccess = PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL,
	 &bytesInPipe, NULL);
	 if (!bSuccess) return bSuccess;
	 }
	 if (bytesInPipe == 0) return TRUE;
	 DWORD dwRead;
	 CHAR *pipeContents = new CHAR[bytesInPipe];
	 bSuccess = ReadFile(g_hChildStd_OUT_Rd, pipeContents,
	 bytesInPipe, &dwRead, NULL);
	 if (!bSuccess || dwRead == 0) return FALSE;
	 
	 // Split the data into lines and add them to the return vector
	 std::stringstream stream(pipeContents);
	 std::string str;
	 while (getline(stream, str))
	 stdOutLines->push_back(CStringW(str.c_str()));
	 */
}
#endif

