#pragma once


#include "SoyArray.h"
#include "HeapArray.hpp"
#include "SoyThread.h"

//	gr: really this should be away from the base header, but for now...
#include <fstream>

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


class TStreamReader : public SoyWorkerThread
{
public:
	TStreamReader(const std::string& Name,std::shared_ptr<TStreamBuffer> ReadBuffer=nullptr);
	~TStreamReader();
	
	virtual bool									Iteration() override;
	virtual bool									Read(TStreamBuffer& Buffer)=0;	//	read next chunk of data into buffer. return false on EOF
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol()=0;				//	alloc a new protocol instance to process incoming data
	bool											IsFinished() const				{	return HasThread();	}

protected:
	virtual void									Shutdown() __noexcept =0;		//	close file/disconnect etc. Currently called after reader or protocol dictates EOF, not called externally

private:
	void											OnError(const std::string& Error)	{	mOnError( Error );	}

public:
	std::function<void(const std::string&)>						mOnError;						//	caused when protocol throws, reader will disconnect shortly afterwards
	std::function<void(std::shared_ptr<Soy::TReadProtocol>&)>	mOnDataRecieved;
	
private:
	std::shared_ptr<TStreamBuffer>			mReadBuffer;
	std::shared_ptr<Soy::TReadProtocol>		mCurrentProtocol;
};


class TStreamReader_Impl : public TStreamReader
{
public:
	TStreamReader_Impl(std::shared_ptr<TStreamBuffer> ReadBuffer,std::function<bool()> ReadFunc,std::function<void()> ShutdownFunc,std::function<std::shared_ptr<Soy::TReadProtocol>()> AllocProtocolFunc,const std::string& ThreadName);
	~TStreamReader_Impl();
	
	virtual bool									Read(TStreamBuffer& Buffer) override	{	return mReadFunc();	}
	virtual std::shared_ptr<Soy::TReadProtocol>		AllocProtocol() override				{	return mAllocProtocolFunc();	}

protected:
	virtual void									Shutdown() __noexcept override			{	mShutdownFunc();	}
	
public:
	std::function<bool()>									mReadFunc;
	std::function<void()>									mShutdownFunc;
	std::function<std::shared_ptr<Soy::TReadProtocol>()>	mAllocProtocolFunc;
};




class TStreamWriter : public SoyWorkerThread
{
public:
	TStreamWriter(const std::string& Name);
	
	virtual bool							Iteration() override;
	void									Push(std::shared_ptr<Soy::TWriteProtocol> Data);
	void									WaitForQueueToFinish();
	size_t									GetBytesWritten() const		{	return mBytesWritten;	}
	size_t									GetPendingWrites() const	{	return GetQueueSize();	}

protected:
	size_t									GetQueueSize() const				{	return mQueue.GetSize();	}
	virtual void							Write(TStreamBuffer& Buffer,const std::function<bool()>& Block)=0;	//	write next chunk, as much as possible (but keep checking block)
	void									OnError(const std::string& Error)	{	if ( mOnStreamError )	mOnStreamError( Error );	}
	void									OnWriteBytes(size_t Bytes)			{	mBytesWritten += Bytes;	}

public:
	std::function<void(bool)>									mOnShutdown;			//	param is true if success (eg. file finished)
	std::function<void(const std::string&)>						mOnStreamError;			//	fatal write or encode error
	std::function<void(std::shared_ptr<Soy::TWriteProtocol>)>	mOnDataRecieved;
	
protected:
	std::shared_ptr<Soy::TWriteProtocol>	mCurrentProtocol;

private:
	size_t											mBytesWritten;
	std::mutex										mQueueLock;
	Array<std::shared_ptr<Soy::TWriteProtocol>>		mQueue;
};





class TFileStreamWriter : public TStreamWriter
{
public:
	TFileStreamWriter(const std::string& Filename);
	~TFileStreamWriter();
	
protected:
	virtual void		Write(TStreamBuffer& Buffer,const std::function<bool()>& Block) override;
	
private:
	std::ofstream		mFile;
};





class TFileStreamReader : public TStreamReader
{
public:
	TFileStreamReader(const std::string& Filename);
	~TFileStreamReader();
	
protected:
	virtual bool		Read(TStreamBuffer& Buffer) override;
	virtual void		Shutdown() __noexcept override;
	
private:
	std::ifstream		mFile;
	Array<char>			mFileReadBuffer;	//	alloc once
};


template<class SUPER=TFileStreamReader>
class TFileStreamReader_ProtocolLambda : public SUPER
{
public:
	TFileStreamReader_ProtocolLambda(const std::string& Filename,std::function<std::shared_ptr<Soy::TReadProtocol>()> ProtocolAllocFunc) :
		mProtocolAllocFunc	( ProtocolAllocFunc ),
		SUPER				( Filename )
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


