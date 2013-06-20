#pragma once

#include "ofxSoylent.h"
#include "SoyThread.h"
#include <MSAOpenCL.h>


class SoyOpenClManager;

class SoyFileChangeDetector
{
public:
	SoyFileChangeDetector(const char* Filename);

	bool					HasChanged();
	Poco::Timestamp			GetCurrentTimestamp();	//	get the file's current timestamp
	void					SetLastModified(Poco::Timestamp Timestamp);

public:
	Poco::Timestamp			mLastModified;
	BufferString<200>		mFilename;
};

//	soy-cl-program which auto-reloads if the file changes
class SoyOpenClShader : public SoyFileChangeDetector
{
public:
	SoyOpenClShader(SoyRef Ref,const char* Filename,SoyOpenClManager& Manager) :
		SoyFileChangeDetector	( Filename ),
		mRef					( Ref ),
		mManager				( Manager ),
		mProgram				( NULL )
	{
	}
	~SoyOpenClShader()	{	UnloadShader();	}

	bool				LoadShader();
	void				UnloadShader();
	msa::OpenCLKernel*	GetKernel(const char* Name);		//	load and return kernel
	SoyRef				GetRef() const						{	return mRef;	}
	inline bool			operator==(const SoyRef& Ref) const	{	return GetRef() == Ref;	}

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyOpenClManager&		mManager;
	SoyRef					mRef;
	Array<SoyPair<BufferString<100>,msa::OpenCLKernel*>>	mKernels;
};

class SoyOpenClManager : public SoyThread
{
public:
	SoyOpenClManager(prmem::Heap& Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	bool					IsValid();			//	if not, cannot use opencl
	SoyOpenClShader*		LoadShader(const char* Filename);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;

public:
	msa::OpenCL				mOpencl;
};


