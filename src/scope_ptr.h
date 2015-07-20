#pragma once
#include <functional>


//	generic smart pointer with a lambda for release
template<typename TYPE>
class scope_ptr : public NonCopyable
{
public:
	scope_ptr(std::function<void(TYPE&)> ReleaseFunc) :
		mObject			( nullptr ),
		mReleaseFunc	( ReleaseFunc )
	{
	}
	~scope_ptr()
	{
		Release();
	}
	
	void		Release()
	{
		if ( mObject && mReleaseFunc )
			mReleaseFunc(mObject);
		mObject = nullptr;
	}
	
public:
	TYPE	mObject;
	std::function<void(TYPE&)>	mReleaseFunc;
};

