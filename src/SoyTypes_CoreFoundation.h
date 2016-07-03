#pragma once


#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#endif

#include <stdint.h>

#define TARGET_POSIX

#if defined(TARGET_IOS)
#if __has_feature(objc_arc)
#define ARC_ENABLED
#endif
#endif

#define __noexcept			_NOEXCEPT
#define __noexcept_prefix
#define __stdcall
#define __export			extern "C"
#define __deprecated_prefix	
#define __deprecated		__attribute__((deprecated))

#if defined(TARGET_IOS)
#define __thread				//	thread local not supported on IOS devices. todo: make a TLS class!
#endif

//	unused variable
#define __unused	__attribute__((unused))

//	smart pointer for core foundation instances
//	gr: note, TYPE for CF types is already a pointer, hence no *'s on types
//	gr: warning! TYPE here passes around without proper type checks!
template<typename TYPE>
class CFPtr : public NonCopyable
{
public:
	explicit CFPtr(TYPE Object,bool DoRetain) :
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
	//template<typename THAT>
	explicit CFPtr(const CFPtr<TYPE>& That) :
	CFPtr	( nullptr )
	{
		Retain( static_cast<TYPE>(That.mObject) );
	}
	
	~CFPtr()
	{
		Release();
	}

	//	explicitly increment retention
	void		Retain()
	{
		if ( mObject )
			CFRetain(mObject);
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
		return static_cast<int>( CFGetRetainCount(mObject) );
	}
	
	//TYPE*		operator->() const	{	return mObject; }
	//operator	TYPE() const	{	return mObject; }
	operator	bool() const	{	return mObject ? (GetRetainCount()>0) : false; }
public:
	TYPE	mObject;
};



//	smart pointer for objective c instances
template<typename TYPE>
class ObjcPtr : public NonCopyable
{
public:
	ObjcPtr(TYPE* Object=nullptr) :
	mObject	( nullptr )
	{
		Retain(Object);
	}
	template<typename THAT>
	explicit ObjcPtr(const ObjcPtr<THAT>& That) :
	mObject	( nullptr )
	{
		Retain( static_cast<THAT*>(That.mObject) );
	}
	
	~ObjcPtr()
	{
		Release();
	}
	
	void		Retain(TYPE* Object)
	{
		Release();
		mObject = Object;
#if defined(__OBJC__)
#if !defined(ARC_ENABLED)
		if ( mObject )
			[mObject retain];
#endif
#endif
	}
	
	void		Release()
	{
#if defined(__OBJC__)
#if !defined(ARC_ENABLED)
		if ( mObject )
			[mObject release];
#endif
#endif
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
	
	//TYPE*		operator->() const	{	return mObject; }
	operator	TYPE*() const	{	return mObject; }
#if defined(__OBJC__)
#if defined(ARC_ENABLED)
	operator	bool() const	{	return mObject ? (mObject!=nullptr) : false; }
#else
	operator	bool() const	{	return mObject ? ([mObject retainCount]>0) : false; }
#endif
#endif
public:
	TYPE*	mObject;
};



