#include "ofxSoylent.h"
#include "SoyOpenCl.h"
#include "SoyApp.h"




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



SoyOpenClManager::SoyOpenClManager(prmem::Heap& Heap) :
	SoyThread	( "SoyOpenClManager" ),
	mHeap		( Heap ),
	mShaders	( mHeap )
{
	//	init opencl
	mOpencl.setup( CL_DEVICE_TYPE_ALL, 10 );

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
	auto& Context = mOpencl.getContext();
	if ( Context == NULL )
		return false;
	return true;
}

SoyOpenClKernel* SoyOpenClManager::GetKernel(SoyOpenClKernelRef KernelRef)
{
	auto* pShader = GetShader( KernelRef.mShader );
	if ( !pShader )
		return NULL;

	return pShader->GetKernel( KernelRef.mKernel );
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

	//	delete kernel programs, but keep entries so we have the function names to reload
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = *mKernels[k];

		Kernel.DeleteKernel();

	}

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

	//	reload kernels
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = mKernels[k];
		GetKernel( Kernel->GetName() );
	}

	return true;
}

SoyOpenClKernel* SoyOpenClShader::FindKernel(const char* Name)
{
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = *mKernels[k];
		if ( Kernel == Name )
			return &Kernel;
	}
	return NULL;
}

SoyOpenClKernel* SoyOpenClShader::GetKernel(const char* Name)
{
	ofMutex::ScopedLock Lock(mLock);

	//	get entry (create if it doesnt exist)
	auto* pKernel = FindKernel( Name );
	if ( !pKernel )
	{
		pKernel = new SoyOpenClKernel( Name, mManager );
		mKernels.PushBack( pKernel );
	}

	//	program exists/loaded
	if ( pKernel->mKernel )
		return pKernel;

	//	try to load
	//	gr: check the program, the MSAopencl implementation allows NULL 
	if ( mProgram )
		pKernel->mKernel = mManager.mOpencl.loadKernel( pKernel->GetName(), mProgram );
	
	return pKernel;
}







SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,const char* BuildOptions) :
	mManager	( Manager ),
	mHeap		( mManager.GetHeap() )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader, BuildOptions );
	if ( pShader )
		mKernelRef.mShader = pShader->GetRef();

	mKernelRef.mKernel = Kernel;
}


void SoyOpenClKernel::DeleteKernel()
{
	//	lock so we dont delete mid-use
	ofMutex::ScopedLock Lock( mArgLock );

	if ( !mKernel )
		return;

	delete mKernel;
	mKernel = NULL;
}

bool SoyOpenClKernel::Begin()
{
	if ( !IsValid() )
		return false;

	//	lock args
	mArgLock.lock();

	return true;
}

bool SoyOpenClKernel::End1D(int Exec1)
{
	//	todo: add begin/end stack check
	//	args have been set now
	mArgLock.unlock();

	if ( mFirstExecution )
	{
		BufferString<1000> Debug;
		Debug << mName << " first execution...";
		ofLogNotice( Debug.c_str() );
		mFirstExecution = false;
	}

	//	execute
	mKernel->run1D( Exec1 );

	//	add a timeout in some way to this? to "detect" video driver crashes
	mManager.mOpencl.finish();

	return true;
}

bool SoyOpenClKernel::End2D(int Exec1,int Exec2)
{
	//	todo: add begin/end stack check
	//	args have been set now
	mArgLock.unlock();

	if ( mFirstExecution )
	{
		BufferString<1000> Debug;
		Debug << mName << " first execution...";
		ofLogNotice( Debug.c_str() );
		mFirstExecution = false;
	}

	//	execute
	mKernel->run2D( Exec1, Exec2 );

	//	gr: not actually neccessary
	//	add a timeout in some way to this? to "detect" video driver crashes
	mManager.mOpencl.finish();

	return true;
}

bool SoyOpenClKernel::End3D(int Exec1,int Exec2,int Exec3)
{
	//	todo: add begin/end stack check
	//	args have been set now
	mArgLock.unlock();

	if ( mFirstExecution )
	{
		BufferString<1000> Debug;
		Debug << mName << " first execution...";
		ofLogNotice( Debug.c_str() );
		mFirstExecution = false;
	}

	//	execute
	mKernel->run3D( Exec1, Exec2, Exec3 );

	//	gr: not actually neccessary
	//	add a timeout in some way to this? to "detect" video driver crashes
	mManager.mOpencl.finish();

	return true;
}

