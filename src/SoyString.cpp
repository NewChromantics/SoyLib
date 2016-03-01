#include "SoyString.h"
#include <regex>
#include <sstream>
#include "array.hpp"
#include "bufferarray.hpp"
#include "heaparray.hpp"
#include "SoyDebug.h"
#include <regex>
#include "RemoteArray.h"



void Soy::StringToLower(std::string& String)
{
	std::transform( String.begin(), String.end(), String.begin(), ::tolower );
}

std::string Soy::StringToLower(const std::string& String)
{
	std::string LowerString = String;
	StringToLower( LowerString );
	return LowerString;
}


bool Soy::StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		//	gr: may need to see WHY this doesn't return npos....
		if ( Needle.empty() )
			return false;
		return (Haystack.find(Needle) != std::string::npos);
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringContains(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringMatches(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		return Haystack == Needle;
	}
	else
	{
		//	do a quick length check before switching to lowercase
		if ( Haystack.length() != Needle.length() )
			return false;
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringMatches(HaystackLow, NeedleLow, true);
	}
}

bool Soy::StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		return (Haystack.find(Needle) == 0 );
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringBeginsWith(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringEndsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		//	gr: may need to see WHY this doesn't return npos....
		if ( Needle.empty() )
			return false;
		
		auto Pos = Haystack.rfind(Needle);
		if ( Pos == std::string::npos )
			return false;
		if ( Pos == Haystack.length() - Needle.length() )
			return true;
		return false;
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringEndsWith(HaystackLow, NeedleLow, true);
	}
	
	/*	gr: regex not working on android! find out exactly what...
	std::regex_constants::syntax_option_type Flags = std::regex_constants::ECMAScript;
	if ( !CaseSensitive )
		Flags |= std::regex::icase;
	
	//	escape some chars...
	std::stringstream Pattern;
	Pattern << ".*";
	for ( auto it=Needle.begin();	it!=Needle.end();	it++ )
	{
		auto Char = *it;
		if ( Char == '.' )
			Pattern << "\\";
		Pattern << Char;
	}
	Pattern << "$";
	
	std::smatch Match;
	std::string HaystackStr = Haystack;
	std::regex Regex( Pattern.str(), Flags );
	if ( std::regex_match( HaystackStr, Match, Regex ) )
		return true;

	std::Debug << "String ends with (haystack=" << Haystack << ", Needle=" << Needle << ") failed" << std::endl;
	return false;
	*/
}

bool Soy::StringEndsWith(const std::string& Haystack,const ArrayBridge<std::string>& Needles, bool CaseSensitive)
{
	for ( int i=0;	i<Needles.GetSize();	i++ )
		if ( StringEndsWith( Haystack, Needles[i], CaseSensitive ) )
			return true;
	return false;
}


std::string	Soy::StringJoin(const std::vector<std::string>& Strings,const std::string& Glue)
{
	//	gr: consider a lambda here?
	std::stringstream Stream;
	for ( auto it=Strings.begin();	it!=Strings.end();	it++ )
	{
		Stream << (*it);
		auto itLast = Strings.end();
		itLast--;
		if ( it != itLast )
			Stream << Glue;
	}
	return Stream.str();
}



std::string Soy::ArrayToString(const ArrayBridge<char>& Array,size_t Limit)
{
	std::stringstream Stream;
	ArrayToString( Array, Stream, Limit );
	return Stream.str();
}

std::string Soy::ArrayToString(const ArrayBridge<uint8>& Array,size_t Limit)
{
	std::stringstream Stream;
	ArrayToString( Array, Stream, Limit );
	return Stream.str();
}


void Soy::ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String,size_t Limit)
{
	if ( Limit == 0 )
		Limit = Array.GetSize();
	if ( Limit < Array.GetSize() )
		Limit = Array.GetSize();
	
	//	todo: be more clever re: invalid characters
	String.write( Array.GetArray(), Array.GetSize() );
}

void Soy::ArrayToString(const ArrayBridge<uint8>& Array,std::stringstream& String,size_t Limit)
{
	if ( Limit == 0 )
		Limit = Array.GetSize();
	if ( Limit < Array.GetSize() )
		Limit = Array.GetSize();
	
	//	todo: be more clever re: invalid characters
	String.write( reinterpret_cast<const char*>(Array.GetArray()), Limit );
}

void Soy::StringToArray(std::string String,ArrayBridge<char>& Array)
{
	auto CommandStrArray = GetRemoteArray( String.c_str(), String.length() );
	Array.PushBackArray( CommandStrArray );
}

void Soy::StringToArray(std::string String,ArrayBridge<uint8>& Array)
{
	auto CommandStrArray = GetRemoteArray( reinterpret_cast<const uint8*>(String.c_str()), String.length() );
	Array.PushBackArray( CommandStrArray );
}

void Soy::StringSplitByString(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty)
{
	auto PushBack = [&Parts,&Delim](const std::string& Part)
	{
		if ( Parts.IsFull() )
		{
			Parts.GetBack() += Delim;
			Parts.GetBack() += Part;
		}
		else
		{
			Parts.PushBack( Part );
		}
		return true;
	};
	
	StringSplitByString( PushBack, String, Delim, IncludeEmpty );
}


void Soy::StringSplitByString(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty)
{
	StringSplitByString( Parts, String, Delim, IncludeEmpty );
}


bool Soy::StringSplitByString(std::function<bool(const std::string&)> Callback,const std::string& String,const std::string& Delim,bool IncludeEmpty)
{
	std::string::size_type Start = 0;
	while ( Start < String.length() )
	{
		auto End = String.find( Delim, Start );
		if ( End == std::string::npos )
			End = String.length();
		
		auto Part = String.substr( Start, End-Start );
		
		if ( IncludeEmpty || !Part.empty() )
		{
			if ( !Callback(Part) )
				return false;
		}
		
		Start = End+Delim.length();
	}
	
	return true;
}


void Soy::StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty)
{
	auto PushBack = [&Parts](const std::string& Part,const char& Delim)
	{
		if ( Parts.IsFull() )
		{
			Parts.GetBack() += Delim;
			Parts.GetBack() += Part;
		}
		else
		{
			Parts.PushBack( Part );
		}
		return true;
	};

	StringSplitByMatches( PushBack, String, MatchingChars, IncludeEmpty );
}

//	gr: merge this and split by string and have a "match" function/policy
bool Soy::StringSplitByMatches(std::function<bool(const std::string&,const char&)> Callback,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty)
{
	//	gr; I think this is the only case where we erroneously return something
	//		if removed, and we split "", then we get 1 empty case
	if ( String.empty() )
		return true;
	
	//	gr: dumb approach as regexp is a little fiddly
	std::stringstream Pending;
	char LastDelim = '\0';
	for ( int i=0;	i<=String.length();	i++ )
	{
		bool Eof = ( i >= String.length() );
		
		auto MatchIndex = MatchingChars.find(String[i]);
		if ( !Eof && MatchIndex == MatchingChars.npos )
		{
			Pending << String[i];
			continue;
		}
		
		auto Delim = Eof ? '\0' : MatchingChars[MatchIndex];
		
		//	pop part
		auto Part = Pending.str();
		Pending.str( std::string() );
		
		if ( !IncludeEmpty && Part.empty() )
		{
			LastDelim = Delim;
			continue;
		}
		
		if ( !Callback( Part, LastDelim ) )
			return false;
		
		LastDelim = Delim;
	}
	return true;
}


std::string Soy::StreamToString(std::stringstream&& Stream)
{
	return Stream.str();
}

std::string Soy::StreamToString(std::ostream& Stream)
{
	std::stringstream TempStream;
	auto* Buffer = Stream.rdbuf();
	if ( Buffer )
		TempStream << Buffer;
	return TempStream.str();
}

bool Soy::StringTrimLeft(std::string& String,std::function<bool(char)> TrimChar)
{
	bool Changed = false;
	while ( !String.empty() )
	{
		if ( !TrimChar(String[0]) )
			break;
		String.erase( String.begin() );
		Changed = true;
	}
	return Changed;
}


bool Soy::StringTrimLeft(std::string& String,char TrimChar)
{
	bool Changed = false;
	while ( !String.empty() )
	{
		if ( String[0] != TrimChar )
			break;
		String.erase( String.begin() );
		Changed = true;
	}
	return Changed;
}


bool Soy::StringTrimLeft(std::string& String,const ArrayBridge<char>&& TrimChars)
{
	bool Changed = false;
	while ( !String.empty() )
	{
		if ( !TrimChars.Find(String[0]) )
			break;
		String.erase( String.begin() );
		Changed = true;
	}
	return Changed;
}

bool Soy::StringTrimRight(std::string& String,const ArrayBridge<char>& TrimChars)
{
	bool Changed = false;
	while ( !String.empty() )
	{
		auto tail = String.back();

		if ( !TrimChars.Find(tail) )
			break;

		String.pop_back();
	}
	return Changed;
}

void Soy::StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty)
{
	StringSplitByMatches( Parts, String, MatchingChars, IncludeEmpty );
}

void Soy::SplitStringLines(std::function<bool(const std::string&,const char&)> Callback,const std::string& String,bool IncludeEmpty)
{
	StringSplitByMatches( Callback, String, "\n\r", IncludeEmpty );
}
	
void Soy::SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String,bool IncludeEmpty)
{
	StringSplitByMatches( StringLines, String, "\n\r", IncludeEmpty );
}
	
void Soy::SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String,bool IncludeEmpty)
{
	SplitStringLines( StringLines, String, IncludeEmpty );
}

bool Soy::IsUtf8String(const std::string& String)
{
	for ( int i=0;	i<String.length();	i++ )
	{
		char c = String[i];
		if ( !IsUtf8Char( c ) )
			return false;
	}
	return true;
}

bool Soy::IsUtf8Char(char c)
{
	switch (static_cast<unsigned char>(c) )
	{
		case 0xC0:
		case 0xC1:
		case 0xF5:
		case 0xF6:
		case 0xF7:
		case 0xF8:
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:
			return false;
	}
	return true;
}


template<>
bool Soy::StringToType(int& Out,const std::string& String)
{
#if defined(TARGET_ANDROID)
	if ( sscanf( String.c_str(), "%d", &Out) == EOF )
		return false;
	return true;
#else
	try
	{
		Out = std::stoi( String );
	}
	catch(std::exception& e)
	{
		//std::invalid_argument for non int string etc
		//	std::out_of_range for numbers that need to be 64bit etc
		//std::Debug << "exception converting string to int; \"" << String << "\"; " << e.what() << std::endl;
		return false;
	}
	return true;
#endif
}



//	returns if changed
bool Soy::StringReplace(std::string& str,const std::string& from,const std::string& to)
{
	if ( from.empty() )
		return false;
	size_t Changes = 0;
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		
		Changes++;
	}
	return (Changes!=0);
}

//	returns if changed
bool Soy::StringReplace(ArrayBridge<std::string>& str,const std::string& from,const std::string& to)
{
	bool Changed = false;
	for ( int i=0;	i<str.GetSize();	i++ )
	{
		Changed |= StringReplace( str[i], from, to );
	}
	return Changed;
}

bool Soy::StringReplace(ArrayBridge<std::string>&& str,const std::string& from,const std::string& to)
{
	return StringReplace(str,from,to);
}


bool Soy::StringTrimLeft(std::string& Haystack,const std::string& Prefix,bool CaseSensitive)
{
	if ( !StringBeginsWith( Haystack, Prefix, CaseSensitive ) )
		return false;
	
	Haystack.erase( Haystack.begin(), Haystack.begin() + Prefix.length() );
	return true;
}

bool Soy::StringTrimRight(std::string& Haystack,const std::string& Suffix,bool CaseSensitive)
{
	if ( !StringEndsWith( Haystack, Suffix, CaseSensitive ) )
		return false;
	
	Haystack.erase( Haystack.end() - Suffix.length(), Haystack.end() );
	return true;
}


void Soy::StringToBuffer(const char* Source,char* Buffer,size_t BufferSize)
{
	Soy::Assert( Buffer!=nullptr, "Soy::StringToBuffer Buffer expected" );
	if ( BufferSize == 0 )
		return;
	
	int Len = 0;
	for ( Len=0;	Source && Len<BufferSize-1;	Len++ )
	{
		if ( Source[Len] == '\0' )
			break;
		Buffer[Len] = Source[Len];
	}
	Soy::Assert( Len < BufferSize, "StringToBuffer Len OOB");
	Buffer[std::min<ssize_t>(Len,BufferSize-1)] = '\0';
}

std::string Soy::StringPopUntil(std::string& Haystack,std::function<bool(char)> IsDelim,bool KeepDelim,bool PopDelim)
{
	std::stringstream Pop;
	
	while ( !Haystack.empty() )
	{
		if ( IsDelim(Haystack[0]) )
		{
			if ( !KeepDelim )
				Haystack.erase( Haystack.begin() );
			if ( PopDelim )
				Pop << Haystack[0];
			break;
		}
		
		Pop << Haystack[0];
		
		Haystack.erase( Haystack.begin() );
	}
	
	return Pop.str();
}



std::string Soy::StringPopUntil(std::string& Haystack,char Delim,bool KeepDelim,bool PopDelim)
{
	//	gr: make this faster! dont use a lambda!
	auto IsDelim = [Delim](char Char)
	{
		return Char == Delim;
	};
	return StringPopUntil( Haystack, IsDelim, KeepDelim, PopDelim );
}

std::string Soy::ByteToHex(uint8 Byte)
{
	char StringChars[3] = { 0,0,0 };
	ByteToHex( Byte, StringChars[0], StringChars[1] );
	return std::string( StringChars );
}

void Soy::ByteToHex(uint8 Byte,char& Stringa,char& Stringb)
{
	auto a = Byte >> 4;
	auto b = Byte & 0xf;
	
	if ( a >= 10 )
		Stringa = 'a' + (a-10);
	else
		Stringa = '0' + (a);
		
	if ( b >= 10 )
		Stringb = 'a' + (b-10);
	else
		Stringb = '0' + (b);
}

void Soy::ByteToHex(uint8 Byte,std::ostream& String)
{
	char StringChars[3] = { 0,0,0 };
	ByteToHex( Byte, StringChars[0], StringChars[1] );
	String << StringChars;
}


uint8 Soy::HexToByte(char Hex)
{
	uint8 Value = 0;
	
	if ( Hex >= '0' && Hex <= '9' )
		Value = Hex - '0';
	else if ( Hex >= 'a' && Hex <= 'f' )
		Value = 10 + Hex - 'a';
	else if ( Hex >= 'A' && Hex <= 'F' )
		Value = 10 + Hex - 'A';
	else
	{
		std::stringstream Error;
		Error << "character " << Hex << " is not a hexidecimal";
		throw Soy::AssertException(Error.str());
	}
	return Value;
}

uint8 Soy::HexToByte(char HexA,char HexB)
{
	auto a = HexToByte(HexA);
	auto b = HexToByte(HexB);
	uint8 Byte = (a << 4) | (b);
	return Byte;
}

void Soy::SplitUrl(const std::string& Url,std::string& Protocol,std::string& Hostname,uint16& Port,std::string& Path)
{
	const std::string& ProtocolDelin("://");
	auto ProtocolPos = Url.find(ProtocolDelin);
	if ( ProtocolPos == std::string::npos )
	{
		ProtocolPos = 0;
	}

	auto HostnamePos = ProtocolPos + ProtocolDelin.length();
	auto PathPos = Url.find('/',HostnamePos);	//	will include the slash

	if ( PathPos == std::string::npos )
		PathPos = Url.length();
	
	Protocol = Url.substr( 0, ProtocolPos );
	Hostname = Url.substr( HostnamePos, PathPos-HostnamePos );
	Path = Url.substr( PathPos );
	
	try
	{
		std::string HostnameAndPort = Hostname;
		Soy::SplitHostnameAndPort( Hostname, Port, HostnameAndPort );
	}
	catch (...)
	{
	}
}

std::string	Soy::GetUrlPath(const std::string& Url)
{
	std::string Protocol,Hostname,Path;
	uint16 Port;
	SplitUrl( Url, Protocol, Hostname, Port, Path );
	return Path;
}

std::string	Soy::GetUrlHostname(const std::string& Url)
{
	std::string Protocol,Hostname,Path;
	uint16 Port;
	SplitUrl( Url, Protocol, Hostname, Port, Path );
	return Hostname;
}

std::string	Soy::GetUrlProtocol(const std::string& Url)
{
	std::string Protocol,Hostname,Path;
	uint16 Port;
	SplitUrl( Url, Protocol, Hostname, Port, Path );
	return Protocol;
}


std::string	Soy::ExtractServerFromUrl(const std::string& Url)
{
	//	parse address
	std::string mServerAddress = Url;
	std::string HttpPrefix("http://");
	if ( !Soy::StringTrimLeft( mServerAddress, HttpPrefix, false ) )
	{
		std::stringstream Error;
		Error << __func__ << " Url " << Url << " did not start with " << HttpPrefix;
		throw Soy::AssertException( Error.str() );
	}
	
	//	now split url from server
	BufferArray<std::string,2> ServerAndUrl;
	Soy::StringSplitByMatches( GetArrayBridge(ServerAndUrl), mServerAddress, "/" );
	Soy::Assert( ServerAndUrl.GetSize() != 0, "Url did not split at all" );
	if ( ServerAndUrl.GetSize() == 1 )
		ServerAndUrl.PushBack("/");
	
	mServerAddress = ServerAndUrl[0];
	
	//	if no port provided, try and add it
	{
		std::string Hostname;
		uint16 Port;
		try
		{
			Soy::SplitHostnameAndPort( Hostname, Port, mServerAddress );
		}
		catch (...)
		{
			mServerAddress += ":80";
		}
		
		//	fail if it doesn't succeed again
		Soy::SplitHostnameAndPort( Hostname, Port, mServerAddress );
	}
	
	return mServerAddress;
}


std::string	Soy::ResolveUrl(const std::string& BaseUrl,const std::string& Path)
{
	//	new path starts with a protocol, return unmodified
	auto ProtocolPos = Path.find("://");
	if ( ProtocolPos != std::string::npos )
		return Path;
	
	//	find where to place path, if it starts with a slash, it goes after the server
	//	otherwise it goes after the last slash
	bool AbsolutePath = Path[0] == '/';
	
	std::stringstream NewUrl;
	
	if ( AbsolutePath )
	{
		auto BaseProtocolPos = BaseUrl.find("://");
		if ( BaseProtocolPos == std::string::npos )
			BaseProtocolPos = 0;
		
		//	find the first / after protocol
		auto BasePathStart = BaseUrl.find( '/', BaseProtocolPos );
		if ( BasePathStart == std::string::npos )
			BasePathStart = BaseUrl.length()-1;
		
		NewUrl << BaseUrl.substr( 0, BasePathStart );
	}
	else
	{
		//	start from last /
		auto BasePathStart = BaseUrl.rfind('/');
		if ( BasePathStart == std::string::npos )
		{
			NewUrl << BaseUrl << "/";
		}
		else
		{
			NewUrl << BaseUrl.substr( 0, BasePathStart ) << "/";
		}
	}

	//	append path
	NewUrl << Path;
	return NewUrl.str();
}

std::wstring Soy::StringToWString(const std::string& s)
{
	std::wstring w;
	w.assign( s.begin(), s.end() );
	return w;
}

std::string Soy::WStringToString(const std::wstring& w)
{
	std::string s;
	s.assign( w.begin(), w.end() );
	return s;
}

std::string Soy::FourCCToString(uint32 Fourcc)
{
	char CodecStrBuffer[5] = {0,0,0,0,0};
	static_assert( sizeof(Fourcc) <= sizeof(CodecStrBuffer), "Bad buffer sizes" );
	memcpy( CodecStrBuffer, &Fourcc, sizeof(Fourcc) );

	auto IsFourccChar = [](char x)
	{
		if ( x >='A' && x <= 'Z' )	return true;
		if ( x >='a' && x <= 'z' )	return true;
		if ( x >='0' && x <= '9' )	return true;
		return false;
	};

	//	check for invalid Fourcc's (ie, integer) and render as hex instead
	if ( !IsFourccChar(CodecStrBuffer[0]) || 
		!IsFourccChar(CodecStrBuffer[1]) || 
		!IsFourccChar(CodecStrBuffer[2]) ||
		!IsFourccChar(CodecStrBuffer[3]) )
	{
		std::stringstream Error;
		Error << "Fourcc{0x";
		Soy::ByteToHex( CodecStrBuffer[0], Error );
		Soy::ByteToHex( CodecStrBuffer[1], Error );
		Soy::ByteToHex( CodecStrBuffer[2], Error );
		Soy::ByteToHex( CodecStrBuffer[3], Error );
		//	draw as integer too
		Error << "/" << Fourcc;
		//	and as reversed
		Error << "/" << Soy::SwapEndian(Fourcc);
		Error << "}";
		return Error.str();
	}
	
	return std::string( CodecStrBuffer );
}


void Soy::SplitHostnameAndPort(std::string& Hostname,uint16& Port,const std::string& Address)
{
	//	extract port from address
	std::regex Pattern("([^:]+):([0-9]+)$" );
	std::smatch Match;
	
	//	address is empty, or malformed
	if ( !std::regex_match( Address, Match, Pattern ) )
	{
		std::stringstream Error;
		Error << "Invalid hostname:port (" << Address << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	Hostname = Match[1].str();
	std::string PortStr = Match[2].str();
	int Porti;
	Soy::StringToType( Porti, PortStr );
	Port = size_cast<uint16>(Porti);
}


std::string Soy::DataToHexString(const ArrayBridge<uint8>&& Data,int MaxBytes)
{
	std::stringstream String;
	DataToHexString( String, Data, MaxBytes );
	return String.str();
}

	
void Soy::DataToHexString(std::ostream& String,const ArrayBridge<uint8>& Data,int MaxBytes)
{
	if ( MaxBytes < 0 )
		MaxBytes = size_cast<int>(Data.GetDataSize());
	else
		MaxBytes = std::min( MaxBytes, size_cast<int>(Data.GetDataSize()) );
	
	Soy::TPushPopStreamSettings StreamSettings(String);
	String << std::hex;
	String.width(2);
	for ( int i=0;	i<MaxBytes;	i++ )
	{
		String << (int)Data[i] << ' ';
	}
}



//	fast simple int->string
bool Soy::StringToUnsignedInteger(size_t& IntegerOut,const std::string& String)
{
	if ( String.empty() )
		return false;
	
	size_t Integer = 0;
	for ( int i=0;	i<String.size();	i++ )
	{
		int CharValue = String[i] - '0';
		if ( CharValue < 0 || CharValue > 9 )
			return false;
		Integer *= 10;
		Integer += CharValue;
	}
	IntegerOut = Integer;
	return true;
}

