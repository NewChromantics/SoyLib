#pragma once


#include "SoyTypes.h"
#include "array.hpp"
#include "bufferarray.hpp"
#include <map>
#include <queue>
#include <limits>

#if defined(TARGET_OSX)||defined(TARGET_ANDROID)||defined(TARGET_IOS)
#include <memory>
#define STD_ALLOC
#endif

#if defined(TARGET_WINDOWS)
//#define ENABLE_STACKTRACE
#endif
#define ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION	false	//	catch corruption in constructors
#define ENABLE_DEBUG_VERIFY_AFTER_DESTRUCTION	false	//	catch corruption in an objects lifetime (kinda)


//	helpful class for tracking allocations or similar in a simple wrapped-up class
#define __LOCATION__	prcore::ofCodeLocation( __FILE__, __LINE__ )



class ofCodeLocation
{
public:
	ofCodeLocation() :
		mLocation	( "unknown(?)")
	{
	}
	ofCodeLocation(const char* Filename,int LineNo)
	{
		std::stringstream str;
		str << Filename << "(" << LineNo << ")";
		mLocation = str.str();
	}
	
	operator const char*() const	{	return mLocation.c_str();	}

public:
	std::string	mLocation;
};
DECLARE_NONCOMPLEX_TYPE( ofCodeLocation );

class ofStackEntry
{
public:
	ofStackEntry(uint64 Address=0) :
		mProcessAddress	( Address )
	{
	}
	
	uint64		mProcessAddress;
};
DECLARE_NONCOMPLEX_TYPE( ofStackEntry );

namespace SoyDebug
{
	bool	GetCallStack(ArrayBridge<ofStackEntry>& Stack,int StackSkip);
	bool	GetSymbolName(std::string& SymbolName, const ofStackEntry& Address, uint64* pSymbolOffset = NULL);
	bool	GetSymbolLocation(ofCodeLocation& Location,const ofStackEntry& Address);
};





//	gr; move all these into soy
namespace prmem
{
	class Heap;				
	class HeapInfo;			//	base type for a heap which just has information and statistics
	class HeapDebugBase;	//	holds all our debug info for a heap
	class CRTHeap;
	class HeapDebugItem;
};

namespace Soy
{
	template<class T>
	class HeapBridge;		//	for stl
};

namespace prcore
{
	//	global heap to replace new/delete/crt default
	//	prcore::Heap
	extern prmem::Heap	Heap;		
}

namespace prmem
{
	//	access all the heaps. 
	//	Though the access is threadsafe, the data inside (ie. the memcounters) isn't
	const ArrayInterface<prmem::HeapInfo*>&	GetHeaps();
	prmem::CRTHeap&							GetCRTHeap();

	//	functor for STD deallocation (eg, with shared_ptr)
	template<typename T>
	class HeapFreeFunctor
	{
	public:
		HeapFreeFunctor(Heap& x) : _x( x ) {}
		void operator()(T* pObject) const;

	public:
		Heap& _x;
	};
};

//	allocator that can be used to bridge our Heap allocator, and an allocator for STD types
//		eg; std::vector<int,prmem::HeapStdAllocator<int> > v;
//	from http://www.josuttis.com/libbook/memory/myalloc.hpp.html
//		http://blogs.msdn.com/b/calvin_hsia/archive/2010/03/16/9979985.aspx
//
//	see unit tests for usage as it's messy and a bit complicated to example here.
//
template <class T>
class Soy::HeapBridge
{
public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;
	
public:
	// rebind allocator to type U
	template <class U>
	struct rebind
	{
		typedef HeapBridge<U> other;
	};
	
	// return address of values
	pointer address (reference value) const
	{
		return &value;
	}
	const_pointer address (const_reference value) const
	{
		return &value;
	}
	
	HeapBridge(prmem::Heap& Heap) throw() :
		mHeap	( Heap )
	{
	}
	HeapBridge(const HeapBridge& that) throw() :
		mHeap	( that.mHeap )
	{
	}
	/*
	 template <class U>
	 HeapBridge (const HeapBridge<U>&) throw()
	 {
	 }
	 */
	~HeapBridge() throw()
	{
	}
	
	// return maximum number of elements that can be allocated
	size_type max_size () const throw();
	
	// allocate but don't initialize num elements of type T
	pointer allocate (size_type num, const void* = 0);
	
	// initialize elements of allocated storage p with value value
	void construct (pointer p, const T& value)
	{
		//	gr: this is done by Heap's alloc... may need to abstract that
	}
	
	void destroy (pointer p)
	{
		//	gr: this is done by Heap's alloc... may need to abstract that
	}
	
	// deallocate storage p of deleted elements
	void deallocate (pointer p, size_type num);
	
	operator	prmem::Heap&()	{	return mHeap;	}	//	really easy bodge to get constructors in constructors in constructors working (messy STL!)
	
public:
	prmem::Heap&	mHeap;
};

template <class T1, class T2>
bool operator== (const Soy::HeapBridge<T1>& a,const Soy::HeapBridge<T2>& b) throw()
{
	return a.mHeap == b.mHeap;
}

template <class T1, class T2>
bool operator!= (const Soy::HeapBridge<T1>& a,const Soy::HeapBridge<T2>& b) throw()
{
	return a.mHeap != b.mHeap;
}

//-----------------------------------------------------------------------
//	base heap interface so we can mix our allocated heaps and the default CRT heap (which is also a heap, but hidden away)
//-----------------------------------------------------------------------
class prmem::HeapInfo
{
public:
	HeapInfo(const char* Name);
	virtual ~HeapInfo();

	inline const std::string&	GetName() const					{	return mName;	}
#if defined(TARGET_WINDOWS)
	virtual HANDLE			GetHandle() const=0;			//	get win32 heap handle
#endif
	virtual bool			IsValid() const=0;				//	heap has been created
	virtual void			EnableDebug(bool Enable)		{}
	inline bool				IsDebugEnabled() const			{	return GetDebug()!=NULL;	}
	virtual const HeapDebugBase*	GetDebug() const		{	return NULL;	}

	inline float			GetAllocatedMegaBytes() const	{	float b = static_cast<float>(mAllocBytes);	return (b / 1024.f / 1024.f);	}
	inline size_t			GetAllocatedBytes() const		{	return mAllocBytes;	}
	inline size_t			GetAllocCount() const			{	return mAllocCount;	}
	inline float			GetAllocatedMegaBytesPeak() const	{	float b = static_cast<float>(mAllocBytesPeak);	return (b / 1024.f / 1024.f);	}
	inline size_t			GetAllocatedBytesPeak() const		{	return mAllocBytesPeak;	}
	inline size_t			GetAllocCountPeak() const			{	return mAllocCountPeak;	}

	//	gr: todo; check heap limits
	size_t					GetMaxAlloc() const				{	return std::numeric_limits<std::size_t>::max();	}

	void					Debug_DumpInfoToOutput(std::ostream& Output) const;		//	print out name, allocation(peak)

	//	validate the heap. Checks for corruption (out-of-bounds writes). If an object is passed, just that one allocation is checked. 
	//	"On a system set up for debugging, the HeapValidate function then displays debugging messages that describe the part of the heap or memory block that is invalid, and stops at a hard-coded breakpoint so that you can examine the system to determine the source of the invalidity"
	//	returns true if heap is OK
	//	http://msdn.microsoft.com/en-us/library/windows/desktop/aa366708(v=vs.85).aspx
	bool					Debug_Validate(const void* Object=NULL) const;

protected:
	//	update tracking information. BlockCount=Allocation count, usually 1 (an Array = 1)
	inline void				OnAlloc(size_t Bytes,size_t BlockCount)
	{
		mAllocBytes += Bytes;
		mAllocBytesPeak = (mAllocBytes>mAllocBytesPeak) ? mAllocBytes : mAllocBytesPeak;
		mAllocCount += BlockCount;
		mAllocCountPeak = (mAllocCount>mAllocCountPeak) ? mAllocCount : mAllocCountPeak;
	}	
	inline void				OnFree(size_t Bytes,size_t BlockCount)
	{
		//	for safety, don't go under zero
		mAllocBytes -= ( Bytes > mAllocBytes ) ? mAllocBytes : Bytes;
		mAllocCount -= ( BlockCount > mAllocCount ) ? mAllocCount : BlockCount;
	}
	void					OnFailedAlloc(std::string TypeName,size_t TypeSize,size_t Elements) const;

protected:
	std::string			mName;	//	name for easy debugging purposes
	size_t				mAllocBytes;	//	number of bytes currently allocated (note; actual mem usage may be greater due to block size & fragmentation)
	size_t				mAllocCount;	//	number of individual allocations (ie, #blocks in heap)
	size_t				mAllocBytesPeak;
	size_t				mAllocCountPeak;
};


//-----------------------------------------------------------------
//	item containing debug tracking information per allocation
//-----------------------------------------------------------------
class prmem::HeapDebugItem
{
public:
	static const int	CallStackSize = 5;
public:
	std::string			ToString() const;

	inline bool			operator==(const void* Object) const	{	return mObject == Object;	}

public:
	const void*						mObject;		//	allocated data
	size_t							mElements;		//	number of elements
	size_t							mTypeSize;		//	sizeof(T)
	const std::string*				mTypename;		//	gr: this SHOULD be safe, all strings from GetTypeName are either compile-time generated or static.
	uint64							mAllocTick;		//	time of allocation (ofGetElapsedTimeMillis())
	BufferArray<uint64,CallStackSize>	mCallStack;		//	each is an address in the process' symbol data
};
DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( prmem::HeapDebugItem );


//-----------------------------------------------------------------
//	debug information for a heap, interface. 
//-----------------------------------------------------------------
class prmem::HeapDebugBase
{
public:
	virtual ~HeapDebugBase()	{}

	template<typename T>
	void			OnAlloc(const T* Object,size_t Elements)
	{
		auto TypeName = Soy::GetTypeName<T>();
		OnAlloc( Object, TypeName, Elements, sizeof(T) );
	}
	virtual void	OnFree(const void* Object)=0;
	virtual void	DumpToOutput(const prmem::HeapInfo& OwnerHeap)const =0;	//	debug-print out all our allocations and their age

protected:
	virtual void	OnAlloc(const void* Object,const std::string& TypeName,size_t ElementCount,size_t TypeSize)=0;
	void			DumpToOutput(const prmem::HeapInfo& OwnerHeap,ArrayBridge<HeapDebugItem>& AllocItems) const;	//	debug-print out all our allocations and their age
};

//-----------------------------------------------------------------------
//	heap - allocates a windows memory heap and provides allocation/free functions
//-----------------------------------------------------------------------
class prmem::Heap : public prmem::HeapInfo
{
public:
	Heap(bool EnableLocks,bool EnableExceptions,const char* Name,size_t MaxSize=0,bool DebugTrackAllocs=false);
	~Heap();

#if defined(TARGET_WINDOWS)
	virtual HANDLE					GetHandle() const			{	return mHandle;	}
	virtual bool					IsValid() const override	{	return mHandle!=NULL;	}	//	same as IsValid, but without using virtual pointers so this can be called before this class has been properly constructed
#elif defined(STD_ALLOC)
	virtual bool					IsValid() const override	{	return true;	}	//	same as IsValid, but without using virtual pointers so this can be called before this class has been properly constructed
#endif
	virtual void					EnableDebug(bool Enable) override;	//	deletes/allocates the debug tracker so we can toggle it at runtime
	virtual const HeapDebugBase*	GetDebug() const override	{	return mHeapDebug;	}

	Soy::HeapBridge<char>&			GetStlAllocator()			{	return mBridge;	}
	/*
	//	probe the system for the allocation size
	uint32	GetAllocSize(void* pData) const
	{
#if defined(TARGET_WINDOWS)
		DWORD Flags = 0;
		SIZE_T AllocSize = HeapSize( GetHandle(), Flags, pData );
		return AllocSize;
#else
		return 0;
#endif
	};
	 */

	template<typename TYPE>
	TYPE*	Alloc()	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;

		//	construct with placement New
		if ( Soy::DoConstructType<TYPE>() )
		{
			pAlloc = new( pAlloc ) TYPE();
		
			if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
				Debug_Validate();
		}
		return pAlloc;
	}

	template<typename TYPE>
	TYPE*	AllocArray(const size_t Elements)
	{
		TYPE* pAlloc = RealAlloc<TYPE>( Elements );
		if ( !pAlloc )
			return NULL;
		
		//	construct with placement New
		if ( Soy::DoConstructType<TYPE>() )
		{
			for ( size_t i=0;	i<Elements;	i++ )
			{
				auto* pAlloci = &pAlloc[i];
				//	gr: changed to have NO parenthisis to remove warning C4345. 
				//		I believe this is better (no zero-initialisation) so POD objects should be explicitly initialised (or add a constructor to your struct ;)
				//		lack of parenthesis still means the default constructor is called on classes.
				pAlloci = new( pAlloci ) TYPE;	
			}
	
			if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
				Debug_Validate();
		}
		return pAlloc;
	}
	
	//	alloc with 1 argument to constructor
	template<typename TYPE,typename ARG1>
	TYPE*	Alloc(ARG1& Arg1)	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;
		//	construct with placement New
		pAlloc = new( pAlloc ) TYPE(Arg1);
		if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
			Debug_Validate();
		return pAlloc;
	}
	
	//	alloc with 2 arguments to constructor
	template<typename TYPE,typename ARG1,typename ARG2>
	TYPE*	Alloc(ARG1& Arg1,ARG2& Arg2)	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;
		//	construct with placement New
		pAlloc = new( pAlloc ) TYPE(Arg1,Arg2);
		if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
			Debug_Validate();
		return pAlloc;
	}
	
	//	alloc with 3 arguments to constructor
	template<typename TYPE,typename ARG1,typename ARG2,typename ARG3>
	TYPE*	Alloc(ARG1& Arg1,ARG2& Arg2,ARG3& Arg3)	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;
		//	construct with placement New
		pAlloc = new( pAlloc ) TYPE(Arg1,Arg2,Arg3);
		if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
			Debug_Validate();
		return pAlloc;
	}
	
	//	alloc with 4 arguments to constructor
	template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4>
	TYPE*	Alloc(ARG1& Arg1,ARG2& Arg2,ARG3& Arg3,ARG4& Arg4)	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;
		//	construct with placement New
		pAlloc = new( pAlloc ) TYPE(Arg1,Arg2,Arg3,Arg4);
		if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
			Debug_Validate();
		return pAlloc;
	}
	
	//	alloc with 5 arguments to constructor
	template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4,typename ARG5>
	TYPE*	Alloc(ARG1& Arg1,ARG2& Arg2,ARG3& Arg3,ARG4& Arg4,ARG5& Arg5)	
	{
		TYPE* pAlloc = RealAlloc<TYPE>( 1 );
		if ( !pAlloc )
			return NULL;
		//	construct with placement New
		pAlloc = new( pAlloc ) TYPE(Arg1,Arg2,Arg3,Arg4,Arg5);
		if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
			Debug_Validate();
		return pAlloc;
	}
	
	//	alloc into a smart pointer
	template<typename TYPE>
	std::shared_ptr<TYPE>	AllocPtr()
	{
		TYPE* pAlloc = Alloc<TYPE>( );
		return std::shared_ptr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
	}

	//	alloc into a smart pointer
	template<typename TYPE,typename ARG1>
	std::shared_ptr<TYPE>	AllocPtr(ARG1& Arg1)
	{
		TYPE* pAlloc = Alloc<TYPE>( Arg1 );
		return std::shared_ptr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
	}

	//	alloc into a smart pointer
	template<typename TYPE,typename ARG1,typename ARG2>
	std::shared_ptr<TYPE>	AllocPtr(ARG1& Arg1,ARG2& Arg2)
	{
		TYPE* pAlloc = Alloc<TYPE>( Arg1, Arg2 );
		return std::shared_ptr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
	}

	//	alloc into a smart pointer
	template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4>
	std::shared_ptr<TYPE>	AllocPtr(ARG1& Arg1,ARG2& Arg2,ARG3& Arg3,ARG4& Arg4)
	{
		TYPE* pAlloc = Alloc<TYPE>( Arg1, Arg2, Arg3, Arg4 );
		return std::shared_ptr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
	}

	//	alloc into a smart pointer
	template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4,typename ARG5>
	std::shared_ptr<TYPE>	AllocPtr(ARG1& Arg1,ARG2& Arg2,ARG3& Arg3,ARG4& Arg4,ARG5& Arg5)
	{
		TYPE* pAlloc = Alloc<TYPE>( Arg1, Arg2, Arg3, Arg4, Arg5 );
		return std::shared_ptr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
	}

	template<typename TYPE>
	bool	Free(TYPE* pObject)
	{
		//	destruct
		pObject->~TYPE();
		if ( ENABLE_DEBUG_VERIFY_AFTER_DESTRUCTION )
			Debug_Validate();

		return RealFree( pObject, 1 );
	}			

	template<typename TYPE>
	bool	FreeArray(TYPE* pObject,size_t Elements)
	{
		//	no need to destruct types we don't construct
		if ( Soy::DoConstructType<TYPE>() )
		{
			//	destruct in reverse order; http://www.ezdefinition.com/cgi-bin/showsubject.cgi?sid=409
			auto e = Elements;
			while ( e )
				pObject[--e].~TYPE();
	
			if ( ENABLE_DEBUG_VERIFY_AFTER_DESTRUCTION )
				Debug_Validate();
		}

		return RealFree( pObject, Elements );
	}

private:
	template<typename TYPE>
	inline TYPE*	RealAlloc(const size_t Elements)
	{
#if defined(TARGET_WINDOWS)
		TYPE* pData = static_cast<TYPE*>( HeapAlloc( mHandle, 0x0, Elements*sizeof(TYPE) ) );
#elif defined(STD_ALLOC)
		TYPE* pData = reinterpret_cast<TYPE*>( mAllocator.allocate( Elements*sizeof(TYPE) ) );
#endif
		if ( !pData )
		{
			//	if we fail, do a heap validation, this will reveal corruption, rather than OOM
			Debug_Validate();

			//	report failed alloc regardless
			auto TypeName = Soy::GetTypeName<TYPE>();
			OnFailedAlloc( TypeName, sizeof(TYPE), Elements );
			return nullptr;
		}
		if ( mHeapDebug )
			mHeapDebug->OnAlloc( pData, Elements );
		OnAlloc( pData ? Elements*sizeof(TYPE) : 0, 1 );
		return pData;
	}

	template<typename TYPE>
	inline bool	RealFree(TYPE* pObject,const size_t Elements)
	{
#if defined(TARGET_WINDOWS)
		//	no need to specify length, mem manager already knows the real size of pObject
		if ( !HeapFree( mHandle, 0, pObject ) )
			return false;
#elif defined(STD_ALLOC)
		mAllocator.deallocate( reinterpret_cast<char*>(pObject), Elements );
#endif

		if ( mHeapDebug )
			mHeapDebug->OnFree( pObject );

		auto BytesFreed = sizeof(TYPE)*Elements;
		OnFree( BytesFreed, 1 );
		return true;
	}

private:
	HeapDebugBase*			mHeapDebug;	//	debug information
	Soy::HeapBridge<char>	mBridge;	//	many/all STL things need a reference, and as this costs so little, just have it as a member
#if defined(TARGET_WINDOWS)
	HANDLE					mHandle;	//	win32 handle to heap
#elif defined(STD_ALLOC)
	std::allocator<char>	mAllocator;	//	gr: this just uses new and delete. Repalce this with memory zones
#endif
};

//-----------------------------------------------------------------------
//	interface to the hidden default crt heap (where new & delete are)
//-----------------------------------------------------------------------
class prmem::CRTHeap : public prmem::HeapInfo
{
public:
	CRTHeap(bool EnableDebug);
	
#if defined(TARGET_WINDOWS)
	virtual HANDLE			GetHandle() const override;
	virtual bool			IsValid() const override	{	return GetHandle() != nullptr;	}
#else
	virtual bool			IsValid() const override	{	return true;	}
#endif

	void					Update();			//	update tracking information
};



//	class that (HUGELY) simplifies an std::Map that uses a Heap for allocation
//	turn 
//		std:Map<A,B> myMap;
//	into
//		StdMapHeap<A,B> myMap( prcore::Heap );
template<typename A,typename B,prmem::Heap& HEAP=prcore::Heap>
class StdMapHeap : public std::map<A,B,std::less<A>,Soy::HeapBridge<std::pair<A,B>>>
{
public:
	typename std::map<A, B, std::less<A>, Soy::HeapBridge<std::pair<A, B>>> SUPER;
public:
	StdMapHeap(prmem::Heap& Heap=HEAP) :
		SUPER	( std::less<A>(), Soy::HeapBridge<std::pair<A const,B>>( Heap ) )
	{
	}
};

template<typename A,prmem::Heap& HEAP=prcore::Heap>
class StdQueueHeap : public std::queue<A,std::deque<A,Soy::HeapBridge<A>>>
{
public:
	typename std::queue<A, std::deque<A, Soy::HeapBridge<A>>> SUPER;
public:
	StdQueueHeap(prmem::Heap& Heap=HEAP) :
		SUPER	( std::deque<A,Soy::HeapBridge<A>>(Heap) )
	{
	}
};
	


template<typename T>
inline void prmem::HeapFreeFunctor<T>::operator()(T* pObject) const
{
	_x.Free(pObject);
}

// return maximum number of elements that can be allocated
template<typename T>
inline typename Soy::HeapBridge<T>::size_type Soy::HeapBridge<T>::max_size () const throw()
{
	return mHeap.GetMaxAlloc() / sizeof(T);
}

// allocate but don't initialize num elements of type T
template<typename T>
inline typename Soy::HeapBridge<T>::pointer Soy::HeapBridge<T>::allocate (size_type num, const void*)
{
	//	gr: note, our Heap type DOES initialise them
	return mHeap.AllocArray<T>( num );
}

// deallocate storage p of deleted elements
template<typename T>
inline void Soy::HeapBridge<T>::deallocate (pointer p, size_type num)
{
	mHeap.FreeArray<T>( p, num );
}
