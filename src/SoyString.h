#pragma once


#include <vector>
#include <string>
#include <sstream>
#include "MemHeap.hpp"
#include "SoyAssert.h"

/*
//	CFStringRef definition
//	don't like the extra dependency, but then CFString->String should be in this file :/
#if defined(TARGET_OSX)||defined(TARGET_IOS)
#include "CoreFoundation/CoreFoundation.h"
#endif
*/
//	foraward declarations
template<typename TYPE>
class ArrayBridge;

namespace Soy
{
	//	Soy::lf
	//	like std::endl without the implicit flush. I prefer this over \n's
	template <class _CharT, class _Traits>
	inline std::basic_ostream<_CharT, _Traits>& lf(std::basic_ostream<_CharT, _Traits>& __os)
	{
		__os.put(__os.widen('\n'));
		return __os;
	}
	
	//	delineators for strings. First is used for output
	static const char* VecNXDelins = ",x";
	
	class TPushPopStreamSettings;	//	scoped push/pop of stream flags
};

//	string that uses a prmem::Heap
//	not compatible with std::string, but maybe we can do an easy conversion func
//	todo: replace heapbridge, with an arraybridge!
namespace Soy
{
	typedef std::basic_string<char, std::char_traits<char>, Soy::HeapBridge<char> > HeapString;
};


namespace Soy
{
	void		StringToLower(std::string& String);
	std::string	StringToLower(const std::string& String);
	
	bool		StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive);
	bool		StringEndsWith(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	template <size_t BUFFERSIZE>
	bool		StringEndsWith(const std::string& Haystack,const char* (& Needles)[BUFFERSIZE], bool CaseSensitive);
	bool		StringEndsWith(const std::string& Haystack,const ArrayBridge<std::string>& Needles, bool CaseSensitive);
	bool		StringMatches(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	
	std::string	StringJoin(const std::vector<std::string>& Strings,const std::string& Glue);
	template<typename TYPE>
	std::string	StringJoin(const ArrayBridge<TYPE>& Elements,const std::string& Glue);
	void		StringSplitByString(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	void		StringSplitByString(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	bool		StringSplitByString(std::function<bool(const std::string&)> Callback,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	bool		StringSplitByString(std::function<bool(const std::string&)> Callback,const std::string& String,char Delim,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	bool		StringSplitByMatches(std::function<bool(const std::string&,const char&)> Callback,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);

	bool		StringTrimLeft(std::string& String, char TrimChar);
	bool		StringTrimLeft(std::string& String, const ArrayBridge<char>&& TrimAnyChars);
	bool		StringTrimLeft(std::string& String,std::function<bool(char)> TrimChar);
	bool		StringTrimRight(std::string& String, char TrimChar);
	bool		StringTrimRight(std::string& String, const ArrayBridge<char>& TrimAnyChars);
	inline bool	StringTrimRight(std::string& String, const ArrayBridge<char>&& TrimAnyChars) {	return StringTrimRight(String, TrimAnyChars);	}
	bool		StringTrimLeft(std::string& Haystack,const std::string& Prefix,bool CaseSensitive);
	bool		StringTrimRight(std::string& Haystack,const std::string& Suffix,bool CaseSensitive);

	std::string	StringPopUntil(std::string& Haystack,char Delim,bool KeepDelim=false,bool PopDelim=false);
	std::string	StringPopUntil(std::string& Haystack,std::function<bool(char)> IsDelim,bool KeepDelim=false,bool PopDelim=false);
	std::string	StringPopRight(std::string& Haystack,char Delim,bool KeepDelim=false,bool PopDelim=false);
	std::string	StringPopRight(std::string& Haystack,std::function<bool(char)> IsDelim,bool KeepDelim=false,bool PopDelim=false);

	bool		StringReplace(std::string& str,const std::string& from,const std::string& to);
	bool		StringReplace(ArrayBridge<std::string>& str,const std::string& from,const std::string& to);
	bool		StringReplace(ArrayBridge<std::string>&& str,const std::string& from,const std::string& to);

	std::string	ArrayToString(const ArrayBridge<char>& Array,size_t Limit=0);
	void		ArrayToString(const ArrayBridge<char>& Array,std::ostream& String,size_t Limit=0);
	std::string	ArrayToString(const ArrayBridge<uint8>& Array,size_t Limit=0);
	void		ArrayToString(const ArrayBridge<uint8>& Array,std::ostream& String,size_t Limit=0);
	inline void	ArrayToString(const ArrayBridge<uint8>&& Array, std::ostream& String, size_t Limit = 0) { ArrayToString(Array, String, Limit); }
	inline void	ArrayToString(const ArrayBridge<char>&& Array, std::ostream& String, size_t Limit = 0) { ArrayToString(Array, String, Limit); }

	void		StringToArray(std::string String,ArrayBridge<char>& Array);
	inline void	StringToArray(std::string String,ArrayBridge<char>&& Array)	{	StringToArray( String, Array );	}
	void		StringToArray(std::string String,ArrayBridge<uint8>& Array);
	inline void	StringToArray(std::string String,ArrayBridge<uint8>&& Array)	{	StringToArray( String, Array );	}
	void		StringToBuffer(const char* Source,char* Buffer,size_t BufferSize);
	inline void	StringToBuffer(const std::string& Source,char* Buffer,size_t BufferSize)	{	StringToBuffer( Source.c_str(), Buffer, BufferSize );	}
	template <size_t BUFFERSIZE>
	void		StringToBuffer(const char* Source,char (& Buffer)[BUFFERSIZE])				{	StringToBuffer( Source, Buffer, BUFFERSIZE );	}
	template <size_t BUFFERSIZE>
	void		StringToBuffer(const std::string&Source,char (& Buffer)[BUFFERSIZE])		{	StringToBuffer( Source.c_str(), Buffer, BUFFERSIZE );	}

	std::string	StreamToString(std::ostream& Stream);	//	windows
	std::string	StreamToString(std::stringstream&& Stream);	//	osx
	std::string	StreamToString(std::istream& Stream);	//	osx
	inline void	StringStreamClear(std::stringstream& Stream)	{	Stream.str(std::string());	}	//	not .clear, not .empty

	void		SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String,bool IncludeEmpty=true);
	void		SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String,bool IncludeEmpty=true);
	void		SplitStringLines(std::function<bool(const std::string&,const char&)> Callback,const std::string& String,bool IncludeEmpty=true);

	bool		IsUtf8String(const std::string& String);
	bool		IsUtf8Char(char c);
	uint8		HexToByte(char Hex);
	uint8		HexToByte(char HexA,char HexB);
	void		ByteToHex(uint8 Byte,std::ostream& String);
	void		ByteToHex(uint8 Byte,char& Stringa,char& Stringb);
	std::string	ByteToHex(uint8 Byte);

	std::string	ResolveUrl(const std::string& BaseUrl,const std::string& Path);	//	work out the full path of Path from the base url. if it starts from / then use the server. if it starts with protocol, don't modify, otherwise place in directory
	std::string	ExtractServerFromUrl(const std::string& Url);
	void		SplitHostnameAndPort(std::string& Hostname,uint16& Port,const std::string& HostnameAndPort);
	void		SplitUrl(const std::string& Url,std::string& Protocol,std::string& Hostname,uint16& Port,std::string& Path);
	std::string	GetUrlPath(const std::string& Url);
	std::string	GetUrlHostname(const std::string& Url);
	std::string	GetUrlProtocol(const std::string& Url);
	void		SplitUrlPathVariables(std::string& Path,std::map<std::string,std::string>& Variables);
	void		UriDecode(std::string& String);
	
	std::wstring	StringToWString(const std::string& s);
	std::string		WStringToString(const std::wstring& w);
	
	template<typename TYPE>
	bool			StringToType(TYPE& Out,const std::string& String);
	template<typename TYPE>
	TYPE			StringToType(const std::string& String,const TYPE& Default);

	//	this version throws
	template<typename TYPE>
	TYPE			StringToType(const std::string& String);
	bool			StringToUnsignedInteger(size_t& IntegerOut,const std::string& String);

	//	max size of vector (ie. buffer array/remote array) dictates expected size
	template<typename TYPE>
	inline bool		StringParseVecNx(const std::string& String,ArrayBridge<TYPE>&& Vector);

	std::string		DataToHexString(const ArrayBridge<uint8>&& Data,int MaxBytes=-1);
	void			DataToHexString(std::ostream& String,const ArrayBridge<uint8>& Data,int MaxBytes=-1);
	inline void		DataToHexString(std::ostream& String,const ArrayBridge<uint8>&& Data,int MaxBytes=-1)	{	DataToHexString( String, Data, MaxBytes );	}

	template <size_t BUFFERSIZE>
	void			PushStringArray(ArrayBridge<std::string>& Destination,const char* (& Source)[BUFFERSIZE])
	{
		for ( int i=0;	i<BUFFERSIZE;	i++ )
			Destination.PushBack( Source[i] );
	}
	template <size_t BUFFERSIZE>
	void			PushStringArray(ArrayBridge<std::string>&& Destination,const char* (& Source)[BUFFERSIZE])	{	PushStringArray( Destination, Source );	}
};

#if defined(__OBJC__)
@class NSString;
namespace Soy
{
	NSString*	StringToNSString(const std::string& String);
	std::string	NSStringToString(NSString* String);
	std::string	CFStringToString(CFStringRef String);
	std::string	NSErrorToString(NSError* Error);
	std::string	NSErrorToString(NSException* Exception);
	void		NSDictionaryToStrings(ArrayBridge<std::pair<std::string,std::string>>&& Elements,NSDictionary* Dictionary);
	std::string	NSDictionaryToString(NSDictionary* Dictionary);
	std::string	NSDictionaryToString(CFDictionaryRef Dictionary);
};
#endif



class Soy::TPushPopStreamSettings
{
public:
	TPushPopStreamSettings(std::ostream& Stream) :
		mPushedFlags	( Stream.flags() ),
		mStream			( Stream )
	{
	}
	~TPushPopStreamSettings()
	{
		mStream.flags( mPushedFlags );
	}
	
	std::ostream&				mStream;
	std::ios_base::fmtflags		mPushedFlags;
};




template<typename TYPE>
inline bool Soy::StringToType(TYPE& Out,const std::string& String)
{
	std::stringstream s( String );
	s >> Out;
	if ( s.fail() )
		return false;

	return true;
}


template<typename TYPE>
inline TYPE Soy::StringToType(const std::string& String)
{
	std::stringstream s( String );
	TYPE Out;
	s >> Out;

	if ( s.fail() )
	{
		std::stringstream Error;
		Error << "Failed to parse " << String << " into " << Soy::GetTypeName<TYPE>();
		throw Soy::AssertException(Error.str());
	}
	return Out;
}


template<>
bool Soy::StringToType(int& Out,const std::string& String);

	
template<typename TYPE>
inline TYPE Soy::StringToType(const std::string& String,const TYPE& Default)
{
	TYPE Value;
	if ( !StringToType( Value, String ) )
		return Default;
	return Value;
}

template<typename TYPE>
inline std::string Soy::StringJoin(const ArrayBridge<TYPE>& Elements,const std::string& Glue)
{
	std::stringstream Stream;
	for ( int i=0;	i<Elements.GetSize();	i++ )
	{
		auto& Element = Elements[i];
		Stream << (Element);
		
		//	look out for bad pushes
		if ( Stream.bad() )
		{
			std::stringstream Error;
			Error << "string << with " << Soy::GetTypeName<TYPE>() << " error'd";
			throw Soy::AssertException(Error);
			break;
		}
		
		if ( i != Elements.GetSize()-1 )
			Stream << Glue;
	}
	return Stream.str();
}

template <size_t BUFFERSIZE>
inline bool Soy::StringEndsWith(const std::string& Haystack,const char* (& Needles)[BUFFERSIZE], bool CaseSensitive)
{
	for ( int i=0;	i<BUFFERSIZE;	i++ )
		if ( StringEndsWith( Haystack, Needles[i], CaseSensitive ) )
			return true;
	return false;
}



template<typename TYPE>
inline bool Soy::StringParseVecNx(const std::string& instr,ArrayBridge<TYPE>&& Components)
{
	auto Append = [&Components](const std::string& Valuestr,const char& Delim)
	{
		TYPE Value;
		if ( !Soy::StringToType( Value, Valuestr ) )
			return false;
		if ( Components.IsFull() )
			return false;
		Components.PushBack( Value );
		return true;
	};
	if ( !Soy::StringSplitByMatches( Append, instr, Soy::VecNXDelins, false ) )
		return false;
	
	//	didn;t find enough components
	if ( !Components.IsFull() )
		return false;

	return true;
}

