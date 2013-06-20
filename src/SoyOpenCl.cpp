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

msa::OpenCLKernel* SoyOpenClManager::GetKernel(SoyOpenClKernelRef KernelRef)
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

	//	delete kernels (keep references to the functions for restoring)
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = mKernels[k];
		auto*& pKernel = Kernel.mSecond;
		if( !pKernel )
			continue;

		delete pKernel;
		pKernel = NULL;
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
	auto* pProgram = mManager.mOpencl.loadProgramFromFile( mFilename.c_str() );
	if ( !pProgram )
		return false;
	SetLastModified( CurrentTimestamp );

	//	reload kernels
	for ( int k=0;	k<mKernels.GetSize();	k++ )
	{
		auto& Kernel = mKernels[k];
		GetKernel( Kernel.mFirst );
	}

	return true;
}


msa::OpenCLKernel* SoyOpenClShader::GetKernel(const char* Name)
{
	ofMutex::ScopedLock Lock(mLock);

	//	get entry (create if it doesnt exist)
	auto* pKernelPair = mKernels.Find( Name );
	if ( !pKernelPair )
	{
		pKernelPair = &mKernels.PushBack();
		pKernelPair->mFirst = Name;
		pKernelPair->mSecond = NULL;
	}

	auto*& pKernel = pKernelPair->mSecond;

	//	program exists/loaded
	if ( pKernel )
		return pKernel;

	//	try to load
	pKernel = mManager.mOpencl.loadKernel( Name, mProgram );
	
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

