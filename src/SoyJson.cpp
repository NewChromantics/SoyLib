#include "SoyJson.h"


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

bool TJsonWriter::Push(const char* Name,const char* Value)
{
	Open();
	
	if ( mElementCount > 0 )
		mStream << ",";
	
	//	gr: encode control characters etc
	//	string in quotes!
	mStream << '"' << Name << '"' << ":" << '"';
 
	//	write value as we encode
	do
	{
		switch ( *Value )
		{
			case '"':	mStream << "\\\"";	break;
			case '\t':	mStream << "\\t";	break;
			case '\r':	mStream << "\\r";	break;
			case '\n':	mStream << "\\n";	break;
			case '\f':	mStream << "\\f";	break;
			case '\b':	mStream << "\\b";	break;
			case '\\':	mStream << "\\\\";	break;
			case '/':	mStream << "\\/";	break;

			default:
				mStream << *Value;
				break;
		}
	}
	while ( *(++Value) );
	
	mStream << '"';
	
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


