#pragma once

#include "SoyThread.h"
#include "SortArray.h"
#include "SoyString.h"
#include "SoyEnum.h"
#include "SoyPixels.h"
#include "SoyUniform.h"



namespace Metal
{
	class TDevice;
	class TContext;
	class TContextThread;		//	job queue

	void		EnumDevices(ArrayBridge<std::shared_ptr<TDevice>>&& Devices);
};





class Metal::TDeviceMeta
{
public:
	TDeviceMeta()
	{
	}
	
public:
	std::string			mName;
};
std::ostream& operator<<(std::ostream &out,const Metal::TDeviceMeta& in);


class Metal::TDevice : public TDeviceMeta
{
public:
	TDevice();
	~TDevice();
	
	std::shared_ptr<TContext>		CreateContext();
	std::shared_ptr<TContextThread>	CreateContextThread(const std::string& Name);
	
	TDeviceMeta			GetMeta() const	{	return *this;	}
	
protected:
	id<MTLDevice>		mDevice;
};


class Metal::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext(TDevice& Device);
	~TContext();
	
	virtual bool	Lock() override;
	virtual void	Unlock() override;
	virtual bool	IsLocked(std::thread::id Thread) override;
	
protected:
	TDevice&			mDevice;
	
private:
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
};

#if defined(__OBJC__)
template<typename TYPE>
class ObjcPtr

class Metal::TContextThread : public SoyWorkerThread, public Metal::TContext
{
public:
	TContextThread(const std::string& Name,TDevice& Device) :
		SoyWorkerThread		( Name, SoyWorkerWaitMode::Wake ),
		TContext			( Device )
	{
		WakeOnEvent( PopWorker::TJobQueue::mOnJobPushed );
		Start();
	}
	
	//	thread
	virtual bool		Iteration() override	{	PopWorker::TJobQueue::Flush(*this);	return true;	}
	virtual bool		CanSleep() override		{	return !PopWorker::TJobQueue::HasJobs();	}	//	break out of conditional with this
};


