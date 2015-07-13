#pragma once

#include <stdint.h>

typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;


//	todo: remove this, only depend in win32 specific code
#define __stdcall
#define __export	extern "C"
#define __noexcept	_NOEXCEPT




//	smart pointer for core foundation instances
//	gr: note, TYPE for CF types is already a pointer, hence no *'s on types
template<typename TYPE>
class CFPtr : public NonCopyable
{
public:
	CFPtr(TYPE Object,bool DoRetain) :
	mObject	( nullptr )
	{
		if ( DoRetain )
		{
			Retain(Object);
		}
		else
		{
			//	already retained by OS
			SetNoRetain(Object);
		}
	}
	CFPtr() :
	mObject	( nullptr )
	{
	}
	template<typename THAT>
	explicit CFPtr(const CFPtr<THAT>& That) :
	CFPtr	( nullptr )
	{
		Retain( static_cast<THAT>(That.mObject) );
	}
	
	~CFPtr()
	{
		Release();
	}
	
	void		Retain(TYPE Object)
	{
		SetNoRetain(Object);
		if ( mObject )
			CFRetain(mObject);
	}
	void		SetNoRetain(TYPE Object)
	{
		Release();
		mObject = Object;
	}
	
	void		Release()
	{
		if ( mObject )
			CFRelease(mObject);
		mObject = nullptr;
	}
	
	/*
	 template<typename THAT>
	 ObjcPtr&	operator=(const ObjcPtr<THAT>& That)
	 {
		Retain( static_cast<THAT*>(That.mObject) );
		return *this;
	 }
	 */
	
	int			GetRetainCount() const
	{
		return CFGetRetainCount(mObject);
	}
	
	//TYPE*		operator->() const	{	return mObject; }
	//operator	TYPE() const	{	return mObject; }
	operator	bool() const	{	return mObject ? (GetRetainCount()>0) : false; }
public:
	TYPE	mObject;
};
