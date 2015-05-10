ofxSoylent
==========

My "little things" lib. Formerly to go with openframeworks, but not any more. Designed to be cross platform for ios, mac and windows, using as much stl c++11 code as possible for simplification.

Trying to clean up this code to remove dependencies so often only a few files need to be included rather than the whole lib.

The Soy::Assert's should be particularly useful as they provide dynamic assert messages but with zero overhead when calling Soy::Assert (the callbacks, or lambda's are only instintiated or called IF the condition is not met)
