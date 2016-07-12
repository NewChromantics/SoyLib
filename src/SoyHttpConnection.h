#pragma once

#include "SoySocket.h"
#include "SoyStream.h"
#include "SoyHttp.h"
#include "SoyProtocol.h"
#include "SoySocketStream.h"



class TSocketConnection : public SoyWorkerThread
{
public:
	TSocketConnection(const std::string& Address,const std::string& ThreadName="TSocketConnection");
	~TSocketConnection()		{	Shutdown();	}
	
	void								SendRequest(std::shared_ptr<Soy::TWriteProtocol> Request);

	std::shared_ptr<TSocketReadThread>	GetReadThread()	{	return mReadThread;	}

protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef)=0;
	
private:
	virtual bool	Iteration() override;
	void			Shutdown();
	void			FlushRequestQueue();		//	we queue requests until we're connected

public:
	SoyEvent<const std::string>		mOnError;
	SoyEvent<bool>					mOnConnected;

protected:
	std::string						mServerAddress;
	
private:
	SoyRef							mConnectionRef;
	std::shared_ptr<SoySocket>		mSocket;
	
	std::shared_ptr<TSocketReadThread>	mReadThread;
	std::shared_ptr<TSocketWriteThread>	mWriteThread;
	
	Array<std::shared_ptr<Soy::TWriteProtocol>>	mRequestQueue;
};



class THttpConnection : public TSocketConnection
{
public:
	THttpConnection(const std::string& Url);
	
	void			SendRequest(const Http::TRequestProtocol& Request);
	void			SendRequest(std::shared_ptr<Http::TRequestProtocol> Request);

protected:
	virtual std::shared_ptr<TSocketReadThread>	CreateReadThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;
	virtual std::shared_ptr<TSocketWriteThread>	CreateWriteThread(std::shared_ptr<SoySocket> Socket,SoyRef ConnectionRef) override;

public:
	SoyEvent<const Http::TResponseProtocol>	mOnResponse;
	
private:
	SoyListenerId						mOnDataRecievedListener;
};




