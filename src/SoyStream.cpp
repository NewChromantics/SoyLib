#include "SoyStream.h"
#include <regex>
#include "RemoteArray.h"
#include "SoyProtocol.h"
#include <future>


bool TStreamBuffer::Pop(std::string Pattern,ArrayBridge<std::string>& Parts)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//assert( Pattern.empty() );
	if ( Pattern.empty() )
		return false;
	
	//	gr: because we NEED to match the start of the buffer, the pattern MUST beging with ^(start of string)
	if ( Pattern[0] != '^' )
	{
		std::Debug << __FUNCTION__ << " regular expression pattern MUST start with ^" << std::endl;
		//assert(Pattern[0] == '^');
		return false;
	}
	
	//	pull out data to test against
	size_t Lookahead = std::min<size_t>( 100, mData.GetSize() );
	std::stringstream Data;
	for ( size_t i=0;	i<Lookahead;	i++ )
	{
		Data << mData[i];
	}
	std::string DataString = Data.str();
	std::smatch Match;
	
	//	execute regex
	//	gr: gotta use search as we might be testing against MORE than just what we want to match
	if ( !std::regex_search( DataString, Match, std::regex(Pattern) ) )
		return false;
	
	//	matched. pop off the data
	auto Length = Match[0].str().length();
	mData.RemoveBlock( 0, Length );
	
	//	get the parts into the result
	for ( int i=1;	i<Match.size();	i++ )
	{
		Parts.PushBack( Match[i].str() );
	}
	
	return true;
}

bool TStreamBuffer::PopAnyMatch(const ArrayInterface<char>&& DelimAny,ArrayBridge<char>& Data,bool KeepDelim)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );

	//	delim is empty??
	if ( !Soy::Assert( !DelimAny.IsEmpty(), "no deliminators sepecified" ) )
		return false;
	
	//	search for match
	for ( int a=0;	a<mData.GetSize();	a++ )
	{
		bool Match = false;
		for ( int d=0;	d<DelimAny.GetSize();	d++ )
			Match |= (DelimAny[d] == mData[a]);
		if ( !Match )
			continue;
		
		//	found match!
		int PopLength = a + 1;
		int OutputLength = KeepDelim ? PopLength : a;
		assert( OutputLength <= PopLength );
		if ( !Pop( PopLength, Data ) )
		{
			//	unexpected!
			Soy::Assert(false, "Unexpectedly failed to pop data we thought we had");
			return false;
		}
		return true;
	}
	
	//	no match
	return false;
}


bool TStreamBuffer::Pop(const ArrayBridge<char>&& Delim,ArrayBridge<char>&& Data,bool KeepDelim)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//	delim is empty??
	if ( !Soy::Assert( !Delim.IsEmpty(), "no deliminator sepecified" ) )
		return false;
	
	//	search for match
	auto MaxSearch = size_cast<ssize_t>(mData.GetSize()-Delim.GetSize());
	for ( int a=0;	a<MaxSearch;	a++ )
	{
		bool Match = memcmp( Delim.GetArray(), &mData[a], Delim.GetDataSize() )==0;
		if ( !Match )
			continue;
		
		//	found match!
		auto PopLength = a + (KeepDelim ? Delim.GetDataSize() : 0);
		//auto OldSize = mData.GetSize();
		if ( !Pop( PopLength, Data ) )
		{
			//	unexpected!
			Soy::Assert(false, "Unexpectedly failed to pop data we thought we had");
			return false;
		}
		
		//	remove the delim from next search
		if ( !KeepDelim )
			Pop( Delim.GetDataSize() );
		
		return true;
	}
	
	//	no match
	return false;
}


bool TStreamBuffer::Pop(const std::string Delim,std::string& Data,bool KeepDelim)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//	search array
	auto DelimLength = Delim.length();
	auto DelimArray = GetRemoteArray( Delim.c_str(), DelimLength, DelimLength );
	
	//	delim is empty??
	assert( !DelimArray.IsEmpty() );
	if ( DelimArray.IsEmpty() )
		return false;
	
	//	search for match
	auto MaxSearch = size_cast<ssize_t>(mData.GetSize()) - size_cast<ssize_t>(DelimArray.GetSize());
	for ( int a=0;	a<=MaxSearch;	a++ )
	{
		bool Match = memcmp( &mData[a], DelimArray.GetArray(), DelimArray.GetDataSize() )==0;
		if ( !Match )
			continue;
		
		//	found match!
		auto PopLength = a + DelimArray.GetSize();
		auto OutputLength = KeepDelim ? PopLength : a;
		assert( OutputLength <= PopLength );
		Array<char> Output;
		auto OutputBridge = GetArrayBridge( Output );
		if ( !Pop( PopLength, OutputBridge ) )
		{
			//	unexpected!
			assert(false);
			return false;
		}
		//	copy into string
		std::stringstream OutputString;
		for ( int i=0;	i<OutputLength;	i++ )
			OutputString << Output[i];
		Data = OutputString.str();
		return true;
	}
	
	//	no match
	return false;
}


bool TStreamBuffer::Pop(size_t Length)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//	enough data ready?
	if ( mData.GetDataSize() < Length )
		return false;
	
	//	remove it
	mData.RemoveBlock( 0, Length );
	return true;
}

bool TStreamBuffer::Pop(size_t Length,ArrayBridge<char>& Data)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//	enough data ready?
	if ( mData.GetDataSize() < Length )
		return false;
	
	//	push back a sub section
	auto DataSection = GetRemoteArray( mData.GetArray(), Length );
	if ( !Data.PushBackArray( DataSection ) )
		return false;
	
	//	remove it now it's copied
	mData.RemoveBlock( 0, Length );
	return true;
}


bool TStreamBuffer::Pop(size_t Length,ArrayBridge<uint8>& Data)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	//	enough data ready?
	if ( mData.GetDataSize() < Length )
		return false;
	
	//	push back a sub section
	auto DataSection = GetRemoteArray( reinterpret_cast<const uint8*>( mData.GetArray() ), Length );
	if ( !Data.PushBackArray( DataSection ) )
		return false;
	
	//	remove it now it's copied
	mData.RemoveBlock( 0, Length );
	return true;
}

bool TStreamBuffer::Push(const std::string& Data)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	auto DataLength = Data.length();
	auto DataArray = GetRemoteArray( Data.c_str(), DataLength );
	mData.PushBackArray( DataArray );
	
	//	gr: needs to be outside of lock?
	OnDataPushed(false);
	return true;
}

bool TStreamBuffer::Push(const ArrayBridge<char>& Data)
{
	if ( Data.IsEmpty() )
		return true;
	
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	
	mData.PushBackArray( Data );
	
	//	gr: needs to be outside of lock?
	OnDataPushed(false);
	return true;
}


bool TStreamBuffer::Push(const ArrayBridge<uint8>& Data)
{
	//	convert to char
	auto DataChar = GetRemoteArray( reinterpret_cast<const char*>( Data.GetArray() ), Data.GetDataSize() );
	return Push( GetArrayBridge( DataChar ) );
}

void TStreamBuffer::PushEof()
{
	mEof = true;
	OnDataPushed(true);
}

void TStreamBuffer::OnDataPushed(bool EofPushed)
{
	if ( mOnDataPushed )
		mOnDataPushed( EofPushed );
}


bool TStreamBuffer::UnPop(const ArrayBridge<uint8>& Data)
{
	//	convert to char
	auto DataChar = GetRemoteArray( reinterpret_cast<const char*>( Data.GetArray() ), Data.GetDataSize() );
	return UnPop( GetArrayBridge( DataChar ) );
}


bool TStreamBuffer::UnPop(const ArrayBridge<char>& Data)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	
	auto mDataBridge = GetArrayBridge( mData );
	if ( !mDataBridge.InsertArray( Data, 0 ) )
		return false;
	
	//	gr: needs to be outside of lock?
	OnDataPushed(false);
	return true;
}


bool TStreamBuffer::UnPop(const std::string& String)
{
	Array<char> Data;
	auto DataBridge = GetArrayBridge( Data );
	Soy::StringToArray( String, DataBridge );
	
	//	gr: needs to be outside of lock?
	OnDataPushed(false);
	return UnPop( DataBridge );
}

ArrayBridgeDef<Array<char>> TStreamBuffer::PeekArray()
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	return GetArrayBridge(mData);	
}

bool TStreamBuffer::Peek(ArrayBridge<char>& Data)
{
	Soy::Assert( !Data.IsEmpty(), "Shouldn't peek for 0 bytes" );
	
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	if ( mData.GetSize() < Data.GetSize() )
		return false;
	
	auto DataHead = GetRemoteArray( mData.GetArray(), Data.GetSize() );
	Data.Copy( DataHead );
	
	return true;
}


bool TStreamBuffer::Peek(ArrayBridge<uint8>&& Data)
{
	Soy::Assert( !Data.IsEmpty(), "Shouldn't peek for 0 bytes" );
	
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	if ( mData.GetSize() < Data.GetSize() )
		return false;
	
	auto DataHead = GetRemoteArray( reinterpret_cast<const uint8*>(mData.GetArray()), Data.GetSize() );
	Data.Copy( DataHead );
	
	return true;
}




bool TStreamBuffer::PeekBack(ArrayBridge<char> &&Data)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	if ( mData.GetSize() < Data.GetSize() )
		return false;
	
	auto DataTail = GetRemoteArray( &mData[mData.GetSize()-Data.GetSize()], Data.GetSize() );
	Data.Copy( DataTail );
	
	return true;
}




TStreamReader::TStreamReader(const std::string& Name,std::shared_ptr<TStreamBuffer> ReadBuffer) :
	SoyWorkerThread	( Name, SoyWorkerWaitMode::Sleep )
{
	//	initialise the callbakcs to debug funcs
	mOnError = [](const std::string& Error)
	{
		std::Debug << "Protocol error: " << Error << std::endl;
	};
	mOnDataRecieved = [](std::shared_ptr<Soy::TReadProtocol>& ReadData)
	{
		std::Debug << "Protocol decoded data (unhandled)" << std::endl;
	};
	
	//	if we're using an external buffer, wake when it's modified
	if ( ReadBuffer )
	{
		mReadBuffer = ReadBuffer;
		
		//	deadlock :(
		//WakeOnEvent( mReadBuffer->mOnDataPushed );
	}
	else
	{
		//	allocate a private buffer
		mReadBuffer.reset( new TStreamBuffer );
	}
}

TStreamReader::~TStreamReader()
{
	//	gr: this may be too late. Shutdown() is pure and if the thread calls this at any point we'll go wrong.
	//	this Wait will need to be in derived classes.
	WaitToFinish();
}

bool TStreamReader::Iteration()
{
	//	read next chunk
	bool KeepAlive = true;
	try
	{
		KeepAlive = Read( *mReadBuffer );
	}
	catch (std::exception& e)
	{
		std::Debug << "Stream " << GetThreadName() << " failed to read: " << e.what();

		//	gr: error thrown... abort the thread if no data?
		//		retry? throttle?
		if ( mReadBuffer->IsEmpty() )
		{
			std::Debug << "; Aborting read" << std::endl;
			return false;
		}
		else
		{
			auto SleepMs = 1000;
			std::Debug << "; retrying (throttled for " << SleepMs << "ms)" << std::endl;
			std::this_thread::sleep_for( std::chrono::milliseconds( SleepMs ) );
			return true;
		}
	}
	
	if ( !KeepAlive )
	{
		mReadBuffer->PushEof();
	}
	
	//	no more data, end of file. Assume protocol has already caught this?
	//	do we need to handle;
	//	<last data handled by protocol>
	//	<no data>
	//	<protocol expecting eof>
	if ( mReadBuffer->IsEmpty() )
	{
		if ( !KeepAlive )
			Shutdown();
		return KeepAlive;
	}
	
	//	gr: with websocket, we may recieve a string of messages from the socket.
	///		but because the recv is blocking, we might process say 100/1000 bytes for a packet
	//		but won't process next message (protocol) until we recv again
	//		so flush through the read buffer until a protocol stops (needing data, disconnects etc)
	while ( !mReadBuffer->IsEmpty() )
	{
		auto ProcessNextProtocol = [&]
		{
			//	alloc protocol instance to process
			if ( !mCurrentProtocol )
				mCurrentProtocol = AllocProtocol();

			//	in case it's been released capture a local copy
			auto CurrentProtocol = mCurrentProtocol;
			if ( !CurrentProtocol )
			{
				std::stringstream Error;
				Error << "TStreamReader(" << Soy::GetTypeName(*this) << ") didn't allocate protocol." << std::endl;
				throw Soy::AssertException(Error.str());
			}
			
			//	process protocol
			//	gr: abort has been replaced with exceptions
			auto DecodeResult = TProtocolState::Invalid;
			try
			{
				DecodeResult = CurrentProtocol->Decode( *mReadBuffer );
			}
			catch(std::exception& e)
			{
				std::Debug << "Protocol " << Soy::GetTypeName(*CurrentProtocol) << "::Decode threw exception (" << e.what() << ") reverting to " << DecodeResult << std::endl;
				CurrentProtocol.reset();
				DecodeResult = TProtocolState::Disconnect;
				OnError( e.what() );
			}
			catch(...)
			{
				std::Debug << "Protocol " << Soy::GetTypeName(*CurrentProtocol)  << "::Decode threw unknown exception reverting to " << DecodeResult << std::endl;
				CurrentProtocol.reset();
				DecodeResult = TProtocolState::Disconnect;
				OnError("Unknown exception");
			}
			
			switch ( DecodeResult )
			{
					//	waiting for more data, just let thread re-iterate
				case TProtocolState::Waiting:
					if ( KeepAlive )
						return TProtocolState::Waiting;

					//	waiting for data, but eof... forcing disconnect (fall through)
					std::Debug << "TStreamReader protocol " << Soy::GetTypeName(*CurrentProtocol) << " waiting for data, but EOF, so disconnecting" << std::endl;
					DecodeResult = TProtocolState::Disconnect;
					break;
					
					//	fall through
				case TProtocolState::Disconnect:
				case TProtocolState::Finished:
					break;
					
					//	release current and re-iterate
				default:
					std::Debug << "Unhandled TProtocolState: " << DecodeResult << " ignoring." << std::endl;
				case TProtocolState::Ignore:
					mCurrentProtocol.reset();
					return TProtocolState::Ignore;
			}
			
			//	notify (with shared ptr so data can be cheaply saved)
			if ( CurrentProtocol )
				mOnDataRecieved( CurrentProtocol );
		
			//	dealloc for next
			mCurrentProtocol.reset();
			return DecodeResult;
		};
		
		//	thread stopped
		if ( !IsWorking() )
			return true;
		
		//	try and process next packet/protocol
		auto DecodeResult = ProcessNextProtocol();
		if ( DecodeResult == TProtocolState::Disconnect )
		{
			//	protocol has told us to clean up, so disconnect/cleanup the reader in case EOF it was triggered by protocol, not reader
			Shutdown();
				
			//	end thread naturally... assume whatever owns the reader will detect when it finishes?
			//	maybe we'll want an event?
			//	or a generic worker event OnWorkerFinished
			return false;
		}
		
		//	waiting for more data
		if ( DecodeResult == TProtocolState::Waiting )
			return true;
	
		//	loop and try and process another packet
		continue;
	}
	
	//	out of data, try and read more
	return true;
}





TStreamWriter::TStreamWriter(const std::string& Name) :
	SoyWorkerThread	( Name, SoyWorkerWaitMode::Sleep ),
	mBytesWritten	( 0 )
{
	
}


bool TStreamWriter::Iteration()
{
	if ( mQueue.IsEmpty() )
		return true;
	
	//	pop next
	mQueueLock.lock();
	auto Data = mQueue[0];
	mQueue.RemoveBlock(0,1);
	mQueueLock.unlock();
	
	//	turn it into data
	//	todo: have two threads, one writing the Buffer, and one that pops the queue and just keeps pushing data
	//		onto the buffer. That way we can encode & write at the same time and have infinite write protocols!
	TStreamBuffer Buffer;
	bool EncodingFinished = false;
	std::stringstream WriteError;

	auto AsyncWrite = [&Buffer,&EncodingFinished,&WriteError,this]
	{
		auto Block = [this] ()->bool
		{
			static bool BreakIfThreadStopped = false;
			if ( !IsWorking() && BreakIfThreadStopped )
				return false;

			return true;
		};
		
		while ( !EncodingFinished || Buffer.GetBufferedSize()>0 )
		{
			try
			{
				Write( Buffer, Block );
			}
			catch (std::exception& e)
			{
				WriteError << e.what();
				break;
			}
			
			//	sleep this thread so it doesn't hammer CPU.
			//	if we've reached here, we can assume the Write()r has done as much as it can
			//	todo: replace with some wake-on-event simple blocking func like
			//		Soy::SleepUntilEvent( Event )
			//	which has a temporary conditional and takes the Block() test func
			//	and maybe an extra event so outer thread can wake it up
			if ( !EncodingFinished && Block() && Buffer.GetBufferedSize() == 0 )
			{
				static auto SleepMs = 10;
				std::this_thread::sleep_for( std::chrono::milliseconds(SleepMs) );
			}
		}
		//Soy::Assert( Buffer.GetBufferedSize() == 0, "Still some data to write");
	};

	auto Future = std::async(/*std::launch::async,*/ AsyncWrite );

	try
	{
		Data->Encode( Buffer );
	}
	catch (std::exception& e)
	{
		//	end the async thread
		EncodingFinished = true;
		Future.wait();

		std::stringstream Error;
		Error << "Failed to write buffer from protocol: " << e.what();
		OnError( Error.str() );
		return true;
	}
	
	//	now wait for writing to finish
	EncodingFinished = true;
	Future.wait();

	if ( !WriteError.str().empty() )
	{
		OnError( WriteError.str() );
	}
	
	return true;
}

void TStreamWriter::Push(std::shared_ptr<Soy::TWriteProtocol> Data)
{
	std::lock_guard<std::mutex> Lock( mQueueLock );
	mQueue.PushBack( Data );
	
	Wake();

	//	auto start
	Start(false);
}

void TStreamWriter::WaitForQueueToFinish()
{
	Wake();
	
	static int SleepMs = 100;
	static bool BreakIfNotWorking = false;
	static int WarningWaitTime = 3000;
	
	int WaitTime = 0;
	
	//	wait for queue to empty
	while ( !mQueue.IsEmpty() )
	{
		//	if the thread has been killed... this loop may never finish...
		if ( !IsWorking() )
			if ( BreakIfNotWorking )
				break;

		std::this_thread::sleep_for( std::chrono::milliseconds(SleepMs) );
		WaitTime += SleepMs;
		
		if ( WaitTime > WarningWaitTime )
		{
			std::Debug << __func__ << "/" << this->GetThreadName() << " has been waiting for " << (WaitTime/1000) << " secs" << std::endl;
		}
	}
}



TFileStreamWriter::TFileStreamWriter(const std::string& Filename) :
	TStreamWriter	( std::string("TFileStreamWriter " ) + Filename ),
	mFile			( Filename, std::ios::out|std::ios::binary )
{
	Soy::Assert( mFile.is_open(), std::string("Failed to open ")+Filename );

}

TFileStreamWriter::~TFileStreamWriter()
{
	WaitToFinish();
	
	auto QueueSize = GetQueueSize();
	if ( QueueSize > 0 )
	{
		std::Debug << "Warning, TFileStreamWriter closed with " << QueueSize << " items unwritten" << std::endl;
	}
	
	mFile.close();
	
	bool Success = true;
	if ( mOnShutdown )
		mOnShutdown(Success);
}

void TFileStreamWriter::Write(TStreamBuffer& Data,const std::function<bool()>& Block)
{
	//	write to file
	//	gr: don't break if thread has finished, let that be controlled externally
	while ( Data.GetBufferedSize() > 0 && Block() )
	{
		try
		{
			Array<char> Buffer;
			if ( !Data.Pop( Data.GetBufferedSize(), GetArrayBridge(Buffer) ) )
				break;
			
			if ( Buffer.IsEmpty() )
				break;
			
			//	write frame
			mFile.write( Buffer.GetArray(), Buffer.GetDataSize() );
			
			//Soy::Assert( File.fail(), "Error writing to file" );
			Soy::Assert( !mFile.bad(), "Error writing to file" );

			OnWriteBytes( Buffer.GetDataSize() );
		}
		catch (std::exception& e)
		{
			std::Debug << "TFileCaster write-file error: " << e.what() << std::endl;
			break;
		}
	}
}



TFileStreamReader::TFileStreamReader(const std::string& Filename) :
	TStreamReader	( std::string("TFileStreamReader " ) + Filename ),
	mFile			( Filename, std::ios::in )
{
	Soy::Assert( mFile.is_open(), std::string("Failed to open ")+Filename );
}

TFileStreamReader::~TFileStreamReader()
{
	//	let the reader finish dealing with all the data before closing the file
	WaitToFinish();
	
	Shutdown();
}

bool TFileStreamReader::Read(TStreamBuffer& Buffer)
{
	mFileReadBuffer.SetSize( 1024*1024 );
	auto& Data = mFileReadBuffer;

	auto Peek = mFile.peek();
	if ( Peek == std::char_traits<char>::eof() )
	{
		return false;
	}

	mFile.read( Data.GetArray(), Data.GetDataSize() );
	if ( mFile.fail() && !mFile.eof() )
	{
		std::stringstream Error;
		Error << "TFileStreamReader error; " << Platform::GetLastErrorString();
		throw Soy::AssertException( Error.str() );
	}

	auto BytesRead = mFile.gcount();
	Data.SetSize( BytesRead );
	Buffer.Push( GetArrayBridge( Data ) );
	return true;
}

void TFileStreamReader::Shutdown() __noexcept 
{
	//	need to lock? does read & shutdown never happen simulatenously?
	std::Debug << __func__ << " start" << std::endl;
	mFile.close();
	std::Debug << __func__ << " end" <<std::endl;
}




TStreamReader_Impl::TStreamReader_Impl(std::shared_ptr<TStreamBuffer> ReadBuffer,std::function<bool()> ReadFunc,std::function<void()> ShutdownFunc,std::function<std::shared_ptr<Soy::TReadProtocol>()> AllocProtocolFunc,const std::string& ThreadName) :
	TStreamReader		( ThreadName, ReadBuffer ),
	mReadFunc			( ReadFunc ),
	mShutdownFunc		( ShutdownFunc ),
	mAllocProtocolFunc	( AllocProtocolFunc )
{
	if ( !mShutdownFunc )
		mShutdownFunc = []{};
	Soy::Assert( mReadFunc !=nullptr, "Read function required");
}

TStreamReader_Impl::~TStreamReader_Impl()
{
	//	the base class does this, but if we finish this destructor and go into the base class, the vtable for in-thread pure virtuals are cleared
	//	so we need to wait for thread to finish BEFORE we destruct vtable.
	//	I guess that means the parent needs to WaitToFinish first, but I think we can avoid that for now with this (prefer less client code)
	WaitToFinish();
}

