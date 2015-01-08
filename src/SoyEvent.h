#pragma once

#include <mutex>
#include <vector>
//#include "ofxSoylent.h"
#include <map>

class SoyListenerId
{
public:
	static SoyListenerId	Alloc()		{	return SoyListenerId( mListenerCounter++ );	}
	
	SoyListenerId() :
		mId		( 0 )
	{
	}
	
	inline bool		IsValid() const	{	return (*this) != SoyListenerId();	}
	inline bool		operator==(const SoyListenerId& That) const	{	return this->mId == That.mId;	}
	inline bool		operator!=(const SoyListenerId& That) const	{	return this->mId != That.mId;	}
	inline bool		operator<(const SoyListenerId& That) const	{	return this->mId < That.mId;	}
	
private:
	SoyListenerId(int Id) :
		mId		( Id )
	{
	}
	
private:
	int					mId;
	static std::atomic<int>	mListenerCounter;
};

template<typename PARAM>
class SoyEvent
{
public:
	typedef std::function<void(PARAM&)> FUNCTION;

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

	//	add static or lambda
	SoyListenerId		AddListener(FUNCTION Function,SoyListenerId ListenerId=SoyListenerId())
	{
		//	gr: need to ensure no duplicates...
		if ( !ListenerId.IsValid() )
			ListenerId = AllocListenerId();
		
		std::lock_guard<std::mutex> lock(mListenerLock);
		mListeners[ListenerId] = Function;
		return ListenerId;
	}
	void			RemoveListener(SoyListenerId Id)
	{
		std::lock_guard<std::mutex> lock(mListenerLock);
		mListeners.erase(Id);
	}
	void			RemoveListener(FUNCTION Function)
	{
		//std::lock_guard<std::mutex> lock(mListenerLock);
		//mListeners.erase(Function);
	}

	void			OnTriggered(PARAM& Param)
	{
		std::lock_guard<std::mutex> lock(mListenerLock);

		//	todo: execute on seperate threads with SoyScheduler
		for ( auto it=mListeners.begin();	it!=mListeners.end();	it++ )
		{
			auto& Function = it->second;
			Function( Param );
		}
	}
	
	bool			HasListeners() const		{	return !mListeners.empty();	}
	int				GetListenerCount() const	{	return mListeners.size();	}

	SoyListenerId	AllocListenerId()			{	return SoyListenerId::Alloc();	}
	
public:
	std::mutex				mListenerLock;
	std::map<SoyListenerId,FUNCTION>	mListeners;
};
