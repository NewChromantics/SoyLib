#pragma once


#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#endif

#include <stdint.h>

//	for NSArray_ForEach; move that to another header if this inclusion is a problem
#include <functional>

#define TARGET_POSIX

#if defined(TARGET_IOS) || defined(TARGET_OSX)
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
//	https://github.com/ericniebler/range-v3/issues/407
//#define __thread	static_assert(false,"Check __thread support on IOS. Supported from IOS7 up? https://github.com/ericniebler/range-v3/issues/407")			//	thread local not supported on IOS devices. todo: make a TLS class!
#endif

#if defined(TARGET_IOS) && defined(TARGET_OSX)
#error IOS and OSX targets defined
#endif

//	unused variable
#define __unused	__attribute__((unused))


namespace Platform
{
#if defined(__OBJC__)
	//	iterate through an nsarray of a distinct type
	//	NSArray_ForEach<NSString*>(...)
	template<typename NSTYPE>
	void NSArray_ForEach(NSArray<NSTYPE>* Array,std::function<void(NSTYPE)> Enum);
#endif
}






//	smart pointer for core foundation instances
//	gr: note, TYPE for CF types is already a pointer, hence no *'s on types
//	gr: warning! TYPE here passes around without proper type checks!
template<typename TYPE>
class CFPtr : public NonCopyable
{
public:
	//	DoRetain is really only for when a function call auto retains for you
	//	we assume the contents of CFPtr are retained generally, otherwise we're holding an unsafe pointer
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
	
	CFPtr(const CFPtr& That) :
		mObject	( nullptr )
	{
		Retain( That.mObject );
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

	CFPtr&	operator=(const CFPtr& That)
	{
		Retain( That.mObject );
		return *this;
	}
	
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



#if defined(__OBJC__)
template<typename NSTYPE>
inline void Platform::NSArray_ForEach(NSArray<NSTYPE>* Array,std::function<void(NSTYPE)> Enum)
{
	auto Size = [Array count];
	for ( auto i=0;	i<Size;	i++ )
	{
		auto Element = [Array objectAtIndex:i];
		Enum( Element );
	}
}
#endif

