#include "SoyTypes.h"
#include "heaparray.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>		//	std::transform
#include "SoyString.h"



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
		::PathStripPath( static_cast<char*>(Buffer) );
#endif
    
	//	new null-terminated string
	return std::string( Buffer.GetArray().GetArray() );
#endif
}
#endif







const uint32_t TCrc32::Crc32Table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
}; // kCrc32Table


int Soy::Platform::GetLastError()
{
#if defined(TARGET_WINDOWS)
	return ::GetLastError();
#else
	//	gr: pop error number. This seems to be getting left set as an error after a successfull winsock call...
	int Error = errno;
	errno = 0;
	return Error;
#endif
}

std::string Soy::Platform::GetErrorString(int Error)
{
#if defined(TARGET_WINDOWS)
	if ( Error == ERROR_SUCCESS )
		return std::string();

	LPVOID lpMsgBuf;
	DWORD bufLen = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		Error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &lpMsgBuf,
		0, NULL );

	if (!bufLen)
		return std::string();
    
	LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
	std::string result(lpMsgStr, lpMsgStr+bufLen);

	LocalFree(lpMsgBuf);
	return result;
#else
	if ( Error == 0 )
		return std::string();

	return strerror(Error);
#endif
}



bool Soy::ReadStreamChunk(ArrayBridge<char>& Data,std::istream& Stream)
{
	//	dunno how much to read
	assert( !Data.IsEmpty() );
	if ( Data.IsEmpty() )
		return false;
	
	Stream.read( Data.GetArray(), Data.GetDataSize() );
	
	if ( Stream.fail() && !Stream.eof() )
		return false;
	
	auto BytesRead = Stream.gcount();
	Data.SetSize( BytesRead );
	
	return true;
}

bool Soy::ReadStream(ArrayBridge<char>&& Data,std::istream& Stream,std::stringstream& Error)
{
	BufferArray<char,5*1024> Buffer;
	while ( !Stream.eof() )
	{
		//	read a chunk
		Buffer.SetSize( Buffer.MaxAllocSize() );
		auto BufferBridge = GetArrayBridge( Buffer );
		if ( !Soy::ReadStreamChunk( BufferBridge, Stream ) )
		{
			Error << "Error reading stream";
			return false;
		}
		
		Data.PushBackArray( Buffer );
	}
	
	return true;
}


bool Soy::ReadStream(ArrayBridge<char>& Data,std::istream& Stream,std::stringstream& Error)
{
	//	gr: todo: re-use function above, just need to send lambda or something for PushBackArary
	BufferArray<char,5*1024> Buffer;
	while ( !Stream.eof() )
	{
		//	read a chunk
		Buffer.SetSize( Buffer.MaxAllocSize() );
		auto BufferBridge = GetArrayBridge( Buffer );
		if ( !Soy::ReadStreamChunk( BufferBridge, Stream ) )
		{
			Error << "Error reading stream";
			return false;
		}
		
		Data.PushBackArray( Buffer );
	}
	
	return true;
}

bool Soy::LoadBinaryFile(ArrayBridge<char>& Data,std::string Filename,std::stringstream& Error)
{
	//	gr: would be nice to have an array! MemFileArray maybe, if it can be cross paltform...
	std::ifstream Stream( Filename, std::ios::binary|std::ios::in );
	if ( !Stream.is_open() )
	{
		Error << "Failed to open " << Filename << " (" << Soy::Platform::GetLastErrorString() << ")";
		return false;
	}
	
	//	read block by block
	bool Result = ReadStream( Data, Stream, Error );
	Stream.close();
	
	return Result;
}




static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

void Soy::base64_encode(ArrayBridge<char>& Encoded,const ArrayBridge<char>& Decoded)
{
	auto in_len = Decoded.GetDataSize();
	auto* bytes_to_encode = Decoded.GetArray();
	if ( !bytes_to_encode )
		return;
	
	auto& ret = Encoded;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; (i <4) ; i++)
				ret.PushBack( base64_chars[char_array_4[i]] );
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++)
			ret.PushBack( base64_chars[char_array_4[j]] );
		
		while((i++ < 3))
			ret.PushBack('=');
		
	}
	
}

void Soy::base64_decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>& Decoded)
{
	//	gr: pre-alloc. base64 turns 3 chars into 4...
	//		so decoding... we only need 3/4 of the data
	//	...	just alloc same amount, it'll be smaller
	Decoded.Reserve( Encoded.GetSize() );
	
	int in_len = Encoded.GetSize();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	auto& ret = Decoded;
	
	while (in_len-- && ( Encoded[in_] != '=') && is_base64(Encoded[in_])) {
		char_array_4[i++] = Encoded[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);
			
			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
			
			for (i = 0; (i < 3); i++)
				ret.PushBack( char_array_3[i] );
			i = 0;
		}
	}
	
	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;
		
		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);
		
		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
		
		for (j = 0; (j < i - 1); j++)
			ret.PushBack( char_array_3[j] );
	}
}


bool Soy::StringToFile(std::string Filename,std::string String)
{
	std::ofstream File( Filename, std::ios::out );
	if ( !File.is_open() )
		return false;
	
	File << String;
	bool Success = !File.fail();
	
	File.close();
	return Success;
}


bool Soy::FileToString(std::string Filename,std::string& String)
{
	//	gr: err surely a better way
	Array<char> StringData;
	auto StringDataBridge = GetArrayBridge( StringData );
	std::stringstream Error;
	if ( !LoadBinaryFile( StringDataBridge, Filename, Error ) )
		return false;

	String = Soy::ArrayToString( StringDataBridge );
	return true;
/*
	std::ifstream File( Filename, std::ios::in );
	if ( !File.is_open() )
		return false;
	
#error this only pulls out first word
	File >> String;
	bool Success = !File.fail();
	
	File.close();
	return Success;
 */
}


//	http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
//#include "type.hpp"
#ifdef __GNUG__
//#include <cstdlib>
//#include <memory>
#include <cxxabi.h>

std::string Soy::DemangleTypeName(const char* name)
{
	
    int status = -4; // some arbitrary value to eliminate the compiler warning
	
    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
	
    return (status==0) ? res.get() : name ;
}

#else

// does nothing if not g++
std::string Soy::DemangleTypeName(const char* name)
{
    return name;
}

#endif

bool Soy::Assert(bool Condition, std::ostream& ErrorMessage ) throw( AssertException )
{
	return Assert( Condition, Soy::StreamToString(ErrorMessage) );
}

bool Soy::Assert(bool Condition, std::string ErrorMessage) throw(AssertException)
{
	if ( Condition )
		return true;
	
	//	throw exception
	throw Soy::AssertException(ErrorMessage);
	return false;
}
