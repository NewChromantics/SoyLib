#include "SoyTypes.h"
#include <sstream>
#include "heaparray.hpp"
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#if defined(NO_OPENFRAMEWORKS)
void ofLogNotice(const char* Message)
{
	OutputDebugStringA( Message );
	OutputDebugStringA("\n");
}
#endif

#if defined(NO_OPENFRAMEWORKS)
void ofLogWarning(const char* Message)
{
	OutputDebugStringA( "[WARN] " );
	OutputDebugStringA( Message );
	OutputDebugStringA("\n");
}
#endif


#if defined(NO_OPENFRAMEWORKS)
void ofLogError(const char* Message)
{
	OutputDebugStringA( "[ERROR] " );
	OutputDebugStringA( Message );
	OutputDebugStringA("\n");
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

	//	modify buffer (will alter and move terminator)
	if ( !Buffer.IsEmpty() )
		::PathStripPath( Buffer.GetArray() );

	//	new null-terminated string
	return Buffer.GetArray();
#endif
}
#endif

#if defined(NO_OPENFRAMEWORKS)
std::string ofBufferFromFile(const char* Filename)
{
	FILE* File = nullptr;
	auto Error = fopen_s( &File, Filename, "r" );
	if ( Error != 0 )
		return std::string();

	//	read
	Array<char> Contents;
	while ( true )
	{
		BufferArray<char,1024> Buffer;
		Buffer.SetSize( Buffer.MaxSize() );
		auto CharsRead = fread_s( Buffer.GetArray(), Buffer.GetDataSize(), Buffer.GetElementSize(), Buffer.GetSize(), File );
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

