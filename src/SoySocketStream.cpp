#include "SoySocketStream.h"



TSocketReadThread::TSocketReadThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef) :
	TStreamReader	( "TSocketReadThread" ),
	mSocket			( Socket ),
	mConnectionRef	( ConnectionRef )
{
	
}



void TSocketReadThread::Read(TStreamBuffer& Buffer)
{
	SoySocketConnection	ClientSocket = mSocket->GetConnection( mConnectionRef );
	if ( !ClientSocket.IsValid() )
	{
		//	lost connection
		throw Soy::AssertException("Connection lost");
	}
	
	//	gr use a larger static buffer so we can stream locally much faster than in 1024 chunks
	//static int BufferSize = 1024*1024*20;	//	Xmb a time should be plenty... maybe query for the actual socket limit jsut so we don't go silly with memory
	static int BufferSize = 100;	//	Xmb a time should be plenty... maybe query for the actual socket limit jsut so we don't go silly with memory
	auto& RecvBuffer = mRecvBuffer;
	RecvBuffer.SetSize( BufferSize );
	
	
	//	throws on error
	if ( !ClientSocket.Recieve( GetArrayBridge(RecvBuffer) ) )
	{
		//	graceful close
		throw Soy::AssertException("Gracefull close");
	}
	
	Buffer.Push( GetArrayBridge( RecvBuffer ) );
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


void TSocketWriteThread::Write(TStreamBuffer& Buffer)
{
	//	keep writing until buffer is empty
	while ( !Buffer.IsEmpty() )
	{
		//	abort if thread is stopped
		//	gr: or do we want to try and finish?
		if ( !IsWorking() )
		{
			//	throw seeing as we're being interrupted?
			std::Debug << "Aborting TSocketWriteThread::Write as thread is stopped, with " << Buffer.GetBufferedSize() << " bytes remaining to send" << std::endl;
			break;
		}
		
		SoySocketConnection	ClientSocket = mSocket->GetConnection( mConnectionRef );
		if ( !ClientSocket.IsValid() )
		{
			//	lost connection
			throw Soy::AssertException("Connection lost");
		}
		
		static size_t PopMaxSize = 1024 * 1024 * 1;
		size_t PopSize = std::min( PopMaxSize, Buffer.GetBufferedSize() );
		if ( !Buffer.Pop( PopSize, GetArrayBridge(mSendBuffer) ) )
		{
			std::Debug << "Buffer data has gone missing. Trying again" << std::endl;
			continue;
		}
		
		//	write to socket
		try
		{
			ClientSocket.Send( GetArrayBridge(mSendBuffer), mSocket->IsUdp() );
		}
		catch (std::exception& e)
		{
			mSocket->OnError( mConnectionRef, e.what() );
			throw;
		}
	}
}




