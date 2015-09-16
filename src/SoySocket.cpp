#include "SoySocket.h"
#include <SoyDebug.h>
#include <regex>


#if defined(TARGET_POSIX)
#include <arpa/inet.h>
#include <fcntl.h>	//	fcntl
#include <unistd.h>	//	close
#include <netdb.h>	//	gethostbyname
#endif

//	one heap for sockets. though this seems bad and would have a lot of clashes. Currently for debug really
prmem::Heap TChannelSocket_Heap( true, true, "TChannelSocket" );

#define PORT_ANY	0

bool Soy::Winsock::HasError(std::stringstream&& ErrorContext, bool BlockIsError,int Error,std::ostream* ErrorStream)
{
	return HasError(ErrorContext.str(), BlockIsError, Error, ErrorStream );
};



SoySockAddr::SoySockAddr(const std::string& Hostname,const uint16 Port)
{
	std::string PortName = Soy::StreamToString( std::stringstream() << Port );

	//	ipv6 friendly host fetch
	struct addrinfo* pHostAddrInfo = nullptr;
	auto Error = getaddrinfo( Hostname.c_str(), PortName.c_str(), nullptr, &pHostAddrInfo );
	if ( Soy::Winsock::HasError( Soy::StreamToString( std::stringstream() << "getaddrinfo(" << Hostname << ":" << PortName << ")"), false, Error ) )
	{
		*this = SoySockAddr();
		return;
	}

	*this = SoySockAddr( *pHostAddrInfo );
	freeaddrinfo( pHostAddrInfo );
}

SoySockAddr::SoySockAddr(struct addrinfo& AddrInfo)
{
	//	copy
	memcpy( &mAddr, AddrInfo.ai_addr, AddrInfo.ai_addrlen );
}

SoySockAddr::SoySockAddr(const sockaddr& Addr,socklen_t AddrLen)
{
	//	check length. field not present in winsock or linux (android)
#if defined(TARGET_OSX)||defined(TARGET_IOS)
	auto ExpectedLength = Addr.sa_len;
#elif defined(TARGET_ANDROID)
	auto ExpectedLength = __SOCK_SIZE__;
#elif defined(TARGET_WINDOWS)
#error what is expected length in windows?
#endif
	if ( AddrLen != ExpectedLength )
	{
		std::stringstream err;
		err << "sockaddr length (" << ExpectedLength << ") doesn't match specification " << AddrLen;
		throw Soy::AssertException( err.str() );
	}

	if ( AddrLen > sizeof(mAddr) )
	{
		std::stringstream err;
		err << "sockaddr length (" << AddrLen << ") too big for storage " << sizeof(mAddr);
		throw Soy::AssertException( err.str() );
	}

	memcpy( &mAddr, &Addr, AddrLen );
}


SoySockAddr::SoySockAddr(in_addr_t AddressIp4,uint16 Port)
{
	auto& Addr4 = *reinterpret_cast<sockaddr_in*>(&mAddr);

	//	gr: find a proper function for this
	Addr4.sin_family = AF_INET;
	Addr4.sin_port = htons(Port);
#if defined(TARGET_OSX)||defined(TARGET_IOS)
	Addr4.sin_len = sizeof(sockaddr_in);
#endif
	Addr4.sin_addr.s_addr = AddressIp4;
}


const sockaddr* SoySockAddr::GetSockAddr() const
{
	return reinterpret_cast<const sockaddr*>( &mAddr );
}

sockaddr* SoySockAddr::GetSockAddr()
{
	return reinterpret_cast<sockaddr*>( &mAddr );
}

socklen_t SoySockAddr::GetSockAddrLength() const
{
	auto* SockAddrIn = GetSockAddr();
	
	if ( SockAddrIn->sa_family == AF_INET )
		return sizeof(sockaddr_in);
	
	if ( SockAddrIn->sa_family == AF_INET6 )
		return sizeof(sockaddr_in6);

	return 0;
}

std::ostream& operator<< (std::ostream &out,const SoySockAddr &Addr)
{
	BufferArray<char,100> Host(100);
	BufferArray<char,100> Port(100);
	int Flags = 0;
	Flags |= NI_NUMERICSERV;	//	also show "service" (port) as a number
	Flags |= NI_NUMERICHOST;

	auto Error = getnameinfo( Addr.GetSockAddr(), Addr.GetSockAddrLength(), Host.GetArray(), size_cast<socklen_t>(Host.GetDataSize()), Port.GetArray(), size_cast<socklen_t>(Port.GetDataSize()), Flags );
	if ( Error != 0 )
	{
		Soy::Winsock::HasError("getnameinfo");
		out << "[failed to get name for sockaddr]";
		return out;
	}
	out << Host.GetArray() << ":" << Port.GetArray();
/*
#elif defined(TARGET_WINDOWS)
	getnameinfo
	BufferArray<char,100> Buffer(100);
	DWORD BufferLen = size_cast<DWORD>(Buffer.GetDataSize());
	//	gr: change this to use W version as A is deprecated
	//	gr: also find proper port extraction
	int Result = WSAAddressToStringA( (LPSOCKADDR)&Addr, sizeof(Addr), nullptr, Buffer.GetArray(), &BufferLen );
	bool Success = ( Result != SOCKET_ERROR );
	//	includes terminator
	Buffer.SetSize( Success ? BufferLen : 0 );
	//uint16 Port = nstoh( Addr.sin_port );
	//Soy::Assert( Port == Addr.GetPort(), "Expected matching ports" )
	
	if ( !Success )
	{
		Soy::Winsock::HasError("AddrToString");
		out << "[failed to get name for sockaddr]";
		return out;
	}
	
	out << Buffer.GetArray();
#endif
*/
	return out;
}


std::ostream& operator<< (std::ostream &out,const SoySocketConnection &in)
{
	out << in.mAddr;
	return out;
}


#include <signal.h>
bool Soy::Winsock::Init()
{
#if defined(TARGET_WINDOWS)
	WORD wVersionRequested = MAKEWORD(2,2);
	WSADATA wsaData;

	auto Error = WSAStartup(wVersionRequested, &wsaData);
	if ( Error != 0 )
	{
		std::Debug << "Failed to initialise Winsock. " << Soy::Platform::GetLastErrorString() << std::endl; 
		return false;
	}
#endif
	return true;
}

void Soy::Winsock::Shutdown()
{
#if defined(TARGET_WINDOWS)
	WSACleanup();
#endif
	HasError("Shutdown");
}
		
int Soy::Winsock::GetError()
{
#if defined(TARGET_WINDOWS)
	return WSAGetLastError();
#elif defined(TARGET_POSIX)
	return Soy::Platform::GetLastError();
#endif
}


#if defined(TARGET_POSIX)
	#define SOCKERROR(e)		(E ## e)
	#define SOCKERROR_SUCCESS	0
	#define SOCKET_ERROR		-1
#elif defined(TARGET_WINDOWS)
	#define SOCKERROR(e)	(WSAE ## e)
	#define SOCKERROR_SUCCESS	ERROR_SUCCESS
#endif

#if !defined(SOCKERROR)
#error SOCKERROR Macro not defined
#endif

//	gr: when something still has a connection to a closed socket, and we try and open it again, we get this error
//		assuming its some file-io error...
//	gr: note this is bit 9(256) | ETIMEDOUT. occurs during accept() and according to this page, may indicate a timeout... on our last socket?
#define SOCKERROR_ZOMBIE	316


bool Soy::Winsock::HasError(const std::string& ErrorContext,bool BlockIsError,int Error,std::ostream* ErrorStream)
{
	std::string ErrorString = Soy::Platform::GetErrorString( Error );

	switch ( Error )
	{
	case SOCKERROR_SUCCESS:
		return false;

	case SOCKERROR(WOULDBLOCK):
		if ( !BlockIsError )
			return false;
		break;
			
	case SOCKERROR_ZOMBIE:
		ErrorString = "failed to accept() zombie connection";
		break;
	};

	//	if we fall through here, it's an error
	//	gr: the . suffix is required here. If we output "Permission denied/n" in the unit
	//		test post-build it "detects" an error, even if a test didn't fail. The dot stops this "clever detection"
	std::Debug << "Winsock error (" << Error << "): " << ErrorString << ". " << ErrorContext << std::endl;
	
	if ( ErrorStream )
		*ErrorStream << ErrorString;

	return true;
}


void SoySocket::Close()
{
	mConnectionLock.lock();
	if ( mSocket != INVALID_SOCKET )
	{
#if defined(TARGET_WINDOWS)
		closesocket( mSocket );
#elif defined(TARGET_POSIX)
		close( mSocket );
#endif
		Soy::Winsock::HasError("CloseSocket");
		mSocket = INVALID_SOCKET;
		mSocketAddr = SoySockAddr();
	}
	mConnectionLock.unlock();
}

bool SoySocket::IsUdp() const
{
	//mConnectionLock.lock();
	
	int SockType = -1;
	socklen_t SockTypeLen = sizeof(SockType);
#if defined(TARGET_WINDOWS)
	auto* pSockType = reinterpret_cast<char*>(&SockType);
#else
	auto* pSockType = &SockType;
#endif
	auto Error = getsockopt( mSocket, SOL_SOCKET, SO_TYPE, pSockType, &SockTypeLen );
	if ( Error != 0 )
		Soy::Winsock::HasError("getsockopt UDP check");
	
	//mConnectionLock.unlock();

	return SockType == SOCK_DGRAM;
}


bool SoySocket::CreateTcp(bool Blocking)
{
	//	already created
	if ( IsCreated() && !IsUdp() )
		return true;

	if ( IsCreated() )
		Close();
	
	if ( !Soy::Winsock::Init() )
		return false;

	mConnectionLock.lock();
	mSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
	if ( mSocket == INVALID_SOCKET )
	{
		Soy::Winsock::HasError("Create socket");
		mConnectionLock.unlock();
		return false;
	}
	//	gr: don't have an ip/port until bound?
	mSocketAddr = SoySockAddr();

	bool Success = false;
#if defined(TARGET_POSIX)
	int flags;
	flags = fcntl( mSocket, F_GETFL, 0 );
	
	Success = (flags != -1);
	if ( Success )
	{
		if ( Blocking )
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		auto Result = fcntl( mSocket, F_SETFL, flags );
		Success &= (Result==0);
	}

	//	turn off SIGPIPE signals for broken pipe read/writing
#if !defined(TARGET_ANDROID)
	bool EnableSigPipe = false;
	if ( !EnableSigPipe )
	{
		int set = 1;
		auto Error = setsockopt( mSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(set) );
		if ( Error != 0 )
			Soy::Winsock::HasError("setsockopt turned of SIGPIPE");
	}
#endif
	
#elif defined(TARGET_WINDOWS)
	//	make non-blocking
	unsigned long arg = !Blocking;
	auto Result = ioctlsocket( mSocket, FIONBIO, &arg );
	Success = (Result != SOCKET_ERROR );
#endif
	
	if ( !Success )
	{
		std::Debug << "Could not make socket " << (Blocking?"":"non-") << "blocking" << std::endl;
		Soy::Winsock::HasError( Soy::StreamToString( std::stringstream()<< "make socket " << (Blocking?"":"non-") << "blocking" ) );
		mConnectionLock.unlock();
		Close();
		return false;
	}

	mConnectionLock.unlock();
	return true;
}



bool SoySocket::CreateUdp(bool Broadcast)
{
	//	already created
	if ( IsCreated() && IsUdp() )
		return true;
	
	if ( !Soy::Winsock::Init() )
		return false;
	
	mConnectionLock.lock();
	mSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( mSocket == INVALID_SOCKET )
	{
		Soy::Winsock::HasError("Create socket");
		mConnectionLock.unlock();
		return false;
	}
	//	gr: don't have an ip/port until bound?
	mSocketAddr = SoySockAddr();


	//	set broadcast mode
	int BroadcastSetting = Broadcast ? 1 : 0;
	auto SetOpError = setsockopt( mSocket, SOL_SOCKET, SO_BROADCAST, (char *)&BroadcastSetting, sizeof BroadcastSetting);

	if ( SetOpError != 0 )
	{
		std::stringstream Dbg;
		Dbg << "Could not make socket " << (Broadcast?"":"non-") << "Broadcast";
		
		std::Debug << Dbg.str() << std::endl;
		Soy::Winsock::HasError( Dbg.str() );
		mConnectionLock.unlock();
		Close();
		return false;
	}

	mConnectionLock.unlock();
	return true;
}


SoyRef SoySocket::WaitForClient()
{
	SoySockAddr SockAddr;
	sockaddr* pSockAddr = reinterpret_cast<sockaddr*>( &SockAddr.mAddr );
	socklen_t SockAddrLen = sizeof(SockAddr.mAddr);

	SoySocketConnection Client;
	Client.mSocket = ::accept( mSocket, pSockAddr, &SockAddrLen );
	if ( Client.mSocket == INVALID_SOCKET )
	{
		//	get error when socket has been closed
		Soy::Winsock::HasError(Soy::StreamToString(std::stringstream() << "Accept(" << Client << ")"), false);
		return SoyRef();
	}
	
	try
	{
		Client.mAddr = SoySockAddr( *pSockAddr, SockAddrLen );
	}
	catch ( std::exception& e )
	{
		std::Debug << e.what() << std::endl;
		//	close socket?
		return SoyRef();
	}

	return OnConnection( Client );
}

SoyRef SoySocket::AllocConnectionRef()
{
	//	gr: should be threadsafe/atomic?
	static SoyRef Ref("conn");
	Ref++;
	return Ref;
}

bool SoySocket::Bind(uint16 Port,SoySockAddr& outSockAddr)
{
	outSockAddr = SoySockAddr( INADDR_ANY, Port );
	
	if ( ::bind( mSocket, outSockAddr.GetSockAddr(), outSockAddr.GetSockAddrLength() ) == SOCKET_ERROR )
	{
		int Error = Soy::Winsock::GetError();
		Soy::Winsock::HasError( Soy::StreamToString(std::stringstream() << "Bind to " << Port), true, Error);
#if defined(TARGET_POSIX)
		if ( Error == EACCES )
			std::Debug << "Access denied binding to port " << Port << " only root user on OSX can have a port < 1024" << std::endl;
#endif
		Close();
		return false;
	}
	
	return true;
}

bool SoySocket::ListenTcp(int Port)
{
	SoySockAddr SockAddr;
	if (!Bind(Port, SockAddr))
		return false;

	auto MaxConnections = SOMAXCONN;
	if ( ::listen(mSocket, MaxConnections) == SOCKET_ERROR)
	{
		Soy::Winsock::HasError(Soy::StreamToString(std::stringstream() << "listen on " << Port));
		Close();
		return false;
	}

	mSocketAddr = SockAddr;
	std::Debug << "Socket listening on " << SockAddr << std::endl;

	return true;
}


bool SoySocket::ListenUdp(int Port)
{
	SoySockAddr SockAddr;
	if (!Bind(Port, SockAddr))
		return false;

	//	udp just binds
	std::Debug << "Socket UDP bound on " << SockAddr << std::endl;

	return true;
}



bool SoySocket::GetHostnameAndPortFromAddress(std::string& Hostname,uint16& Port,const std::string Address)
{
//	extract port from address
	std::regex Pattern("([^:]+):([0-9]+)$" );
	std::smatch Match;

	//	address is empty, or malformed
	if ( !std::regex_match( Address, Match, Pattern ) )
	{
		std::Debug << "Invalid hostname:port: " << Address << std::endl;
		return false;
	}
	
	Hostname = Match[1].str();
	std::string PortStr = Match[2].str();
	int Porti;
	Soy::StringToType( Porti, PortStr );
	Port = size_cast<uint16>(Porti);

	return true;
}

bool SoySocket::IsConnected()
{
	bool Connected = !mConnections.empty();
	return Connected;
}

SoyRef SoySocket::Connect(std::string Address)
{
	if ( mSocket == INVALID_SOCKET )
		return SoyRef();

	u_short Port;
	std::string Hostname;
	if ( !GetHostnameAndPortFromAddress( Hostname, Port, Address ) )
		return SoyRef();

	SoySockAddr HostAddr( Hostname, Port );
	if ( !HostAddr.IsValid() )
	{
		std::Debug << "couldn't get sock address for " << Address << std::endl;
		return SoyRef();
	}

	//	gr: no connection lock here as this is blocking
	
	SoySocketConnection Connection;
	Connection.mSocket = mSocket;
	Connection.mAddr = HostAddr;
	
	//	try and connect
	std::Debug << "Connecting to " << Address << "..." << std::endl;
	int Return = ::connect( mSocket, Connection.mAddr.GetSockAddr(), Connection.mAddr.GetSockAddrLength() );

	//	immediate connection success (when in blocking mode)
	if ( Return == 0 )
	{
		return OnConnection( Connection );
	}
	
	bool WaitToConnect = true;
	
	{
		int Error = Soy::Winsock::GetError();
		if ( Error == SOCKERROR(WOULDBLOCK) || Error == SOCKERROR(INPROGRESS) )
		{
			WaitToConnect = true;
		}
		else
		{
			std::Debug << "connect(" << Address << ") error: " << Soy::Platform::GetErrorString( Error ) << std::endl;
			return SoyRef();
		}
	}

	if ( WaitToConnect )
	{
		//	http://stackoverflow.com/questions/5843810/in-a-non-blocking-socket-connect-select-always-returns-1
		//	wtf is this? I see examples, but no docs
		//	gr: not important in windows. But in unix... it's sometimes *8?
		int Socketfd = static_cast<int>(mSocket+1);	
		fd_set fd;
		FD_ZERO( &fd);
		FD_SET( mSocket, &fd );
		struct timeval Timeout;
		Timeout.tv_sec = 99;
		Timeout.tv_usec = 1;
		int ReadyFileDescriptorCount = ::select( Socketfd, nullptr, &fd, NULL, &Timeout );
		
		//	0 is timedout
		//	1 is 1-ready file
		//	else more-than-one ready file, and not expecting that
		if ( ReadyFileDescriptorCount == SOCKET_ERROR )
		{
			int Error = (ReadyFileDescriptorCount==0) ? SOCKERROR(TIMEDOUT) : Soy::Winsock::GetError();
			Soy::Winsock::HasError( "Waiting for connection (select)", true, Error );
			return SoyRef();
		}
		else if ( ReadyFileDescriptorCount == 0 )
		{
			std::Debug << "Timed out waiting for connection (select)" << std::endl;
			return SoyRef();
		}
		
		if ( !Soy::Assert( ReadyFileDescriptorCount == 1, [ReadyFileDescriptorCount]{	return Soy::StreamToString( std::stringstream() << "unexpected return from select() -> " << ReadyFileDescriptorCount << " (expected 1)"	);	} ) )
		{
			//	gr: what to do with socket if it's valid?
			Close();
			return SoyRef();
		}

		//	todo: double check state of socket here?
	}
	
	return OnConnection( Connection );
}

SoyRef SoySocket::UdpConnect(const char* Address,uint16 Port)
{
	return UdpConnect( SoySockAddr(inet_addr(Address), Port ) );
}


SoyRef SoySocket::UdpConnect(SoySockAddr Address)
{
	//	lock when
	mConnectionLock.lock();
	
	//	socket has become invalid
	if ( mSocket == INVALID_SOCKET )
	{
		mConnectionLock.unlock();
		return SoyRef();
	}
	
	SoySocketConnection Connection;
	Connection.mSocket = mSocket;
	
	//	gr: errr not using Address?
	Connection.mAddr = Address;
	
	//	udp has to explicitly bind() in order to recv.
	//	gr: OSX does NOT require this, windows does
	if ( !ListenUdp(PORT_ANY) )
	{
		mConnectionLock.unlock();
		return SoyRef();
	}
	
	mConnectionLock.unlock();
	
	return OnConnection( Connection );
}

SoyRef SoySocket::OnConnection(SoySocketConnection Connection)
{
	mConnectionLock.lock();
	
	SoyRef ConnectionRef = AllocConnectionRef();
	mConnections[ConnectionRef] = Connection;
	std::Debug << "New connection (" << ConnectionRef << ") established to " << Connection << std::endl;

	mConnectionLock.unlock();

	mOnConnect.OnTriggered( ConnectionRef );
	return ConnectionRef;
}


void SoySocket::OnError(SoyRef ConnectionRef,const std::string& Reason)
{
	Disconnect( ConnectionRef, Reason );
}

void SoySocket::Disconnect(SoyRef ConnectionRef,const std::string& Reason)
{
	mConnectionLock.lock();
	
	//	get the connection
	auto Connection = GetConnection( ConnectionRef );
	//	if missing we've probably already dealt with it
	if ( !Connection.IsValid() )
	{
		mConnectionLock.unlock();
		return;
	}
	
	bool ClosingSelf = ( mSocket == Connection.mSocket );
	std::Debug << "Disconnect " << (ClosingSelf?"self" : "client") << ConnectionRef << "(" << Connection << ") reason: " << Reason << std::endl;

	//	remove from list before callback? or after, so list is accurate for callbacks
	mConnections.erase( ConnectionRef );

	//	unlock for disconnect callback...
	//	gr: could still cause race condition?
	mConnectionLock.unlock();

	//	do callback in case we want to try a hail mary
	//	gr: the old Disconnect() had this AFTER closing socket, OnError did NOT
	mOnDisconnect.OnTriggered(ConnectionRef);

	//	if the connection's socket is ourselves, (we are a client connecting to a server) close
	if ( ClosingSelf )
	{
		Close();
		return;
	}

	//	else it's probably client connection
	if ( Connection.mSocket != INVALID_SOCKET )
	{
	#if defined(TARGET_WINDOWS)
		if ( closesocket(Connection.mSocket) == SOCKET_ERROR )
	#elif defined(TARGET_POSIX)
			if ( ::close(Connection.mSocket) == SOCKET_ERROR )
	#endif
		{
			Soy::Winsock::HasError("CloseSocket");
		}
	}
	else
	{
		std::Debug << "trying to disconnect non-socket " << Connection << std::endl;
	}
}


SoySocketConnection SoySocket::GetFirstConnection() const
{
	//	unsafe due to lack of lock! on const func
	auto FirstConn = mConnections.begin();
	
	if ( FirstConn == mConnections.end() )
		return SoySocketConnection();

	return FirstConn->second;
}

SoySocketConnection SoySocket::GetConnection(SoyRef ConnectionRef)
{
	mConnectionLock.lock();
	
	//	accessing the map will create a new key, so look for it first
	SoySocketConnection Connection;
	auto Find = mConnections.find( ConnectionRef );
	if ( Find != mConnections.end() )
		Connection = Find->second;

	mConnectionLock.unlock();
	
	return Connection;
}



bool SoySocketConnection::Recieve(ArrayBridge<char>&& Buffer)
{
	//	gr: if you ask for 0 bytes in the buffer, we'll get 0 result, which we assume is "gracefully closed"
	if ( !Soy::Assert( Buffer.GetDataSize() > 0, "Should ask for more than 0 bytes or we can get confused" ) )
		Buffer.SetSize( 1024 );
		
	int Flags = 0;
	auto Result = recv( mSocket, Buffer.GetArray(), size_cast<socket_data_size_t>(Buffer.GetDataSize()), Flags );
	/*
		sockaddr FromAddr;
		int FromAddrLen = sizeof(FromAddr);
		auto Result = recvfrom(ClientSocket.mSocket, Buffer.GetArray(), Buffer.GetDataSize(), Flags, &FromAddr, &FromAddrLen);
	 */
	if ( Result == 0 )
	{
		//	socket closed gracefully
		//	gr: sometimes chrome seems to close the socket, but websocket is still waiting for it to connect... try forcing it to disconnect rather than assume its disconnected...
		//		maybe an issue with recv???
		//	gr: just in case it's not graceful...
		auto SockError = Soy::Winsock::GetError();
		std::stringstream ErrorString;
		Soy::Winsock::HasError("recv",false,SockError,&ErrorString);
		if ( SockError == 0 && ErrorString.str().empty() )
		{
			//	graceful close, don't throw
			return false;
		}
		
		throw Soy::AssertException( ErrorString.str() );
		//mParent.Disconnect( mClientRef, ErrorString.str() );
	}
	else if ( Result == SOCKET_ERROR )
	{
		if ( Soy::Winsock::HasError("recv",false) )
		{
			throw Soy::AssertException("recv socket error");
		}
	
		//	error, must be non blocking, nowt to recieve so sleep a bit longer than normal
		Buffer.SetSize(0);
		return true;
	}
	
	//	resize buffer to reflect real contents size
	Buffer.SetSize( Result );
	return true;
}


void SoySocketConnection::Send(ArrayBridge<char>&& Buffer,bool IsUdp)
{
	auto& Out = Buffer;
	
	//	gr: this code suggests, it HAS happened? or could happen.
	static int FailMax = 10;
	int FailCount = 0;

	//	todo: change this to an offset rather than re-allocating
	while ( !Out.IsEmpty() )
	{
		int Flags = 0;
		ssize_t Result = 0;
		
		if ( IsUdp )
		{
			Result = ::sendto( mSocket, Out.GetArray(), size_cast<socket_data_size_t>(Out.GetDataSize()), Flags, mAddr.GetSockAddr(), mAddr.GetSockAddrLength() );
		}
		else
		{
			Result = ::send( mSocket, Out.GetArray(), size_cast<socket_data_size_t>(Out.GetDataSize()), Flags );
		}
		
		//	gr; not sure when this might happen, but don't get stuck in a loop
		if ( Result == 0 )
		{
			FailCount++;
			if ( FailCount > FailMax )
			{
				std::stringstream Error;
				Error << "Socket send() failed too many (" << FailCount << ") times";
				throw Soy::AssertException( Error.str() );
			}
			continue;
		}
		//	error
		if ( Result == SOCKET_ERROR )
		{
			std::stringstream SocketError;
			if ( Soy::Winsock::HasError("send",false,Soy::Winsock::GetError(),&SocketError) )
				throw Soy::AssertException( SocketError.str() );

			continue;
		}
		
		auto DataToRemove = size_cast<size_t>( Result );
		
		//	pop the data that's been sent
		Out.RemoveBlock( 0, DataToRemove );
	}
}


