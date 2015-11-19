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
	inline bool	Pop(size_t Length,ArrayBridge<char>&& Data)		{	return Pop(Length, Data);	}
	bool		Pop(std::string RegexPattern,ArrayBridge<std::string>& Parts);	//	gr: using string pattern because you can't get the pattern from std::regex
	
	bool		Push(const std::string& Data);
	bool		Push(const ArrayBridge<char>& Data);
	bool		Push(const ArrayBridge<uint8>& Data);
	inline bool	Push(const ArrayBridge<uint8>&& Data)	{	return Push( Data );	}
	bool		UnPop(const ArrayBridge<char>& Data);
	inline bool	UnPop(const ArrayBridge<char>&& Data)	{	return UnPop( Data );	}
	bool		UnPop(const ArrayBridge<uint8>& Data);
	inline bool	UnPop(const ArrayBridge<uint8>&& Data)	{	return UnPop( Data );	}
	bool		UnPop(const std::string& Data);
	
	bool		IsEmpty() const				{	return GetBufferedSize() == 0;	}
	size_t		GetBufferedSize() const		{	return mData.GetDataSize();	}
	
	bool		Peek(ArrayBridge<char>&& Data);		//	copy first X bytes without modifying. fails if this many bytes don't exist
	bool		PeekBack(ArrayBridge<char>&& Data);	//	copy last X bytes without modifying. fails if this many bytes don't exist
	
public:
	SoyEvent<bool>	mOnDataPushed;
	
private:
	//	gr: make this a ring buffer for speed!
	std::recursive_mutex	mLock;
	Array<char>				mData;		//	change this to uint8!
};


//	todo: handle EOF
class TStreamReader : public SoyWorkerThread
{
public:
	TStreamReader(const std::string& Name,std::shared_ptr<TStreamBuffer> ReadBuffer=nullptr);
	~TStreamReader();
	
	virtual bool									Iteration() override;
	virtual void									Read(TStreamBuffer& Buffer)=0;	//	read next chunk of data into buffer
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol()=0;				//	alloc a new protocol instance to process incoming data
	
public:
	SoyEvent<std::shared_ptr<Soy::TReadProtocol>>	mOnDataRecieved;
	
private:
	std::shared_ptr<TStreamBuffer>			mReadBuffer;
	std::shared_ptr<Soy::TReadProtocol>		mCurrentProtocol;
};


class TStreamReader_Impl : public TStreamReader
{
public:
	TStreamReader_Impl(std::shared_ptr<TStreamBuffer> ReadBuffer,std::function<void(void)> ReadFunc,std::function<std::shared_ptr<Soy::TReadProtocol>()> AllocProtocolFunc,const std::string& ThreadName) :
		TStreamReader		( ThreadName, ReadBuffer ),
		mReadFunc			( ReadFunc ),
		mAllocProtocolFunc	( AllocProtocolFunc )
	{
	}
	
	virtual void									Read(TStreamBuffer& Buffer) override	{	return mReadFunc();	}
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol() override				{	return mAllocProtocolFunc();	}
	
public:
	std::function<void()>									mReadFunc;
	std::function<std::shared_ptr<Soy::TReadProtocol>()>	mAllocProtocolFunc;
};




class TStreamWriter : public SoyWorkerThread
{
public:
	TStreamWriter(const std::string& Name);
	
	virtual bool							Iteration() override;
	void									Push(std::shared_ptr<Soy::TWriteProtocol> Data);

protected:
	virtual void							Write(TStreamBuffer& Buffer)=0;	//	write next chunk
	void									OnError(const std::string& Error)	{	mOnStreamError.OnTriggered( Error );	}

public:
	SoyEvent<const std::string>				mOnStreamError;			//	fatal write or encode error
	SoyEvent<std::shared_ptr<Soy::TWriteProtocol>>	mOnDataRecieved;
	
protected:
	std::shared_ptr<Soy::TWriteProtocol>	mCurrentProtocol;

private:
	std::mutex										mQueueLock;
	Array<std::shared_ptr<Soy::TWriteProtocol>>		mQueue;
};




//	gr: really this should be away from the base header, but for now...
#include <fstream>

class TFileStreamWriter : public TStreamWriter
{
public:
	TFileStreamWriter(const std::string& Filename);
	~TFileStreamWriter();
	
protected:
	virtual void		Write(TStreamBuffer& Buffer) override;
	
private:
	std::ofstream		mFile;
};





class TFileStreamReader : public TStreamReader
{
public:
	TFileStreamReader(const std::string& Filename);
	~TFileStreamReader();
	
protected:
	virtual void		Read(TStreamBuffer& Buffer) override;
	
private:
	std::ifstream		mFile;
};


class TFileStreamReader_ProtocolLambda : public TFileStreamReader
{
public:
	TFileStreamReader_ProtocolLambda(const std::string& Filename,std::function<std::shared_ptr<Soy::TReadProtocol>()> ProtocolAllocFunc) :
		mProtocolAllocFunc	( ProtocolAllocFunc ),
		TFileStreamReader	( Filename )
	{
	}
	
protected:
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol() override
	{
		return mProtocolAllocFunc();
	}
	
public:
	std::function<std::shared_ptr<Soy::TReadProtocol>()>	mProtocolAllocFunc;
};


