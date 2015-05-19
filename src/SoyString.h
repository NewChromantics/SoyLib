#pragma once


#include <vector>
#include <string>
#include <sstream>
#include "memheap.hpp"

//	foraward declarations
template<typename TYPE>
class ArrayBridge;

namespace Soy
{
	class AssertException;
	//bool	Assert(bool Condition, std::stringstream&& ErrorMessage) throw(AssertException);
	bool	Assert(bool Condition, std::stringstream&& ErrorMessage);
};

//	string that uses a prmem::Heap
//	not compatible with std::string, but maybe we can do an easy conversion func
//	todo: replace heapbridge, with an arraybridge!
namespace Soy
{
	typedef std::basic_string<char, std::char_traits<char>, prmem::HeapBridge<char> > HeapString;
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
inline bool Soy::StringToType(int& Out,const std::string& String)
{
	Out = std::stoi( String );
	return true;
}

	
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
