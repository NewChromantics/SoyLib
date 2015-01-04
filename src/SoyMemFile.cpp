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


MemFileArray::MemFileArray(std::string Filename,int DataSize,bool ReadOnly) :
	mFilename	( Filename ),
	mHandle		( Soy::Platform::InvalidFileHandle ),
	mMap		( nullptr ),
	mMapSize	( 0 ),
	mOffset		( 0 )
{
	if ( !Soy::Assert( DataSize>0, "Trying to allocate/get shared memfile <1 byte" ) )
		return;
	
#if defined(TARGET_OSX)

	std::stringstream Error;
	
	if ( ReadOnly )
	{
		if ( !OpenReadOnlyFile(Error) )
		{
			Close();
			std::Debug << Error.str() << std::endl;
			return;
		}
	}
	else
	{
		//	attempt to create file
		if ( !CreateFile(DataSize,Error) )
		{
			if ( !OpenWriteFile(Error) )
			{
				Close();
				std::Debug << Error.str() << std::endl;
				return;
			}
		}
	}
	
	int MapProtectionFlags = ReadOnly ? PROT_READ : (PROT_READ|PROT_WRITE);
	mMap = mmap(0, DataSize, MapProtectionFlags, MAP_SHARED, mHandle, 0);
	if ( mMap == MAP_FAILED)
	{
		Error << Soy::Platform::GetLastErrorString();
		std::Debug << "MemFileArray(" << mFilename << ") error: mmap failed; " << Soy::Platform::GetLastErrorString() << std::endl;
		Close();
		return;
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
		std::Debug << "MemFileArray(" << mFilename << ") error: " << Soy::Platform::GetLastErrorString() << std::endl;
		return;
	}


	//	map to file
	mMap = MapViewOfFile(	mHandle,   // handle to map object
							FILE_MAP_ALL_ACCESS, // read/write permission
							0,
							0,
							DataSize );
	if ( !mMap )
	{
		std::Debug << "MemFileArray(" << mFilename << ") error: " << Soy::Platform::GetLastErrorString() << std::endl;
		Close();
		return;
	}
	mMapSize = DataSize;
	mOffset = mMapSize;
#endif
}

bool MemFileArray::IsValid() const
{
	return mHandle != Soy::Platform::InvalidFileHandle;
}

bool OpenFile(int& FileHandle,std::string& Filename,int CreateFlags,std::stringstream& Error)
{
	std::string preError = Soy::Platform::GetLastErrorString();
	FileHandle = shm_open( Filename.c_str(), CreateFlags );
	if (FileHandle == Soy::Platform::InvalidFileHandle)
	{
		auto PlatformError = Soy::Platform::GetLastErrorString();
		Error << "MemFileArray(" << Filename << ") error: shm_open failed; " << PlatformError;
		return false;
	}
	return true;
}

bool MemFileArray::CreateFile(size_t Size,std::stringstream& Error)
{
	//http://stackoverflow.com/questions/9409079/linux-dynamic-shared-memory-in-different-programs
	//	sysconf(_SC_PAGE_SIZE);
	
	//	gr: exclusive as if we creat, and it already exists, ftruncate fails
	mode_t Permission = 0666;
	static int CreateFlags = O_CREAT|O_EXCL|O_RDWR;
	if ( !OpenFile( mHandle, mFilename, CreateFlags, Error ) )
	{
		Close();
		return false;
	}

	//	initialise size - only done on create
	auto r = ftruncate(mHandle, Size);
	if (r != 0)
	{
		Error << "MemFileArray(" << mFilename << ") error: ftruncate(" << Size << ") failed; " << Soy::Platform::GetLastErrorString();
		Close();
		return false;
	}
	return true;
}

bool MemFileArray::OpenWriteFile(std::stringstream& Error)
{
	if ( !OpenFile( mHandle, mFilename, O_RDWR, Error ) )
	{
		Close();
		return false;
	}
	return true;
}

bool MemFileArray::OpenReadOnlyFile(std::stringstream& Error)
{
	if ( !OpenFile( mHandle, mFilename, O_RDONLY, Error ) )
	{
		Close();
		return false;
	}
	return true;
}


void MemFileArray::Destroy()
{
#if defined(TARGET_OSX)
	//	ignore if opened in read-only?
	auto Result = shm_unlink( mFilename.c_str() );
	Soy::Assert(Result==0, "error unmapping" );
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
	ofPtr<MemFileArray> File = ofPtr<MemFileArray>( new MemFileArray( Filename, Data.GetDataSize() ) );
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

