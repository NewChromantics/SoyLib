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

			//	reload shader
			Shader.LoadShader();
		}

		sleep(1000);
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


SoyOpenClShader* SoyOpenClManager::LoadShader(const char* Filename)
{
	if ( !IsValid() )
		return NULL;

	//	see if it already exists
	auto* pShader = GetShader( Filename );

	//	make new one if it doesnt exist
	if ( !pShader )
	{
		//	generate a ref
		BufferString<100> BaseFilename = ofFilePath::getFileName(Filename).c_str();
		SoyRef ShaderRef = GetUniqueRef( SoyRef(BaseFilename) );
		if ( !ShaderRef.IsValid() )
			return NULL;

		//	make new shader
		ofMutex::ScopedLock Lock( mShaderLock );
		pShader = mHeap.Alloc<SoyOpenClShader>( ShaderRef, Filename, *this );
		if ( !pShader )
			return NULL;

		mShaders.PushBack( pShader );
	}

	//	load (in case it needs it)
	if ( pShader->HasChanged() )
		pShader->LoadShader();

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
		if ( Shader.mFilename.StartsWith(Filename,false) )
			return &Shader;
	}
	return NULL;
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
	mProgram = mManager.mOpencl.loadProgramFromFile( mFilename.c_str() );
	if ( !mProgram )
		return false;
	SetLastModified( CurrentTimestamp );

	//	reload kernels
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = mKernels[k];
		GetKernel( Kernel->mName );
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
		pKernel = new SoyOpenClKernel( Name );
		mKernels.PushBack( pKernel );
	}

	//	program exists/loaded
	if ( pKernel->mKernel )
		return pKernel;

	//	try to load
	//	gr: check the program, the MSAopencl implementation allows NULL 
	if ( mProgram )
		pKernel->mKernel = mManager.mOpencl.loadKernel( pKernel->mName.c_str(), mProgram );
	
	return pKernel;
}







SoyClShaderRunner::SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager) :
	mManager	( Manager )
{
	//	load shader
	auto* pShader = mManager.LoadShader( Shader );
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
