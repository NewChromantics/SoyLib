#pragma once

#include <ofxSoylent.h>

namespace msa { 
	
	class OpenCL;
	class OpenCLKernel;
	
	class OpenCLProgram {
	public:
		OpenCLProgram(OpenCL& Parent);
		~OpenCLProgram();
		
		bool			loadFromFile(std::string filename,bool isBinary,const char* BuildOptions);
		bool			loadFromSource(std::string source,const char* sourceLocation,const char* BuildOptions);
		static void		GetIncludePaths(Array<BufferString<MAX_PATH>>& Paths);
		bool			ParseIncludes(const char* Filename,TString& Source,Array<BufferString<MAX_PATH>>& FilesAlreadyIncluded,const Array<BufferString<MAX_PATH>>& Paths);
		bool			ParseIncludes(TString& Source,Array<BufferString<MAX_PATH>>& FilesAlreadyIncluded,const Array<BufferString<MAX_PATH>>& Paths);

		//	create kernel instance for this program on this queue/device
		OpenCLKernel*	loadKernel(std::string kernelName,cl_command_queue Queue);
		
		const std::string&	getName() const	{	return mName;	}
		cl_program&		getCLProgram()	{	return mProgram;	}
		
	protected:	
		bool			build(const char* sourceLocation,const char* BuildOptions);

	protected:	
		OpenCL&			mParent;
		cl_program		mProgram;
		std::string		mName;		//	for debug purposes only, most likely the filename the program was loaded from
	};
	
}