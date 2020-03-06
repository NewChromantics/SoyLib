#include "SoyStreamBuffer.h"
#include <regex>
#include "RemoteArray.h"


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



