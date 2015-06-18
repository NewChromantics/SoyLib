#pragma once


#include <vector>
#include <string>
#include <sstream>
#include "memheap.hpp"
#include "SoyAssert.h"
#include "SoyMath.h"	//	for vec <-> string funcs


//	foraward declarations
template<typename TYPE>
class ArrayBridge;


namespace Soy
{
	//	Soy::lf
	//	like std::endl without the implicit flush. I prefer this over \n's
	template <class _CharT, class _Traits>
	inline _LIBCPP_INLINE_VISIBILITY std::basic_ostream<_CharT, _Traits>& lf(std::basic_ostream<_CharT, _Traits>& __os)
	{
		__os.put(__os.widen('\n'));
		return __os;
	}
	
	//	delineators for strings. First is used for output
	static const char* VecNXDelins = ",x";
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
	bool		StringMatches(const std::string& Haystack,const std::string& Needle, bool CaseSensitive);
	
	std::string	StringJoin(const std::vector<std::string>& Strings,const std::string& Glue);
	template<typename TYPE>
	std::string	StringJoin(const ArrayBridge<TYPE>& Elements,const std::string& Glue);
	void		StringSplitByString(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	void		StringSplitByString(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	bool		StringSplitByString(std::function<bool(const std::string&)> Callback,const std::string& String,const std::string& Delim,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	void		StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);
	bool		StringSplitByMatches(std::function<bool(const std::string&)> Callback,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty=true);

	bool		StringTrimLeft(std::string& String, char TrimChar);
	bool		StringTrimRight(std::string& String, const ArrayBridge<char>& TrimAnyChars);
	inline bool	StringTrimRight(std::string& String, const ArrayBridge<char>&& TrimAnyChars) {	return StringTrimRight(String, TrimAnyChars);	}

	std::string	ArrayToString(const ArrayBridge<char>& Array);
	void		ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String);
	
	void		StringToArray(std::string String,ArrayBridge<char>& Array);
	inline void	StringToArray(std::string String,ArrayBridge<char>&& Array)	{	StringToArray( String, Array );	}

	std::string	StreamToString(std::ostream& Stream);	//	windows
	std::string	StreamToString(std::stringstream&& Stream);	//	osx

	void		SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String);
	void		SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String);

	bool		IsUtf8String(const std::string& String);
	bool		IsUtf8Char(char c);
	
	template<typename TYPE>
	bool		StringToType(TYPE& Out,const std::string& String);
	template<typename TYPE>
	TYPE		StringToType(const std::string& String,const TYPE& Default);

	//	max size of vector (ie. buffer array/remote array) dictates expected size
	template<typename TYPE>
	inline bool	StringParseVecNx(const std::string& String,ArrayBridge<TYPE>&& Vector);
};

#if defined(__OBJC__)
@class NSString;
namespace Soy
{
	NSString*	StringToNSString(const std::string& String);
	std::string	NSStringToString(NSString* String);
};
#endif




template<typename TYPE>
inline bool Soy::StringToType(TYPE& Out,const std::string& String)
{
	std::stringstream s( String );
	s >> Out;
	if ( s.fail() )
		return false;

	return true;
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


template<typename TYPE>
inline bool Soy::StringParseVecNx(const std::string& instr,ArrayBridge<TYPE>&& Components)
{
	auto Append = [&Components](const std::string& Valuestr)
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

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix4x1 &in)
{
	out << in.x() << Soy::VecNXDelins[0] << in.y() << Soy::VecNXDelins[0] << in.z() << Soy::VecNXDelins[0] << in.w();
	return out;
}

inline std::ostream& operator<<(std::ostream &out,const Soy::Matrix4x4 &in)
{
	for ( int i=0;	i<in.kElements;	i++ )
	{
		if ( i > 0 )
			out << Soy::VecNXDelins[0];
		out << in[i];
	}
	
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

