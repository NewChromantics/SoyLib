#include "SoySocketStream.h"



TSocketReadThread::TSocketReadThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef) :
	TStreamReader	( "TSocketReadThread" ),
	mSocket			( Socket ),
	mConnectionRef	( ConnectionRef )
{
	
}



bool TSocketReadThread::Read(TStreamBuffer& Buffer)
{
	auto pSocket = mSocket;
	if ( !pSocket )
	{
		std::Debug << __func__ << " eof; no socket" << std::endl;
		return false;
	}
	auto& Socket = *pSocket;
	
	SoySocketConnection	ClientSocket = Socket.GetConnection( mConnectionRef );
	if ( !ClientSocket.IsValid() )
	{
		std::Debug << "TSocketReadThread::Read Connection lost" << std::endl;
		return false;
	}
	
	//	gr use a larger static buffer so we can stream locally much faster than in 1024 chunks
	static int BufferSize = 1024*1024*4;	//	Xmb a time should be plenty... maybe query for the actual socket limit jsut so we don't go silly with memory
	auto& RecvBuffer = mRecvBuffer;
	RecvBuffer.SetSize( BufferSize );
	
	
	//	throws on error
	if ( !ClientSocket.Recieve( GetArrayBridge(RecvBuffer) ) )
	{
		//	graceful close
		std::Debug << "TSocketReadThread::Read Gracefull close" << std::endl;
		return false;
	}
	
	Buffer.Push( GetArrayBridge( RecvBuffer ) );
	return true;
}


void TSocketReadThread::Shutdown() __noexcept
{
	//	kill socket
	//	gr: dealloc?
	if ( mSocket )
	{
		mSocket->Disconnect( mConnectionRef, "TSocketReadThread::Shutdown" );
		mSocket.reset();
	}
}


TSocketWriteThread::TSocketWriteThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef) :
	TStreamWriter	( "TSocketWriteThread" ),
	mSocket			( Socket ),
	mConnectionRef	( ConnectionRef )
{
}

TSocketWriteThread::~TSocketWriteThread()
{
	//	try to abort the thread before we wait for it
	mSocket.reset();
	WaitToFinish();
}


void TSocketWriteThread::Write(TStreamBuffer& Buffer,const std::function<bool()>& Block)
{
	//	keep writing until buffer is empty
	while ( !Buffer.IsEmpty() && Block() )
	{
		SoySocketConnection	ClientSocket = mSocket->GetConnection( mConnectionRef );
		if ( !ClientSocket.IsValid() )
		{
			//	lost connection
			throw Soy::AssertException("Connection lost");
		}
		
		static size_t PopMaxSize = 1024 * 1024 * 1;
		size_t PopSize = std::min( PopMaxSize, Buffer.GetBufferedSize() );
		BufferArray<char,1024 * 1024 * 1> SendBuffer;
		if ( !Buffer.Pop( PopSize, GetArrayBridge(SendBuffer) ) )
		{
			std::Debug << "Buffer data has gone missing. Trying again" << std::endl;
			continue;
		}
		
		//	write to socket
		try
		{
			ClientSocket.Send( GetArrayBridge(SendBuffer), mSocket->IsUdp() );
		}
		catch (std::exception& e)
		{
			mSocket->OnError( mConnectionRef, e.what() );
			throw;
		}
	}
}

SoySockAddr TSocketWriteThread::GetSocketAddress() const
{
	if ( !mSocket )
		throw Soy::AssertException("TSocketWriteThread::GetSocketAddress has no socket");
	
	auto Connection = mSocket->GetConnection(mConnectionRef);
	return Connection.mAddr;
}



