#pragma once


#include <MSAOpenCL.h>


class SoyOpenClManager;

//	soy-cl-program which auto-reloads if the file changes
class SoyOpenClShader
{
public:
	SoyOpenClShader(SoyRef Ref,const char* Filename,msa::OpenCLProgram& Program,SoyOpenClManager& Manager) :
		mRef		( Ref ),
		mFilename	( Filename ),
		mManager	( Manager ),
		mProgram	( &Program )
	{
	}

	msa::OpenCLKernel*	GetKernel(const char* Name);		//	load and return kernel
	SoyRef				GetRef() const						{	return mRef;	}
	inline bool			operator==(const SoyRef& Ref) const	{	return GetRef() == Ref;	}

private:
	msa::OpenCLProgram*	mProgram;
	SoyOpenClManager&	mManager;
	SoyRef				mRef;
	BufferString<1000>	mFilename;
	Array<SoyPair<BufferString<100>,msa::OpenCLKernel*>>	mKernels;
};

class SoyOpenClManager// : class SoyThread
{
public:
	SoyOpenClManager(prmem::Heap& Heap);

	bool					IsValid();			//	if not, cannot use opencl
	SoyRef					LoadShader(const char* Filename);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;

public:
	msa::OpenCL				mOpencl;
};


