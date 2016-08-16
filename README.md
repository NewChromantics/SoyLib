SoyLib
==========
Soylib is all my common & open source c++ code.

- Much of it is being culled in place of c++11 improvements. All the old thread stuff is gone, the strings etc
- There are a few files which are redundant now and need deleting
- Opengl, directx, metal, opencl and other GPU/device style work run on a rough job system for very simple interfaces.
- My JNI java implementation I discovered is INCREDIBLY similar to another JNI implementation (I forget which, I think it's used in chromium) and could do with removing.
- The array system was very good for it's initial fast, concurrency and explicit management usage (game engines), and fast usage(explicit complex & non complex types) but these days probably a bit redundant for modern work. It was useful in 2009 :)


Noteable work
==========
- The protocol system is VERY useful, flexible, and allows quick interchange of formats (and nested protocols). It also, in conjunction with the SoyBuffers can read & write protocols with streams, so files, pipes etc don't have to be finished in order to read chunks of data.
- Job system for opengl, directx*, metal, opencl works very well for concurrency
- heap implementations, although not the best, allow very good concurrency
- opengl, directx etc implementations are very RAII
- Socket wrapper is designed for explicit client/server use


There is no documentation other than the source. This is mostly for me, but any questions, just message/tweet. 

(@SoylentGraham)[http://twitter.com/soylentgraham/]
