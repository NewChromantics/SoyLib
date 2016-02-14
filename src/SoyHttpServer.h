#pragma once

#include <SoySocket.h>
#include <SoyStream.h>
#include <SoyHttp.h>
#include <SoyProtocol.h>
#include <SoySocketStream.h>



class TSocketClient
{
public:
	
};

class TSocketServer : public SoyWorkerThread
{
public:
	TSocketServer(size_t& Port,const std::string& ThreadName="TSocketServer");
	~TSocketServer()		{	Shutdown();	}
	

protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	
private:
	virtual bool	Iteration() override;
	void			Shutdown();

public:

protected:
	size_t							mPort;
	
private:
	std::shared_ptr<SoySocket>		mSocket;
	std::mutex						mClientsLock;
	Array<std::shared_ptr<TSocketClient>>	mClients;
};




class THttpServer : public TSocketServer
{
public:
	THttpServer(size_t ListenPort,std::function<void(const Http::TRequestProtocol&,SoyRef)> OnRequest);
	
	void			SendResponse(const Http::TResponseProtocol& Response,SoyRef Client);
	
protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;
	
public:
	std::function<void(const Http::TRequestProtocol&,SoyRef)>	mOnRequest;
};

