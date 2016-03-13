#include "SoyHttpServer.h"




TSocketClient::TSocketClient(std::shared_ptr<TSocketReadThread> ReadThread,std::shared_ptr<TSocketWriteThread> WriteThread) :
	mWriteThread		( WriteThread ),
	mReadThread			( ReadThread )
{
}


TSocketClient::~TSocketClient()
{
	if ( mWriteThread )
	{
		mWriteThread->WaitToFinish();
		mWriteThread.reset();
	}
	
	if ( mReadThread )
	{
		mReadThread->WaitToFinish();
		mReadThread.reset();
	}
}

void TSocketClient::Send(std::shared_ptr<Soy::TWriteProtocol> Data)
{
	Soy::Assert( mWriteThread!=nullptr, "Write thread expected");
	Soy::Assert( Data!=nullptr, "Write Data expected");
	mWriteThread->Push( Data );
}


TSocketServer::TSocketServer(size_t& Port,const std::string& ThreadName) :
	SoyWorkerThread		( ThreadName, SoyWorkerWaitMode::Sleep ),
	mPort				( Port )
{
	mSocket.reset( new SoySocket() );
	
	//	blocking on threads
	static bool Blocking = true;
	mSocket->CreateTcp(Blocking);
	
	//	gr: change this so socket throws
	if ( !mSocket->ListenTcp( size_cast<uint16>(Port) ) )
	{
		std::stringstream Error;
		Error << "Failed to open listening socket on port " << Port;
		throw Soy::AssertException( Error.str() );
	}
	
	auto OnConnected = [this](SoyRef ConnectionRef)
	{
		CreateClient( ConnectionRef );
	};

	auto OnDisconnected = [this](SoyRef ConnectionRef)
	{
		DestroyClient( ConnectionRef );
	};
	
	mSocket->mOnConnect.AddListener( OnConnected );
	mSocket->mOnDisconnect.AddListener( OnDisconnected );
	
	//	may need to defer this later
	Start();
}

TSocketServer::~TSocketServer()
{
	Shutdown();
	
	//	wait for futures
	while ( true )
	{
		std::lock_guard<std::mutex> Lock( mAsyncsLock );
		if ( !mAsyncs.IsEmpty() )
			break;
		
		auto Future = mAsyncs.PopAt(0);
		Future->Wait();
	}
}

void TSocketServer::Shutdown()
{
	mSocket->Close();
	mSocket.reset();
}
	

bool TSocketServer::Iteration()
{
	if ( !mSocket )
		return false;
	
	//	just keep doing this and use the callbacks
	auto ClientRef = mSocket->WaitForClient();
	if ( !ClientRef.IsValid() )
	{
		//	check socket hasn't been closed...
		if ( !mSocket->IsConnected() )
			return false;
		
		return true;
	}
	
	return true;
}

std::shared_ptr<TSocketClient> TSocketServer::GetClient(SoyRef Connection)
{
	std::lock_guard<std::mutex> Lock( mClientsLock );
	auto it = mClients.find( Connection );
	return it->second;
}


void TSocketServer::CreateClient(SoyRef Connection)
{
	{
		std::lock_guard<std::mutex> Lock( mClientsLock );
		auto it = mClients.find( Connection );
		Soy::Assert( it == mClients.end(), "Client already exists");
	}

	auto OnRecvData = [this,Connection](std::shared_ptr<Soy::TReadProtocol>& ReadData)
	{
		Soy::Assert( ReadData != nullptr, "ReadData expected" );
	
		OnRecievedData( *ReadData, Connection );
	};

	//	create threads
	auto ReadThread = CreateReadThread( mSocket, Connection );
	auto WriteThread = CreateWriteThread( mSocket, Connection );
	
	{
		std::lock_guard<std::mutex> Lock( mClientsLock );
		auto Client = std::make_shared<TSocketClient>( ReadThread, WriteThread );
		mClients[Connection] = Client;
	}
						 
	ReadThread->mOnDataRecieved.AddListener( OnRecvData );
	ReadThread->Start();
	WriteThread->Start();
}


void TSocketServer::DestroyClient(SoyRef Connection)
{
	std::shared_ptr<Soy::TSemaphore> Semaphore( new Soy::TSemaphore );

	auto AsyncDelete = [Semaphore,this,Connection]()
	{
		//	pop client
		std::shared_ptr<TSocketClient> Client;
		{
			std::lock_guard<std::mutex> Lock( mClientsLock );
			auto it = mClients.find( Connection );
			if ( it != mClients.end() )
			{
				Client = it->second;
				mClients.erase( it );
			}
		}
		Client.reset();
		Semaphore->OnCompleted();
	};
	
	std::lock_guard<std::mutex> Lock(mAsyncsLock);
	mAsyncs.PushBack( Semaphore );
	std::thread( AsyncDelete ).detach();
}


THttpServer::THttpServer(size_t ListenPort,std::function<void(const Http::TRequestProtocol&,SoyRef,THttpServer&)> OnRequest) :
	TSocketServer	( ListenPort ),
	mOnRequest		( OnRequest )
{
	
}

void THttpServer::SendResponse(const Http::TResponseProtocol& Response,SoyRef ClientRef)
{
	auto pResponse = std::make_shared<Http::TResponseProtocol>( Response );
	SendResponse( pResponse, ClientRef );
}


void THttpServer::SendResponse(std::shared_ptr<Soy::TWriteProtocol> Response,SoyRef ClientRef)
{
	try
	{
		//	grab writer for client
		auto Client = GetClient( ClientRef );
		Soy::Assert( Client!=nullptr, "Client expected");
		Client->Send( Response );
	}
	catch(std::exception& e)
	{
		std::Debug << "Send-response failed: " << e.what() << std::endl;
	}
}


std::shared_ptr<TSocketReadThread> THttpServer::CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)
{
	return std::make_shared<TSocketReadThread_Impl<Http::TRequestProtocol>>( Socket, ConnectionRef );
}



std::shared_ptr<TSocketWriteThread> THttpServer::CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)
{
	return std::make_shared<THttpWriteThread>( Socket, ConnectionRef );
}


void THttpServer::OnRecievedData(Soy::TReadProtocol& ReadData,SoyRef Connection)
{
	auto& HttpData = dynamic_cast<Http::TRequestProtocol&>( ReadData );
	mOnRequest( HttpData, Connection, *this );
}




