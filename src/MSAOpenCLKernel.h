/***********************************************************************
 
 OpenCL Kernel
 Do not instanciate this directly.
 Instead use OpenCL::loadKernel or OpenCLProgram::loadKernel
 
 ************************************************************************/ 


#pragma once

//#include "ofMain.h"
#include "ofxSoylent.h"
#include "MSAOpenCLMemoryObject.h"


namespace msa { 
	class OpenCL;
	
	class OpenCLKernel {
		friend class OpenCLProgram;
		
	public:
		OpenCLKernel(OpenCL& Parent,cl_kernel Kernel,cl_command_queue Queue,std::string Name);
		~OpenCLKernel();
		
		// assign buffer to arguments
		//	void setArg(int argNumber, cl_mem clMem);
		//	void setArg(int argNumber, float f);
		//	void setArg(int argNumber, int i);
		
		template<class T>
		bool setArg(int argNumber, T &arg){
			//		ofLog(OF_LOG_VERBOSE, "OpenCLKernel::setArg " + name + ": " + ofToString(argNumber));	
			assert( mKernel );
			if ( !mKernel )
				return false;
			
			cl_int err  = clSetKernelArg( mKernel, argNumber, sizeof(T), &arg);
			assert(err == CL_SUCCESS);
			return (err==CL_SUCCESS);
		}
		
		// run the kernel
		// globalSize and localSize should be int arrays with same number of dimensions as numDimensions
		// leave localSize blank to let OpenCL determine optimum
		bool	run(bool Blocking,int numDimensions, size_t *globalSize, size_t *localSize);
		
		// some wrappers for above to create the size arrays on the run
		bool	run1D(bool Blocking,size_t globalSize, size_t localSize = 0);
		bool	run2D(bool Blocking,size_t globalSizeX, size_t globalSizeY, size_t localSizeX = 0, size_t localSizeY = 0);
		bool	run3D(bool Blocking,size_t globalSizeX, size_t globalSizeY, size_t globalSizeZ, size_t localSizeX = 0, size_t localSizeY = 0, size_t localSizeZ = 0);
		
		cl_kernel&			getCLKernel()	{	return mKernel;	}
		const std::string&	getName() const	{	return mName;	}
		cl_command_queue	getQueue()		{	return mQueue;	}

	protected:
		std::string			mName;
		cl_kernel			mKernel;
		cl_command_queue	mQueue;

	public:
		OpenCL&				mParent;
	};
}
