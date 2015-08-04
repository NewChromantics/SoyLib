#include "MSAOpenCL.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLKernel.h"
#include <SoyApp.h>
#include <SoyDebug.h>


namespace msa { 
	
	
	OpenCLProgram::OpenCLProgram(OpenCL& Parent) :
		mParent		( Parent ),
		mProgram	( NULL )
	{
		ofLogNotice( __FUNCTION__ );
	}
	
	
	OpenCLProgram::~OpenCLProgram() 
	{
		ofLogNotice( __FUNCTION__ );
		if ( mProgram )
			clReleaseProgram(mProgram);
	}
	
	
	bool OpenCLProgram::loadFromFile(std::string filename, bool isBinary,std::string BuildOptions)
	{
		BufferString<1000> Debug;
		Debug << "OpenCLProgram::loadFromFile " << filename << ", isBinary: " << isBinary << ", buildoptions: " << BuildOptions;
		ofLogNotice( Debug.c_str() );
		
		std::string fullPath = ofToDataPath(filename.c_str());
		
		std::string FileContents;
		/*
		if ( !mParent.IsIncludesSuported() )
		{
			std::string ParsedContents;
			Array<BufferString<MAX_PATH>> FilesAlreadyIncluded;
			Array<BufferString<MAX_PATH>> Paths;
			GetIncludePaths( Paths );
#pragma warning todo: file path utils
			//Paths.PushBack( SoyFileSys::GetParentDirectory( fullPath.c_str() ) );

			if ( !ParseIncludes( fullPath.c_str(), ParsedContents, FilesAlreadyIncluded, Paths ) )
			{
				BufferString<1000> Debug;
				Debug << "Error loading program file: " << fullPath;
				ofLogError( Debug.c_str() );
				return false;
			}
			FileContents = ParsedContents.c_str();

			//	write out the file for debugging
			static bool SavePatchedFile = true;
			if ( SavePatchedFile )
			{
				std::stringstream DebugFilename;
				DebugFilename << fullPath << ".patched.tmp";
				//	make sure we don't overwrite the source filename
				if ( DebugFilename.str() != fullPath )
				{
					Soy::StringToFile( DebugFilename.str(), ParsedContents.c_str() );
				}
			}
		}
		else*/
		{
			Soy::FileToString( fullPath, FileContents );
		}

		if ( !FileContents.size() )
		{
			BufferString<1000> Debug;
			Debug << "Error loading program file: " << fullPath;
			ofLogError( Debug.c_str() );
			return false;
		}
			
		
		bool Success = loadFromSource( FileContents, filename.c_str(), BuildOptions );
		return Success;
	}
	

	bool OpenCLProgram::ParseIncludes(std::string Filename,std::string& Source,Array<SoyFilesystem::File>& FilesAlreadyIncluded,const Array<SoyFilesystem::Path>& Paths)
	{
#if defined(ENABLE_PARSE_INCLUDES)
		std::string PathFilename;

		SoyFilesystem::File File( Filename );
		if ( File.HasAbsolutePath() )
		{
			PathFilename = File.GetFullPathFilename();
		}
		else
		{
			//	try and find the filename
			for ( int p=0;	p<Paths.GetSize();	p++ )
			{
				auto& Path = Paths[p];

				SoyFilesystem::File PathFile( Path, Filename );
				if ( !PathFile.Exists() )
					continue;
				
				PathFilename = PathFile.GetFullPathFilename();
				break;
			}
		}

		//	already included, don't need to include again (we'll get duplicate symbols)
		if ( FilesAlreadyIncluded.Find( SoyFilesystem::File(PathFilename) ) )
		{
			assert( Source.empty() );
			return true;
		}

		//	load file
		if( !::Soy::FileToString( PathFilename.c_str(), Source ) )
		{
			std::Debug << "Failed to read include filename \"" << PathFilename << "\"" << std::endl;
			return false;
		}

		//	add to list now (bit premature... but need to for the recursion)
		FilesAlreadyIncluded.PushBack( PathFilename );

		//	recursively parse this file
		if ( !ParseIncludes( Source, FilesAlreadyIncluded, Paths ) )
			return false;

		return true;
#endif
		return false;
	}

	bool OpenCLProgram::ParseIncludes(std::string& Source,Array<SoyFilesystem::File>& FilesAlreadyIncluded,const Array<SoyFilesystem::Path>& Paths)
	{
#if defined(ENABLE_PARSE_INCLUDES)
		//	find & replace includes
		int StringPos = 0;
		while ( true )
		{
			//	gr: change this to a regex!
			BufferString<100> IncludePrefix = "#include \"";
			BufferString<100> IncludeSuffix = "\"";
			
			//	gr: NOT CASE INSENTIVE
			auto IncludePrefixPos = Source.find( IncludePrefix, StringPos );

			//	no more includes
			if ( IncludePrefixPos == std::string::npos )
				break;
			int IncludeFilenameStart = IncludePrefixPos + IncludePrefix.GetLength();

			//	find the end pos
			int IncludeSuffixPos = Source.find( IncludeSuffix, IncludeFilenameStart );
			if ( IncludeSuffixPos == std::string::npos )
				break;
			int IncludeFilenameEnd = IncludeSuffixPos;
			assert( IncludeFilenameEnd > IncludeFilenameStart );

			//	first evaluate the include...
			std::string IncludePath;
			//IncludePath.CopyString( &Source[IncludeFilenameStart], IncludeFilenameEnd - IncludeFilenameStart );
			Source.substr( IncludeFilenameStart, IncludeFilenameEnd - IncludeFilenameStart );

			std::string IncludeContents;
			if ( !ParseIncludes( IncludePath, IncludeContents, FilesAlreadyIncluded, Paths ) )
				return false;

			//	remove the include line
			//	gr: comment out for debugging?
			int IncludeSuffixEnd = IncludeSuffixPos + IncludeSuffix.GetLength();
			Source.erase( IncludePrefixPos, IncludeSuffixEnd-IncludePrefixPos );
		
			{
				auto IncludeIncludePos = Source.find( IncludePrefix );
				assert( IncludeIncludePos == std::string::npos || IncludeIncludePos > IncludePrefixPos );
			}

			//	insert included content
			Source.insert( IncludePrefixPos, IncludeContents );

			//	move along past the file we inserted and parsed
			//	gr: maybe better to remove the recursiveness and just find the next include from where we inserted
			StringPos = IncludePrefixPos + IncludeContents.length();
		}
		
		return true;
#endif
		return false;
	}

	
	bool OpenCLProgram::loadFromSource(std::string source,std::string sourceLocation,std::string BuildOptions) {
		ofLogNotice( __FUNCTION__ );
		
		assert( !mProgram );
		if ( mProgram )
			return false;

		cl_int err;
		
		const char* csource = source.c_str();
		mProgram = clCreateProgramWithSource( mParent.getContext(), 1, &csource, NULL, &err);
		
		if ( sourceLocation.empty() )
			sourceLocation = "(From source)";
		return build( sourceLocation, BuildOptions );
	} 
	
	
	OpenCLKernel* OpenCLProgram::loadKernel(std::string kernelName,cl_command_queue Queue) 
	{
		if ( !mProgram )
			return nullptr;
		
		assert( Queue );
		if ( !Queue )
			return nullptr;

		cl_int err = CL_SUCCESS;
	
		cl_kernel Kernel = clCreateKernel( mProgram, kernelName.c_str(), &err );
		if ( !Kernel || err != CL_SUCCESS )
		{
			BufferString<1000> Debug;
			Debug << "Error creating kernel: " << kernelName << " [" << OpenCL::getErrorAsString(err) << "]";
			ofLogError( Debug.c_str() );
			return NULL;
		}

		OpenCLKernel *k = new OpenCLKernel( mParent, Kernel, Queue, kernelName );
		return k;
	}
	
	void OpenCLProgram::GetIncludePaths(Array<SoyFilesystem::Path>& Paths)
	{
		//	always have this for full paths
		//Paths.PushBack( std::string("./") );
	}
	
	bool OpenCLProgram::build(std::string ProgramSource,std::string BuildOptions)
	{
		assert( mProgram );
		if ( !mProgram )
			return false;
		
		std::string Options;

		//	auto-include path
		static bool IncludePath = true;
		if ( IncludePath )
		{
			Array<SoyFilesystem::Path> Paths;
			GetIncludePaths( Paths );

			for ( int p=0;	p<Paths.GetSize();	p++)
			{
				std::string Path = Paths[p].GetFullPath();

				//	opencl [amd APP sdk] requires paths with forward slashes [on windows]
				std::replace( Path.begin(), Path.end(), '\\', '/' );
				Options += "-I \"";
				Options += Path;
				Options += "\" ";
			}

		}
		
		if ( !BuildOptions.empty() )
			Options += BuildOptions;
	
		//	build for all our devices
		cl_device_id Devices[100];
		int DeviceCount = mParent.GetDevices( Devices );

		std::stringstream Debug;
		Debug << "Build OpenCLProgram " << ProgramSource << ": (" << Options << ") for " << DeviceCount << " devices...";
		std::Debug << Debug.str() << std::endl;

		cl_int err = clBuildProgram( mProgram, DeviceCount, Devices, Options.c_str(), NULL, NULL);
		
		if ( err == CL_SUCCESS )
			Debug << "CL_SUCESS";
		else
			Debug << "Failed [" << OpenCL::getErrorAsString(err) << "]";
		Debug << "\n------------------------------------\n\n";
		if ( err == CL_SUCCESS )
			std::Debug << Debug.str() << std::endl;
		else
			std::Debug << Debug.str() << std::endl;
	
		//	get build log size first so we always have errors to display
		for ( int d=0;	d<DeviceCount;	d++ )
		{
			auto DeviceId = Devices[d];
			auto* pDevice = mParent.GetDevice( DeviceId );
			std::string DeviceName;
			if ( pDevice )
				DeviceName += reinterpret_cast<char*>( &pDevice->mInfo.deviceName[0] );
			else
				DeviceName += "<device missing from opencl manager>";
			size_t len = 0;
			int BuildInfoErr = clGetProgramBuildInfo( mProgram, DeviceId, CL_PROGRAM_BUILD_LOG, 0, NULL, &len );
			std::vector<char> buffer( len+1 );
			BuildInfoErr = clGetProgramBuildInfo( mProgram, DeviceId, CL_PROGRAM_BUILD_LOG, buffer.size(), &buffer.at(0), NULL );
			buffer[len] = '\0';
			
			if ( len > 1 )
			{
				//	errors might contain % symbols, which will screw up the va_args system when it tries to parse them...
				std::replace( buffer.begin(), buffer.end(), '%', '@' );

				const char* bufferString = &buffer[0];
				TString Debug;
				Debug << "Device[" << d << "] " << DeviceName << ":";
				Debug << bufferString;
				Debug << "\n------------------------------------\n";
				if ( err == CL_SUCCESS )
					ofLogNotice( Debug.c_str() );
				else
					ofLogError( Debug.c_str() );
			}
		}
		return ( err == CL_SUCCESS );
	}
}
