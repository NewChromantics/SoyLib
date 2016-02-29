#include "SoyMetal.h"
#include <SoyDebug.h>
#include <SoyString.h>
#include <Metal/Metal.h>

namespace Metal
{
	std::mutex						DevicesLock;
	Array<std::shared_ptr<TDevice>>	Devices;
}



void Metal::EnumDevices(ArrayBridge<std::shared_ptr<TDevice>>&& Devices)
{
	//	refresh list
	std::lock_guard<std::mutex> Lock( DevicesLock );
	
	NSArray<id<MTLDevice>>* MtlDevices = MTLCopyAllDevices();
	for ( int d=0;	MtlDevices && d<[MtlDevices count];	d++ )
	{
		MTLDevice* MtlDevice = [MtlDevices objectAtIndex:d];
		if ( !MtlDevice )
		{
			std::Debug << "Warning: Metal device " << d << "/" << ([MtlDevices count]) << " null " << std::endl;
			continue;
		}

		
}


