#include "SoyOpenCl.h"
//#include "SoyApp.h"
#include <SoyDebug.h>
#include <SoyString.h>




/*

//	default settings
bool SoyOpenCl::DefaultReadBlocking = true;
bool SoyOpenCl::DefaultWriteBlocking = false;
bool SoyOpenCl::DefaultExecuteBlocking = true;
msa::OpenClDevice::Type SoyOpenCl::DefaultDeviceType = msa::OpenClDevice::Any;



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



SoyFileChangeDetector::SoyFileChangeDetector(std::string Filename) :
	mFile			( Filename )
{
}


bool SoyFileChangeDetector::HasChanged()
{
	auto CurrentTimestamp = GetCurrentTimestamp();
	return mLastModified != CurrentTimestamp;
}

void SoyFileChangeDetector::SetLastModified(SoyFilesystem::Timestamp Timestamp)
{
	mLastModified = Timestamp;
}



SoyOpenClManager::SoyOpenClManager(std::string PlatformName,prmem::Heap& Heap) :
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
		//	setup some shaders for pre-loading
		{
			ofMutex::ScopedLock PreLoadLock( mPreloadShaders );
			while ( mPreloadShaders.GetSize() )
			{
				auto FilenameAndBuildOptions = mPreloadShaders.PopBack();

				//	create a simple shader entry which "needs reloading"
				AllocShader( FilenameAndBuildOptions.mFirst.c_str(), FilenameAndBuildOptions.mSecond.c_str() );
			}
		}

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

		Sleep(1);
		
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

SoyOpenClShader* SoyOpenClManager::AllocShader(std::string Filename,std::string BuildOptions)
{
	if ( !IsValid() )
		return nullptr;

	ofMutex::ScopedLock Lock( mShaderLock );

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
			return nullptr;

		//	make new shader
		pShader = mHeap.Alloc<SoyOpenClShader>( ShaderRef, Filename, BuildOptions, *this );
		if ( !pShader )
			return nullptr;

		mShaders.PushBack( pShader );
	}

	return pShader;
}

void SoyOpenClManager::PreLoadShader(std::string Filename,std::string BuildOptions)
{
	ofMutex::ScopedLock Lock( mPreloadShaders );

	auto& FilenameAndBuildOptions = mPreloadShaders.PushBack();
	FilenameAndBuildOptions.mFirst = Filename;
	FilenameAndBuildOptions.mSecond = BuildOptions;

	//	gr: could just AllocShader() here??
}

SoyOpenClShader* SoyOpenClManager::LoadShader(std::string Filename,std::string BuildOptions)
{
	auto* pShader = AllocShader( Filename, BuildOptions );
	if ( !pShader )
		return nullptr;

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


SoyOpenClShader* SoyOpenClManager::GetShader(std::string Filename)
{
	ofMutex::ScopedLock Lock( mShaderLock );
	for ( int s=0;	s<mShaders.GetSize();	s++ )
	{
		auto& Shader = *mShaders[s];
		if ( Soy::StringBeginsWith( Shader.GetFilename(), Filename, false ) )
			return &Shader;
	}
	return NULL;
}

cl_command_queue SoyOpenClManager::GetQueueForThread(msa::OpenClDevice::Type DeviceType)
{
	auto CurrentThreadId = SoyThread::GetCurrentThreadId();
	return GetQueueForThread( CurrentThreadId, DeviceType );
}

cl_command_queue SoyOpenClManager::GetQueueForThread(std::thread::id ThreadId,msa::OpenClDevice::Type DeviceType)
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

		std::Debug << "Created opencl queue " << (ptrdiff_t)(MatchQueue.mQueue) << " for thread id " << std::endl;
	
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

	auto CurrentTimestamp = GetCurrentTimestamp();

	//	file gone missing
	if ( !CurrentTimestamp.IsValid() )
		return false;

	//	let this continue if we have build errors? so it doesnt keep trying to reload...
	mProgram = mManager.mOpencl.loadProgramFromFile( GetFilename(), false, GetBuildOptions() );
	if ( !mProgram )
		return false;
	SetLastModified( CurrentTimestamp );

	//	gr: todo: mark kernels to reload

	return true;
}


SoyOpenClKernel* SoyOpenClShader::GetKernel(std::string Name,cl_command_queue Queue)
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


SoyOpenClKernel::SoyOpenClKernel(std::string Name,SoyOpenClShader& Parent) :
	mName				( Name ),
	mKernel				( NULL ),
	mShader				( Parent ),
	mManager			( Parent.mManager ),
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

bool SoyOpenClKernel::IsValidLocalExecCount(ArrayBridge<int>& LocalExec,bool ForceCorrection)
{
	if ( LocalExec.IsEmpty() )
		return true;
	
	std::stringstream Debug;

	//	check against device max's
	//	gr: don't think i need to check mDevice.maxWorkGroupSize as it's implied by the individual limits...
	bool Error = false;
	for ( int d=0;	d<LocalExec.GetSize();	d++ )
	{
		int MaxSize = mDeviceInfo.maxWorkItemSizes[d];
		int Size = LocalExec[d];
		
		Debug << Size << "/" << MaxSize << " ";
		
		if ( Size > MaxSize )
		{
			Error = true;
		
			if ( ForceCorrection )
				LocalExec[d] = MaxSize;
		}
	}
	
	if ( Error )
	{
		std::Debug << GetName() << ": too many local iterations for kernel; " << Debug.str();
		if ( ForceCorrection )
			std::Debug << " (corrected)";
		std::Debug << std::endl;
		return ForceCorrection;
	}
	
	return true;
}

bool SoyOpenClKernel::End(bool Blocking,const ArrayBridge<int>& OrigGlobalExec,const ArrayBridge<int>& OrigLocalExec)
{
	BufferArray<int,3> GlobalExec( OrigGlobalExec );
	BufferArray<int,6> LocalExec( OrigLocalExec );
	assert( !GlobalExec.IsEmpty() );
	if ( GlobalExec.IsEmpty() )
		return true;

	for ( int i=0;	i<GlobalExec.GetSize();	i++ )
	{
		auto& ExecCount = GlobalExec[i];

		//	dimensions need to be at least 1, zero size is not a failure, just don't execute
		if ( ExecCount <= 0 )
			return true;

		if ( IsValidGlobalExecCount(ExecCount) )
			continue;

		BufferString<1000> Debug;
		Debug << GetName() << ": Too many iterations for kernel: " << ExecCount << "/" << GetMaxWorkGroupSize() << "... execution count truncated.";
		ofLogWarning( Debug.c_str() );
		ExecCount = std::min( ExecCount, GetMaxGlobalWorkGroupSize() );
	}

	//	local size must match global size if specified
	if ( !LocalExec.IsEmpty() && LocalExec.GetSize() != GlobalExec.GetSize() )
	{
		assert( false );
		return false;
	}

	auto LocalExecBridge = GetArrayBridge(LocalExec);
	if ( !IsValidLocalExecCount( LocalExecBridge, true ) )
		return false;


	//	check 		CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and
	//			the total number of work-items in the work-group computed as
	//			local_work_size[0] *... local_work_size[work_dim - 1] is greater than
	//			the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.
	if ( !LocalExec.IsEmpty() )
	{
		int TotalLocalWorkGroupSize = LocalExec[0];
		for ( int d=1;	d<LocalExec.GetSize();	d++ )
			TotalLocalWorkGroupSize *= LocalExec[d];

		if ( TotalLocalWorkGroupSize > GetMaxLocalWorkGroupSize() )
		{
			std::Debug << "Too many local work items for device: " << TotalLocalWorkGroupSize << "/" << GetMaxLocalWorkGroupSize() << std::endl;
			return false;
		}
	}

	//	CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of work-items specified by
	//	global_work_size is not evenly divisable by size of work-group given by local_work_size
	if ( !LocalExec.IsEmpty() )
	{
		bool Error = false;
		for ( int Dim=0;	Dim<LocalExec.GetSize();	Dim++ )
		{
			int Local = LocalExec[Dim];
			int Global = GlobalExec[Dim];
			int Remainer = Global % Local;
			if ( Remainer != 0 )
			{
				std::Debug << "Local work items[" << Dim << "] " << Local << " not divisible by global size " << Global << std::endl;
				Error = true;
			}
		}
		if ( Error )
			return false;
	}
	

	//	if we're about to execute, make sure all writes are done
	//	gr: only if ( Blocking ) ?
	if ( !WaitForPendingWrites() )
		return false;

	//	pad out local exec in case it's not specified
	LocalExec.PushBack(0);
	LocalExec.PushBack(0);
	LocalExec.PushBack(0);

	//	execute
	if ( GlobalExec.GetSize() == 3 )
	{
		if ( !mKernel->run3D( Blocking, GlobalExec[0], GlobalExec[1], GlobalExec[2], LocalExec[0], LocalExec[1], LocalExec[2] ) )
			return false;
	}
	else if ( GlobalExec.GetSize() == 2 )
	{
		if ( !mKernel->run2D( Blocking, GlobalExec[0], GlobalExec[1], LocalExec[0], LocalExec[1] ) )
			return false;
	}
	else if ( GlobalExec.GetSize() == 1 )
	{
		if ( !mKernel->run1D( Blocking, GlobalExec[0], LocalExec[0] ) )
			return false;
	}

	return true;
}

void SoyOpenClKernel::OnUnloaded()
{
	mDeviceInfo = msa::clDeviceInfo();
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
	
	mDeviceInfo = Device->mInfo;
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
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = Exec1;

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;

	for ( int ita=0;	ita<Iterationsa;	ita++ )
	{
		SoyOpenclKernelIteration<1> Iteration;
		Iteration.mFirst[0] = ita * KernelWorkGroupMax;
		Iteration.mCount[0] = ofMin( KernelWorkGroupMax, Exec1 - Iteration.mFirst[0] );
		Iteration.mBlocking = BlockLast && (ita==Iterationsa-1);
		assert( Iteration.mCount[0] > 0 );
		Iterations.PushBack( Iteration );
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = ofMax( Exec1, Exec2 );

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	bool Overflowb = (Exec2 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;
	int Iterationsb = (Exec2 / KernelWorkGroupMax) + Overflowb;

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
			assert( Iteration.mCount[0] > 0 );
			assert( Iteration.mCount[1] > 0 );
			Iterations.PushBack( Iteration );
		}
	}
}

void SoyOpenClKernel::GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast)
{
	int KernelWorkGroupMax = GetMaxWorkGroupSize();
	if ( KernelWorkGroupMax < 1 )
		KernelWorkGroupMax = ofMax( Exec1, ofMax( Exec2, Exec3 ) );

	bool Overflowa = (Exec1 % KernelWorkGroupMax) > 0;
	bool Overflowb = (Exec2 % KernelWorkGroupMax) > 0;
	bool Overflowc = (Exec3 % KernelWorkGroupMax) > 0;
	int Iterationsa = (Exec1 / KernelWorkGroupMax) + Overflowa;
	int Iterationsb = (Exec2 / KernelWorkGroupMax) + Overflowb;
	int Iterationsc = (Exec3 / KernelWorkGroupMax) + Overflowc;

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
				assert( Iteration.mCount[0] > 0 );
				assert( Iteration.mCount[1] > 0 );
				assert( Iteration.mCount[2] > 0 );
				Iterations.PushBack( Iteration );
			}
		}
	}
}


int SoyOpenClKernel::GetMaxGlobalWorkGroupSize() const		
{
	int DeviceAddressBits = mDeviceInfo.deviceAddressBits;
	int MaxSize = (1<<(DeviceAddressBits-1))-1;	
	//	gr: if this is negative opencl kernels lock up (or massive loops?), code should bail out beforehand
	assert( MaxSize > 0 );
	return MaxSize;
}

int SoyOpenClKernel::GetMaxLocalWorkGroupSize() const		
{
	return mDeviceInfo.maxWorkGroupSize;	
}

*/
