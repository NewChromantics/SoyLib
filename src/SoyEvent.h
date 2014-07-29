#pragma once

#include <mutex>
#include <vector>
//#include "ofxSoylent.h"



template<typename PARAM>
class SoyEvent
{
public:
	typedef std::function<void(PARAM&)> FUNCTION;

public:
	//	helper for member functions
	template<class CLASS>
	void			AddListener(CLASS& This,void(CLASS::*Member)(PARAM&))
	{
		//	http://stackoverflow.com/a/7582576
		auto bound = std::bind( Member, &This, std::placeholders::_1  );
		AddListener( bound );
	}
	template<class CLASS>
	void			RemoveListener(CLASS& This,FUNCTION Member)
	{
		RemoveListener( std::bind( &Member, &This ) );
	}

	//	add static or lambda
	void			AddListener(FUNCTION Function)
	{
		std::lock_guard<std::mutex> lock(mListenerLock);
		mListeners.push_back(Function);
	}
	void			RemoveListener(FUNCTION Function)
	{
		std::lock_guard<std::mutex> lock(mListenerLock);
		mListeners.erase(Function);
	}

	void			OnTriggered(PARAM& Param)
	{
		std::lock_guard<std::mutex> lock(mListenerLock);

		//	todo: execute on seperate threads with SoyScheduler
		for ( auto it=mListeners.begin();	it!=mListeners.end();	it++ )
		{
			auto& Function = *it;
			Function( Param );
		}
	}
	
	bool			HasListeners() const		{	return !mListeners.empty();	}

public:
	std::mutex				mListenerLock;
	std::vector<FUNCTION>	mListeners;
};
