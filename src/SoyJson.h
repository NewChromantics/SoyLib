#pragma once

#include <sstream>
#include "array.hpp"

namespace SoyJson
{
	void	UnitTest();
}

class TJsonWriter
{
public:
	TJsonWriter() :
		mOpen			( false ),
		mElementCount	( 0 )
	{
		Open();
	}

	void	Open();
	void	Close();
	
	bool	Push(const char* Name,const std::string& Value);
	bool	Push(const char* Name,const std::string&& Value)		{	return Push( Name, Value );	}
	bool	Push(const char* Name,const std::stringstream&& Value)	{	return Push( Name, Value.str() );	}
	bool	Push(const char* Name,const char* Value);
	bool	Push(const char* Name,const int& Value);
	bool	Push(const char* Name,const float& Value);
	bool	Push(const char* Name,const bool& Value)	{	return Push( Name, Value ? "true" : "false" );	}
	bool	PushNull(const char* Name)					{	return Push( Name, "null" );	}
	bool	Push(const char* Name,const TJsonWriter& Value);
	
	template<typename TYPE>
	bool	Push(const char* Name,const ArrayBridge<TYPE>&& Array);

private:
	bool	PushStringRaw(const char* Name,const std::string& Value);
	
public:
	std::stringstream	mStream;
	bool	mOpen;	//	needs closing!
	int		mElementCount;
};

std::ostream& operator<< (std::ostream &out,const TJsonWriter &in);


template<typename TYPE>
inline bool TJsonWriter::Push(const char* Name,const ArrayBridge<TYPE>&& Array)
{
	std::stringstream Value;
	Value << "[";
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		auto& Element = Array[i];
		if ( i > 0 )
			Value << ",";
		Value << Element;
	}
	Value << "]";
	return PushStringRaw( Name, Value.str() );
}
