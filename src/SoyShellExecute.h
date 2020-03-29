#pragma once

#include "SoyThread.h"

namespace Soy
{
	class TShellExecute;
	class TProcessInfo;
}

namespace Platform
{
	class TProcessInfo;		//	per platform
	
	std::shared_ptr<Soy::TProcessInfo>	AllocProcessInfo(const std::string& RunCommand);
}


class Soy::TProcessInfo
{
public:
	virtual int32_t		WaitForProcessHandle()=0;
};
	
//	gr: this might be nice as an RAII wrapper, but for JS, I want the initial creation to throw,
//		and if it was RAII, i'd need a thread on the wrapper, which wouldn't immediately throw
//		unless we have a semaphore or something to indicate it has started
class Soy::TShellExecute : public SoyThread
{
public:
	TShellExecute(const std::string& RunCommand, std::function<void(int)> OnExit, std::function<void(const std::string&)> OnStdOut, std::function<void(const std::string&)> OnStdErr);
	~TShellExecute();
	
protected:
	virtual bool				ThreadIteration() override;
	void						CreateProcessHandle(const std::string& Command);
	
	std::function<void(int32_t)>	mOnExit;
	std::function<void(const std::string&)>	mOnStdOut;
	std::function<void(const std::string&)>	mOnStdErr;
	
	std::shared_ptr<Soy::TProcessInfo>	mProcessInfo;
};
