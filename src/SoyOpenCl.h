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


class SoyOpenClKernelRef
{
public:
	SoyOpenClKernelRef()
	{
	}
	SoyOpenClKernelRef(SoyRef Shader,const char* Kernel) :
		mShader	( Shader ),
		mKernel	( Kernel )
	{
	}
public:
	SoyRef				mShader;
	BufferString<100>	mKernel;
};


class SoyOpenClKernel
{
public:
	SoyOpenClKernel(const char* Name) :
		mName	( Name ),
		mKernel	( NULL )
	{
	}
	~SoyOpenClKernel()
	{
		DeleteKernel();
	}

	bool				IsValid() const			{	return mKernel!=NULL;	}
	void				DeleteKernel();
	inline bool			operator==(const char* Name) const	{	return mName == Name;	}

public:
	BufferString<100>	mName;
	msa::OpenCLKernel*	mKernel;
	ofMutex				mArgLock;	//	http://www.khronos.org/message_boards/showthread.php/6788-Multiple-host-threads-with-single-command-queue-and-device
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
	SoyOpenClKernel*	GetKernel(const char* Name);		//	load and return kernel
	SoyRef				GetRef() const							{	return mRef;	}
	inline bool			operator==(const char* Filename) const	{	return mFilename.StartsWith( Filename, false );	}
	inline bool			operator==(const SoyRef& Ref) const		{	return GetRef() == Ref;	}

private:
	SoyOpenClKernel*		FindKernel(const char* Name);

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyOpenClManager&		mManager;
	SoyRef					mRef;
	Array<SoyOpenClKernel*>	mKernels;
};

class SoyOpenClManager : public SoyThread
{
public:
	SoyOpenClManager(prmem::Heap& Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	SoyOpenClKernel*		GetKernel(SoyOpenClKernelRef KernelRef);
	bool					IsValid();			//	if not, cannot use opencl
	SoyOpenClShader*		LoadShader(const char* Filename);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyOpenClShader*		GetShader(const char* Filename);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;

public:
	msa::OpenCL				mOpencl;
	ofEvent<SoyOpenClShader*>	mOnShaderLoaded;	//	or [successfully] reloaded - after kernels have loaded
};



class SoyClShaderRunner
{
public:
	SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager);

	bool				IsValid()		
	{
		auto* pKernel = GetKernel();	
		return pKernel ? pKernel->IsValid() : false;
	}
	SoyOpenClKernel*	GetKernel()		{	return mManager.GetKernel( mKernelRef );	}

public:
	SoyOpenClKernelRef	mKernelRef;
	SoyOpenClManager&	mManager;
};




class SoyClDataBuffer
{
public:
	template<typename TYPE>
	explicit SoyClDataBuffer(const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE)
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE)
	{
		Write( Array, ReadWriteMode );
	}

	template<typename TYPE>
	bool		Write(const Array<TYPE>& Array,cl_int ReadWriteMode)
	{
		return mBuffer.initBuffer( Array.GetDataSize(), ReadWriteMode, const_cast<TYPE*>(Array.GetArray()), true );
	}
	template<typename TYPE>
	bool		Write(const TYPE& Data,cl_int ReadWriteMode)
	{
		return mBuffer.initBuffer( sizeof(TYPE), ReadWriteMode, &const_cast<TYPE&>(Data), true );
	}

	template<typename TYPE>
	bool		Read(Array<TYPE>& Array,int ElementCount=-1)
	{
		if ( ElementCount < 0 )
			ElementCount = Array.GetSize();
		Array.SetSize( ElementCount );
		if ( Array.IsEmpty() )
			return true;
		return mBuffer.read( Array.GetArray(), 0, Array.GetDataSize(), true );
	}

	template<typename TYPE>
	bool		Read(TYPE& Data)
	{
		return mBuffer.read( &Data, 0, sizeof(TYPE), true );
	}

public:
	msa::OpenCLBuffer	mBuffer;
};

