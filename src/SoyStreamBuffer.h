#pragma once

#include "SoyArray.h"
#include "HeapArray.hpp"


//	try and integrate this more closely with std streams
//	but for now, it's a buffer of data we've (probbaly) read in, and want to probe it in various ways
class TStreamBuffer
{
public:
	TStreamBuffer() :
		mEof	( false )
	{
	}
	
	//	gr: turn these into some kinda "binary reg exp"
	bool		PopAnyMatch(const ArrayInterface<char>&& DelimAny,ArrayBridge<char>& Data,bool KeepDelim);
	bool		Pop(const std::string Delim,std::string& Data,bool KeepDelim);
	bool		Pop(const ArrayBridge<char>&& Delim,ArrayBridge<char>&& Data,bool KeepDelim);
	
	bool		Pop(size_t Length);
	bool		Pop(size_t Length,ArrayBridge<char>& Data);
	inline bool	Pop(size_t Length,ArrayBridge<char>&& Data)		{	return Pop(Length, Data);	}
	bool		Pop(size_t Length,ArrayBridge<uint8>& Data);
	inline bool	Pop(size_t Length,ArrayBridge<uint8>&& Data)	{	return Pop(Length, Data);	}
	bool		Pop(std::string RegexPattern,ArrayBridge<std::string>& Parts);	//	gr: using string pattern because you can't get the pattern from std::regex
	
	bool		Push(const std::string& Data);
	bool		Push(const ArrayBridge<char>& Data);
	bool		Push(const ArrayBridge<uint8>& Data);
	inline bool	Push(const ArrayBridge<uint8>&& Data)	{	return Push( Data );	}
	void		PushEof();								//	named "push" in case this becomes queue related in future to suggest the func is not atomic
	
	bool		UnPop(const ArrayBridge<char>& Data);
	inline bool	UnPop(const ArrayBridge<char>&& Data)	{	return UnPop( Data );	}
	bool		UnPop(const ArrayBridge<uint8>& Data);
	inline bool	UnPop(const ArrayBridge<uint8>&& Data)	{	return UnPop( Data );	}
	bool		UnPop(const std::string& Data);
	
	bool		IsEmpty() const				{	return GetBufferedSize() == 0;	}
	size_t		GetBufferedSize() const		{	return mData.GetDataSize();	}
	bool		HasEndOfStream() const		{	return mEof;	}	//	this keeps coming up
	
	bool		Peek(uint8_t& Data,size_t Offset=0);
	bool		Peek(ArrayBridge<char>& Data);			//	copy first X bytes without modifying. fails if this many bytes don't exist
	bool		Peek(ArrayBridge<char>&& Data)			{	return Peek( Data );	}
	bool		Peek(ArrayBridge<uint8>&& Data);		//	copy first X bytes without modifying. fails if this many bytes don't exist
	bool		PeekBack(ArrayBridge<char>&& Data);	//	copy last X bytes without modifying. fails if this many bytes don't exist
	ArrayBridgeDef<Array<char>>	PeekArray();		//	get an UNSAFE array bridge to the data
	
protected:
	void		OnDataPushed(bool EofPushed);
	
public:
	std::function<void(bool)>	mOnDataPushed;		//	bool now represents EofPushed
	bool			mEof;
	
private:
	//	gr: make this a ring buffer for speed!
	std::recursive_mutex	mLock;
	Array<char>				mData;		//	change this to uint8!
};

