#pragma once

#include <string>
#include <SoyThread.h>
#include <SoyPixels.h>

#include <cuda.h>

#define CUDA_INVALID_VALUE	0


namespace Cuda
{
	class TContext;
	class TDevice;
	class TBuffer;
	class TStream;

	std::string	GetEnumString(CUresult Error);
	bool		IsOkay(CUresult Error, const std::string& Context, bool ThrowException = true);
	#define Cuda_IsOkay(Error)	Cuda::IsOkay( (Error), __func__ )
	void		GetDevices(ArrayBridge<TDevice>&& Metas);
}



template<typename TYPE>
inline void MemsetZero(TYPE& Value)
{
	memset(&Value, 0, sizeof(TYPE));
}



class Cuda::TDevice
{
public:
	TDevice() :
		mIndex	( -1 ),
		mHandle	( CUDA_INVALID_VALUE )
	{
	}
	
	TDevice(size_t DeviceIndex,CUdevice Handle,const std::string& Name) :
		mIndex	( DeviceIndex),
		mHandle	( Handle ),
		mName	( Name )
	{
	}

public:
	ssize_t		mIndex;
	CUdevice	mHandle;
	std::string	mName;
};



class Cuda::TContext : public PopWorker::TContext
{
public:
	TContext();
	~TContext();

	virtual bool	Lock() override;
	virtual void	Unlock() override;

	CUcontext		GetContext()	{	return mContext;	}

private:
	CUcontext		mContext;
	TDevice			mDevice;
};


class Cuda::TStream
{
public:
	TStream(bool Blocking=true);
	~TStream();

public:
	CUstream			mStream;
};



class Cuda::TBuffer
{
public:
	TBuffer(size_t DataSize);
	~TBuffer();

	void				Read(CUdeviceptr DeviceData,TStream& Stream,bool Block);
	uint8*				GetData()	{	return mData;	}

public:
	//	cuda-allocated host data
	uint8*				mData;
	size_t				mAllocSize;
};

