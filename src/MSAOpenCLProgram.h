#pragma once

#include <ofxSoylent.h>

namespace SoyFilesystem
{
	class Path;
	class File;
};

//#define ENABLE_PARSE_INCLUDES

namespace msa { 
	
	class OpenCL;
	class OpenCLKernel;
	
	class OpenCLProgram {
	public:
		OpenCLProgram(OpenCL& Parent);
		~OpenCLProgram();
		
		bool			loadFromFile(std::string filename,bool isBinary,std::string BuildOptions);
		bool			loadFromSource(std::string source,std::string sourceLocation,std::string BuildOptions);
		static void		GetIncludePaths(Array<SoyFilesystem::Path>& Paths);
		bool			ParseIncludes(std::string Filename,std::string& Source,Array<SoyFilesystem::File>& FilesAlreadyIncluded,const Array<SoyFilesystem::Path>& Paths);
		bool			ParseIncludes(std::string& Source,Array<SoyFilesystem::File>& FilesAlreadyIncluded,const Array<SoyFilesystem::Path>& Paths);

		//	create kernel instance for this program on this queue/device
		OpenCLKernel*	loadKernel(std::string kernelName,cl_command_queue Queue);
		
		const std::string&	getName() const	{	return mName;	}
		cl_program&		getCLProgram()	{	return mProgram;	}
		
	protected:	
		bool			build(std::string sourceLocation,std::string BuildOptions);

	protected:	
		OpenCL&			mParent;
		cl_program		mProgram;
		std::string		mName;		//	for debug purposes only, most likely the filename the program was loaded from
	};
	
}