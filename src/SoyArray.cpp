#include "array.hpp"
#include "bufferarray.hpp"
#include "heaparray.hpp"


bool TArrayReader::Read(uint8& Pop)
{
	typedef uint8 TYPE;
	if ( mOffset+sizeof(TYPE) > mArray.GetDataSize() )
		return false;
	auto& Data = *reinterpret_cast<const TYPE*>( &mArray[mOffset] );
	Pop = Data;
	mOffset += sizeof(TYPE);
	return true;
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

bool TArrayReader::ReadCompare(ArrayBridge<char>& Match)
{
	Array<char> Pop( Match.GetSize() );
	auto PopBridge = GetArrayBridge( Pop );
	if ( !Read( PopBridge ) )
		return false;
	
	assert( Match.GetSize() == Pop.GetSize() );
	
	return 0==memcmp( Pop.GetArray(), Match.GetArray(), Pop.GetDataSize() );
}


