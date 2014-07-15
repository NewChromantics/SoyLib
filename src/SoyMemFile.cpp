#include "SoyMemFile.h"
#include "BufferArray.hpp"	//	remotearray
#include "SoyDebug.h"





MemFileArray::MemFileArray(std::string& Filename,int DataSize,bool ReadOnly) :
	mFilename	( Filename ),
#if defined(TARGET_WINDOWS)
	mHandle		( nullptr ),
#endif
	mMap		( nullptr ),
	mMapSize	( 0 ),
	mOffset		( 0 )
{
	assert(DataSize>=0);

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
		std::Debug << "MemFileArray(" << mFilename << ") error: " << Soy::Windows::GetLastErrorString() << std::endl;
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
		std::Debug << "MemFileArray(" << mFilename << ") error: " << Soy::Windows::GetLastErrorString() << std::endl;
		Close();
		return;
	}
	mMapSize = DataSize;
	mOffset = mMapSize;
#endif
}

bool MemFileArray::IsValid() const
{
#if defined(TARGET_WINDOWS)
	return mHandle != nullptr;
#else
	return false;
#endif
}


void MemFileArray::Close()
{
#if defined(TARGET_WINDOWS)
	if ( mMap )
	{
		UnmapViewOfFile( mMap );
		mMap = nullptr;
		mMapSize = 0;
	}

	if ( mHandle != nullptr )
	{
		CloseHandle(mHandle);
		mHandle = nullptr;
	}
#endif
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

