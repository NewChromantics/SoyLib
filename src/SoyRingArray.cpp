#include "SoyRingArray.h"
#include "SoyDebug.h"


int DoRingArrayTest()
{
	RingArray<Array<int>> Ring(10);
	
	//	overflow
	for ( int i=0;	i<20;	i++ )
		Ring.PushBack(i);
	
	BufferArray<int,5> More;
	More.PushBack(101);
	More.PushBack(102);
	More.PushBack(103);
	More.PushBack(104);
	More.PushBack(105);
	Ring.PushBack( GetArrayBridge(More) );
	
	//
	int Zero,one,two;
	Ring.PopFront(Zero);
	Ring.PopFront(one);
	Ring.PopFront(two);
	BufferArray<int,50> PopTheRest;
	Ring.PopFront( 25-3, GetArrayBridge(PopTheRest) );
	
	
	std::Debug << "Zero = " << Zero << std::endl;
	
	return Zero;
}

