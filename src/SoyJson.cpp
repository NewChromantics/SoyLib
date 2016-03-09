#include "SoyJson.h"
#include "heaparray.hpp"
#include "SoyDebug.h"

void Json::UnitTest()
{
	TJsonWriter Json;
	
	Json.Push("a","one");
	Json.Push("b",2);
	Json.Push("c",3.4f);
	
	Array<int> Ints;
	Ints.PushBack(5);
	Ints.PushBack(67);
	Ints.PushBack(8910);
	Json.Push("d", GetArrayBridge(Ints) );
	
	Json.Push("special chars", "\n\t\r/\\" );
	
	TJsonWriter Empty;
	Json.Push("EmptyJson", Empty );
	
	TJsonWriter Other;
	Other.Push("x",0);
	Other.Push("y",1);
	Json.Push("other",Other);
	
	std::Debug << Json << std::endl;
}


std::string Json::EscapeString(const char* RawString)
{
	std::stringstream Stream;

	do
	{
		switch ( *RawString )
		{
			case '"':	Stream << "\\\"";	break;
			case '\t':	Stream << "\\t";	break;
			case '\r':	Stream << "\\r";	break;
			case '\n':	Stream << "\\n";	break;
			case '\f':	Stream << "\\f";	break;
			case '\b':	Stream << "\\b";	break;
			case '\\':	Stream << "\\\\";	break;
			case '/':	Stream << "\\/";	break;

			default:
				Stream << *RawString;
				break;
		}
	}
	while ( *(++RawString) );

	return Stream.str();
}

std::ostream& operator<< (std::ostream &out,const TJsonWriter &in)
{
	auto& Json = const_cast<TJsonWriter&>(in);
	Json.Close();
	out << Json.mStream.str();
	return out;
}


void TJsonWriter::Open()
{
	if ( mOpen )
		return;
	mStream << "{";
	mOpen = true;
}

void TJsonWriter::Close()
{
	if ( !mOpen )
		return;
	mStream << "}";
	mOpen = false;
}

template<typename TYPE>
bool PushElement(const char* Name,const TYPE& Value,int& mElementCount,std::ostream& mStream,TJsonWriter& Json)
{
	Json.Open();
	
	if ( mElementCount > 0 )
		mStream << ",";
	
	mStream << '"' << Name << '"' << ":" << Value;
	mElementCount++;
	return true;
}
	
bool TJsonWriter::Push(const char* Name,const std::string& Value)
{
	return Push( Name, Value.c_str() );
}

bool TJsonWriter::Push(const char* Name,const char* Value,bool EscapeString)
{
	Open();
	
	if ( mElementCount > 0 )
		mStream << ",";
	
	//	gr: encode control characters etc
	if ( EscapeString )
	{

		//	string in quotes!
		mStream << '"' << Name << '"' << ":" << '"';
 
		//	write value as we encode
		std::string ValueEscaped = Json::EscapeString( Value );
		mStream << ValueEscaped;

	
		mStream << '"';
	}

	mElementCount++;
	
	return true;
}

bool TJsonWriter::PushStringRaw(const char* Name,const std::string& Value)
{
	return PushElement( Name, Value, mElementCount, mStream, *this );
}

bool TJsonWriter::Push(const char* Name,const int& Value)
{
	return PushElement( Name, Value, mElementCount, mStream, *this );
}

bool TJsonWriter::Push(const char* Name,const float& Value)
{
	return PushElement( Name, Value, mElementCount, mStream, *this );
}

bool TJsonWriter::Push(const char* Name,const TJsonWriter& Value)
{
	return PushElement( Name, Value, mElementCount, mStream, *this );
}


