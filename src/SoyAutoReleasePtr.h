#pragma once


namespace Soy
{
	template<typename TYPE>
	class AutoReleasePtr;
}

/*
	For things link Directx where a pointer needs to call Release().
	This is NOT refcounted. If you want that, I'd wrap it in shared_ptr<> :)
 
*/
template<typename TYPE>
class Soy::AutoReleasePtr
{
public:
	AutoReleasePtr() :
	mObject	( nullptr )
	{
	}
	AutoReleasePtr(TYPE* Object,bool AddRef) :
	mObject	( nullptr )
	{
		Set( Object, AddRef );
	}
	AutoReleasePtr(const AutoReleasePtr& that) :
	mObject	( nullptr )
	{
		Set( that.mObject, true );
	}
	~AutoReleasePtr()		{	Release();	}
	
	TYPE*		operator ->()		{	return mObject;	}
	operator	TYPE*()				{	return mObject;	}
	operator	const TYPE*() const	{	return mObject;	}
	operator	bool() const		{	return mObject!=nullptr; }
	
	void		Set(TYPE* Object,bool AddRef)
	{
		Release();
		mObject = Object;
		if ( AddRef )
			Retain();
	}

	void		Retain()
	{
		if ( mObject )
		{
			mObject->AddRef();
		}
	}
	
	void		Release()
	{
		if ( mObject )
		{
			auto NewRefCount = mObject->Release();
			mObject = nullptr;
		}
	}

	size_t		GetReferenceCount()
	{
		if ( !mObject )
			return 0;

		//	directx returns ref count when we release, so add & decrease
		//	maybe need a policy/lambda for specific auto-release implementations?
		mObject->AddRef();
		auto RefCount = mObject->Release();
		return size_cast<size_t>( RefCount );
	}

	AutoReleasePtr&	operator=(const AutoReleasePtr& That)
	{
		if ( this != &That )
		{
			Set( That.mObject, true );
		}
		return *this;
	}

	AutoReleasePtr&	operator=(AutoReleasePtr&& That)
	{
		if ( this != &That )
		{
			Set( That.mObject, true );
			That.Release();
		}
		return *this;
	}
	
public:
	TYPE*	mObject;
};

