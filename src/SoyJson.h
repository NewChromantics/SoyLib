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

	void		Open();
	void		Close();
	std::string	GetString() const;		//	bake json
	
	template<typename TYPE>
	void	Push(const char* Name,const TYPE& Value);
	void	Push(const char* Name,const std::string& Value);
	void	Push(const char* Name,const std::string&& Value)		{	return Push( Name, Value );	}
	void	Push(const char* Name,const std::stringstream&& Value)	{	return Push( Name, Value.str() );	}
	void	Push(const char* Name,const std::stringstream& Value)	{	return Push( Name, Value.str() );	}
	void	Push(const char* Name,const char* Value)				{	return Push( Name, Value, true );	}
	void	Push(const char* Name,const size_t& Value);
	void	Push(const char* Name,const ssize_t& Value);
	void	Push(const char* Name,const float& Value);
	void	Push(const char* Name,const int& Value)			{	Push( Name, static_cast<ssize_t>( Value ) );	}
	void	Push(const char* Name,const int8_t& Value)		{	Push( Name, static_cast<ssize_t>( Value ) );	}
	void	Push(const char* Name,const int16_t& Value)		{	Push( Name, static_cast<ssize_t>( Value ) );	}
	//void	Push(const char* Name,const int32_t& Value)		{	Push( Name, static_cast<ssize_t>( Value ) );	}
	void	Push(const char* Name,const int64_t& Value)		{	Push( Name, static_cast<ssize_t>( Value ) );	}
	void	Push(const char* Name,const uint8_t& Value)		{	Push( Name, static_cast<size_t>( Value ) );	}
	void	Push(const char* Name,const uint16_t& Value)	{	Push( Name, static_cast<size_t>( Value ) );	}
	void	Push(const char* Name,const uint32_t& Value)	{	Push( Name, static_cast<size_t>( Value ) );	}
	void	Push(const char* Name,const uint64_t& Value)	{	Push( Name, static_cast<size_t>( Value ) );	}
	void	Push(const char* Name,const bool& Value)	{	return Push( Name, Value ? "true" : "false" );	}
	void	PushNull(const char* Name)					{	return Push( Name, "null" );	}
	void	Push(const char* Name,const TJsonWriter& Value);
	

	//	need to overload with ArrayBridgeDef if we have a generic template<>
	//template<typename TYPE>
	//bool	Push(const char* Name,const ArrayBridge<TYPE>&& Array);
	template<typename TYPE>
	void	Push(const char* Name,const ArrayBridgeDef<TYPE>&& Array);

	void	PushJson(const char* Name,const ArrayBridge<std::string>&& Array);
	void	PushJson(const char* Name,const std::string& Value)			{	Json::IsValidJson(Value);	return Push( Name, Value.c_str(), false );	}

	template<typename TYPE>
	void	Push(const std::string& Name,const TYPE& Value)
	{
		Push( Name.c_str(), Value );
	}
	
	//	attempt to merge this json into this object at the same level
	void	MergeJson(const std::string& Json);
	
private:
	void	PushStringRaw(const char* Name,const std::string& Value);
	void	Push(const char* Name,const char* Value,bool EscapeString);


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
inline void TJsonWriter::Push(const char* Name,const TYPE& Value)
{
	std::stringstream Stream;
	Stream << Value;
	Push( Name, Stream );
}

template<typename TYPE>
inline void TJsonWriter::Push(const char* Name,const ArrayBridgeDef<TYPE>&& Array)
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
	PushStringRaw( Name, Value.str() );
}


inline void TJsonWriter::PushJson(const char* Name,const ArrayBridge<std::string>&& Array)
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
	PushStringRaw( Name, Value.str() );
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
