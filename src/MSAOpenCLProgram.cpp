#include "MSAOpenCL.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLKernel.h"

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
	
	
	bool OpenCLProgram::loadFromFile(std::string filename, bool isBinary,const char* BuildOptions) 
	{
		BufferString<1000> Debug;
		Debug << "OpenCLProgram::loadFromFile " << filename << ", isBinary: " << isBinary << ", buildoptions: " << BuildOptions;
		ofLogNotice( Debug.c_str() );
		
		std::string fullPath = ofToDataPath(filename.c_str());
		
		if(isBinary) {
			//		clCreateProgramWithBinary
			ofLogError( "Binary programs not implemented yet");
			return false;
			
		} else {
			//	http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
			//std::string FileContents( ofBufferFromFile( fullPath ).getText() );
			std::string FileContents = ofBufferFromFile( fullPath );

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
	}
	
	
	
	bool OpenCLProgram::loadFromSource(std::string source,const char* sourceLocation,const char* BuildOptions) {
		ofLogNotice( __FUNCTION__ );
		
		assert( !mProgram );
		if ( mProgram )
			return false;

		cl_int err;
		
		const char* csource = source.c_str();
		mProgram = clCreateProgramWithSource( mParent.getContext(), 1, &csource, NULL, &err);
		
		return build( sourceLocation ? sourceLocation : "(From source)", BuildOptions );
	} 
	
	
	OpenCLKernel* OpenCLProgram::loadKernel(std::string kernelName,cl_command_queue Queue) {
		BufferString<1000> Debug;
		Debug << __FUNCTION__ << " " << kernelName;
		ofLogNotice( Debug.c_str() );

		//assert( mProgram );
		if ( !mProgram )
			return nullptr;
		
		assert( Queue );
		if ( !Queue )
			return false;

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
	
	
	void OpenCLProgram::getBinary()
	{
		/*
		//	gr: if this is to be used, it needs an overhaul
		if ( !mProgram )
			return;

		cl_uint program_num_devices;
		cl_int err;
		err = clGetProgramInfo( mProgram, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &program_num_devices, NULL);
		assert(err == CL_SUCCESS);
		
		if (program_num_devices == 0) {
			std::cerr << "no valid binary was found" << std::endl;
			return;
		}
		
		size_t* binaries_sizes = new size_t[program_num_devices];
		
		err = clGetProgramInfo( mProgram, CL_PROGRAM_BINARY_SIZES, program_num_devices*sizeof(size_t), binaries_sizes, NULL);
		assert(err = CL_SUCCESS);
		
		char **binaries = new char*[program_num_devices];
		
		for (size_t i = 0; i < program_num_devices; i++)
			binaries[i] = new char[binaries_sizes[i]+1];
		
		err = clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, program_num_devices*sizeof(size_t), binaries, NULL);
		assert(err = CL_SUCCESS);
		
		for (size_t i = 0; i < program_num_devices; i++) {
			binaries[i][binaries_sizes[i]] = '\0';
			std::cout << "Program " << i << ":" << std::endl;
			std::cout << binaries[i];
		}
		
		for (size_t i = 0; i < program_num_devices; i++)
			delete [] binaries[i];
		
		delete [] binaries;
		delete [] binaries_sizes;
		*/
	}
	
	
	bool OpenCLProgram::build(const char* ProgramSource,const char* BuildOptions) 
	{
		assert( mProgram );
		if ( !mProgram )
			return false;
		
		std::string Options;

		//	auto-include path
		bool IncludePath = true;
		if ( IncludePath )
		{
			std::string Path = ofToDataPath("",true).c_str();

			//	opencl [amd APP sdk] requires paths with forward slashes [on windows]
			std::replace( Path.begin(), Path.end(), '\\', '/' );
			Options += "-I \"";
			Options += Path;
			Options += "\" ";
		}
		
		if ( BuildOptions )
			Options += BuildOptions;
	
		//	build for all our devices
		cl_device_id Devices[100];
		int DeviceCount = mParent.GetDevices( Devices );

		TString Debug;
		Debug << "Build OpenCLProgram " << ProgramSource << ": (" << Options << ") for " << DeviceCount << " devices...";
		ofLogNotice( Debug.c_str() );

		cl_int err = clBuildProgram( mProgram, DeviceCount, Devices, Options.c_str(), NULL, NULL);
		
		if ( err == CL_SUCCESS )
			Debug << "CL_SUCESS";
		else
			Debug << "Failed [" << OpenCL::getErrorAsString(err) << "]";
		Debug << "\n------------------------------------\n\n";
		if ( err == CL_SUCCESS )
			ofLogNotice( Debug.c_str() );
		else
			ofLogError( Debug.c_str() );

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
