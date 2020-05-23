#pragma once

#include "SoyTypes.h"
#include "SoyRef.h"
#include "Array.hpp"
#include <map>


#if defined(TARGET_LINUX)
#define TARGET_POSIX
#endif

#if defined(TARGET_WINDOWS)

	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
	typedef int socklen_t;
	typedef ULONG in_addr_t;	//	note, not IN_ADDR as that has extra fields we don't need
	typedef int socket_data_size_t;

#elif defined(TARGET_POSIX)

	#include <sys/socket.h>
	#include <arpa/inet.h>	//	in_addr_t
	//	typedef __in_addr_t in_addr_t;

	#if defined(TARGET_PS4)||defined(TARGET_LINUX)
		#include <netinet/in.h>
	#endif

	#if defined(TARGET_LINUX)
	#define __SOCK_SIZE__	_SS_SIZE
	#endif

	#define INVALID_SOCKET -1
	typedef int SOCKET;
	typedef size_t socket_data_size_t;

#endif

namespace Soy
{
	namespace Winsock
	{
		void		Init();
		void		Shutdown();
		int			GetError();
		bool		HasError(const std::string& ErrorContext,bool BlockIsError=true,int Error=GetError(),std::ostream* ErrorStream=nullptr);
		bool		HasError(std::stringstream&& ErrorContext, bool BlockIsError=true,int Error=GetError(),std::ostream* ErrorStream=nullptr);
	}
}


class SoySocket;


//	internal socket address structure. Handles different sizes, different functio calls, different types on different platforms
//	shouldn't really use it externally
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
	
	void				SetPort(uint16 Port);
	uint16_t			GetPort() const;

	socklen_t			GetSockAddrLength() const;
	const sockaddr*		GetSockAddr() const;
	sockaddr*			GetSockAddr();

	bool				operator==(const SoySockAddr& That) const;
	
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

	//	throws on error. false on graceful close
	//	if SoyRef is returned, it's the client-ref of the reciver (invalid SoyRef if graceful close)
	//	- this is allocated on UDP so we have a persistent record of the SockAddr (ie so we can send back)
	//	- on TCP client, this should be itself
	bool		Recieve(ArrayBridge<char>&& Buffer);
	SoyRef		Recieve(ArrayBridge<char>&& Buffer,SoySocket& Parent);

	void		Send(const ArrayBridge<char>& Buffer,bool IsUdp);		//	throws on error. keeps writing until all sent
	void		Send(const ArrayBridge<char>&& Buffer,bool IsUdp)	{	Send(Buffer,IsUdp);	}

private:
	SoyRef		Recieve(ArrayBridge<char>& Buffer,SoySocket* Parent);

public:
	SoySockAddr		mAddr;
	SOCKET			mSocket;	//	gr: currently if we're a client connecting to a server, this is the client's DUPLICATED socket (connect() doesnt give us a socket)
};

std::ostream& operator<< (std::ostream &out,const SoySocketConnection &in);
std::ostream& operator<< (std::ostream &out,const SoySockAddr &in);


class SoySocket
{
	friend class SoySocketConnection;	//	to allow UDP connectionref alloc
	
public:
	SoySocket() :
		mSocket			( INVALID_SOCKET )
	{
	}
	~SoySocket()	{	Close();	}

	void		CreateTcp(bool Blocking=false);
	void		CreateUdp(bool Broadcast);
	bool		IsCreated() const		{	return mSocket != INVALID_SOCKET;	}
	void		Close();

	SoyRef		AllocConnectionRef();
	void		ListenTcp(int Port);
	void		ListenUdp(int Port, bool SaveListeningConnection);
	SoyRef		WaitForClient();
	SoyRef		Connect(const char* Hostname, uint16 Port);
	SoyRef		UdpConnect(const char* Hostname,uint16 Port);	//	this doesn't "do" a connect, but fakes one as a success, and starts listening
	SoyRef		UdpConnect(SoySockAddr Address);

	bool		IsConnected();
	bool		IsUdp() const;

	SoySocketConnection	GetFirstConnection() const;					//	get first one which is useful for clients to show which server they're attached to
	SoySocketConnection	GetConnection(SoyRef ConnectionRef);
	void		Disconnect(SoyRef ConnectionRef,const std::string& Reason);

	void		OnError(SoyRef ConnectionRef,const std::string& Error);	//	error occured, disconnect from this

	//	we [currently] create sockets on IN_ANYADDR so we open sockets on all interfaces
	//	which gives us multiple IP's, but our ip is 0.0.0.0, so
	//	for each interface this socket is connected on, get it's IP
	void		GetSocketAddresses(std::function<void(std::string&,SoySockAddr&)> EnumAdress) const;
	SOCKET		GetSocket()	{	return mSocket;	}	//	gr: don't think I should be providing access here....
	
	//	run a function on each connection (client) with appropriate locking
	void		EnumConnections(std::function<void(SoyRef,SoySocketConnection)> EnumConnection);

protected:
	SoyRef		GetConnectionRef(const SoySockAddr& SockAddr);	//	get/alloc client reference to this address
	
private:
	SoyRef		OnConnection(SoySocketConnection Connection);
	void		Bind(uint16 Port,SoySockAddr& outSockAddr);

public:
	std::function<void(SoyRef)>						mOnConnect;
	std::function<void(SoyRef,const std::string&)>	mOnDisconnect;
	SoySockAddr										mSocketAddr;		//	current socket address; gr: find this from the socket? Is it the first connection?

private:
	//	lock to control ordered connect/disconnect (not strictly for race conditions)
	//	also lock whenever manipulating/passing on mSocket to help control flow
	std::recursive_mutex					mConnectionLock;
	std::map<SoyRef,SoySocketConnection>	mConnections;
	SOCKET									mSocket;
};
