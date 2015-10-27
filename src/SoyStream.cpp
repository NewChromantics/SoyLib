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
	auto MaxSearch = size_cast<ssize_t>(mData.GetSize()-DelimArray.GetSize());
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

bool TStreamBuffer::Push(const std::string& Data)
{
	std::lock_guard<std::recursive_mutex>	Lock( mLock );
	
	auto DataLength = Data.length();
	auto DataArray = GetRemoteArray( Data.c_str(), DataLength );
	mData.PushBackArray( DataArray );
	
	//	gr: needs to be outside of lock?
	bool Dummy;
	mOnDataPushed.OnTriggered(Dummy);
	return true;
}

bool TStreamBuffer::Push(const ArrayBridge<char>& Data)
{
	if ( Data.IsEmpty() )
		return true;
	
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	
	mData.PushBackArray( Data );
	
	//	gr: needs to be outside of lock?
	bool Dummy;
	mOnDataPushed.OnTriggered(Dummy);
	return true;
}


bool TStreamBuffer::Push(const ArrayBridge<uint8>&& Data)
{
	//	convert to char
	auto DataChar = GetRemoteArray( reinterpret_cast<const char*>( Data.GetArray() ), Data.GetDataSize() );
	return Push( GetArrayBridge( DataChar ) );
}


bool TStreamBuffer::UnPop(const ArrayBridge<char>& Data)
{
	std::lock_guard<std::recursive_mutex> Lock( mLock );
	
	auto mDataBridge = GetArrayBridge( mData );
	if ( !mDataBridge.InsertArray( Data, 0 ) )
		return false;
	
	//	gr: needs to be outside of lock?
	bool Dummy;
	mOnDataPushed.OnTriggered(Dummy);
	return true;
}


bool TStreamBuffer::UnPop(const std::string& String)
{
	Array<char> Data;
	auto DataBridge = GetArrayBridge( Data );
	Soy::StringToArray( String, DataBridge );
	
	//	gr: needs to be outside of lock?
	bool Dummy;
	mOnDataPushed.OnTriggered(Dummy);
	return UnPop( DataBridge );
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




TStreamReader::TStreamReader(const std::string& Name) :
	SoyWorkerThread	( Name, SoyWorkerWaitMode::Sleep )
{
	
}


bool TStreamReader::Iteration()
{
	//	read next chunk
	try
	{
		Read();
	}
	catch (std::exception& e)
	{
		std::Debug << "Stream " << GetThreadName() << " failed to read: " << e.what() << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds(5000) );
		return true;
	}
	
	//	nothing to do
	if ( mReadBuffer.IsEmpty() )
		return true;
	
	//	alloc protocol instance to process
	if ( !mCurrentProtocol )
	{
		mCurrentProtocol = AllocProtocol();
		if ( !mCurrentProtocol )
			return true;
	}
	
	//	process
	auto DecodeResult = mCurrentProtocol->Decode( mReadBuffer );
	
	switch ( DecodeResult )
	{
			//	waiting for more data, just let thread re-iterate
		case TProtocolState::Waiting:
			return true;
			
			//	fall through
		case TProtocolState::Disconnect:
		case TProtocolState::Finished:
			break;
			
			//	release current and re-iterate
		default:
			std::Debug << "Unhandled TProtocolState: " << DecodeResult << " ignoring." << std::endl;
		case TProtocolState::Ignore:
			mCurrentProtocol.reset();
			return true;
	}
	
	if ( DecodeResult == TProtocolState::Disconnect )
	{
		std::Debug << "Todo: protocol says, disconnect stream. Currently unhandled." << std::endl;
	}
	
	//	notify (with shared ptr so data can be cheaply saved)
	mOnDataRecieved.OnTriggered( mCurrentProtocol );
	
	//	dealloc for next
	mCurrentProtocol.reset();
	return true;
}






TStreamWriter::TStreamWriter(const std::string& Name) :
	SoyWorkerThread	( Name, SoyWorkerWaitMode::Sleep )
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
		while ( !EncodingFinished )
		{
			try
			{
				Write( Buffer );
			}
			catch (std::exception& e)
			{
				WriteError << e.what();
				break;
			}
		}
	};

	auto Future = std::async( AsyncWrite );

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

void TStreamWriter::Push(std::shared_ptr<Soy::TWriteProtocol>& Data)
{
	std::lock_guard<std::mutex> Lock( mQueueLock );
	mQueue.PushBack( Data );
	Wake();
}




