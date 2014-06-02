#include "SoyMemFile.h"
#include "BufferArray.hpp"	//	remotearray




std::string GetWindowsLastError()
{
	DWORD error = GetLastError();
	if (!error)
		return std::string();

	LPVOID lpMsgBuf;
	DWORD bufLen = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &lpMsgBuf,
		0, NULL );

	if (!bufLen)
		return std::string();
    
	LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
	std::string result(lpMsgStr, lpMsgStr+bufLen);

	LocalFree(lpMsgBuf);
	return result;
}


MemFileArray::MemFileArray(std::string& Filename,int DataSize) :
	mFilename	( Filename ),
	mHandle		( nullptr ),
	mMap		( nullptr ),
	mMapSize	( 0 ),
	mOffset		( 0 )
{
	assert(DataSize>=0);

	//	If lpName matches the name of an existing event, semaphore, mutex, waitable timer, or job object, the function fails, and the GetLastError function returns ERROR_INVALID_HANDLE. This occurs because these objects share the same namespace.
	DWORD MaxSizeHi = 0;
	DWORD MaxSizeLo = DataSize;
	mHandle = CreateFileMappingA(	INVALID_HANDLE_VALUE,    // use paging file
									NULL,                    // default security
									PAGE_READWRITE,          // read/write access
									MaxSizeHi,                       // maximum object size (high-order DWORD)
									MaxSizeLo,                // maximum object size (low-order DWORD)
									mFilename.c_str()	// name of mapping object
									);
	if ( mHandle == nullptr )
	{
		std::Debug << GetWindowsLastError();
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
		std::Debug << GetWindowsLastError();
		Close();
		return;
	}
	mMapSize = DataSize;
	mOffset = mMapSize;
}

bool MemFileArray::IsValid() const
{
	return mHandle != nullptr;
}


void MemFileArray::Close()
{
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

