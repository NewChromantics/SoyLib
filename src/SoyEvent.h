#pragma once

#include <mutex>
#include <vector>
//#include "ofxSoylent.h"
#include <map>
#include <atomic>



class SoyEventBase
{
public:
};

//	gr: make a derivitive of this that can un-listen itself by keeping a pointer to the event
class SoyListenerId
{
public:
	static SoyListenerId	Alloc(SoyEventBase& Event) { return SoyListenerId(mListenerCounter++, Event); }
	static SoyListenerId	Alloc() { return SoyListenerId(mListenerCounter++); }

	SoyListenerId() :
		mId		( 0 )
	{
	}
	
	inline bool		IsValid() const	{	return (*this) != SoyListenerId();	}
	inline bool		operator==(const SoyListenerId& That) const	{	return this->mId == That.mId;	}
	inline bool		operator!=(const SoyListenerId& That) const	{	return this->mId != That.mId;	}
	inline bool		operator<(const SoyListenerId& That) const	{	return this->mId < That.mId;	}
	
private:
	SoyListenerId(uint64_t Id,SoyEventBase& Event) :
		mId		( Id ),
		mEvent	( &Event )
	{
	}
	SoyListenerId(uint64_t Id) :
		mId		( Id ),
		mEvent	( nullptr )
	{
	}

private:
	uint64_t			mId;
	SoyEventBase*		mEvent;		//	only for debug atm
	static std::atomic<uint64_t>	mListenerCounter;
};


template<typename PARAM>
class SoyEvent : public SoyEventBase
{
public:
	typedef std::function<void(PARAM&)> FUNCTION;
	typedef std::mutex MUTEX;		//	gr: recursive mutex DOES NOT help

public:
	//	helper for member functions
	template<class CLASS>
	SoyListenerId		AddListener(CLASS& This,void(CLASS::*Member)(PARAM&))
	{
		//	http://stackoverflow.com/a/7582576
		auto bound = std::bind( Member, &This, std::placeholders::_1  );
		return AddListener( bound );
	}
	template<class CLASS>
	void			RemoveListener(CLASS& This,FUNCTION Member)
	{
		RemoveListener( std::bind( &Member, &This ) );
	}

	SoyListenerId		AddListener(SoyEvent<PARAM>& Event)
	{
		auto Relay = [&Event](PARAM& p)
		{
			Event.OnTriggered(p);
		};
		return AddListener( Relay );
	}
	
	//	add static or lambda
	SoyListenerId		AddListener(FUNCTION Function,SoyListenerId ListenerId=SoyListenerId())
	{
		//	gr: need to ensure no duplicates...
		if ( !ListenerId.IsValid() )
			ListenerId = AllocListenerId();
		
		std::lock_guard<MUTEX> lock(mListenerLock);
		mListeners[ListenerId] = Function;
		return ListenerId;
	}
	void			RemoveListener(SoyListenerId Id)
	{
		//	it's very easy to get a deadlock here
		if ( !mListenerLock.try_lock() )
		{
			std::lock_guard<std::mutex> DeadLock( mDeadListenerLock );
			mDeadListeners.push_back( Id );
			//	gr: recursive include
			//std::Debug << "Failed to RemoveLister() in case of deadlock" << std::endl;
			return;
		}
		mListeners.erase(Id);
		mListenerLock.unlock();
	}
	void			RemoveListener(FUNCTION Function)
	{
		//std::lock_guard<MUTEX> lock(mListenerLock);
		//mListeners.erase(Function);
	}

	void			OnTriggered(PARAM& Param)
	{
		//	kill off dead listeners
		if ( !mDeadListeners.empty() )
		{
			std::lock_guard<std::mutex> DeadLock( mDeadListenerLock );
			std::lock_guard<MUTEX> ListenerLock(mListenerLock);
			while ( !mDeadListeners.empty() )
			{
				auto Id = mDeadListeners.back();
				mListeners.erase(Id);
				mDeadListeners.pop_back();
			}
		}
		
		std::lock_guard<MUTEX> lock(mListenerLock);

		//	todo: execute on seperate threads with SoyScheduler
		for ( auto it=mListeners.begin();	it!=mListeners.end();	it++ )
		{
			auto& Function = it->second;
			Function( Param );
		}
	}
	
	bool			HasListeners() const		{	return !mListeners.empty();	}
	int				GetListenerCount() const	{	return mListeners.size();	}

	SoyListenerId	AllocListenerId()			{	return SoyListenerId::Alloc(*this);	}
	
public:
	MUTEX								mListenerLock;
	std::map<SoyListenerId,FUNCTION>	mListeners;
	
	std::mutex							mDeadListenerLock;
	std::vector<SoyListenerId>			mDeadListeners;
};
