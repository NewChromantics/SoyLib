#include "SoyMemFile.h"
#include "BufferArray.hpp"	//	remotearray
#include "SoyDebug.h"


#if defined(TARGET_OSX)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#endif

MemFileArray::MemFileArray(std::string Filename,bool AllowOtherFilename) :
	mFilename	( Filename ),
	mAllowOtherFilename	( AllowOtherFilename ),
	mHandle		( Soy::Platform::InvalidFileHandle ),
	mMap		( nullptr ),
	mMapSize	( 0 ),
	mOffset		( 0 )
{
}


MemFileArray::MemFileArray(std::string Filename,bool AllowOtherFilename,int DataSize,bool ReadOnly) :
	mFilename	( Filename ),
	mAllowOtherFilename	( AllowOtherFilename ),
	mHandle		( Soy::Platform::InvalidFileHandle ),
	mMap		( nullptr ),
	mMapSize	( 0 ),
	mOffset		( 0 )
{
	Init( DataSize, ReadOnly );
}

bool MemFileArray::Init(int DataSize,bool ReadOnly)
{
	std::stringstream Error;
	bool Result = Init( DataSize, ReadOnly, Error );
	
	if ( !Error.str().empty() )
		std::Debug << Error.str() << std::endl;

	return Result;
}

bool MemFileArray::Init(int DataSize,bool ReadOnly,std::stringstream& Error)
{
	if ( !Soy::Assert( DataSize>0, "Trying to allocate/get shared memfile <1 byte" ) )
		return false;

	//	already allocated, success is if we have enough allocated
	if ( mMapSize > 0 )
	{
		if ( DataSize <= mMapSize )
			return true;
		Error << "Trying to re-alloc memfile (" << Soy::FormatSizeBytes(mMapSize) << ") to larger size (" << Soy::FormatSizeBytes(DataSize) << ")";
		return false;
	}
	
#if defined(TARGET_OSX)

	if ( ReadOnly )
	{
		if ( !OpenReadOnlyFile( DataSize, Error ) )
		{
			Close();
			return false;
		}
	}
	else
	{
		//	attempt to create file
		if ( !OpenWriteFile( DataSize, Error ) )
		{
			Close();
			return false;
		}
	}
	
	int MapProtectionFlags = ReadOnly ? PROT_READ : (PROT_READ|PROT_WRITE);
	mMap = mmap(0, DataSize, MapProtectionFlags, MAP_SHARED, mHandle, 0);
	if ( mMap == MAP_FAILED )
	{
		//	don't store error code as unmap will fail
		mMap = nullptr;
		
		auto PlatformError = Soy::Platform::GetLastErrorString();
		Error << "MemFileArray(" << mFilename << ") error: mmap failed; " << PlatformError;
		Close();
		return false;
	}

	//close(fd);
	mMapSize = DataSize;
	mOffset = mMapSize;
	
#endif

#if defined(TARGET_WINDOWS)
	//	If lpName matches the name of an existing event, semaphore, mutex, waitable timer, or job object, the function fails, and the GetLastError function returns ERROR_INVALID_HANDLE. This occurs because these objects share the same namespace.
	DWORD MaxSizeHi = 0;
	DWORD MaxSizeLo = DataSize;
	mHandle = CreateFileMappingA(	INVALID_HANDLE_VALUE,    // use paging file
									NULL,                    // default security
									ReadOnly ? PAGE_READONLY : PAGE_READWRITE,          // read/write access
									MaxSizeHi,                       // maximum object size (high-order DWORD)
									MaxSizeLo,                // maximum object size (low-order DWORD)
									mFilename.c_str()	// name of mapping object
									);
	if ( mHandle == nullptr )
	{
		Error << "MemFileArray(" << mFilename << ") error: " << Soy::Platform::GetLastErrorString();
		return false;
	}


	//	map to file
	mMap = MapViewOfFile(	mHandle,   // handle to map object
							FILE_MAP_ALL_ACCESS, // read/write permission
							0,
							0,
							DataSize );
	if ( !mMap )
	{
		auto PlatformError = Soy::Platform::GetLastErrorString();
		Error << "MemFileArray(" << mFilename << ") error: " << PlatformError;
		Close();
		return false;
	}
	mMapSize = DataSize;
	mOffset = mMapSize;
#endif
	
	return true;
}

bool MemFileArray::IsValid() const
{
	return mHandle != Soy::Platform::InvalidFileHandle;
}

#if defined(TARGET_OSX)
bool OpenFile(int& FileHandle,std::string& Filename,size_t Size,int CreateFlags,std::stringstream& Error)
{
	//http://stackoverflow.com/questions/9409079/linux-dynamic-shared-memory-in-different-programs
	//	sysconf(_SC_PAGE_SIZE);

	//	docs say this is optional, but it's needed.
	//http://stackoverflow.com/questions/21368355/macos-x-c-shm-open-fails-with-errno-13-permission-denied-on-subsequent-runs
	int Mode = S_IRWXU|S_IRWXG|S_IRWXO;
	std::string preError = Soy::Platform::GetLastErrorString();
	FileHandle = shm_open( Filename.c_str(), CreateFlags, Mode );
	if (FileHandle == Soy::Platform::InvalidFileHandle)
	{
		auto PlatformError = Soy::Platform::GetLastErrorString();
		Error << "MemFileArray(" << Filename << ") error: shm_open failed; " << PlatformError;
		return false;
	}
	
	//	initialise size - only done on create
	if ( CreateFlags & O_CREAT )
	{
		auto r = ftruncate( FileHandle, Size);
		if (r != 0)
		{
			auto ErrorVal = Soy::Platform::GetLastError();
			auto ErrorString = Soy::Platform::GetErrorString( ErrorVal );
			
			if ( ErrorVal == EINVAL )
			{
				Error << "MemFileArray(" << Filename << ") ftruncate error; " << Size << " must be larger than original size. " << ErrorString;
			}
			else
			{
				Error << "MemFileArray(" << Filename << ") error: ftruncate(" << Size << ") failed; " << ErrorString;
			}
		
			//	fail regardless, mmap will probably fail next
			return false;
		}
	}
	
	return true;
}
#endif

#if defined(TARGET_OSX)
bool MemFileArray::OpenWriteFile(size_t Size,std::stringstream& Error)
{
	BufferArray<std::string,100> FilenameTries;
	FilenameTries.PushBack( mFilename );
	
	if ( mAllowOtherFilename )
	{
		for ( int i=1;	i<FilenameTries.MaxSize();	i++ )
		{
			std::stringstream Filename2;
			Filename2 << mFilename << "_" << i;
			FilenameTries.PushBack( Filename2.str() );
		}
	}

	for ( int i=0;	i<FilenameTries.GetSize();	i++ )
	{
		auto& Filename = FilenameTries[i];

		//	clear so only the last error is left
		Error.clear();
		if ( !OpenFile( mHandle, Filename, Size, O_CREAT|O_RDWR, Error ) )
			continue;

		mFilename = Filename;
		return true;
	}

	return false;
}
#endif

#if defined(TARGET_OSX)
bool MemFileArray::OpenReadOnlyFile(size_t Size,std::stringstream& Error)
{
	if ( !OpenFile( mHandle, mFilename, Size, O_RDONLY, Error ) )
	{
		Close();
		return false;
	}
	return true;
}
#endif

void MemFileArray::Destroy()
{
#if defined(TARGET_OSX)
	//	ignore if opened in read-only?
	auto Result = shm_unlink( mFilename.c_str() );
	Soy::Assert(Result==0, "error unlinking memory" );
#endif
}

void MemFileArray::Close()
{
	if ( mMap )
	{
#if defined(TARGET_OSX)
		auto Result = munmap( mMap, mMapSize );
		Soy::Assert(Result==0, "error unmapping" );
#endif
#if defined(TARGET_WINDOWS)
		UnmapViewOfFile( mMap );
#endif
		mMap = nullptr;
		mMapSize = 0;
	}

	if ( mHandle != Soy::Platform::InvalidFileHandle )
	{
#if defined(TARGET_OSX)
		close(mHandle);
#endif
#if defined(TARGET_WINDOWS)
		CloseHandle(mHandle);
#endif
		mHandle = Soy::Platform::InvalidFileHandle;
	}
}


bool MemFileArray::SetSize(int size, bool preserve,bool AllowLess)
{
	if ( !Soy::Assert( size >= 0, "Invalid size specified in MemFileArray::SetSize" ) )
		size = 0;
	
	//	if we haven't allocated yet, we'll need to
	if ( size > 0 )
	{
		//	gr: errr do we need this to be readonly sometimes?
		bool ReadOnly = false;
		Init( size, ReadOnly );
	}
	
	//	if we hit the limit, don't change and fail
	//	gr: this is rare enough that an assert should be okay to add to debug
	auto Error = [size,this]
	{
		std::stringstream Error;
		Error << "Cannot allocate " << size << " > maxsize " << MaxSize();
		return Error.str();
	};
	if ( !Soy::Assert( size <= MaxSize(), Error ) )
		return false;
	
	mOffset = size;
	return true;
}



ofPtr<MemFileArray> SoyMemFileManager::AllocFile(const ArrayBridge<char>& Data)
{
	ofMutex::ScopedLock Lock(mFileLock);

	//	make up filename
	mFilenameRef++;
	std::string Filename;
	Filename += mFilenamePrefix;
	Filename << mFilenameRef;
	//	todo: see if file already exists?
	//	alloc new file
	ofPtr<MemFileArray> File = ofPtr<MemFileArray>( new MemFileArray( Filename, true, Data.GetDataSize(), false ) );
	if ( !File->IsValid() )
		return ofPtr<MemFileArray>();
	File->Copy( Data );

	//	save
	mFiles.PushBack( File );
	return File;
}




class SoyMemFileArray : public ArrayInterface<char>
{
public:
	SoyMemFileArray(std::string Filename,int Size);
	~SoyMemFileArray();
};

