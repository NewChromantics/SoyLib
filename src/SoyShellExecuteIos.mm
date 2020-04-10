#include "SoyShellExecute.h"



std::shared_ptr<Soy::TProcessInfo> Platform::AllocProcessInfo(const std::string& Command,const ArrayBridge<std::string>& Arguments, std::function<void(const std::string&)>& OnStdOut, std::function<void(const std::string&)>& OnStdErr)
{
	Soy_AssertTodo();
}

