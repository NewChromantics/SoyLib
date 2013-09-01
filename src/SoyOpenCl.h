#pragma once

#include "ofxSoylent.h"
#include "SoyThread.h"
#include <MSAOpenCL.h>
#include "SortArray.h"

class SoyOpenClManager;

class SoyFileChangeDetector
{
public:
	SoyFileChangeDetector(const char* Filename);

	bool						HasChanged();
	Poco::Timestamp				GetCurrentTimestamp();	//	get the file's current timestamp
	void						SetLastModified(Poco::Timestamp Timestamp);
	const BufferString<200>&	GetFilename() const	{	return mFilename;	}

private:
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

	BufferString<200>	Debug_GetName() const	{	return BufferString<200>() << mShader << "[" << mKernel << "]";	}

public:
	SoyRef				mShader;
	BufferString<100>	mKernel;
};


class SoyOpenClKernel
{
public:
	SoyOpenClKernel(const char* Name,SoyOpenClManager& Manager) :
		mName			( Name ),
		mKernel			( NULL ),
		mFirstExecution	( true ),
		mManager		( Manager )
	{
	}
	~SoyOpenClKernel()
	{
		DeleteKernel();
	}

	const char*		GetName() const			{	return mName.c_str();	}
	bool			IsValid() const			{	return mKernel!=NULL;	}
	void			DeleteKernel();
	inline bool		operator==(const char* Name) const	{	return mName == Name;	}
	bool			Begin();
	bool			End1D(int Exec1);
	bool			End2D(int Exec1,int Exec2);
	bool			End3D(int Exec1,int Exec2,int Exec3);

private:
	BufferString<100>	mName;
	ofMutex				mArgLock;	//	http://www.khronos.org/message_boards/showthread.php/6788-Multiple-host-threads-with-single-command-queue-and-device
	bool				mFirstExecution;	//	for debugging (ie. working out which kernel crashes the driver)
	SoyOpenClManager&	mManager;

public:
	msa::OpenCLKernel*	mKernel;
};

//	soy-cl-program which auto-reloads if the file changes
class SoyOpenClShader : public SoyFileChangeDetector
{
public:
	SoyOpenClShader(SoyRef Ref,const char* Filename,const char* BuildOptions,SoyOpenClManager& Manager) :
		SoyFileChangeDetector	( Filename ),
		mRef					( Ref ),
		mManager				( Manager ),
		mProgram				( NULL ),
		mBuildOptions			( BuildOptions )
	{
	}
	~SoyOpenClShader()	{	UnloadShader();	}

	bool				IsLoading();
	bool				LoadShader();
	void				UnloadShader();
	SoyOpenClKernel*	GetKernel(const char* Name);		//	load and return kernel
	SoyRef				GetRef() const							{	return mRef;	}
	const char*			GetBuildOptions() const					{	return mBuildOptions;	}
	inline bool			operator==(const char* Filename) const	{	return GetFilename().StartsWith( Filename, false );	}
	inline bool			operator==(const SoyRef& Ref) const		{	return GetRef() == Ref;	}

private:
	SoyOpenClKernel*		FindKernel(const char* Name);

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyOpenClManager&		mManager;
	SoyRef					mRef;
	Array<SoyOpenClKernel*>	mKernels;
	BufferString<100>		mBuildOptions;
};

class SoyOpenClManager : public SoyThread
{
public:
	SoyOpenClManager(prmem::Heap& Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	SoyOpenClKernel*		GetKernel(SoyOpenClKernelRef KernelRef);
	bool					IsValid();			//	if not, cannot use opencl
	SoyOpenClShader*		LoadShader(const char* Filename,const char* BuildOptions);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyOpenClShader*		GetShader(const char* Filename);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));
	prmem::Heap&			GetHeap()		{	return mHeap;	}

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
	SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,const char* BuildOptions=NULL);

	bool				IsValid()		
	{
		auto* pKernel = GetKernel();	
		return pKernel ? pKernel->IsValid() : false;
	}
	SoyOpenClKernel*	GetKernel()		{	return mManager.GetKernel( mKernelRef );	}
	BufferString<200>	Debug_GetName() const	{	return mKernelRef.Debug_GetName();	}

public:
	SoyOpenClKernelRef	mKernelRef;
	SoyOpenClManager&	mManager;
	prmem::Heap&		mHeap;
};




class SoyClDataBuffer
{
private:
	SoyClDataBuffer(const SoyClDataBuffer& That) :
		mManager	( *(SoyOpenClManager*)NULL)
	{
	}
public:
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& Manager,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( NULL ),
		mManager	( Manager )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& Manager,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( NULL ),
		mManager	( Manager )
	{
		Write( Array, ReadWriteMode );
	}

	template<typename ARRAYTYPE>
	bool		Write(const ArrayBridgeDef<ARRAYTYPE>& Array,cl_int ReadWriteMode)
	{
		assert( mBuffer == NULL );
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(ARRAYTYPE::TYPE) > 4 )
		{
			int Remainder = sizeof(ARRAYTYPE::TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<ARRAYTYPE::TYPE>() << " not aligned to 16 bytes; " << sizeof(ARRAYTYPE::TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}

		//	gr: only write data if we're NOT write-only memory
		auto* pArrayData = Array.GetArray();
		void* pData = (ReadWriteMode&CL_MEM_WRITE_ONLY) ? NULL : const_cast<ARRAYTYPE::TYPE*>(pArrayData);
		mBuffer = mManager.mOpencl.createBuffer( Array.GetDataSize(), ReadWriteMode, pData, true );
		return (mBuffer!=NULL);
	}
	template<typename TYPE>	bool					Write(const Array<TYPE>& Array,cl_int ReadWriteMode)			{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,int SIZE> bool			Write(const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE> bool					Write(const RemoteArray<TYPE>& Array,cl_int ReadWriteMode)		{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,class SORTPOLICY> bool	Write(const SortArray<TYPE,SORTPOLICY>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array.mArray ), ReadWriteMode );	}
	
	template<typename TYPE>
	bool		Write(const TYPE& Data,cl_int ReadWriteMode)
	{
		assert( mBuffer == NULL );
		void* pData = (ReadWriteMode&CL_MEM_WRITE_ONLY) ? NULL : &const_cast<TYPE&>(Data);
		mBuffer = mManager.mOpencl.createBuffer( sizeof(TYPE), ReadWriteMode, pData, true );
		return (mBuffer!=NULL);
	}

	~SoyClDataBuffer()
	{
		if ( mBuffer )
		{
			mManager.mOpencl.deleteBuffer( mBuffer );
		}
	}

	template<typename TYPE>
	bool		Read(ArrayBridgeDef<TYPE>& Array,int ElementCount=-1)
	{
		if ( !mBuffer )
			return false;
		if ( ElementCount < 0 )
			ElementCount = Array.GetSize();
		Array.SetSize( ElementCount );
		if ( Array.IsEmpty() )
			return true;
		return mBuffer->read( Array.GetArray(), 0, Array.GetDataSize(), true );
	}

	template<typename TYPE>	bool					Read(Array<TYPE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE,int SIZE> bool			Read(BufferArray<TYPE,SIZE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE>	bool					Read(RemoteArray<TYPE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE,class SORTPOLICY> bool	Read(SortArray<TYPE,SORTPOLICY>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array.mArray ), ElementCount );	}
		
	template<typename TYPE>
	bool		Read(TYPE& Data)
	{
		if ( !mBuffer )
			return false;
		return mBuffer->read( &Data, 0, sizeof(TYPE), true );
	}

	bool		IsValid() const		{	return mBuffer;	}
	operator	bool()				{	return IsValid();	}
	cl_mem&		getCLMem()			{	static cl_mem Dummy;	assert( mBuffer );	return mBuffer ? mBuffer->getCLMem() : Dummy;	}

public:
	msa::OpenCLBuffer*	mBuffer;
	SoyOpenClManager&	mManager;
};

