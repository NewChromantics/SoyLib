#include "SoyHttpServer.h"



TSocketServer::TSocketServer(size_t& Port,const std::string& ThreadName) :
	SoyWorkerThread		( ThreadName, SoyWorkerWaitMode::Sleep )
{
	mSocket.reset( new SoySocket() );
	
	//	blocking on threads
	static bool Blocking = true;
	mSocket->CreateTcp(Blocking);
	
	//	gr: change this so socket throws
	if ( !mSocket->ListenUdp( size_cast<uint16>(Port) ) )
	{
		std::stringstream Error;
		Error << "Failed to open listening socket on port " << Port;
		throw Soy::AssertException( Error.str() );
	}
}

void TSocketServer::Shutdown()
{
	mSocket->Close();
	mSocket.reset();
}
	

bool TSocketServer::Iteration()
{
	auto ClientRef = mSocket->WaitForClient();
	if ( !ClientRef.IsValid() )
	{
		//	check socket hasn't been closed...
		return true;
	}
	
	//	client connected, alloc etc
	{
		std::lock_guard<std::mutex> Lock( mClientsLock );
		auto pClient = std::make_shared<TSocketClient>();
		mClients.PushBack( pClient );
	}
	
	return true;
}



THttpServer::THttpServer(size_t ListenPort,std::function<void(const Http::TRequestProtocol&,SoyRef)> OnRequest) :
	TSocketServer	( ListenPort ),
	mOnRequest		( OnRequest )
{
	
}

void THttpServer::SendResponse(const Http::TResponseProtocol& Response,SoyRef Client)
{
	
}


std::shared_ptr<TSocketReadThread> THttpServer::CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)
{
	return nullptr;
}

std::shared_ptr<TSocketWriteThread> THttpServer::CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)
{
	return nullptr;
}
