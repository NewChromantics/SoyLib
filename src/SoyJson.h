#pragma once

#include <sstream>
#include "array.hpp"


namespace Json
{
	void				UnitTest();

	template<typename TYPE>	std::string	ValueToString(const TYPE& Value);
	template<> std::string				ValueToString(const bool& Value);
	template<> std::string				ValueToString(const int& Value);
	template<> std::string				ValueToString(const float& Value);

	//	gr: escaping also adds quotes; Cat turns into "Cat"
	std::string			EscapeString(const char* RawString);
	std::string			EscapeString(const std::string& RawString);
	
	void				IsValidJson(const std::string& Json);
}

//	gr: turn this into a read & write protocol...if we abstracted it to objects
//	really want to abstract this entirely into a parse-on-demand system though for parallel parsing and random seeking
class TJsonWriter
{
public:
	TJsonWriter() :
		mOpen				( false ),
		mElementCount		( 0 ),
		mAllocatedStream	( new std::stringstream ),
		mStream				( *mAllocatedStream ),
		mPrettyFormatting	( true )
	{
	}
	explicit TJsonWriter(std::stringstream& Stream) :
		mOpen				( false ),
		mElementCount		( 0 ),
		mStream				( Stream ),
		mPrettyFormatting	( true )
	{
		Open();
	}
	TJsonWriter(const TJsonWriter& That) :
		mOpen				( That.mOpen ),
		mElementCount		( That.mElementCount ),
		mPrettyFormatting	( That.mPrettyFormatting ),
		mAllocatedStream	( That.mAllocatedStream ),
		mStream				( That.mStream )	//	danger! alloc a stream and copy its contents
	{
	}
	
	~TJsonWriter()
	{
		if ( mOpen )
		{
			throw Soy::AssertException("Json was not closed");
		}
	}

	void	Open();
	void	Close();
	
	template<typename TYPE>
	bool	Push(const char* Name,const TYPE& Value);
	bool	Push(const char* Name,const std::string& Value);
	bool	Push(const char* Name,const std::string&& Value)		{	return Push( Name, Value );	}
	bool	Push(const char* Name,const std::stringstream&& Value)	{	return Push( Name, Value.str() );	}
	bool	Push(const char* Name,const std::stringstream& Value)	{	return Push( Name, Value.str() );	}
	bool	Push(const char* Name,const char* Value)				{	return Push( Name, Value, true );	}
	bool	Push(const char* Name,const int& Value);
	bool	Push(const char* Name,const float& Value);
	bool	Push(const char* Name,const bool& Value)	{	return Push( Name, Value ? "true" : "false" );	}
	bool	PushNull(const char* Name)					{	return Push( Name, "null" );	}
	bool	Push(const char* Name,const TJsonWriter& Value);
	

	//	need to overload with ArrayBridgeDef if we have a generic template<>
	//template<typename TYPE>
	//bool	Push(const char* Name,const ArrayBridge<TYPE>&& Array);
	template<typename TYPE>
	bool	Push(const char* Name,const ArrayBridgeDef<TYPE>&& Array);

	bool	PushJson(const char* Name,const ArrayBridge<std::string>&& Array);
	bool	PushJson(const char* Name,const std::string& Value)			{	Json::IsValidJson(Value);	return Push( Name, Value.c_str(), false );	}

private:
	bool	PushStringRaw(const char* Name,const std::string& Value);
	bool	Push(const char* Name,const char* Value,bool EscapeString);


private:
	std::shared_ptr<std::stringstream>	mAllocatedStream;
	bool	mOpen;	//	needs closing!
	int		mElementCount;
	bool	mPrettyFormatting;

public:
	std::stringstream&					mStream;	//	must be declared AFTER mAllocatedStream!
};

std::ostream& operator<< (std::ostream &out,const TJsonWriter &in);


template<typename TYPE>
inline TYPE EscapeType(const TYPE& Value)
{
	return Value;
}

template<>
inline std::string EscapeType(const std::string& Value)
{
	return Json::EscapeString(Value);
}

template<typename TYPE>
inline bool TJsonWriter::Push(const char* Name,const TYPE& Value)
{
	std::stringstream Stream;
	Stream << Value;
	return Push( Name, Stream );
}

template<typename TYPE>
inline bool TJsonWriter::Push(const char* Name,const ArrayBridgeDef<TYPE>&& Array)
{
	std::stringstream Value;
	Value << "[";
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		auto& Element = Array[i];
		if ( i > 0 )
			Value << ",";
		Value << EscapeType(Element);
	}
	Value << "]";
	return PushStringRaw( Name, Value.str() );
}


inline bool TJsonWriter::PushJson(const char* Name,const ArrayBridge<std::string>&& Array)
{
	std::stringstream Value;
	Value << "[";
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		auto& Element = Array[i];
		if ( i > 0 )
			Value << ",";
		
		Json::IsValidJson(Element);
		//	not escaped!
		Value << Element;
		
		if ( mPrettyFormatting )
			Value << '\n';
	}
	Value << "]";
	return PushStringRaw( Name, Value.str() );
}

	
template<typename TYPE>
inline std::string Json::ValueToString(const TYPE& Value)
{
	std::stringstream ValueStr;
	ValueStr << Value;
	return EscapeString( ValueStr.str() );
}
	
template<> 
inline std::string Json::ValueToString(const bool& Value)
{
	return Value ? "true" : "false";
}

template<> 
inline std::string Json::ValueToString(const int& Value)
{
	return ValueToString<float>( Value );
}

template<>
inline std::string Json::ValueToString(const float& Value)
{
	std::stringstream ValueStr;
	ValueStr << Value;
	return ValueStr.str();
}
