#include "ofxSoylent.h"


SoyOpenClManager::SoyOpenClManager(prmem::Heap& Heap) :
	mHeap		( Heap ),
	mShaders	( mHeap )
{
	//	init opencl
	mOpencl.setup( CL_DEVICE_TYPE_ALL, 10 );
}

bool SoyOpenClManager::IsValid()
{
	auto& Context = mOpencl.getContext();
	if ( Context == NULL )
		return false;
	return true;
}

SoyRef SoyOpenClManager::LoadShader(const char* Filename)
{
	if ( !IsValid() )
		return SoyRef();

	//	generate a ref
	BufferString<100> BaseFilename = ofFilePath::getFileName(Filename).c_str();
	SoyRef ShaderRef = GetUniqueRef( SoyRef(BaseFilename) );
	if ( !ShaderRef.IsValid() )
		return SoyRef();

	auto* pProgram = mOpencl.loadProgramFromFile( Filename );
	if ( !pProgram )
		return SoyRef();

	//	make new shader
	ofMutex::ScopedLock Lock( mShaderLock );
	SoyOpenClShader* pShader = mHeap.Alloc<SoyOpenClShader>( ShaderRef, Filename, *pProgram, *this );
	if ( !pShader )
		return SoyRef();

	mShaders.PushBack( pShader );
	return pShader->GetRef();
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


//	load and return kernel
msa::OpenCLKernel* SoyOpenClShader::GetKernel(const char* Name)
{
	//	see if it already exists
	auto* pKernelPair = mKernels.Find( Name );
	if ( pKernelPair )
		return pKernelPair->mSecond;

	//	load
	auto* pKernel = mManager.mOpencl.loadKernel( Name, mProgram );
	if ( !pKernel )
		return NULL;

	//	store
	auto& KernelPair = mKernels.PushBack();
	KernelPair.mFirst = Name;
	assert( KernelPair == Name );
	KernelPair.mSecond = pKernel;
	return pKernel;
}




