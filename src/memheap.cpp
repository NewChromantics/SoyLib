/*

Improved performance in mutli-threaded applications with changes to memory usage
=======================================================
Here is an outline to how to improve performance of applications with changes to memory
management rather than general code optimisation;

Multithread Problems;
-------------------------------------------
In traditional single threaded applications (and inidivually busy threads) the main bottleneck
is simply execution time (waiting for disk i/o, large loops, lots of iteration etc etc).
We can improve performance here still with our own objects which have several cpu-time-
saving improvements; though usually the one-size-fits-all approach does work we can include 
minor improvements. These are outlined below under "Other objects".

In multi-threaded applications a significant amount of time is (unneccessarily) wasted with locks, 
waiting for other threads to finish working with a shared object. But one transparent area this 
includes is memory allocation and deallocation. When we use new/delete (or malloc/free) all the 
allocations go to one default heap (known as the CRT heap as it's created in the CRT).
Every time we allocate or free memory the heap is locked to do the work. When many threads are
allocating and deallocating a lot of little things (eg. building strings) this causes a lot of locking
and pausing and increasing the contention between threads.

Then there is the added cpu time taken when allocating all our data from one heap; there's a lot of fragmentation
and the more we have allocated the longer the heap-code takes to allocate and deallocate (deallocating large
blocks of data is especially slow)
Reducing fragmentation(speeding up alloc&dealloc time) is a minor improvement compared to the main
issue of blocking threads with locks, so that is our main target.


Solution A) Reduce Allocations with Buffer objects
-------------------------------------------
The first big solution is to wholly reduce allocations, this is the biggest improvement not only because
we're reducing contention to the threads but we're reducing the use of the memory routines which can be slow
(locking, searching for free blocks, setting debug patterns, checking for corruption etc).

The main way to do this is allocate objects on the stack. To do this in most cases we just want to replace
strings and arrays with "Buffer" strings and arrays. These are fixed-allocation sized versions of the normal
dynamic strings & arrays. They use the same (or near-identical) templates for implementation so in most cases
you just need to replace the variable type and no need to change any other code.

The worst offender for allocations is definately strings. This example will incur around 19 allocations due 
to the use of unknown-length strings, and copies (and an additional copy for the caller). If the string "abc" 
becomes a 100-character string the number of allocations will be around 400.

[BAD]
	String RepeatString(const char* xyz)
	{
		String Result;
		Result << xyz << xyz << xyz << xyz;
		return Result;
	}
	String MyString = RepeatString( "abc" );

One easy and common way to improve this would be to make the Result a static and return a reference to it. A good
improvement, but then makes the function non-threadsafe and we can no longer do
	MyString << RepeatString("x") << RepeatString("y");

This replacement will cause zero allocations, (no locks!) be magnitudes faster (huge amounts will be inlined) and 
easy to replace.

[BEST]
	BufferString<1000> RepeatString(const BufferString<100>& xyz)
	{
		BufferString<1000> Result;
		Result << xyz << xyz << xyz << xyz;
		return Result;
	}
	BufferString<1000> MyString = RepeatString( "abc" );

When deciding what to replace from a String to a BufferString, the most obvious and best cases to replace first are for
very short strings that we KNOW will be short, or a restricted length (eg. filenames should never be more than MAX_PATH)

[BAD]
	String Input;
	Input << "Input" << (x+1);
	SetName( Input );

[BEST]
	BufferString<10> InputName;
	InputName << "Input" << (x+1);
	SetName( InputName );

There are also many cases where we return a string from a function, notable examples are the file-system related functions to
extract parts of a filename. These cause unneccessary allocations by returning String's (which need to be allocated and then copied).
Though this can be improved by sending an output string by reference, a less obtrusive change is just to make it return a buffer string.
Again, we know these filenames are going to be a limited length, so we can pretty-safely return fixed length strings instead;

	BufferString<MAX_PATH>	GetFilenameEXT(const BufferString<MAX_PATH>& filename);		// get extension of given filename
	BufferString<MAX_PATH>	GetFilenamePATH(const BufferString<MAX_PATH>& filename);	// get path of given filename
	BufferString<MAX_PATH>	GetFilenameNOEXT(const BufferString<MAX_PATH>& filename);	// get filename without extension
	BufferString<MAX_PATH>	GetFilenameNOPATH(const BufferString<MAX_PATH>& filename);	// get filename without path

Passing the initial argument as either a const char* or BufferString<> is up to the programmer, but generally if various String2 
routines are going to be used (Split(), searches, tolower() etc) and we construct a temporary string anyway, we can gain a small 
benefit by having the argument as a buffer string for a quicker string-copy to initialise the 'filename' argument. 
This is only a minor benefit though, the main benefit is removing any String parameters;
	
	[BAD]	String					GetFilenamePath(const String& filename);
	[OKAY]	BufferString<MAX_PATH>	GetFilenamePath(const String& filename);
	[GOOD]	BufferString<MAX_PATH>	GetFilenamePath(const char* filename);
	[BEST*]	BufferString<MAX_PATH>	GetFilenamePath(const BufferString<MAX_PATH>& filename);
		
		(* if a BufferString is to be constructed inside the function anyway)


Caveats;
	* If the buffer array/string is too small, we will throw an assert. Worse, in release we return NULL results
		which our code generally does not handle very well (for example PushBack() will dereference this regardless and crash)
		So ALWAYS make sure the buffer is big enough. Some safety can be implemented using the MaxSize() function which
		will give you the limit. This function also exists for the dynamic arrays but only reports the current allocation rather
		than the theoretical maximum.

	* In our applications' cases we don't have to worry about the size of the stack objects so very large buffer Array/Strings are
		fine, but in some cases where the stack can get very large (ie, very long with lots of temporaries & arguments or very 
		recursive) functions, we will run out of stack space.
		If this is ever the case and speed is still a neccessity, we may be able to increase stack size.


Arrays work in very much the same way as the strings. BufferArray<TYPE,SIZE> works just like an Array, it's dynamic, grows and
shrinks but has a limited amount of space. Other than the same benefit as the strings the buffer array can also be used as a 
bounds-safe version of a C-array. Once all the debug has been removed in a release build and everything is inlined, there is 
very little difference (if any) performance wise making this a wise decision to update code... just-in-case;

	//color32	VertexColours[3];				//	only line that needs changing
	BufferArray<color32,3>	VertexColours(3);	//	initialise to size of 3 so [] doesn't assert
	VertexColours[0] = prcore::Color::Red;
	VertexColours[1] = prcore::Color::Green;
	VertexColours[2] = prcore::Color::Blue;

Unfortunetly we CANNOT do this;

	//color32	VertexColours[3] =
	BufferArray<color32,3>	VertexColours(3) =
	{
		prcore::Color::Red, prcore::Color::Green, prcore::Color::Blue
	};

But the difference should be negligable (especially in a project our size in a release build) and adding the safety is worth 
it as well as being able to use VertexColours.GetSize() instead of magic numbers or sizeof(x/x) which could be missed when 
changing code.



Solution B) Remove contention
-------------------------------------------
One of the biggest speed ups we got from RVWS was to remove thread-contention during allocations. As described above, the quick benefit
is to remove allocations altogether. Obviously there are many cases where this isn't possible so we want to remove the contention to
the memheap lock. We do this by allocating from a different heap. This is what the prmem::Heap class is for, it is an interface to the windows
API heaps; We use the winAPI heaps rather than a 3rd party implementation just because we know it's reliable, other windbg style tools can use
it and it has inherent,reliable features (exceptions, guard checking, and turning OFF features - see "Faster HEAP allocations")
http://msdn.microsoft.com/en-us/library/windows/desktop/aa366599(v=vs.85).aspx
This is the constructor. Make it a member, a global, or even on the stack (though aside from explicitly checking for leaks and corruption this
is probably a bit of a waste at initilisation & freeing)

	//	in most cases, Locks and Exceptions should be on. A zero max size is "infinite"
	prmem::Heap(bool EnableLocks,bool EnableExceptions,const char* Name,uint32 MaxSize=0);

The heap is still threadsafe as it has locks when allocating or freeing memory still, but if only one thread is using this heap then 
the contention disapears. As a general rule it seems good to give each thread it's own heap as a member. Global heaps are also fine, the 
directx code has it's own heap, though hidden in the directx namespace. The benefit of any global heaps is that you can specific the heap
that a HeapArray uses at type-time, though this is used less and less in favour of SetHeap() (really just because we don't have many global 
heaps)
As it's threadsafe, you can also delete memory from another thread's heap, you just need access to it. This is harder to work with. 
The Bitmap class has been explicitly modified to allow this as we pass bitmaps around to different threads, but due to the size of the 
allocations we DON'T want to be allocating it in the default heap. Though a global bitmap heap might help here, we allocate a number of 
bitmaps "at the same time" when we kick off some bitmap processing so the contention might not really disapear.

The bitmaps in particular highlighted the fact that DEALLOCATING large blocks of data (512x512x4) can be slow, and thus if there is contention
(for example threadB is deleting a bitmap from threadA's heap, whilst threadA is allocating (perhaps another bitmap, or maybe just some temporary 
arrays) this can still be noticed in profiling. 
In this case it's something we have to live with, without a custom solution (A bitmap pool/cache perhaps), but when the contention is just between
2-4 threads this is a significant gain from 20 threads using the default heap.

Where we used to have temporary arrays (which cannot be converted to BufferArrays) on a thread, these should be converted to a HeapArray, 
again, it might not seem much, but in the grand scheme of things, the less that use the default heap the better.

	//Array<RVPolySkin*> Polyskins;
	HeapArray<RVPolySkin*> Polyskins( mHeap );
	//	or
	HeapArray<RVPolySkin*> Polyskins;
	Polyskins.SetHeap( mHeap );

HeapArray's are the same as normal Array's except they allocate from a specific heap IF one has been set. If not, the default heap (new&delete) will
be used when allocating.
The heap can be set at type-time (this can only be used with global heaps) or after construction with .SetHeap().
Currently the code does not support changing of heaps, so if SetHeap() is called and some memory has already been allocated[on a different heap] an assert
will occur and the change won't happen.

To help the transition from one array type to another without having to change a lot of functions, duplicating them or templating (and thereby forcing the
code into headers) an ArrayBridge type has been created so these functions can use any array type. (See ArrayBridge.h for details)

These heaps can also be used with STD containers for any code that's using some STD array types we haven't implemented (eg. Maps)
See the prmem::StdHeap type.

Another benefit to using our own memory heap is that we can monitor the usage of the heaps, for example RVWS now shows how much memory is currently allocated
with a simple byte counter, we also track the peak amount of allocation and could easily generate things like allocations/second, and even (if we wanted to go
this far) we could track WHERE blocks of data were allocated from (using a macro and __LOCATION__ and storing this info with the allocated block) to detect leaks.



Solution C) Faster Array/Memory Allocations/Copies
-------------------------------------------

	struct MyNonComplexStruct	//	or "POD type" (Plain old data)
	{
		matrix4x4f			mMatrix;
		BufferArray<int,4>	mInts;
		BufferString<255>	mName;
	};
	class MyComplexClass
	{
		MyComplexClass() : mPointer	( NULL )	{}
		int*				mPointer;
	};
	class MyDerived : public MyBaseClass
	{
		virtual ~MyDerived()	{}	//	I have a vtable
	};

Our Array implemention, when copying elements (either from a reallocation, or copying another array) does a simple for loop and copies the elements
one by one with the = operator. This is fine, and will copy complex and non-complex alike. When you're copying 1000 floats though, the loop is a little
excessive, especially when each = is using the [] operator (with it's debug bound-checking asserts) for safety. A memcpy will do this faster and just as 
safe. A memcpy is good for base types and MyNonComplexStruct, but a memcpy is no good for MyComplexClass or MyDerived.
To allow this, we have a macro which defines a type (inlined at compile-time, using templates, no lookup tables etc) as Non-Complex which we then use to
allow a memcpy, or memset, or memmove when copying/reallocating data. A good speed up and explicitly defined by us.
	
	DECLARE_NONCOMPLEX_TYPE( float );
	DECLARE_NONCOMPLEX_TYPE( prcore::MyNonComplexStruct );	//	note: needs to be done at global scope outside of namespaces!

Similarly with the copies, most, if not all, implementations of memory managers or dynamic arrays construct their elements (eg. new and new[] do this)
after allocating their space. By default, we still do this, even for the non-complex types. A non-complex type can be dumb-copied, but may still want 
to be initialised.
But as we're allocating the data ourselves for a type we have to explicitly construct and destruct these elements (via "placement new").
Sometimes, we might have a dumb-non-complex type that doesn't NEED or have a constructor (for example, data which is then always initialised manually)
By using the macro DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE, we not only decalre it non-complex, but when an array of our type is allocated from one of OUR 
memheaps the data is NOT constructed. Instead the data stays as the heap gave it to us; "uninitialised", in debug this should be a pattern (0xCD for CleanData)
and in release, it could be anything.
After some tests, this IS a noticable speedup, even with arrays of floats or bytes. It's especially a big improvement if we're allocating (for example)
1000 vertexes in one-array which no-longer need construction.

	DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE( prcore::MyNonComplexStruct );



Solution D) Faster HEAP allocations
-------------------------------------------
Even in release, the windows heap's (including the default CRT heap) are doing debug work when allocating. Checking for bounds corruption, throwing exceptions
setting memory patterns etc. Turning this off gives a noticable speed increase and is a simple option;
	set _NO_DEBUG_HEAP environment variable to 1.
	http://stackoverflow.com/questions/6486282/set-no-debug-heap

Another option we have availible to us with these heaps is to remove the locks in our heap. Obviously this should ONLY be done when we KNOW a heap is only going
to be accessed by one heap. The private directx heap does this, though it does not show any significant improvement in that case.
This stackoverflow question suggests this might not be a good idea, BUT it is an option for us for future improvements if we really want to squeeze more speed out.
http://stackoverflow.com/questions/1983563/reason-for-100x-slowdown-with-heap-memory-functions-using-heap-no-serialize-on



Object Summary
-------------------------------------------
Strings;
	The string type has been refactored so that it can take any array type that fits functional critera (Resizing functions)
	so if we need to add any other array types in future (eg. pools) we can use them too.

[BEST]
	BufferString<XXX>	Dynamic string on the stack (when temporary) or of a fixed length member. Allows an object to become non-complex, 
						causes no allocations and is by far the fastest and most preffered use when length can be vaguely determined.

[GOOD]
	HeapString<HEAP*>	String allocated from a particular heap. The heap can be specified at type-time or set manually afterwards.
						There is rarely a need for this string, but added for completeness and we may need this sometime for very
						large strings such as XML, file or HTTP output seperated on threads.
[BAD]
	String				The original string, allocates from the default heap. Whereever possible, replace with a BufferString.


Arrays;
	This is the core of our speedups as the String types use these arrays, and we [rightly] use these arrays all over the code for safety etc

[BEST]
	BufferArray<TYPE,XX>	This is a dynamic array, just like the Array but of a fixed size. By far the most efficient and inlines to pretty
							much just a normal c-array access.
							It is also non-complex so if a BufferArray is inside a struct with no other complex types (no virtuals, no pointers etc)
							it can be declared DECLARE_NONCOMPLEX_TYPE and speed up reallocations etc inside other arrays.
[GOOD]
	HeapArray<TYPE,HEAP*>	Same as Array, data is allocated when needed and re-allocated. The HEAP* in the type is optional and infrequently used
							due to the flexiblity we need.
							This type can wholly replace Array as if no Heap is assigned the default heap (new&delete) is used.
[BAD]
	Array					The original dynamic array, always allocates from the default heap and really should be phased out for HeapArrays.


SortArrays;
	The same derivitives exist for the SortArrays as Arrays and Strings. Like the strings, a base-sort-array type has been allocated which uses
	an array as a policy class for storage, ensuring all interfaces and implementations are the same no matter where the data is allocated from.

	

Improvement summary
-------------------------------------------
	* Reduce allocations entirely by using BufferArrays/BufferStrings (or at least pre-allocs) to replace Arrays/Strings
	* Don't use non-reference/pointer String's as parameters or returned objects.
	* Don't return static strings in small functions (not threadsafe)
	* When allocating individual objects and using arrays use thread-specific heaps, even for temporary arrays and non-members.
	* Avoid using new/delete (default heap) entirely.
	* Declare memcpy'able structs & clases with DECLARE_NON_COMPLEX and DECLARE_NONCOMPLEX_NO_CONSTRUCT_TYPE whenever possible.
	* When constructing very large strings (eg. html/xml output) build it up with smaller bufferstrings rather than continually building one giant String to reduce allocations)
*/
#include "ofxSoylent.h"



namespace prmem
{
	//	global list of our heaps
	//	this needs to be before any other global Heap!
	BufferArray<HeapInfo*,100>		gMemHeapRegister;
}

namespace prcore
{
	//	"default" heap for prnew and prdelete which we'd prefer to use rather than the [unmonitorable] crt default heap
	prmem::Heap		Heap( true, true, "prcore::Heap" );
};

namespace prmem
{
	//	heap for unit tests
	prmem::Heap		gTestHeap( true, true, "Test Heap" );

	//	crt heap interface
	prmem::CRTHeap	gCRTHeap;

	//	real debug tracker class. Cannot be declared in header!
	class HeapDebug : public HeapDebugBase
	{
	public:
		HeapDebug(const Heap& OwnerHeap);
		virtual ~HeapDebug()	{}

		virtual void	OnFree(const void* Object);
		virtual void	OnAlloc(const void* Object,const char* Typename,uint32 ElementCount,uint32 TypeSize);
		virtual void	DumpToOutput(const prmem::HeapInfo& OwnerHeap) const	{	HeapDebugBase::DumpToOutput( OwnerHeap, GetArrayBridge( mItems ) );	}

	protected:
		Heap					mHeap;	//	heap to contain our data to keep it away from the heaps we're debugging!
		Array<HeapDebugItem>	mItems;
	};
}
DECLARE_TYPE_NAME( prmem::HeapDebug );


prmem::CRTHeap& prmem::GetCRTHeap()
{
	return gCRTHeap;
}


prmem::HeapDebug::HeapDebug(const Heap& OwnerHeap) :
	mHeap	( true, true, BufferString<100>() << "DEBUG " << OwnerHeap.GetName() , 0, false ),
	mItems	( mHeap )
{
	//	prealloc items so we don't slow down
	mItems.Reserve( 10000 );
}


//	in prgen/timeservices.hpp
namespace prgen
{
	extern uint32 GetTick32();
}


void prmem::HeapDebug::OnAlloc(const void* Object,const char* Typename,uint32 ElementCount,uint32 TypeSize)
{
	//	do some prealloc when we get to the edge
	if ( mItems.MaxSize() - mItems.GetSize() < 10 )
	{
		mItems.Reserve( 10000, false );
	}

#if defined(ENABLE_STACKTRACE)
	//	get the callstack
	BufferArray<PRExceptionHandler::PRStackEntry,HeapDebugItem::CallStackSize> Stack;
	int StackSkip = 0;
	StackSkip ++;	//	HeapDebug::OnAlloc
	StackSkip ++;	//	Heap::RealAlloc
	StackSkip ++;	//	Heap::Alloc*
	GetCallStack( GetArrayBridge(Stack), StackSkip );
	
	HeapDebugItem& AllocItem = mItems.PushBack();
	AllocItem = HeapDebugItem();	//	make sure buffer arays are initialised
	AllocItem.mObject = Object;
	AllocItem.mElements = ElementCount;
	AllocItem.mTypeSize = TypeSize;
	AllocItem.mTypename = Typename;
	AllocItem.mAllocTick = prgen::GetTick32();

	//	save callstack address
	for ( int i=0;	i<prmin(AllocItem.mCallStack.MaxSize(),Stack.GetSize());	i++ )
		AllocItem.mCallStack.PushBack( Stack[i].mProcessAddress );		
#endif
}

void prmem::HeapDebug::OnFree(const void* Object)
{
	int Index = mItems.FindIndex( Object );

	//	not found?
	if ( Index < 0 )
		return;

	assert( mItems[Index].mObject == Object );
	mItems.RemoveBlock( Index, 1);	
}


const BufferArray<prmem::HeapInfo*,100>& prmem::GetHeaps()
{
	//	update the CRT heap info on-request
	gCRTHeap.Update();

	return gMemHeapRegister;
}



prmem::HeapInfo::HeapInfo(const char* Name) :
	mName			( Name ),
	mAllocBytes		( 0 ),
	mAllocCount		( 0 ),
	mAllocBytesPeak	( 0 ),
	mAllocCountPeak	( 0 )
{
	gMemHeapRegister.PushBack( this );
}

prmem::HeapInfo::~HeapInfo()
{
	//	unregister
	gMemHeapRegister.Remove(this);
}





prmem::Heap::Heap(bool EnableLocks,bool EnableExceptions,const char* Name,uint32 MaxSize,bool DebugTrackAllocs) :
	HeapInfo		( Name ),
	mHandle			( NULL ),
	mHeapDebug		( NULL )
{
	DWORD Flags = 0x0;
	Flags |= EnableLocks ? 0x0 : HEAP_NO_SERIALIZE;
	Flags |= EnableExceptions ? HEAP_GENERATE_EXCEPTIONS : 0x0;
	mHandle = HeapCreate( Flags, 0, MaxSize );

	EnableDebug( DebugTrackAllocs );
}

prmem::Heap::~Heap()
{
	if ( mHandle != NULL )
	{
		if ( !HeapDestroy( mHandle ) )
		{
			//error
		}
		mHandle = NULL;
	}
}


BufferString<200> prmem::HeapDebugItem::ToString() const
{
	BufferString<200> out;

	out.PrintText("0x%08x", mObject );
	out << " " << Soy::FormatSizeBytes( mTypeSize*mElements );
	out << " (" << mTypename << " x" << mElements << ")";

	return out;
}

//----------------------------------------------
//	deletes/allocates the debug tracker so we can toggle it at runtime
//----------------------------------------------
void prmem::Heap::EnableDebug(bool Enable)
{
	if ( (mHeapDebug!=NULL) == Enable )
		return;

	if ( !Enable )
	{
		//	NULL the heap debug pointer first as we're going to remove it, then try and report the delete to itself
		HeapDebugBase* pHeapDebug = mHeapDebug;
		mHeapDebug = NULL;
		Free( pHeapDebug );
	}
	else
	{
		mHeapDebug = Alloc<prmem::HeapDebug>( *this );
	}
}


//----------------------------------------------
//	debug-print out all our allocations and their age
//----------------------------------------------
void prmem::HeapDebugBase::DumpToOutput(const prmem::HeapInfo& OwnerHeap,ArrayBridge<HeapDebugItem>& AllocItems) const
{
	{
		BufferString<1200> Debug;
		Debug << "-------------------------------------------------\n";
		Debug << AllocItems.GetSize() << " objects allocated on heap " << OwnerHeap.GetName();
		Debug << " (Heap reports " << OwnerHeap.GetAllocCount() << " objects allocated.)";
		Debug << "\n";
		ofLog( OF_LOG_VERBOSE, Debug );
	}

#if defined(ENABLE_STACKTRACE)
	for ( int i=0;	i<AllocItems.GetSize();	i++ )
	{
		//	gr: big string, but if we break out OutputDebugString, other threads will interrupt it
		BufferString<1200> Debug;
		auto& AllocInfo = AllocItems[i];

		Debug << i << "/" << (AllocItems.GetSize()-1) << ": ";
		Debug << AllocInfo.ToString();
		
		//	show age
		int AgeSecs = (prgen::GetTick32() - AllocInfo.mAllocTick) / 1000;
		Debug << " " << AgeSecs << " secs ago.";

		//	show callstack
		if ( !AllocInfo.mCallStack.IsEmpty() )
		{
			Debug << " Allocated from...\n";
		
			for ( int i=0;	i<AllocInfo.mCallStack.GetSize();	i++ )
			{
				Debug << "\t";
				CodeLocation Location;
				PRExceptionHandler::GetSymbolLocation( Location, AllocInfo.mCallStack[i] );
				Debug << static_cast<const char*>(Location) << ": ";

				BufferString<200> FunctionName;
				PRExceptionHandler::GetSymbolName( FunctionName, AllocInfo.mCallStack[i] );
				Debug << FunctionName << "\n";
			}
		}
		else
		{
			Debug << " No callstack.\n";
		}

		ofLog( OF_LOG_VERBOSE, Debug );
	}
#endif
}

void GetProcessHeapUsage(uint32& AllocCount,uint32& AllocBytes)
{
	/*
	//	count process heaps so we can buffer for it
	uint32 NumHeaps = GetProcessHeaps( 0, NULL );

	//	error, check win32 lasterror
	if ( NumHeaps == 0 )
		return;
		
	PHANDLE aHeaps;
	SIZE_T BytesToAllocate;
	HRESULT Result = SIZETMult( NumHeaps, sizeof(*aHeaps), &BytesToAllocate);
    if (Result != S_OK)
        return;

	//	allocate the handle
	aHeaps = static_cast<PHANDLE>( new char[BytesToAllocate] );

	//	get heaps
	uint16 NewNumHeaps = GetProcessHeaps( NumHeaps, aHeaps );

	//	make sure result was okay
	if ( NewNumHeaps == NumHeaps )
	{
		for ( int h=0;	h<NumHeaps;	h++ )
		{
			void* HeapStartAddr = aHeaps[h];
		}
    }

	delete[] aHeaps;
	*/
}

void GetCRTHeapUsage(uint32& AllocCount,uint32& AllocBytes)
{
	_CrtMemState MemState;
	ZeroMemory(&MemState,sizeof(MemState));
	_CrtMemCheckpoint( &MemState );
	
	uint32 RuntimeAllocCount = static_cast<uint32>( MemState.lCounts[_NORMAL_BLOCK] );
	uint32 RuntimeAllocBytes = static_cast<uint32>( MemState.lSizes[_NORMAL_BLOCK] );

	//	gr: not including this (only a few kb) so we can hopefuly track allocations down to zero on main heap
	//	_CRT_BLOCK is statics/globals
	uint32 CrtAllocCount = static_cast<uint32>( MemState.lCounts[_CRT_BLOCK] );
	uint32 CrtAllocBytes = static_cast<uint32>( MemState.lSizes[_CRT_BLOCK] );

	AllocCount += RuntimeAllocCount;
	AllocBytes += RuntimeAllocBytes;

	//AllocCount += CrtAllocCount;
	//AllocBytes += CrtAllocBytes;
}

//----------------------------------------------
//	update tracking information
//----------------------------------------------
void prmem::CRTHeap::Update()
{
	//	gr: to hook HeapCreate (ie from external libs) 
	//	http://src.chromium.org/svn/trunk/src/tools/memory_watcher/memory_hook.cc

	//	gr: for runtime debugging, just turn off this static to avoid the calls
	static bool EnableHeapUpdate = true;
	if( !EnableHeapUpdate )
		return;

	//	gr: I've added a throttle to this, _CrtMemCheckpoint() actually takes between 
	//		0 and 1 ticks to execute, but it may be locking the heap which might intefere with other threads.
	/*
	//	throttle this call to 30fps
	//static VTPollTimer UpdateHeapStatsTimer( 1000/30 );
	//if ( !UpdateHeapStatsTimer.Trigger() )
	static Timer UpdateHeapStatsTimer;
	if ( UpdateHeapStatsTimer.GetTick() < (1000/30) )
		return;
	UpdateHeapStatsTimer.Reset();
		*/
	return;
	
	uint32 CrtAllocCount=0,CrtAllocBytes=0,ProcessHeapAllocCount=0,ProcessHeapAllocBytes=0;
	GetProcessHeapUsage( ProcessHeapAllocCount, ProcessHeapAllocBytes );
	GetCRTHeapUsage( CrtAllocCount, CrtAllocBytes );

	uint32 AllocCount = CrtAllocCount + ProcessHeapAllocCount;
	uint32 AllocBytes = CrtAllocBytes + ProcessHeapAllocBytes;

	//	work out the change from last sample so we can emulate normal Heap behaviour
	uint32 AllocatedBytes = 0;
	uint32 AllocatedBlocks = 0;
	uint32 FreedBytes = 0;
	uint32 FreedBlocks = 0;

	//	more blocks have been allocated
	if ( AllocCount > mAllocCount )
		AllocatedBlocks = AllocCount - mAllocCount;
	else
		FreedBlocks = AllocCount - mAllocCount;

	//	more bytes have been allocated (may not match block increase/decrease)
	if ( AllocBytes > mAllocBytes )
		AllocatedBytes = AllocBytes - mAllocBytes;
	else
		FreedBytes = AllocBytes - mAllocBytes;

	//	call normal heap alloc/free tracker funcs
	OnAlloc( AllocatedBytes, AllocatedBlocks );
	OnFree( FreedBytes, FreedBlocks );
}
		

//----------------------------------------------
//----------------------------------------------
HANDLE prmem::CRTHeap::GetHandle() const	
{
	return GetProcessHeap()/*_get_heap_handle()*/;	
}

/*

class TestConstruct
{
public:
	TestConstruct() :
		mInteger	( 1234 )
	{
	};
	TestConstruct(int Specific) :
		mInteger	( Specific )
	{
	}

	int		mInteger;
};



TEST(HeapAllocTest, TestConstructor)
{
	TestConstruct a;
	EXPECT_EQ( 1234, a.mInteger );
	
	TestConstruct* b = prmem::gTestHeap.Alloc<TestConstruct>();
	EXPECT_EQ( 1234, b->mInteger );
	
	TestConstruct* c = prmem::gTestHeap.Alloc<TestConstruct>(8);
	EXPECT_EQ( 8, c->mInteger );
	
	//	test for any deallocation crashes etc immediadtely after (and freeing FIFO instead of FILO)
	prmem::gTestHeap.Free( b );
	prmem::gTestHeap.Free( c );
	
	
	HeapArray<TestConstruct,prmem::gTestHeap> Array;
	
	Array.SetSize(10);
	EXPECT_EQ( 1234, Array[0].mInteger );
	EXPECT_EQ( 1234, Array[1].mInteger );
	EXPECT_EQ( 1234, Array[2].mInteger );
	EXPECT_EQ( 1234, Array[3].mInteger );
	EXPECT_EQ( 1234, Array[4].mInteger );
	EXPECT_EQ( 1234, Array[5].mInteger );
	EXPECT_EQ( 1234, Array[6].mInteger );
	EXPECT_EQ( 1234, Array[7].mInteger );
	Array.Clear(true);
	
}


#include <queue>
using namespace std;

TEST(HeapAllocTest, StdHeapQueueUsage)
{
	prmem::Heap TestHeap( true, true, "std test heap" );
	{
		StdQueueHeap<int> a( TestHeap );		
		a.push(111);
		EXPECT_NE( 0, TestHeap.GetAllocCount() );
		a.push(222);
		a.push(333);
		EXPECT_EQ( 111, a.front() );
		//EXPECT_EQ( 1, a.back() );
		EXPECT_EQ( 333, a.back() );
		a.empty();
	}
	EXPECT_EQ( 0, TestHeap.GetAllocCount() );
}


TEST(HeapAllocTest, StdHeapVectorUsage)
{
	prmem::Heap TestHeap( true, true, "std test heap" );
	{
		std::vector<int,prmem::StdHeap<int>> a( TestHeap );
	
		a.push_back(111);
		EXPECT_NE( 0, TestHeap.GetAllocCount() );
		a.push_back(222);
		a.push_back(333);
		EXPECT_EQ( 111, a[0] );
		EXPECT_EQ( 222, a[1] );
		EXPECT_EQ( 333, a[2] );
		a.empty();
	}
	EXPECT_EQ( 0, TestHeap.GetAllocCount() );
}

*/
