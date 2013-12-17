#include "SoyTypes.h"
#include "heaparray.hpp"
#include <sstream>

#if defined(TARGET_WINDOWS)
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif

#if defined(NO_OPENFRAMEWORKS)
void ofLogNotice(const std::string& Message)
{
#if defined(TARGET_WINDOWS)
	OutputDebugStringA( Message.c_str() );
	OutputDebugStringA("\n");
#else
    std::cout << Message << "\n";
#endif
}
#endif

#if defined(NO_OPENFRAMEWORKS)
void ofLogWarning(const std::string& Message)
{
#if defined(TARGET_WINDOWS)
	OutputDebugStringA( "[WARN] " );
	OutputDebugStringA( Message.c_str() );
	OutputDebugStringA("\n");
#else
    std::cout << "[WARN] " << Message << "\n";
#endif
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofLogError(const std::string& Message)
{
#if defined(TARGET_WINDOWS)
	OutputDebugStringA( "[ERROR] " );
	OutputDebugStringA( Message.c_str() );
	OutputDebugStringA("\n");
#else
    std::cout << "[ERROR] " << Message << "\n";
#endif
}
#endif

#if defined(NO_OPENFRAMEWORKS)
std::string ofToString(int Integer)
{
	std::ostringstream s;
	s << Integer;
	//return std::string( s.str() );
	return s.str();
}
#endif


#if defined(NO_OPENFRAMEWORKS)
std::string ofFilePath::getFileName(const std::string& Filename,bool bRelativeToData)
{
#if defined(UNICODE)

	//	convert to widestring buffer
	std::wstring Filenamew( Filename.begin(), Filename.end() ); 
	BufferArray<wchar_t,MAX_PATH> Buffer;
	for ( int i=0;	i<static_cast<int>(Filename.size());	i++ )
		Buffer.PushBack( Filenamew[i] );

	//	alter buffer
	::PathStripPath( Buffer.GetArray() );
	Filenamew = Buffer.GetArray();

	//	convert abck to ansi
	return std::string( Filenamew.begin(), Filenamew.end() );
#else
	//	copy contents to buffer
	BufferString<MAX_PATH> Buffer = Filename.c_str();

#if defined(TARGET_WINDOWS)
	//	modify buffer (will alter and move terminator)
	if ( !Buffer.IsEmpty() )
		::PathStripPath( Buffer.GetArray() );
#endif
    
	//	new null-terminated string
	return std::string( Buffer.GetArray().GetArray() );
#endif
}
#endif

#if defined(NO_OPENFRAMEWORKS)
std::string ofBufferFromFile(const char* Filename)
{
	//	fopen is safe, but supress warning anyway
#if defined(TARGET_WINDOWS)
	FILE* File = nullptr;
	fopen_s( &File, Filename, "r" );
#else
	FILE* File = fopen( Filename, "r" );
#endif
	if ( !File )
		return std::string();

	//	read
	Array<char> Contents;
	while ( true )
	{
		BufferArray<char,1024> Buffer;
		Buffer.SetSize( Buffer.MaxSize() );
#if defined(TARGET_WINDOWS)
		auto CharsRead = fread_s( Buffer.GetArray(), Buffer.GetDataSize(), Buffer.GetElementSize(), Buffer.GetSize(), File );
#else
		auto CharsRead = std::fread( Buffer.GetArray(), Buffer.GetElementSize(), Buffer.GetSize(), File );
#endif
		Buffer.SetSize( CharsRead );

		Contents.PushBackArray( Buffer );

		//	EOF
		if ( Buffer.GetSize() != Buffer.MaxSize() )
			break;
	}
	fclose( File );
	File = nullptr;

	if ( Contents.IsEmpty() )
		return std::string();

	std::string ContentsStr = Contents.GetArray();
	return ContentsStr;
}
#endif

