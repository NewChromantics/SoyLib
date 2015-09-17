#pragma once


#include "SoyEvent.h"
#include "SoyArray.h"
#include "HeapArray.hpp"
#include "SoyThread.h"


namespace Soy
{
	class TWriteProtocol;
	class TReadProtocol;
};


//	try and integrate this more closely with std streams
//	but for now, it's a buffer of data we've (probbaly) read in, and want to probe it in various ways
class TStreamBuffer
{
public:
	//	gr: turn these into some kinda "binary reg exp"
	bool		PopAnyMatch(const ArrayInterface<char>&& DelimAny,ArrayBridge<char>& Data,bool KeepDelim);
	bool		Pop(const std::string Delim,std::string& Data,bool KeepDelim);
	bool		Pop(const ArrayBridge<char>&& Delim,ArrayBridge<char>&& Data,bool KeepDelim);
	
	bool		Pop(size_t Length);
	bool		Pop(size_t Length,ArrayBridge<char>& Data);
	inline bool	Pop(size_t Length,ArrayBridge<char>&& Data)	{	return Pop(Length, Data);	}
	bool		Pop(std::string RegexPattern,ArrayBridge<std::string>& Parts);	//	gr: using string pattern because you can't get the pattern from std::regex
	
	bool		Push(const std::string& Data);
	bool		Push(const ArrayBridge<char>& Data);
	bool		UnPop(const ArrayBridge<char>& Data);
	inline bool	UnPop(const ArrayBridge<char>&& Data)	{	return UnPop( Data );	}
	bool		UnPop(const std::string& Data);
	
	bool		IsEmpty() const				{	return GetBufferedSize() == 0;	}
	size_t		GetBufferedSize() const		{	return mData.GetDataSize();	}
	
	bool		PeekBack(ArrayBridge<char>&& Data);	//	copy last X bytes without modifying. fails if this many bytes don't exist
	
public:
	SoyEvent<bool>	mOnDataPushed;
	
private:
	//	gr: make this a ring buffer for speed!
	std::recursive_mutex	mLock;
	Array<char>				mData;
};



class TStreamReader : public SoyWorkerThread
{
public:
	TStreamReader(const std::string& Name);
	
	virtual bool							Iteration() override;
	virtual void							Read()=0;			//	read next chunk of data into buffer
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol()=0;	//	alloc a new protocol instance to process incoming data
	
public:
	SoyEvent<std::shared_ptr<Soy::TReadProtocol>>	mOnDataRecieved;
	
protected:
	TStreamBuffer							mReadBuffer;
	std::shared_ptr<Soy::TReadProtocol>		mCurrentProtocol;
};




class TStreamWriter : public SoyWorkerThread
{
public:
	TStreamWriter(const std::string& Name);
	
	virtual bool							Iteration() override;
	void									Push(std::shared_ptr<Soy::TWriteProtocol>& Data);

protected:
	virtual void							Write(TStreamBuffer& Buffer)=0;	//	write next chunk

public:
	SoyEvent<std::shared_ptr<Soy::TWriteProtocol>>	mOnDataRecieved;
	
protected:
	std::shared_ptr<Soy::TWriteProtocol>	mCurrentProtocol;

private:
	std::mutex										mQueueLock;
	Array<std::shared_ptr<Soy::TWriteProtocol>>		mQueue;
};



