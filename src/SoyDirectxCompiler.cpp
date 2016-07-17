#include "SoyDirectxCompiler.h"
#include "SoyRuntimeLibrary.h"


//	if compiling against win8 lib/runtime, then we can include the new d3d compiler lib directly
#if WINDOWS_TARGET_SDK >= 8
#include <d3dcompiler.h>
//	gr: do not link to d3dcompiler. This means it will not load the DLL and we load it manually (InitCompilerExtension)
//#pragma comment(lib, "D3DCompiler.lib")
#endif



//	we never use this, but for old DX11 support, we need the type name
class ID3DX11ThreadPump;

namespace Directx
{
	bool				IsOkay(HRESULT Error,const std::string& Context,bool ThrowException=true);
}

namespace DirectxCompiler
{

	//	gr: not sure why I can't do this (pD3DCompile is in the SDK header)
	//typedef pD3DCompile D3DCompileFunc;
	typedef HRESULT (__stdcall D3DCompileFunc)(LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);
	typedef void (__stdcall LPD3DX11COMPILEFROMMEMORY)(LPCSTR pSrcData, SIZE_T SrcDataLen, LPCSTR pFileName, const D3D10_SHADER_MACRO *pDefines, LPD3D10INCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, ID3DX11ThreadPump *pPump, ID3D10Blob **ppShader, ID3D10Blob **ppErrorMsgs, HRESULT *pHResult);
}


class DirectxCompiler::TCompilerImpl
{
public:
	std::function<D3DCompileFunc>	mD3dCompileFunc;
};



DirectxCompiler::TCompiler::TCompiler() :
	mImpl	( new TCompilerImpl )
{
	//	dynamically link compiler functions
	//	https://github.com/pathscale/nvidia_sdk_samples/blob/master/matrixMul/common/inc/dynlink_d3d11.h
	//	https://gist.github.com/rygorous/7936047
	auto& mD3dCompileFunc = mImpl->mD3dCompileFunc;

	class DllReference
	{
	public:
		DllReference()	{}
		DllReference(const char* Dll,const char* Func,const char* Desc) :
			LibraryName		( Dll ),
			FunctionName	( Func ),
			Description		( Desc )
		{
		};

		std::string	LibraryName;
		std::string	FunctionName;
		std::string	Description;
	};

	std::function<LPD3DX11COMPILEFROMMEMORY> CompileFromMemoryFunc;
	Array<DllReference> DllFunctions;

	//	https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
	//	remember, 47 & 46 are MUCH FASTER!
	DllFunctions.PushBack( DllReference("d3dcompiler_47.dll","D3DCompile","Win10/Win8.1 sdk") );
	DllFunctions.PushBack( DllReference("d3dcompiler_46.dll","D3DCompile","Win8.0 sdk") );

	DllFunctions.PushBack( DllReference("D3DX11d_43.dll","D3DX11CompileFromMemory","DXSDK 2010 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_42.dll","D3DX11CompileFromMemory","DXSDK 2010 Feb") );
	DllFunctions.PushBack( DllReference("D3DX11d_41.dll","D3DX11CompileFromMemory","DXSDK 2009 March") );
	DllFunctions.PushBack( DllReference("D3DX11d_40.dll","D3DX11CompileFromMemory","DXSDK 2008 November") );
	DllFunctions.PushBack( DllReference("D3DX11d_39.dll","D3DX11CompileFromMemory","DXSDK 2008 August") );
	DllFunctions.PushBack( DllReference("D3DX11d_38.dll","D3DX11CompileFromMemory","DXSDK 2008 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_37.dll","D3DX11CompileFromMemory","DXSDK 2008 March") );
	DllFunctions.PushBack( DllReference("D3DX11d_36.dll","D3DX11CompileFromMemory","DXSDK 2007 November") );
	DllFunctions.PushBack( DllReference("D3DX11d_35.dll","D3DX11CompileFromMemory","DXSDK 2007 August") );
	DllFunctions.PushBack( DllReference("D3DX11d_34.dll","D3DX11CompileFromMemory","DXSDK 2007 June") );
	DllFunctions.PushBack( DllReference("D3DX11d_33.dll","D3DX11CompileFromMemory","DXSDK 2007 Aprli") );

	for ( int d=0;	d<DllFunctions.GetSize();	d++ )
	{
		auto& Dll = DllFunctions[d];
		try
		{
			mCompileLib.reset( new Soy::TRuntimeLibrary(Dll.LibraryName) );

			if ( Dll.FunctionName == "D3DCompile" )
				mCompileLib->SetFunction( mD3dCompileFunc, Dll.FunctionName.c_str() );
			else
				mCompileLib->SetFunction( CompileFromMemoryFunc, Dll.FunctionName.c_str() );
			std::Debug << "Using DX compiler from " << Dll.Description << " (" << Dll.LibraryName << ")" << std::endl;
			break;
		}
		catch(std::exception& e)
		{
			std::Debug << "Failed to load " << Dll.FunctionName << " from " << Dll.LibraryName << " (" << Dll.Description << ")" << std::endl;

			mCompileLib.reset();
			mD3dCompileFunc = nullptr;
			CompileFromMemoryFunc = nullptr;
		}
	}

	//	assigned direct func
	if ( mD3dCompileFunc )
		return;

	//	need a wrapper to old func
	if ( !mD3dCompileFunc && CompileFromMemoryFunc )
	{
		std::function<D3DCompileFunc> WrapperFunc = [CompileFromMemoryFunc](LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
		{
			//	make wrapper!
			//	https://msdn.microsoft.com/en-us/library/windows/desktop/ff476262(v=vs.85).aspx
			ID3DX11ThreadPump* Pump = nullptr;
			const char* Source = reinterpret_cast<const char*>( pSrcData );
			auto* Profile = pTarget;
			HRESULT Result = S_FALSE;
			CompileFromMemoryFunc( Source, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, Profile, Flags1, Flags2, Pump, ppCode, ppErrorMsgs, &Result );
			return Result;
		};
		mD3dCompileFunc = WrapperFunc;
		return;
	}

	//	didn't find any func
	//	gr: should this throw here? or during compile...as current code does...
//	throw Soy::AssertException("Failed to load directx shader compiler");
	std::function<D3DCompileFunc> UnsupportedFunc = [](LPCVOID pSrcData,SIZE_T SrcDataSize,LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
	{
		throw Soy::AssertException( "D3D shader compiling unsupported." );
		return (HRESULT)S_FALSE;
	};
	mD3dCompileFunc = UnsupportedFunc;
}



void GetBlobString(ID3DBlob* Blob,std::ostream& String)
{
	if ( !Blob )
		return;

	Array<char> Buffer;
	auto BlobSize = Blob->GetBufferSize();
	auto* BlobData = reinterpret_cast<char*>( Blob->GetBufferPointer() );
	if ( !BlobData )
	{
		String << "<BlobBufferMissing[" << BlobSize << "]>";
		return;
	}

	//	clip size to something sensible
	if ( BlobSize > 1024*1024*1 )
		BlobSize = 1024*1024*1;

	auto BlobBuffer = GetRemoteArray( BlobData, BlobSize );
	String << BlobBuffer.GetArray();
}



void DirectxCompiler::TCompiler::Compile(ArrayBridge<uint8_t>&& Compiled,const std::string& Source,const std::string& Function,const std::string& Target,const std::string& Name)
{
	Array<char> SourceBuffer;
	Soy::StringToArray( Source, GetArrayBridge(SourceBuffer) );
	const D3D_SHADER_MACRO* Macros = nullptr;
	ID3DInclude* IncludeMode = nullptr;
	UINT ShaderOptions = D3D10_SHADER_ENABLE_STRICTNESS;
	UINT EffectOptions = 0;

	AutoReleasePtr<ID3DBlob> DataBlob;
	AutoReleasePtr<ID3DBlob> ErrorBlob;

	auto Compile = mImpl->mD3dCompileFunc;
	Soy::Assert( Compile!=nullptr, "Compile func missing" );

	auto Result = Compile( SourceBuffer.GetArray(), SourceBuffer.GetDataSize(),
						  Name.c_str(), Macros, IncludeMode, Function.c_str(), Target.c_str(), ShaderOptions,
						  EffectOptions,
						  &DataBlob.mObject,
						  &ErrorBlob.mObject );

	std::stringstream Error;
	Error << "D3DCompile()->";
	GetBlobString( ErrorBlob.mObject, Error );

	Directx::IsOkay( Result, Error.str() );

	auto BlobData = reinterpret_cast<uint8_t*>( DataBlob->GetBufferPointer() );
	auto BlobDataSize = DataBlob->GetBufferSize();
	auto BlobArray = GetRemoteArray( BlobData, BlobDataSize );
	Compiled.Copy( BlobArray );
}
