#pragma once


#include <vector>
#include <string>
#include <sstream>
#include "MemHeap.hpp"
#include "SoyAssert.h"
#include "SoyMath.h"	//	for vec <-> string funcs

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
	
	bool		StringReplace(std::string& str,const std::string& from,const std::string& to);
	bool		StringReplace(ArrayBridge<std::string>& str,const std::string& from,const std::string& to);
	bool		StringReplace(ArrayBridge<std::string>&& str,const std::string& from,const std::string& to);

	std::string	ArrayToString(const ArrayBridge<char>& Array,size_t Limit=0);
	void		ArrayToString(const ArrayBridge<char>& Array,std::ostream& String,size_t Limit=0);
	std::string	ArrayToString(const ArrayBridge<uint8>& Array,size_t Limit=0);
	void		ArrayToString(const ArrayBridge<uint8>& Array,std::ostream& String,size_t Limit=0);
	inline void	ArrayToString(const ArrayBridge<uint8>&& Array,std::ostream& String,size_t Limit=0)	{	ArrayToString( Array, String, Limit );	}
	
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

	std::string		FourCCToString(uint32 Fourcc);	//	on IOS, don't forget CFSwapInt32BigToHost()
	
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
		if ( !Soy::Assert( !Stream.bad(), std::stringstream() << "string << with " << Soy::GetTypeName<TYPE>() << " error'd" ) )
			break;

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


//	gr: in string rather than math to reduce math's dependancies.
//		this header already requires array's so here it is
template<typename TYPE>
inline std::istream& operator>>(std::istream &in,vec2x<TYPE> &out)
{
	std::string instr;
	in >> instr;
	if ( in.fail() )
		return in;
	
	BufferArray<TYPE,2> VecArray;
	if ( !Soy::StringParseVecNx( instr, GetArrayBridge( VecArray ) ) )
	{
		in.setstate( std::ios::failbit );
		return in;
	}

	out.x = VecArray[0];
	out.y = VecArray[1];
	
	return in;
}



template<typename TYPE>
inline std::istream& operator>>(std::istream &in,vec3x<TYPE> &out)
{
	std::string instr;
	in >> instr;
	if ( in.fail() )
		return in;
	
	BufferArray<TYPE,3> VecArray;
	if ( !Soy::StringParseVecNx( instr, GetArrayBridge( VecArray ) ) )
	{
		in.setstate( std::ios::failbit );
		return in;
	}
	
	out.x = VecArray[0];
	out.y = VecArray[1];
	out.z = VecArray[2];
	
	return in;
}

template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out,const vec2x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y;
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix2x1 &in)
{
	out << in.x() << Soy::VecNXDelins[0] << in.y();
	return out;
}

template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out,const vec3x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.z;
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix3x1 &in)
{
	out << in.x() << Soy::VecNXDelins[0] << in.y() << Soy::VecNXDelins[0] << in.z();
	return out;
}

template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out,const vec4x<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.z << Soy::VecNXDelins[0] << in.w;
	return out;
}

namespace Soy
{
template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out,const Soy::Rectx<TYPE> &in)
{
	out << in.x << Soy::VecNXDelins[0] << in.y << Soy::VecNXDelins[0] << in.w << Soy::VecNXDelins[0] << in.h;
	return out;
}
}


namespace Soy
{
template<typename TYPE>
inline std::ostream& operator<<(std::ostream &out,const Soy::Boundsx<TYPE> &in)
{
	out << in.min << Soy::VecNXDelins[1] << in.max;
	return out;
}
}


inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix4x1 &in)
{
	out << in.x() << Soy::VecNXDelins[0] << in.y() << Soy::VecNXDelins[0] << in.z() << Soy::VecNXDelins[0] << in.w();
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix4x4 &in)
{
	auto d = Soy::VecNXDelins[0];

	//	output row by row
	//	mat(row,col)
	out << in(0,0) << d << in(0,1) << d << in(0,2) << d << in(0,3) << d;
	out << in(1,0) << d << in(1,1) << d << in(1,2) << d << in(1,3) << d;
	out << in(2,0) << d << in(2,1) << d << in(2,2) << d << in(2,3) << d;
	out << in(3,0) << d << in(3,1) << d << in(3,2) << d << in(3,3);
	
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix3x3 &in)
{
	auto d = Soy::VecNXDelins[0];
	
	//	output row by row
	//	mat(row,col)
	out << in(0,0) << d << in(0,1) << d << in(0,2) << d;
	out << in(1,0) << d << in(1,1) << d << in(1,2) << d;
	out << in(2,0) << d << in(2,1) << d << in(2,2);
	
	return out;
}


inline std::ostream& operator<<(std::ostream &out,const float4x4&in)
{
	out << in.rows[0] << Soy::VecNXDelins[0]
	<< in.rows[1] << Soy::VecNXDelins[0]
	<< in.rows[2] << Soy::VecNXDelins[0]
	<< in.rows[3];
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const float3x3&in)
{
	out << in.GetRow(0) << Soy::VecNXDelins[0]
	<< in.GetRow(1) << Soy::VecNXDelins[0]
	<< in.GetRow(2);
	return out;
}
