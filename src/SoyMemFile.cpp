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
	//http://stackoverflow.com/questions/9409079/linux-dynamic-shared-memory-in-different-programs
	/*
	int pid = getpid();
	std::stringstream TestFilename;
	TestFilename << Filename << getpid();
	//const char *memname = mFilename.c_str();
	const char *memname = TestFilename.str().c_str();
	 */
	const size_t region_size = DataSize;//sysconf(_SC_PAGE_SIZE);
	auto& fd = mHandle;
	
	int CreateFlags = ReadOnly ? O_RDONLY : O_CREAT| O_RDWR;
	int Permission = 0666;
	//int Permission = 0700;
	//fd = shm_open( memname, O_CREAT /*| O_TRUNC | O_RDWR*/, Permission);
	std::string preError = Soy::Platform::GetLastErrorString();
	fd = shm_open( mFilename.c_str(), CreateFlags, Permission );
	if (fd == Soy::Platform::InvalidFileHandle)
	{
		std::string Error = Soy::Platform::GetLastErrorString();
		std::Debug << "MemFileArray(" << mFilename << ") error: shm_open failed; " << Error << std::endl;
		Close();
		return;
	}
	std::Debug << "opened shared file " << mFilename << std::endl;
	
	//	initialise size
	if ( !ReadOnly )
	{
		auto r = ftruncate(fd, region_size);
		if (r != 0)
		{
			std::Debug << "MemFileArray(" << mFilename << ") error: ftruncate failed; " << Soy::Platform::GetLastErrorString() << std::endl;
			Close();
			return;
		}
	}
	
	int MapProtectionFlags = ReadOnly ? PROT_READ : (PROT_READ|PROT_WRITE);
	mMap = mmap(0, region_size, MapProtectionFlags, MAP_SHARED, fd, 0);
	if ( mMap == MAP_FAILED)
	{
		std::string Error = Soy::Platform::GetLastErrorString();
		std::Debug << "MemFileArray(" << mFilename << ") error: mmap failed; " << Soy::Platform::GetLastErrorString() << std::endl;
		Close();
		return;
	}
	std::Debug << "opened shared file " << mFilename << " size: " << region_size << std::endl;
//	close(fd);
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

