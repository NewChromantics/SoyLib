#pragma once

#include "SoyTypes.h"
#include "SoyRef.h"
#include "SoyEvent.h"


#if defined(TARGET_WINDOWS)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
	typedef int socklen_t;
	typedef ULONG in_addr_t;	//	note, not IN_ADDR as that has extra fields we don't need
	typedef int socket_data_size_t;
#elif defined(TARGET_POSIX)||defined(TARGET_PS4)
	#include <sys/socket.h>
	#define INVALID_SOCKET -1
	typedef int SOCKET;
	typedef size_t socket_data_size_t;
#endif

#if defined(TARGET_POSIX)
	#include <arpa/inet.h>	//	in_addr_t
//	typedef __in_addr_t in_addr_t;
#endif

namespace Soy
{
	namespace Winsock
	{
		bool		Init();
		void		Shutdown();
		int			GetError();
		bool		HasError(const std::string& ErrorContext,bool BlockIsError=true,int Error=GetError(),std::ostream* ErrorStream=nullptr);
		bool		HasError(std::stringstream&& ErrorContext, bool BlockIsError=true,int Error=GetError(),std::ostream* ErrorStream=nullptr);
	}
}


class SoySockAddr
{
public:
	SoySockAddr()
	{
		memset( &mAddr, 0, sizeof(mAddr) );
	}
	SoySockAddr(const std::string& Hostname,const uint16 Port);
	SoySockAddr(struct addrinfo& AddrInfo);
	SoySockAddr(const sockaddr& Addr,socklen_t AddrLen);
	explicit SoySockAddr(in_addr_t AddressIpv4,const uint16 Port);
	//explicit SoySockAddr(in6_addr AddressIpv6,const uint16 Port);
	
	//	gr: remove this for a try/catch
	bool				IsValid() const
	{
		//	check for "not all zero"
		SoySockAddr Null;
		return memcmp( &Null.mAddr, &mAddr, sizeof(mAddr) ) != 0;
	}

	socklen_t			GetSockAddrLength() const;
	const sockaddr*		GetSockAddr() const;
	sockaddr*			GetSockAddr();

public:
	sockaddr_storage	mAddr;
};


class SoySocketConnection
{
public:
	SoySocketConnection() :
		mSocket		( INVALID_SOCKET )
	{
		memset( &mAddr, 0, sizeof(mAddr) );
		assert( !IsValid() );
	}

	bool			IsValid() const		
	{
		return mSocket != INVALID_SOCKET;	
	}

	bool		Recieve(ArrayBridge<char>&& Buffer);	//	throws on error. Returns false on graceful disconnection
	void		Send(ArrayBridge<char>&& Buffer,bool IsUdp);		//	throws on error. keeps writing until all sent

public:
	SoySockAddr		mAddr;
	SOCKET			mSocket;	//	gr: currently if we're a client connecting to a server, this is the client's DUPLICATED socket (connect() doesnt give us a socket)
};

std::ostream& operator<< (std::ostream &out,const SoySocketConnection &in);
std::ostream& operator<< (std::ostream &out,const SoySockAddr &in);


class SoySocket
{
public:
	SoySocket() :
		mSocket			( INVALID_SOCKET )
	{
	}
	~SoySocket()	{	Close();	}

	bool		CreateTcp(bool Blocking=false);
	bool		CreateUdp(bool Broadcast);
	bool		IsCreated() const		{	return mSocket != INVALID_SOCKET;	}
	void		Close();

	SoyRef		AllocConnectionRef();
	bool		ListenTcp(int Port);
	bool		ListenUdp(int Port);
	SoyRef		WaitForClient();
	SoyRef		Connect(std::string Address);
	SoyRef		UdpConnect(const char* Address,uint16 Port);	//	this doesn't "do" a connect, but fakes one as a success, and starts listening (required only on windows
	SoyRef		UdpConnect(SoySockAddr Address);

	bool		IsConnected();
	bool		IsUdp() const;

	SoySocketConnection	GetFirstConnection() const;					//	get first one which is useful for clients to show which server they're attached to
	SoySocketConnection	GetConnection(SoyRef ConnectionRef);
	void		Disconnect(SoyRef ConnectionRef,const std::string& Reason);

	void		OnError(SoyRef ConnectionRef,const std::string& Error);	//	error occured, disconnect from this

	SOCKET		GetSocket()	{	return mSocket;	}	//	gr: don't think I should be providing access here....
	
	
protected:
	SoyRef		OnConnection(SoySocketConnection Connection);
	bool		Bind(uint16 Port,SoySockAddr& outSockAddr);

public:
	SoyEvent<const SoyRef>					mOnConnect;
	SoyEvent<const SoyRef>					mOnDisconnect;
	SoySockAddr								mSocketAddr;		//	current socket address; gr: find this from the socket? Is it the first connection?

private:
	//	lock to control ordered connect/disconnect (not strictly for race conditions)
	//	also lock whenever manipulating/passing on mSocket to help control flow
	std::recursive_mutex					mConnectionLock;
	std::map<SoyRef,SoySocketConnection>	mConnections;
	SOCKET									mSocket;
};
