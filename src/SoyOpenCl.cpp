#include "ofxSoylent.h"
#include "SoyOpenCl.h"
//#include "SoyApp.h"

//	default settings
bool SoyOpenCl::DefaultReadBlocking = true;
bool SoyOpenCl::DefaultWriteBlocking = false;
bool SoyOpenCl::DefaultExecuteBlocking = true;
msa::OpenClDevice::Type SoyOpenCl::DefaultDeviceType = msa::OpenClDevice::GPU;



class TSortPolicy_SoyThreadQueue
{
public:
	//template<typename TYPEB>
	static int		Compare(const SoyThreadQueue& a,const SoyThreadQueue& b)
	{
		if ( a.mThreadId < b.mThreadId )		return -1;
		if ( a.mThreadId > b.mThreadId )		return 1;
		if ( a.mDeviceType < b.mDeviceType )	return -1;
		if ( a.mDeviceType > b.mDeviceType )	return 1;
		return 0;
	}
};



SoyFileChangeDetector::SoyFileChangeDetector(const char* Filename) :
	mFilename		( Filename ),
	mLastModified	( 0 )
{
}


bool SoyFileChangeDetector::HasChanged()
{
	Poco::Timestamp CurrentTimestamp = GetCurrentTimestamp();
	return mLastModified != CurrentTimestamp;
}

Poco::Timestamp SoyFileChangeDetector::GetCurrentTimestamp()
{
	return ofFileLastModified( mFilename );
}

void SoyFileChangeDetector::SetLastModified(Poco::Timestamp Timestamp)
{
	mLastModified = Timestamp;
}



SoyOpenClManager::SoyOpenClManager(const char* PlatformName,prmem::Heap& Heap) :
	SoyThread	( "SoyOpenClManager" ),
	mHeap		( Heap ),
	mShaders	( mHeap )
{
	if ( !mOpencl.setup(PlatformName) )
	{
		assert( false );
		ofLogError("Failed to initialise opencl");
	}

	startThread( true, false );
}

SoyOpenClManager::~SoyOpenClManager()
{
	waitForThread();
}

		
void SoyOpenClManager::threadedFunction()
{
	while ( isThreadRunning() )
	{
		//	check if any shader's files have changed
		Array<SoyOpenClShader*> Shaders;
		mShaderLock.lock();
		Shaders = mShaders;
		mShaderLock.unlock();
		
		for ( int i=0;	i<Shaders.GetSize();	i++ )
		{
			auto& Shader = *Shaders[i];

			if ( !Shader.HasChanged() )
				continue;
			
			if ( Shader.IsLoading() )
				continue;

			//	reload shader
			if ( Shader.LoadShader() )
			{
				auto* pShader = &Shader;
				ofNotifyEvent( mOnShaderLoaded, pShader );
			}
		}

		sleep(2000);
	}
}


bool SoyOpenClManager::IsValid()
{
	auto Context = mOpencl.getContext();
	if ( Context == NULL )
		return false;
	return true;
}

SoyOpenClKernel* SoyOpenClManager::GetKernel(SoyOpenClKernelRef KernelRef,cl_command_queue Queue)
{
	auto* pShader = GetShader( KernelRef.mShader );
	if ( !pShader )
		return NULL;

	return pShader->GetKernel( KernelRef.mKernel, Queue );
}

void SoyOpenClManager::DeleteKernel(SoyOpenClKernel* Kernel)
{
	if ( !Kernel )
		return;
	
	delete Kernel;
}


SoyOpenClShader* SoyOpenClManager::LoadShader(const char* Filename,const char* BuildOptions)
{
	if ( !IsValid() )
		return NULL;

	//	see if it already exists
	auto* pShader = GetShader( Filename );

	//	make new one if it doesnt exist
	bool IsNewShader = (!pShader);
	if ( !pShader )
	{
		//	generate a ref
		BufferString<100> BaseFilename = ofFilePath::getFileName(Filename).c_str();
		SoyRef ShaderRef = GetUniqueRef( SoyRef(BaseFilename) );
		if ( !ShaderRef.IsValid() )
			return NULL;

		//	make new shader
		ofMutex::ScopedLock Lock( mShaderLock );
		pShader = mHeap.Alloc<SoyOpenClShader>( ShaderRef, Filename, BuildOptions, *this );
		if ( !pShader )
			return NULL;

		mShaders.PushBack( pShader );
	}

	//	load (in case it needs it)
	if ( pShader->HasChanged() && !pShader->IsLoading() )
	{
		if ( pShader->LoadShader() )
		{
			ofNotifyEvent( mOnShaderLoaded, pShader );
		}
	}

	return pShader;
}

SoyRef SoyOpenClManager::GetUniqueRef(SoyRef BaseRef)
{
	ofMutex::ScopedLock Lock( mShaderLock );

	//	check each shader for dupe ref
	int s = 0;
	while ( s < mShaders.GetSize() )
	{
		auto& Shader = *mShaders[s];

		//	ref used, try next and re-start the search
		if ( Shader == BaseRef )
		{
			BaseRef.Increment();
			s = 0;
			continue;
		}

		//	try next
		s++;
	}
	return BaseRef;
}

SoyOpenClShader* SoyOpenClManager::GetShader(SoyRef ShaderRef)
{
	ofMutex::ScopedLock Lock( mShaderLock );
	for ( int s=0;	s<mShaders.GetSize();	s++ )
	{
		auto& Shader = *mShaders[s];
		if ( Shader == ShaderRef )
			return &Shader;
	}
	return NULL;
}


SoyOpenClShader* SoyOpenClManager::GetShader(const char* Filename)
{
	ofMutex::ScopedLock Lock( mShaderLock );
	for ( int s=0;	s<mShaders.GetSize();	s++ )
	{
		auto& Shader = *mShaders[s];
		if ( Shader.GetFilename().StartsWith(Filename,false) )
			return &Shader;
	}
	return NULL;
}

cl_command_queue SoyOpenClManager::GetQueueForThread(msa::OpenClDevice::Type DeviceType)
{
	auto CurrentThreadId = SoyThread::GetCurrentThreadId();
	return GetQueueForThread( CurrentThreadId, DeviceType );
}

cl_command_queue SoyOpenClManager::GetQueueForThread(SoyThreadId ThreadId,msa::OpenClDevice::Type DeviceType)
{
	ofMutex::ScopedLock Lock( mThreadQueues );

	//	look for matching queue meta
	SoyThreadQueue MatchQueue;
	MatchQueue.mDeviceType = DeviceType;
	MatchQueue.mThreadId = ThreadId;
	auto ThreadQueues = GetSortArray( mThreadQueues, TSortPolicy_SoyThreadQueue() );
	auto* pQueue = ThreadQueues.Find( MatchQueue );

	//	create new entry
	if ( !pQueue )
	{
		MatchQueue.mQueue = mOpencl.createQueue( DeviceType );

		BufferString<100> Debug;
		Debug << "Created opencl queue " << PtrToInt(MatchQueue.mQueue) << " for thread id ";
		ofLogNotice( Debug.c_str() );

		if ( !MatchQueue.mQueue )
			return NULL;
		
		pQueue = &ThreadQueues.Push( MatchQueue );
	}
	return pQueue->mQueue;
}


bool SoyOpenClShader::IsLoading()
{
	//	if currently locked then it's loading
	if ( !mLock.tryLock() )
		return true;

	mLock.unlock();
	return false;
}

void SoyOpenClShader::UnloadShader()
{
	ofMutex::ScopedLock Lock(mLock);

	//	delete program
	if ( mProgram )
	{
		delete mProgram;
		mProgram = NULL;
	}
}


bool SoyOpenClShader::LoadShader()
{
	ofMutex::ScopedLock Lock(mLock);
	UnloadShader();

	//	
	Poco::Timestamp CurrentTimestamp = GetCurrentTimestamp();

	//	file gone missing
	if ( CurrentTimestamp == Poco::Timestamp(0) )
		return false;

	//	let this continue if we have build errors? so it doesnt keep trying to reload...
	mProgram = mManager.mOpencl.loadProgramFromFile( GetFilename().c_str(), false, GetBuildOptions() );
	if ( !mProgram )
		return false;
	SetLastModified( CurrentTimestamp );

	//	gr: todo: mark kernels to reload

	return true;
}


SoyOpenClKernel* SoyOpenClShader::GetKernel(const char* Name,cl_command_queue Queue)
{
	ofMutex::ScopedLock Lock(mLock);
	ofScopeTimerWarning Warning( BufferString<1000>()<<__FUNCTION__<<" "<<Name, 3 );
	SoyOpenClKernel* pKernel = new SoyOpenClKernel( Name, *this );
	if ( !pKernel )
		return NULL;

	//	try to load
	if ( mProgram )
	{
		pKernel->mKernel = mManager.mOpencl.loadKernel( pKernel->GetName(), *mProgram, Queue );
		pKernel->OnLoaded();
	}
	
	return pKernel;
}







SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,bool UseThreadQueue,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device,const char* BuildOptions) :
	mManager		( Manager ),
	mKernel			( NULL ),
	mRequestDevice	( Device ),
	mHeap			( mManager.GetHeap() ),
	mUseThreadQueue	( UseThreadQueue )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader, BuildOptions );
	if ( pShader )
		mKernelRef.mShader = pShader->GetRef();

	mKernelRef.mKernel = Kernel;
}

SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device,const char* BuildOptions) :
	mManager		( Manager ),
	mKernel			( NULL ),
	mRequestDevice	( Device ),
	mHeap			( mManager.GetHeap() ),
	mUseThreadQueue	( true )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader, BuildOptions );
	if ( pShader )
		mKernelRef.mShader = pShader->GetRef();

	mKernelRef.mKernel = Kernel;
}


SoyOpenClKernel* SoyClShaderRunner::GetKernel()
{
	if ( !mKernel )
	{
		cl_command_queue Queue = mUseThreadQueue ? mManager.GetQueueForThread( mRequestDevice ) : NULL;
		mKernel = mManager.GetKernel( mKernelRef, Queue );
	}
	return mKernel;
}


SoyOpenClKernel::SoyOpenClKernel(const char* Name,SoyOpenClShader& Parent) :
	mName				( Name ),
	mKernel				( NULL ),
	mShader				( Parent ),
	mManager			( Parent.mManager ),
	mMaxWorkGroupSize	( -1 ),
	mDevice				( msa::OpenClDevice::Invalid ),
	mKernelRef			( Parent.GetRef(), Name )
{
}
	
msa::OpenCL& SoyOpenClKernel::GetOpenCL() const
{
	return mManager.mOpencl;
}

void SoyOpenClKernel::DeleteKernel()
{
	if ( !mKernel )
		return;

	mManager.mOpencl.deleteKernel( *mKernel );
	mKernel = NULL;
}

void SoyOpenClKernel::OnBufferPendingWrite(cl_event PendingWriteEvent)
{
	assert( PendingWriteEvent != NULL );
	mPendingBufferWrites.PushBack( PendingWriteEvent );
}
	
bool SoyOpenClKernel::WaitForPendingWrites()
{
	if ( !mKernel )
		return false;

	static bool UseEvents = true;
	if ( UseEvents )
	{
		if ( mPendingBufferWrites.IsEmpty() )
			return true;
		auto Err = clWaitForEvents( mPendingBufferWrites.GetSize(), mPendingBufferWrites.GetArray() );
		return (Err == CL_SUCCESS);
	}
	else
	{
		mPendingBufferWrites.Clear();
		auto Err = clFinish( mKernel->getQueue() );
		return (Err == CL_SUCCESS);
	}
}


bool SoyOpenClKernel::Begin()
{
	if ( !IsValid() )
		return false;

	return true;
}

bool SoyOpenClKernel::End1D(int Exec1)
{
	bool Blocking = SoyOpenCl::DefaultExecuteBlocking;
	return End1D( Blocking, Exec1 );
}

bool SoyOpenClKernel::End2D(int Exec1,int Exec2)
{
	bool Blocking = SoyOpenCl::DefaultExecuteBlocking;
	return End2D( Blocking, Exec1, Exec2 );
}

bool SoyOpenClKernel::End3D(int Exec1,int Exec2,int Exec3)
{
	bool Blocking = SoyOpenCl::DefaultExecuteBlocking;
	return End3D( Blocking, Exec1, Exec2, Exec3 );
}

bool SoyOpenClKernel::End1D(bool Blocking,int Exec1)
{
	if ( !IsValidExecCount(Exec1) )
	{
		BufferString<1000> Debug;
		Debug << GetName() << ": Too many iterations for kernel: " << Exec1 << "/" << mMaxWorkGroupSize << "... execution count truncated.";
		ofLogWarning( Debug.c_str() );
		Exec1 = ofMin( Exec1, mMaxWorkGroupSize );
	}

	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	if ( !WaitForPendingWrites() )
		return false;

	//	execute
	if ( !mKernel->run1D( Blocking, Exec1 ) )
		return false;

	return true;
}

bool SoyOpenClKernel::End2D(bool Blocking,int Exec1,int Exec2)
{
	if ( !IsValidExecCount(Exec1) || !IsValidExecCount(Exec2) )
	{
		BufferString<1000> Debug;
 		Debug << GetName() << ": Too many iterations for kernel: " << Exec1 << "," << Exec2 << "/" << mMaxWorkGroupSize << "... execution count truncated.";
		ofLogWarning( Debug.c_str() );
		Exec1 = ofMin( Exec1, mMaxWorkGroupSize );
		Exec2 = ofMin( Exec2, mMaxWorkGroupSize );
	}

	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	if ( !WaitForPendingWrites() )
		return false;

	//	execute
	if ( !mKernel->run2D( Blocking, Exec1, Exec2 ) )
		return false;

	return true;
}

bool SoyOpenClKernel::End3D(bool Blocking,int Exec1,int Exec2,int Exec3)
{
	if ( !IsValidExecCount(Exec1) || !IsValidExecCount(Exec2) || !IsValidExecCount(Exec3) )
	{
		BufferString<1000> Debug;
		Debug << GetName() << ": Too many iterations for kernel: " << Exec1 << "," << Exec2 << "," << Exec3 << "/" << mMaxWorkGroupSize << "... execution count truncated.";
		ofLogWarning( Debug.c_str() );
		Exec1 = ofMin( Exec1, mMaxWorkGroupSize );
		Exec2 = ofMin( Exec2, mMaxWorkGroupSize );
		Exec3 = ofMin( Exec3, mMaxWorkGroupSize );
	}

	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	if ( !WaitForPendingWrites() )
		return false;

	//	execute
	if ( !mKernel->run3D( Blocking, Exec1, Exec2, Exec3 ) )
		return false;

	return true;
}

void SoyOpenClKernel::OnUnloaded()
{
	mMaxWorkGroupSize = -1;
	mDevice = msa::OpenClDevice::Invalid;
}

void SoyOpenClKernel::OnLoaded()
{
	//	failed to load (syntax error etc)
	if ( !mKernel )
	{
		OnUnloaded();
		return;
	}

	//	query for max work group items
	//	http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/clGetKernelWorkGroupInfo.html
	auto Queue = GetQueue();
	auto* Device = mManager.mOpencl.GetDevice( Queue );
	if ( !Device )
	{
		OnUnloaded();
		return;
	}
	
	mDevice = Device->GetType();
	mMaxWorkGroupSize = Device->mInfo.maxWorkGroupSize;
}



bool SoyOpenClKernel::CheckPaddingChecksum(const int* Padding,int Length)
{
#if !defined(PADDING_CHECKSUM_1)
	#define PADDING_CHECKSUM_1		123
	#define PADDING_CHECKSUM_2		456
	#define PADDING_CHECKSUM_3		789
#endif
	BufferArray<int,3> Checksums;
	Checksums.PushBack( PADDING_CHECKSUM_1 );
	Checksums.PushBack( PADDING_CHECKSUM_2 );
	Checksums.PushBack( PADDING_CHECKSUM_3 );

	for ( int i=0;	i<Length;	i++ )
	{
		int Pad = Padding[i];
		int Checksum = Checksums[i];
		assert( Pad == Checksum );
		if ( Pad != Checksum )
			return false;
	}
	return true;
}


void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<1>>& Iterations,int Exec1,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax == -1 )
		KernelWorkGroupMax = Exec1;
	int Iterationsa = (Exec1 / KernelWorkGroupMax)+1;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		SoyOpenclKernelIteration<1> Iteration;
		Iteration.mFirst[0] = ita * KernelWorkGroupMax;
		Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
		Iteration.mBlocking = BlockLast && (ita==Iterationsa-1);
		Iterations.PushBack( Iteration );
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax == -1 )
		KernelWorkGroupMax = ofMax( Exec1, Exec2 );
	int Iterationsa = (Exec1 / KernelWorkGroupMax)+1;
	int Iterationsb = (Exec2 / KernelWorkGroupMax)+1;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		for ( int itb=0;	itb<Iterationsb;	itb++ )
		{
			SoyOpenclKernelIteration<2> Iteration;
			Iteration.mFirst[0] = ita * KernelWorkGroupMax;
			Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
			Iteration.mFirst[1] = itb * KernelWorkGroupMax;
			Iteration.mCount[1] = ofMin( KernelWorkGroupMax, Exec2 - Iteration.mFirst[1] );
			Iteration.mBlocking = BlockLast && (ita==Iterationsa-1) && (itb==Iterationsb-1);
			Iterations.PushBack( Iteration );
		}
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax == -1 )
		KernelWorkGroupMax = ofMax( Exec1, ofMax( Exec2, Exec3 ) );
	int Iterationsa = (Exec1 / KernelWorkGroupMax)+1;
	int Iterationsb = (Exec2 / KernelWorkGroupMax)+1;
	int Iterationsc = (Exec3 / KernelWorkGroupMax)+1;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		for ( int itb=0;	itb<Iterationsb;	itb++ )
		{
			for ( int itc=0;	itc<Iterationsc;	itc++ )
			{
				SoyOpenclKernelIteration<3> Iteration;
				Iteration.mFirst[0] = ita * KernelWorkGroupMax;
				Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
				Iteration.mFirst[1] = itb * KernelWorkGroupMax;
				Iteration.mCount[1] = ofMin( KernelWorkGroupMax, Exec2 - Iteration.mFirst[1] );
				Iteration.mFirst[2] = itc * KernelWorkGroupMax;
				Iteration.mCount[2] = ofMin( KernelWorkGroupMax, Exec3 - Iteration.mFirst[2] );
				Iteration.mBlocking = BlockLast && (ita==Iterationsa-1) && (itb==Iterationsb-1) && (itc==Iterationsc-1);
				Iterations.PushBack( Iteration );
			}
		}
	}
}


