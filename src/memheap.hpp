#pragma once


#include "types.hpp"
#include "array.hpp"
#include "bufferarray.hpp"
#include "string.hpp"
#include <map>
#include <queue>

#define ENABLE_STACKTRACE
#define ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION	true


//	helpful class for tracking allocations or similar in a simple wrapped-up class
#define __LOCATION__	prcore::ofCodeLocation( __FILE__, __LINE__ )



class ofCodeLocation
{
public:
	ofCodeLocation()
	{
		mLocation << "Unknown(??)";
	}
	ofCodeLocation(const char* Filename,int LineNo)
	{
		mLocation << Filename << "(" << LineNo << ")";
	}

	operator const char*() const	{	return mLocation;	}

public:
	BufferString<256>	mLocation;
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
	bool	GetSymbolName(BufferString<200>& SymbolName,const ofStackEntry& Address,uint64* pSymbolOffset=NULL);
	bool	GetSymbolLocation(ofCodeLocation& Location,const ofStackEntry& Address);
};






namespace prmem
{
	class Heap;				
	class HeapInfo;			//	base type for a heap which just has information and statistics
	class HeapDebugBase;	//	holds all our debug info for a heap
	class CRTHeap;
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
	const BufferArray<prmem::HeapInfo*,100>&	GetHeaps();
	prmem::CRTHeap&										GetCRTHeap();

	//	functor for STD deallocation (eg, with shared_ptr)
	template<class T>
	class HeapFreeFunctor
	{
	public:
		HeapFreeFunctor(Heap& x) : _x( x ) {}
		void operator()(T* pObject) const { _x.Free(pObject); }

	public:
		Heap& _x;
	};

	//-----------------------------------------------------------------------
	//	base heap interface so we can mix our allocated heaps and the default CRT heap (which is also a heap, but hidden away)
	//-----------------------------------------------------------------------
	class HeapInfo
	{
	public:
		HeapInfo(const char* Name);
		virtual ~HeapInfo();

		inline uint64			GetId() const					{	return mId;	}
		inline const char*		GetName() const					{	return mName;	}
		virtual HANDLE			GetHandle() const=0;			//	get win32 heap handle
		inline bool				IsValid() const					{	return GetHandle()!=NULL;	}	//	heap has been created
		virtual void			EnableDebug(bool Enable)		{}
		inline bool				IsDebugEnabled() const			{	return GetDebug()!=NULL;	}
		virtual const HeapDebugBase*	GetDebug() const		{	return NULL;	}

		inline float			GetAllocatedMegaBytes() const	{	float b = static_cast<float>(mAllocBytes);	return (b / 1024.f / 1024.f);	}
		inline uint32			GetAllocatedBytes() const		{	return mAllocBytes;	}
		inline uint32			GetAllocCount() const			{	return mAllocCount;	}
		inline float			GetAllocatedMegaBytesPeak() const	{	float b = static_cast<float>(mAllocBytesPeak);	return (b / 1024.f / 1024.f);	}
		inline uint32			GetAllocatedBytesPeak() const		{	return mAllocBytesPeak;	}
		inline uint32			GetAllocCountPeak() const			{	return mAllocCountPeak;	}

		//	validate the heap. Checks for corruption (out-of-bounds writes). If an object is passed, just that one allocation is checked. 
		//	"On a system set up for debugging, the HeapValidate function then displays debugging messages that describe the part of the heap or memory block that is invalid, and stops at a hard-coded breakpoint so that you can examine the system to determine the source of the invalidity"
		//	returns true if heap is OK
		//	http://msdn.microsoft.com/en-us/library/windows/desktop/aa366708(v=vs.85).aspx
		bool					Debug_Validate(const void* Object=NULL) const;

	protected:
		//	update tracking information. BlockCount=Allocation count, usually 1 (an Array = 1)
		inline void				OnAlloc(uint32 Bytes,uint32 BlockCount)
		{
			mAllocBytes += Bytes;
			mAllocBytesPeak = (mAllocBytes>mAllocBytesPeak) ? mAllocBytes : mAllocBytesPeak;
			mAllocCount += BlockCount;
			mAllocCountPeak = (mAllocCount>mAllocCountPeak) ? mAllocCount : mAllocCountPeak;
		}	
		inline void				OnFree(uint32 Bytes,uint32 BlockCount)
		{
			//	for safety, don't go under zero
			mAllocBytes -= ( Bytes > mAllocBytes ) ? mAllocBytes : Bytes;
			mAllocCount -= ( BlockCount > mAllocCount ) ? mAllocCount : BlockCount;
		}

	protected:
		BufferString<100>	mName;	//	name for easy debugging purposes
		uint32				mAllocBytes;	//	number of bytes currently allocated (note; actual mem usage may be greater due to block size & fragmentation)
		uint32				mAllocCount;	//	number of individual allocations (ie, #blocks in heap)
		uint32				mAllocBytesPeak;
		uint32				mAllocCountPeak;

	private:
		uint64				mId;			//	unique id. Just a counter atm
	};


	//-----------------------------------------------------------------
	//	item containing debug tracking information per allocation
	//-----------------------------------------------------------------
	class HeapDebugItem
	{
	public:
		static const int	CallStackSize = 5;
	public:
		BufferString<200>	ToString() const;

		inline bool					operator==(const void* Object) const	{	return mObject == Object;	}

	public:
		const void*						mObject;		//	allocated data
		uint32							mElements;		//	number of elements
		uint32							mTypeSize;		//	sizeof(T)
		const char*						mTypename;		//	gr: this SHOULD be safe, all strings from GetTypeName are either compile-time generated or static.
		uint64							mAllocTick;		//	time of allocation (ofGetElapsedTimeMillis())
		BufferArray<uint64,CallStackSize>	mCallStack;		//	each is an address in the process' symbol data
	};

	//-----------------------------------------------------------------
	//	debug information for a heap, interface. 
	//-----------------------------------------------------------------
	class HeapDebugBase
	{
	public:
		virtual ~HeapDebugBase()	{}

		template<typename T>
		void			OnAlloc(const T* Object,uint32 Elements)
		{
			const char* TypeName = Soy::GetTypeName<T>();
			OnAlloc( Object, TypeName, Elements, sizeof(T) );
		}
		virtual void	OnFree(const void* Object)=0;
		virtual void	DumpToOutput(const prmem::HeapInfo& OwnerHeap)const =0;	//	debug-print out all our allocations and their age

	protected:
		virtual void	OnAlloc(const void* Object,const char* TypeName,uint32 ElementCount,uint32 TypeSize)=0;
		void			DumpToOutput(const prmem::HeapInfo& OwnerHeap,ArrayBridge<HeapDebugItem>& AllocItems) const;	//	debug-print out all our allocations and their age
	};

	//-----------------------------------------------------------------------
	//	heap - allocates a windows memory heap and provides allocation/free functions
	//-----------------------------------------------------------------------
	class Heap : public HeapInfo
	{
	public:
		Heap(bool EnableLocks,bool EnableExceptions,const char* Name,uint32 MaxSize=0,bool DebugTrackAllocs=false);
		~Heap();

		virtual HANDLE					GetHandle() const			{	return mHandle;	}
		virtual void					EnableDebug(bool Enable);	//	deletes/allocates the debug tracker so we can toggle it at runtime
		virtual const HeapDebugBase*	GetDebug() const			{	return mHeapDebug;	}
		inline bool						IsHeapValid() const			{	return mHandle!=NULL;	}	//	same as IsValid, but without using virtual pointers so this can be called before this class has been properly constructed

		//	prob the system for the allocation size
		uint32	GetAllocSize(void* pData) const
		{
			DWORD Flags = 0;
			SIZE_T AllocSize = HeapSize( GetHandle(), Flags, pData );
			return AllocSize;
		};

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
		TYPE*	AllocArray(const uint32 Elements)	
		{
			TYPE* pAlloc = RealAlloc<TYPE>( Elements );
			if ( !pAlloc )
				return NULL;
			
			//	construct with placement New
			if ( Soy::DoConstructType<TYPE>() )
			{
				for ( uint32 i=0;	i<Elements;	i++ )
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
		TYPE*	Alloc(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3)	
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
		TYPE*	Alloc(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3,const ARG4& Arg4)	
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
		TYPE*	Alloc(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3,const ARG4& Arg4,const ARG5& Arg5)	
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
		ofPtr<TYPE>	AllocPtr()	
		{
			TYPE* pAlloc = Alloc<TYPE>( );
			return ofPtr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
		}

		//	alloc into a smart pointer
		template<typename TYPE,typename ARG1>
		ofPtr<TYPE>	AllocPtr(ARG1& Arg1)	
		{
			TYPE* pAlloc = Alloc<TYPE>( Arg1 );
			return ofPtr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
		}

		//	alloc into a smart pointer
		template<typename TYPE,typename ARG1,typename ARG2>
		ofPtr<TYPE>	AllocPtr(ARG1& Arg1,ARG2& Arg2)	
		{
			TYPE* pAlloc = Alloc<TYPE>( Arg1, Arg2 );
			return ofPtr<TYPE>( pAlloc, HeapFreeFunctor<TYPE>(*this) );
		}

		template<typename TYPE>
		bool	Free(TYPE* pObject)
		{
			//	destruct
			pObject->~TYPE();
			if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
				Debug_Validate();

			return RealFree( pObject, 1 );
		}			

		template<typename TYPE>
		bool	FreeArray(TYPE* pObject,uint32 Elements)
		{
			//	no need to destruct types we don't construct
			if ( Soy::DoConstructType<TYPE>() )
			{
				//	destruct in reverse order; http://www.ezdefinition.com/cgi-bin/showsubject.cgi?sid=409
				uint32 e = Elements;
				while ( e )
					pObject[--e].~TYPE();
		
				if ( ENABLE_DEBUG_VERIFY_AFTER_CONSTRUCTION )
					Debug_Validate();
			}

			return RealFree( pObject, Elements );
		}

	private:
		template<typename TYPE>
		inline TYPE*	RealAlloc(const uint32 Elements)	
		{
			Debug_Validate();
			TYPE* pData = static_cast<TYPE*>( HeapAlloc( mHandle, 0x0, Elements*sizeof(TYPE) ) );
			if ( mHeapDebug )
				mHeapDebug->OnAlloc( pData, Elements );
			OnAlloc( pData ? Elements*sizeof(TYPE) : 0, 1 );
			return pData;
		}

		template<typename TYPE>
		inline bool	RealFree(TYPE* pObject,const uint32 Elements)	
		{
			//	no need to specify length, mem manager already knows the real size of pObject
			if ( !HeapFree( mHandle, 0, pObject ) )
				return false;

			if ( mHeapDebug )
				mHeapDebug->OnFree( pObject );

			uint32 BytesFreed = sizeof(TYPE)*Elements;
			OnFree( BytesFreed, 1 );
			return true;
		}

	private:
		HeapDebugBase*		mHeapDebug;	//	debug information
		HANDLE				mHandle;	//	win32 handle to heap
	};

	//-----------------------------------------------------------------------
	//	interface to the hidden default crt heap (where new & delete are)
	//-----------------------------------------------------------------------
	class CRTHeap : public HeapInfo
	{
	public:
		CRTHeap() : 
			HeapInfo	( "Default CRT Heap" )
		{
		}

		virtual HANDLE			GetHandle() const;

		void					Update();			//	update tracking information
	};



	//	allocator that can be used to bridge our Heap allocator, and an allocator for STD types
	//		eg; std::vector<int,prmem::HeapStdAllocator<int> > v;
	//	from http://www.josuttis.com/libbook/memory/myalloc.hpp.html
	//		http://blogs.msdn.com/b/calvin_hsia/archive/2010/03/16/9979985.aspx
	//	
	//	see unit tests for usage as it's messy and a bit complicated to example here.
	//	
	template <class T>
	class StdHeap
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
			typedef StdHeap<U> other;
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

		/* constructors and destructor
		* - nothing to do because the allocator has no state
		*/
		StdHeap(prmem::Heap& Heap=prcore::Heap) throw() :
			mHeap	( &Heap )
		{
		}
		StdHeap(const StdHeap& that) throw() :
			mHeap	( that.mHeap )
		{
		}
		/*
		template <class U>
		StdHeap (const StdHeap<U>&) throw() 
		{
		}
		*/
		~StdHeap() throw() 
		{
		}

		// return maximum number of elements that can be allocated
		size_type max_size () const throw()
		{
			//	gr: maybe add this to Heap for when there's a heap limit...
			return std::numeric_limits<std::size_t>::max() / sizeof(T);
		}

		// allocate but don't initialize num elements of type T
		pointer allocate (size_type num, const void* = 0) 
		{
			return mHeap->AllocArray<T>( static_cast<uint32>(num) );
		}

		// initialize elements of allocated storage p with value value
		void construct (pointer p, const T& value) 
		{
			// initialize memory with placement new
			new((void*)p)T(value);
		}

		// destroy elements of initialized storage p
		void destroy (pointer p) 
		{
			// destroy objects by calling their destructor
			p->~T();
		}

		// deallocate storage p of deleted elements
		void deallocate (pointer p, size_type num) 
		{
			mHeap->FreeArray( p, static_cast<uint32>(num) );
		}

		operator	prmem::Heap&()	{	return *mHeap;	}	//	really easy bodge to get constructors in constructors in constructors working (messy STL!)

	public:
		prmem::Heap*	mHeap;
	};

	// return that all specializations of this allocator are interchangeable
	template <class T1, class T2>
	bool operator== (const StdHeap<T1>&,const StdHeap<T2>&) throw() 
	{
		return true;
	}

	template <class T1, class T2>
	bool operator!= (const StdHeap<T1>&,const StdHeap<T2>&) throw() 
	{
		return false;
	}


};

DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( prmem::HeapDebugItem );

//	class that (HUGELY) simplifies an std::Map that uses a Heap for allocation
//	turn 
//		std:Map<A,B> myMap;
//	into
//		StdMapHeap<A,B> myMap( prcore::Heap );
template<typename A,typename B,prmem::Heap& HEAP=prcore::Heap>
class StdMapHeap : public std::map<A,B,std::less<A>,prmem::StdHeap<std::pair<A,B>>>
{
public:
	StdMapHeap(prmem::Heap& Heap=HEAP) :
		std::map<A,B,std::less<A>,prmem::StdHeap<std::pair<A,B>>>	( std::less<A>(), prmem::StdHeap<std::pair<A const,B>>( Heap ) )
	{
	}
};

template<typename A,prmem::Heap& HEAP=prcore::Heap>
class StdQueueHeap : public std::queue<A,std::deque<A,prmem::StdHeap<A>>>
{
public:
	StdQueueHeap(prmem::Heap& Heap=HEAP) :
		std::queue<A,std::deque<A,prmem::StdHeap<A>>>	( std::deque<A,prmem::StdHeap<A>>(Heap) )
	{
	}
};
	

/*
//-------------------------------------------------
//	not as a simple replace of new with pr_new
//		Bitmap* pBitmap = new Bitmap( X, Y, Z );
//	changes to
//		Bitmap* pBitmap = prnew<Bitmap>( X, Y, Z );
//	and 
//		delete pBitmap;
//	with
//		prdelete(pBitmap);
//-------------------------------------------------
template<typename TYPE> 
inline TYPE*	prnew()						
{
	return prcore::Heap.Alloc<TYPE>();
}
template<typename TYPE,typename ARG1> 
inline TYPE*	prnew(const ARG1& Arg1)						
{
	return prcore::Heap.Alloc<TYPE>( Arg1 );	
}
template<typename TYPE,typename ARG1,typename ARG2> 
inline TYPE*	prnew(const ARG1& Arg1,const ARG2& Arg2)	
{	
	return prcore::Heap.Alloc<TYPE>( Arg1, Arg2 );	
}
template<typename TYPE,typename ARG1,typename ARG2,typename ARG3> 
inline TYPE*	prnew(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3)	
{	
	return prcore::Heap.Alloc<TYPE>( Arg1, Arg2, Arg3 );	
}
template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4> 
inline TYPE*	prnew(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3,const ARG4& Arg4)	
{	
	return prcore::Heap.Alloc<TYPE>( Arg1, Arg2, Arg3, Arg4 );	
}
template<typename TYPE,typename ARG1,typename ARG2,typename ARG3,typename ARG4,typename ARG5> 
inline TYPE*	prnew(const ARG1& Arg1,const ARG2& Arg2,const ARG3& Arg3,const ARG4& Arg4,const ARG5& Arg5)	
{	
	return prcore::Heap.Alloc<TYPE>( Arg1, Arg2, Arg3, Arg4, Arg5 );	
}
template<typename TYPE>
inline void	prdelete(TYPE* pObject)	
{
	prcore::Heap.Free( pObject );	
} 

*/
