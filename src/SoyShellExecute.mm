#include "SoyShellExecute.h"
#if defined(TARGET_OSX)
#include <Foundation/NSTask.h>
#endif
#include <SoyString.h>



class Platform::TProcessInfo : public Soy::TProcessInfo
{
public:
	TProcessInfo(const std::string& RunCommand);
	
	virtual int32_t	WaitForProcessHandle() override;

	void			RunTask();
	
	NSTask*			mTask = nullptr;
	NSPipe*			mStdOutPipe;
	NSFileHandle*	mStdOutFileHandle;
	NSPipe*			mStdErrPipe;
	NSFileHandle*	mStdErrFileHandle;
};


std::shared_ptr<Soy::TProcessInfo> Platform::AllocProcessInfo(const std::string& RunCommand)
{
	return std::make_shared<Platform::TProcessInfo>(RunCommand);
}


Platform::TProcessInfo::TProcessInfo(const std::string& RunCommand)
{
	//	https://stackoverflow.com/a/412573/355753
	//int pid = [[NSProcessInfo processInfo] processIdentifier];
	mStdOutPipe = [NSPipe pipe];
	mStdOutFileHandle = mStdOutPipe.fileHandleForReading;
	mStdErrPipe = [NSPipe pipe];
	mStdErrFileHandle = mStdErrPipe.fileHandleForReading;
	
	mTask = [[NSTask alloc] init];
	mTask.launchPath = Soy::StringToNSString(RunCommand);
	//task.arguments = @[@"foo", @"bar.txt"];
	mTask.standardOutput = mStdErrPipe;
	mTask.standardError = mStdErrPipe;
	
	RunTask();
}

void Platform::TProcessInfo::RunTask()
{
	//	gr: this is blocking
	@try
	{
		NSError* Error = nullptr;
		auto Result = [mTask launchAndReturnError:&Error];
		if ( !Result )
		{
			auto ErrorStr = Soy::NSErrorToString(Error);
			throw Soy::AssertException(ErrorStr);
		}
		//[mTask launch];
		//auto Result = [mTask launchAndReturnError];
		//NSData *data = [file readDataToEndOfFile];
		//[file closeFile];
		
		//NSString *grepOutput = [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
		//NSLog (@"grep returned:\n%@", grepOutput);
		
		//	read pipes
	}
	@catch (NSException* e)
	{
		auto Error = Soy::NSErrorToString(e);
		throw Soy::AssertException(Error);
	}
}

int32_t Platform::TProcessInfo::WaitForProcessHandle()
{
	auto ExitCode = [mTask terminationStatus];
	mTask = nullptr;
	return ExitCode;
}

