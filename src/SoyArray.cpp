#include "SoyArray.h"
#include "array.hpp"
#include "bufferarray.hpp"
#include "heaparray.hpp"
#include "RemoteArray.h"

std::string	SoyArray::OnCheckBoundsError(size_t Index,size_t Size,const std::string& Typename)
{
	std::stringstream Error;
	Error << "Array<" << Typename << "> Index " << Index << "/" << Size << " out of bounds";
	return Error.str();
}

bool TArrayReader::ReadReverse(ArrayBridge<char>& Pop)
{
	assert( Pop.GetDataSize() > 0 );
	for ( int i=0;	i<Pop.GetDataSize();	i++,mOffset++ )
	{
		//	out of data
		if ( mOffset >= mArray.GetDataSize() )
			return false;
		
		Pop[Pop.GetDataSize()-1-i] = mArray[mOffset];
	}
	return true;
}
/*
bool TArrayReader::Read(ArrayBridge<char>& Pop)
{
	//	allow zero length? this is assuming programming error
	assert( Pop.GetDataSize() > 0 );
	for ( int i=0;	i<Pop.GetDataSize();	i++,mOffset++ )
	{
		//	out of data
		if ( mOffset >= mArray.GetDataSize() )
			return false;
		
		Pop[i] = mArray[mOffset];
	}
	return true;
}
*/
bool TArrayReader::ReadCompare(ArrayBridge<char>& Match)
{
	Array<char> Pop( Match.GetSize() );
	auto PopBridge = GetArrayBridge( Pop );
	if ( !ReadArray( PopBridge ) )
		return false;
	
	assert( Match.GetSize() == Pop.GetSize() );
	
	return 0==memcmp( Pop.GetArray(), Match.GetArray(), Pop.GetDataSize() );
}


