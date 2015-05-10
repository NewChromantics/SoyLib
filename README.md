ofxSoylent
==========

My "little things" lib. Formerly to go with openframeworks, but not any more. Designed to be cross platform for ios, mac and windows, using as much stl c++11 code as possible for simplification.

Trying to clean up this code to remove dependencies so often only a few files need to be included rather than the whole lib.

Noteable code
==========
The Soy::Assert's should be particularly useful as they provide dynamic assert messages but with zero overhead when calling Soy::Assert (the callbacks, or lambda's are only instintiated or called IF the condition is not met)

SoyOpencl
==========
Multi-threaded/context, safe, auto-reloading opencl implementation. Much of the MSAOpencl code is now obsolete and has been pretty much wholly replaced. (SoyOpencl is high level, MSAOpencl is low level)
This could do with refactoring to seperate the content (.cl concatonation, includes, reloading) from the SoyRunners, which themselves might be better implemented with a generic system for mapping params, uniforms etc which can work with metal, opengl multi-threaded shaders, vulkan as they all execute in similar ways.

Heaps
==========
Multiple heap implementation on windows was AMAZING for concurrency, memory management, memory monitoring for different threads/uses. 
I want to expand this type for Shared memory (instead of shared memory as an array), heaps on buffers (so nested heaps could be possible)


Code debt
==========
Note: a lot of these are out of date, or have been replaced with content from my private PopLib (which will be moved over at some point)

Out of date:
Sockets - replaced in poplib with socket channel and cross-platform SOCKET implementation
Packet - replaced with SoyData encapsulation
XML - needs a faster implementation for large XML
Json - Finish SoyJson for a faster implementation
Commandline args - repalced with pop lib "CLI" protocol
Shape/math code - replaced with mathfu
String.hpp - replaced with using std::string and various helper functions in SoyString. Still need a good non-allocated buffer string (one based on ArrayBridge)
SoyRef - still used, but would be better in a more specific 64bit format
