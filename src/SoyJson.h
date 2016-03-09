#pragma once

#include <sstream>
#include "array.hpp"


namespace Json
{
	void				UnitTest();
	std::string			EscapeString(const char* RawString);
	inline std::string	EscapeString(const std::string& RawString)	{	return RawString.empty() ? std::string() : EscapeString( RawString.c_str() );	}
}

//	gr: turn this into a read & write protocol...if we abstracted it to objects
class TJsonWriter
{
public:
	TJsonWriter() :
		mOpen				( false ),
		mElementCount		( 0 ),
		mAllocatedStream	( new std::stringstream ),
		mStream				( *mAllocatedStream )
	{
	}
	explicit TJsonWriter(std::stringstream& Stream) :
		mOpen			( false ),
		mElementCount	( 0 ),
		mStream			( Stream )
	{
		Open();
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
	
	template<typename TYPE>
	bool	Push(const char* Name,const ArrayBridge<TYPE>&& Array);

	bool	PushJson(const char* Name,const ArrayBridge<std::string>&& Array);
	bool	PushJson(const char* Name,const std::string& Value)			{	return Push( Name, Value.c_str(), false );	}

private:
	bool	PushStringRaw(const char* Name,const std::string& Value);
	bool	Push(const char* Name,const char* Value,bool EscapeString);


private:
	std::shared_ptr<std::stringstream>	mAllocatedStream;
	bool	mOpen;	//	needs closing!
	int		mElementCount;

public:
	std::stringstream&					mStream;	//	must be declared AFTER mAllocatedStream!
};

std::ostream& operator<< (std::ostream &out,const TJsonWriter &in);


template<typename TYPE>
inline bool TJsonWriter::Push(const char* Name,const ArrayBridge<TYPE>&& Array)
{
	std::stringstream Value;
	Value << "[";
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		//	gr: this needs to escape strings
		auto& Element = Array[i];
		if ( i > 0 )
			Value << ",";
		Value << Element;
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
		Value << Json::EscapeString(Element.c_str());
	}
	Value << "]";
	return PushStringRaw( Name, Value.str() );
}

