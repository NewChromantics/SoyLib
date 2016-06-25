#pragma once

#include "SoySocket.h"
#include "SoyStream.h"
#include "SoyHttp.h"
#include "SoyProtocol.h"
#include "SoySocketStream.h"
#include <future>




class TSocketClient
{
public:
	TSocketClient(std::shared_ptr<TSocketReadThread> ReadThread,std::shared_ptr<TSocketWriteThread> WriteThread);
	~TSocketClient();
	
	void			Send(std::shared_ptr<Soy::TWriteProtocol> Data);

public:
	std::shared_ptr<TSocketReadThread>	mReadThread;
	std::shared_ptr<TSocketWriteThread>	mWriteThread;
};




class TSocketServer : public SoyWorkerThread
{
public:
	TSocketServer(size_t& Port,const std::string& ThreadName="TSocketServer");
	~TSocketServer();
	
	size_t			GetListeningPort() const	{	return mPort;	}	//	get from socket?

protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	virtual void								OnRecievedData(Soy::TReadProtocol& ReadData,SoyRef Connection)=0;
	std::shared_ptr<TSocketClient>				GetClient(SoyRef Connection);
	
private:
	virtual bool	Iteration() override;
	void			Shutdown();
	void			CreateClient(SoyRef Connection);
	void			DestroyClient(SoyRef Connection);
	
public:

protected:
	size_t							mPort;
	
private:
	std::shared_ptr<SoySocket>		mSocket;
	std::mutex						mClientsLock;
	std::map<SoyRef,std::shared_ptr<TSocketClient>>	mClients;
	
	//	defered stuff
	std::mutex						mAsyncsLock;
	Array<std::shared_ptr<Soy::TSemaphore>>		mAsyncs;
};




class THttpServer : public TSocketServer
{
public:
	THttpServer(size_t ListenPort,std::function<void(const Http::TRequestProtocol&,SoyRef,THttpServer&)> OnRequest);
	
	void			SendResponse(const Http::TResponseProtocol& Response,SoyRef Client);
	void			SendResponse(std::shared_ptr<Soy::TWriteProtocol> Response,SoyRef Client);
	
protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;
	virtual void								OnRecievedData(Soy::TReadProtocol& ReadData,SoyRef Connection) override;

public:
	std::function<void(const Http::TRequestProtocol&,SoyRef,THttpServer&)>	mOnRequest;
};

