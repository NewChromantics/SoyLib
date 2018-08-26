#include "SoyJson.h"
#include "HeapArray.hpp"
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

void Json::IsValidJson(const std::string& Json)
{
	Soy::Assert( !Json.empty(), "Invalid json, empty" );
	Soy::Assert( *Json.begin()=='{', "Invalid json does not begin with {" );
	
	//	count opening & closing braces
	int BraceCounter = 0;
	for ( int i=0;	i<Json.length();	i++ )
	{
		if ( Json[i] == '{' )
			BraceCounter++;
		if ( Json[i] == '}' )
			BraceCounter--;
	}
	
	Soy::Assert( BraceCounter==0, "Json braces don't balance" );
}

std::string Json::EscapeString(const char* RawString)
{
	std::stringstream Stream;

	Stream << '"';
	while ( *RawString )
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
		RawString++;
	}

	Stream << '"';

	return Stream.str();
}

std::string Json::EscapeString(const std::string& RawString)	
{	
	if ( RawString.empty() )
		return "\"\"";
	return EscapeString( RawString.c_str() );	
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
	
	mStream << '{';
	if ( mPrettyFormatting )
		mStream << "\n";
	
	mOpen = true;
}

void TJsonWriter::Close()
{
	if ( !mOpen )
		return;
	
	if ( mPrettyFormatting )
		mStream << '\n';
	mStream << '}';
	
	mOpen = false;
}

template<typename TYPE>
void PushElement(const char* Name,const TYPE& Value,int& mElementCount,std::ostream& mStream,TJsonWriter& Json,bool mPrettyFormatting)
{
	Json.Open();
	
	if ( mElementCount > 0 )
	{
		mStream << ',';
		if ( mPrettyFormatting )
			mStream << '\n';
	}

	if ( mPrettyFormatting )
		mStream << '\t';
	
	mStream << Json::EscapeString(Name) << ':' << Value;
	mElementCount++;
}
	
void TJsonWriter::Push(const char* Name,const std::string& Value)
{
	Push( Name, Value.c_str() );
}

void TJsonWriter::Push(const char* Name,const char* Value,bool EscapeString)
{
	if ( EscapeString )
	{
		std::string ValueEscaped = Json::EscapeString( Value );
		PushElement( Name, ValueEscaped, mElementCount, mStream, *this, mPrettyFormatting );
	}
	else
	{
		PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
	}
}

void TJsonWriter::PushStringRaw(const char* Name,const std::string& Value)
{
	PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
}

void TJsonWriter::Push(const char* Name,const uint64_t& Value)
{
	PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
}

void TJsonWriter::Push(const char* Name,const int64_t& Value)
{
	PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
}

void TJsonWriter::Push(const char* Name,const float& Value)
{
	PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
}

void TJsonWriter::Push(const char* Name,const TJsonWriter& Value)
{
	PushElement( Name, Value, mElementCount, mStream, *this, mPrettyFormatting );
}

//	attempt to merge this json into this object at the same level
void TJsonWriter::MergeJson(const std::string& Json)
{
	//	extract the structure
	auto MergeJson = Json;
	
	char TrimLeft[] = {'{','\t','\n'};
	char TrimRight[] = {'}','\t','\n'};
	
	Soy::StringTrimLeft( MergeJson, GetArrayBridge(GetRemoteArray(TrimLeft)) );
	Soy::StringTrimRight( MergeJson, GetArrayBridge(GetRemoteArray(TrimRight)) );
	
	Open();
	
	if ( mElementCount > 0 )
	{
		mStream << ',';
		if ( mPrettyFormatting )
			mStream << '\n';
	}
	
	if ( mPrettyFormatting )
		mStream << '\t';
	
	mStream << MergeJson;
	mElementCount++;
}

std::string TJsonWriter::GetString() const
{
	std::stringstream str;
	str << *this;
	return str.str();
}
