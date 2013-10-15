#pragma once

#include "ofxSoylent.h"
#include "SoyThread.h"
#include <MSAOpenCL.h>
#include "SortArray.h"

class SoyOpenClManager;
class SoyOpenClShader;


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



template<int DIMENSIONS>
class SoyOpenclKernelIteration
{
public:
	SoyOpenclKernelIteration() :
		mFirst		( DIMENSIONS ),
		mCount		( DIMENSIONS ),
		mBlocking	( true )
	{
	}
public:
	BufferArray<int,DIMENSIONS>		mFirst;
	BufferArray<int,DIMENSIONS>		mCount;
	bool							mBlocking;
};
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<1> );
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<2> );
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<3> );



class SoyOpenClKernel
{
public:
	SoyOpenClKernel(const char* Name,SoyOpenClShader& Parent);
	~SoyOpenClKernel()
	{
		DeleteKernel();
	}

	void			OnLoaded();				//	kernel will be null if it failed
	const char*		GetName() const			{	return mName.c_str();	}
	bool			IsValid() const			{	return mKernel!=NULL;	}
	bool			IsValidExecCount(int ExecCount)	{	return (mMaxWorkGroupSize==-1) ? true : (ExecCount<=mMaxWorkGroupSize);	}
	int				GetMaxWorkGroupSize() const	{	return mMaxWorkGroupSize;	}
	cl_command_queue	GetQueue()			{	return mKernel ? mKernel->getQueue() : NULL;	}

	void			DeleteKernel();
	inline bool		operator==(const char* Name) const	{	return mName == Name;	}
	bool			Begin();
	bool			End1D(int Exec1);
	bool			End2D(int Exec1,int Exec2);
	bool			End3D(int Exec1,int Exec2,int Exec3);
	bool			End1D(bool Blocking,int Exec1);
	bool			End2D(bool Blocking,int Exec1,int Exec2);
	bool			End3D(bool Blocking,int Exec1,int Exec2,int Exec3);
	bool			End(const SoyOpenclKernelIteration<1>& Iteration)	{	return End1D( Iteration.mBlocking, Iteration.mCount[0] );	}
	bool			End(const SoyOpenclKernelIteration<2>& Iteration)	{	return End2D( Iteration.mBlocking, Iteration.mCount[0], Iteration.mCount[1] );	}
	bool			End(const SoyOpenclKernelIteration<3>& Iteration)	{	return End3D( Iteration.mBlocking, Iteration.mCount[0], Iteration.mCount[1], Iteration.mCount[2] );	}

	void			GetIterations(Array<SoyOpenclKernelIteration<1>>& Iterations,int Exec1,bool BlockLast=true);
	void			GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast=true);
	void			GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast=true);

	bool									CheckPaddingChecksum(const int* Padding,int Length);
	template<typename TYPE> bool			CheckPaddingChecksum(const TYPE& Object)						{	return CheckPaddingChecksum( Object.mPadding, sizeofarray(Object.mPadding) );	}
	template<typename ARRAYTYPE> bool		CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray);
	template<typename TYPE> bool			CheckPaddingChecksum(const Array<TYPE>& ObjectArray)			{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,int SIZE> bool	CheckPaddingChecksum(const BufferArray<TYPE,SIZE>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE> bool			CheckPaddingChecksum(const RemoteArray<TYPE>& ObjectArray)		{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,class SORT> bool	CheckPaddingChecksum(const SortArray<TYPE,SORT>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray.mArray ) );	}
	
private:
	int					mMaxWorkGroupSize;
	BufferString<100>	mName;

public:
	SoyOpenClManager&	mManager;
	SoyOpenClShader&	mShader;		//	parent
	msa::OpenCLKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
};


template<typename ARRAYTYPE>
inline bool SoyOpenClKernel::CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray)	
{	
	for ( int i=0;	i<ObjectArray.GetSize();	i++ )
		if ( !CheckPaddingChecksum( ObjectArray[i] ) )
			return false;
	return true;
}


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
	SoyOpenClKernel*	GetKernel(const char* Name,cl_command_queue Queue);			//	load and return a kernel
	SoyRef				GetRef() const							{	return mRef;	}
	const char*			GetBuildOptions() const					{	return mBuildOptions;	}
	inline bool			operator==(const char* Filename) const	{	return GetFilename().StartsWith( Filename, false );	}
	inline bool			operator==(const SoyRef& Ref) const		{	return GetRef() == Ref;	}

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyRef					mRef;
	BufferString<100>		mBuildOptions;

public:
	SoyOpenClManager&		mManager;
};

class SoyOpenClManager : public SoyThread
{
public:
	static bool OPENCL_DATA_BLOCKING_READ;
	static bool OPENCL_DATA_BLOCKING_WRITE;
	static bool OPENCL_EXECUTE_BLOCKING;

public:
	SoyOpenClManager(prmem::Heap& Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	SoyOpenClKernel*		GetKernel(SoyOpenClKernelRef KernelRef,cl_command_queue Queue);
	void					DeleteKernel(SoyOpenClKernel* Kernel);
	bool					IsValid();			//	if not, cannot use opencl
	SoyOpenClShader*		LoadShader(const char* Filename,const char* BuildOptions);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyOpenClShader*		GetShader(const char* Filename);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));
	prmem::Heap&			GetHeap()		{	return mHeap;	}

	cl_command_queue		GetQueueForThread();				//	get/alloc a specific queue for the current thread
	cl_command_queue		GetQueueForThread(int ThreadId);	//	get/alloc a specific queue for a thread

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;
	ofMutexT<Array<SoyPair<int,cl_command_queue>>>	mThreadQueues;

public:
	msa::OpenCL				mOpencl;
	ofEvent<SoyOpenClShader*>	mOnShaderLoaded;	//	or [successfully] reloaded - after kernels have loaded
};



class SoyClShaderRunner
{
public:
	SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,const char* BuildOptions=NULL);
	SoyClShaderRunner(const char* Shader,const char* Kernel,bool UseThreadQueue,SoyOpenClManager& Manager,const char* BuildOptions=NULL);
	virtual ~SoyClShaderRunner()				{	DeleteKernel();	}

	bool				IsValid()		
	{
		auto* pKernel = GetKernel();	
		return pKernel ? pKernel->IsValid() : false;
	}
	SoyOpenClKernel*	GetKernel();
	void				DeleteKernel()			{	return mManager.DeleteKernel( mKernel );	}
	BufferString<200>	Debug_GetName() const	{	return mKernelRef.Debug_GetName();	}

public:
	bool				mUseThreadQueue;
	SoyOpenClKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
	SoyOpenClManager&	mManager;
	prmem::Heap&		mHeap;
};




class SoyClDataBuffer
{
private:
	SoyClDataBuffer(const SoyClDataBuffer& That) :
		mManager	( *(SoyOpenClManager*)NULL ),
		mKernel		( *(SoyOpenClKernel*)NULL )
	{
	}
public:
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( NULL ),
		mManager	( Kernel.mManager ),
		mKernel		( Kernel )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( NULL ),
		mManager	( Kernel.mManager ),
		mKernel		( Kernel )
	{
		Write( Array, ReadWriteMode );
	}

	template<typename ARRAYTYPE>
	bool		Write(const ArrayBridgeDef<ARRAYTYPE>& Array,cl_int ReadWriteMode)
	{
		assert( mBuffer == NULL );
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(ARRAYTYPE::TYPE) > 8 )
		{
			int Remainder = sizeof(ARRAYTYPE::TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<ARRAYTYPE::TYPE>() << " not aligned to 16 bytes; " << sizeof(ARRAYTYPE::TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}

		if ( !mKernel.IsValid() )
			return false;

		//	gr: only write data if we're NOT write-only memory
		auto* pArrayData = Array.GetArray();
		void* pData = (ReadWriteMode&CL_MEM_WRITE_ONLY) ? NULL : const_cast<ARRAYTYPE::TYPE*>(pArrayData);
		mBuffer = mManager.mOpencl.createBuffer( Array.GetDataSize(), ReadWriteMode, pData, SoyOpenClManager::OPENCL_DATA_BLOCKING_WRITE, mKernel.GetQueue() );
		return (mBuffer!=NULL);
	}
	template<typename TYPE>				bool	Write(const Array<TYPE>& Array,cl_int ReadWriteMode)			{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,int SIZE>	bool	Write(const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE>				bool	Write(const RemoteArray<TYPE>& Array,cl_int ReadWriteMode)		{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,class SORT>	bool	Write(const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array.mArray ), ReadWriteMode );	}
	
	template<typename TYPE>
	bool		Write(const TYPE& Data,cl_int ReadWriteMode)
	{
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(TYPE) > 8 )
		{
			int Remainder = sizeof(TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<TYPE>() << " not aligned to 16 bytes; " << sizeof(TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}

		if ( !mKernel.IsValid() )
			return false;

		assert( mBuffer == NULL );
		void* pData = (ReadWriteMode&CL_MEM_WRITE_ONLY) ? NULL : &const_cast<TYPE&>(Data);
		mBuffer = mManager.mOpencl.createBuffer( sizeof(TYPE), ReadWriteMode, pData, SoyOpenClManager::OPENCL_DATA_BLOCKING_WRITE, mKernel.GetQueue() );
		return (mBuffer!=NULL);
	}

	~SoyClDataBuffer()
	{
		if ( mBuffer )
		{
			mManager.mOpencl.deleteBuffer( *mBuffer );
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
		if ( !mKernel.IsValid() )
			return false;
		return mBuffer->read( Array.GetArray(), 0, Array.GetDataSize(), SoyOpenClManager::OPENCL_DATA_BLOCKING_READ, mKernel.GetQueue() );
	}

	template<typename TYPE>				bool	Read(Array<TYPE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE,int SIZE>	bool	Read(BufferArray<TYPE,SIZE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE>				bool	Read(RemoteArray<TYPE>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array ), ElementCount );	}
	template<typename TYPE,class SORT>	bool	Read(SortArray<TYPE,SORT>& Array,int ElementCount=-1)	{	return Read( GetArrayBridge( Array.mArray ), ElementCount );	}
		
	template<typename TYPE>
	bool		Read(TYPE& Data)
	{
		if ( !mBuffer )
			return false;
		if ( !mKernel.IsValid() )
			return false;
		return mBuffer->read( &Data, 0, sizeof(TYPE), SoyOpenClManager::OPENCL_DATA_BLOCKING_READ, mKernel.GetQueue() );
	}

	bool		IsValid() const		{	return mBuffer;	}
	operator	bool()				{	return IsValid();	}
	cl_mem&		getCLMem()			{	static cl_mem Dummy;	assert( mBuffer );	return mBuffer ? mBuffer->getCLMem() : Dummy;	}

public:
	msa::OpenCLBuffer*	mBuffer;
	SoyOpenClManager&	mManager;
	SoyOpenClKernel&	mKernel;
};

