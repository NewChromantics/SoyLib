#include "SoySocket.h"
#include "SoyDebug.h"
#include <regex>
#include "HeapArray.hpp"

#if defined(TARGET_POSIX)
#error TARGET_POSIX should not be defined any more
#endif


#if defined(TARGET_PS4)

#include <netinet\in.h>
#define SOMAXCONN	0
#define close sceKernelClose
in_addr_t inet_addr(const char*)
{
	return 0; 
}

#elif defined(TARGET_WINDOWS)

#include <signal.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#else

#include <fcntl.h>	//	fcntl
#include <unistd.h>	//	close
#include <netdb.h>	//	gethostbyname
#include <signal.h>
#include <ifaddrs.h>	//	getifaddrs

#endif



#define PORT_ANY	0

bool Soy::Winsock::HasError(std::stringstream&& ErrorContext, bool BlockIsError,int Error,std::ostream* ErrorStream)
{
	return HasError(ErrorContext.str(), BlockIsError, Error, ErrorStream );
};



SoySockAddr::SoySockAddr(const std::string& Hostname,const uint16 Port)
{
#if defined(TARGET_PS4)
	throw Soy::AssertException("SoySockAddr not implemented");
#else
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
#endif
}

SoySockAddr::SoySockAddr(struct addrinfo& AddrInfo)
{
#if defined(TARGET_PS4)
	throw Soy::AssertException("SoySockAddr not implemented");
#else
	//	copy
	memcpy( &mAddr, AddrInfo.ai_addr, AddrInfo.ai_addrlen );
#endif
}

SoySockAddr::SoySockAddr(const sockaddr& Addr,socklen_t AddrLen)
{
	//	check length. field not present in winsock or linux (android)
#if defined(TARGET_OSX)||defined(TARGET_IOS)
	auto ExpectedLength = Addr.sa_len;
#elif defined(TARGET_ANDROID) || defined(TARGET_LINUX)
	//	gr: on jetson linux(amd64) AddrLen is typically 128 (same as sockaddr_storage)
	auto ExpectedLength = __SOCK_SIZE__;
#elif defined(TARGET_WINDOWS)
	auto ExpectedLength = sizeof(sockaddr_storage);
#elif defined(TARGET_PS4)
	auto ExpectedLength = sizeof(sockaddr); 
	//	_SS_SIZE = size of sockaddr_storage
#endif

	//	gr: on windows, accepting unity WWW connection was 16 bytes...
	//	on windows, the address from probing addaptors is 128
	//	gr: ^^ same on linux
	if ( AddrLen != ExpectedLength )
	{
		std::stringstream err;
		err << "sockaddr length (" << ExpectedLength << ") doesn't match incoming address " << AddrLen << " (Using smallest)";
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
		//std::Debug << err.str() << std::endl;
#else
		throw Soy::AssertException( err.str() );
#endif
	}

	if ( AddrLen > sizeof(mAddr) )
	{
		std::stringstream err;
		err << "sockaddr length (" << AddrLen << ") too big for storage " << sizeof(mAddr);
		throw Soy::AssertException( err.str() );
	}

	//	safety first!
	AddrLen = std::min<socklen_t>( sizeof(mAddr), AddrLen);
	memcpy( &mAddr, &Addr, AddrLen );
}


SoySockAddr::SoySockAddr(in_addr_t AddressIp4,uint16 Port)
{
#if defined(TARGET_PS4)
	throw Soy::AssertException("Not implemented");
#else
	auto& Addr4 = *reinterpret_cast<sockaddr_in*>(&mAddr);

	//	gr: find a proper function for this
	Addr4.sin_family = AF_INET;
	Addr4.sin_port = htons(Port);
#if defined(TARGET_OSX)||defined(TARGET_IOS)
	Addr4.sin_len = sizeof(sockaddr_in);
#endif
	Addr4.sin_addr.s_addr = AddressIp4;
#endif
}


const sockaddr* SoySockAddr::GetSockAddr() const
{
	return reinterpret_cast<const sockaddr*>( &mAddr );
}

sockaddr* SoySockAddr::GetSockAddr()
{
	return reinterpret_cast<sockaddr*>( &mAddr );
}

void SoySockAddr::SetPort(uint16 Port)
{
	//	cast for ipv4
	//	https://stackoverflow.com/a/12811907/355753
	if ( this->mAddr.ss_family == AF_INET )
	{
		auto* sin = (struct sockaddr_in*)&mAddr;
		sin->sin_port = Port;
		return;
	}
	
	throw Soy::AssertException("Don't know how to set port for non IPV4 address");
}

uint16_t SoySockAddr::GetPort() const
{
	//	cast for ipv4
	//	https://stackoverflow.com/a/12811907/355753
	if ( this->mAddr.ss_family == AF_INET )
	{
		auto* sin = (struct sockaddr_in*)&mAddr;
		return sin->sin_port;
	}
	
	throw Soy::AssertException("Don't know how to get port for non IPV4 address");
}



socklen_t SoySockAddr::GetSockAddrLength() const
{
	auto* SockAddrIn = GetSockAddr();
	
	if ( SockAddrIn->sa_family == AF_INET )
	{
#if defined(TARGET_PS4)
		return sizeof(sockaddr);
#else
		return sizeof(sockaddr_in);
#endif
	}

	if ( SockAddrIn->sa_family == AF_INET6 )
	{
#if defined(TARGET_PS4)
		throw Soy::AssertException("IPv6 not supported on ps4");
#else
		return sizeof(sockaddr_in6);
#endif
	}

	//	uninitialised (empty struct), return our struct size
	if ( SockAddrIn->sa_family == 0 )
	{
		return sizeof(mAddr);
	}
	
	return 0;
}

bool SoySockAddr::operator==(const SoySockAddr& That) const
{
	auto& ThisAddr = mAddr;
	auto& ThatAddr = That.mAddr;
	
#if defined(TARGET_WINDOWS)
	Soy::AssertException("Test this!");
	//	gr: need to check this
	auto ThisLength = this->GetSockAddrLength();
	auto ThatLength = That.GetSockAddrLength();
#elif defined(TARGET_LINUX) || defined(TARGET_ANDROID)
	//	this is sockaddr_storage size, but may not be the socket size...
	auto ThisLength = _SS_SIZE;
	auto ThatLength = _SS_SIZE;
#else
	auto ThisLength = ThisAddr.ss_len;
	auto ThatLength = ThatAddr.ss_len;
#endif	
	if ( ThisLength != ThatLength )
		return false;

	auto Compare = memcmp( &ThisAddr, &ThatAddr, ThisLength );
	return Compare == 0;
}

std::ostream& operator<< (std::ostream &out,const SoySockAddr &Addr)
{
#if defined(TARGET_PS4)
	out << "<some ip>";
#else
	BufferArray<char,100> Host(100);
	BufferArray<char,100> Port(100);
	int Flags = 0;
	Flags |= NI_NUMERICSERV;	//	also show "service" (port) as a number
	Flags |= NI_NUMERICHOST;

	auto Error = getnameinfo( Addr.GetSockAddr(), Addr.GetSockAddrLength(), Host.GetArray(), size_cast<socklen_t>(Host.GetDataSize()), Port.GetArray(), size_cast<socklen_t>(Port.GetDataSize()), Flags );
	if ( Error != 0 )
	{
		out << "[failed to get name for sockaddr getnameinfo " << ::Platform::GetErrorString(Soy::Winsock::GetError()) << "]";
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
#endif
	return out;
}


std::ostream& operator<< (std::ostream &out,const SoySocketConnection &in)
{
	out << in.mAddr;
	return out;
}


void Soy::Winsock::Init()
{
#if defined(TARGET_WINDOWS)
	WORD wVersionRequested = MAKEWORD(2,2);
	WSADATA wsaData;

	auto Error = WSAStartup(wVersionRequested, &wsaData);
	if ( Error != 0 )
	{
		std::stringstream Error;
		Error << "Failed to initialise Winsock. " << ::Platform::GetLastErrorString();
		throw Soy::AssertException(Error.str());
	}
#endif
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
#else
	return ::Platform::GetLastError();
#endif
}


#if defined(TARGET_WINDOWS)
	#define SOCKERROR(e)	(WSAE ## e)
	#define SOCKERROR_SUCCESS	ERROR_SUCCESS
#else
	#define SOCKERROR(e)		(E ## e)
	#define SOCKERROR_SUCCESS	0
	#define SOCKET_ERROR		-1
#endif

#if !defined(SOCKERROR)
#error SOCKERROR Macro not defined
#endif


bool Soy::Winsock::HasError(const std::string& ErrorContext,bool BlockIsError,int Error,std::ostream* ErrorStream)
{
	std::string ErrorString = ::Platform::GetErrorString( Error );

	switch ( Error )
	{
	case SOCKERROR_SUCCESS:
		return false;

	case SOCKERROR(WOULDBLOCK):
		if ( !BlockIsError )
			return false;
		break;
	};

	//	if we fall through here, it's an error
	//	gr: the . suffix is required here. If we output "Permission denied/n" in the unit
	//		test post-build it "detects" an error, even if a test didn't fail. The dot stops this "clever detection"
	std::Debug << "Winsock error (" << Error << "): " << ErrorString << ". " << ErrorContext << std::endl;
	
	if ( ErrorStream )
		*ErrorStream << "Winsock error (" << Error << "): " << ErrorString << ". " << ErrorContext;

	return true;
}


void SoySocket::Close()
{
	{
		std::lock_guard<std::recursive_mutex> Lock( mConnectionLock );
		if ( mSocket != INVALID_SOCKET )
		{
	#if defined(TARGET_WINDOWS)
			closesocket( mSocket );
	#else
			close( mSocket );
	#endif
			Soy::Winsock::HasError("CloseSocket");
			mSocket = INVALID_SOCKET;
			mSocketAddr = SoySockAddr();
		}
	}
	
	//	disconnect all our clients/connections
	while ( true )
	{
		std::lock_guard<std::recursive_mutex> Lock( mConnectionLock );
		if ( mConnections.empty() )
			break;
		auto it = mConnections.begin();
		Disconnect( it->first, "Socket Close" );
	}
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


void SoySocket::CreateTcp(bool Blocking)
{
	//	already created
	if ( IsCreated() && !IsUdp() )
		return;

	if ( IsCreated() )
		Close();
	
	Soy::Winsock::Init();
	
	mConnectionLock.lock();
	mSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
	if ( mSocket == INVALID_SOCKET )
	{
		Soy::Winsock::HasError("Create socket");
		mConnectionLock.unlock();
		throw Soy::AssertException("Failed to create socket");
	}
	//	gr: don't have an ip/port until bound?
	mSocketAddr = SoySockAddr();

	bool Success = false;
#if !defined(TARGET_WINDOWS)
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
#if !defined(TARGET_ANDROID) && !defined(TARGET_PS4) && !defined(TARGET_LINUX)
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
		std::stringstream Error;
		Error << "Could not make socket " << (Blocking?"":"non-") << "blocking";
		Soy::Winsock::HasError( Soy::StreamToString( std::stringstream()<< "make socket " << (Blocking?"":"non-") << "blocking" ) );
		mConnectionLock.unlock();
		Close();
		throw Soy::AssertException(Error.str());
	}

	mConnectionLock.unlock();
}



void SoySocket::CreateUdp(bool Broadcast)
{
	//	already created
	if ( IsCreated() && IsUdp() )
		return;
	
	Soy::Winsock::Init();
	
	mConnectionLock.lock();
	mSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( mSocket == INVALID_SOCKET )
	{
		Soy::Winsock::HasError("Create socket");
		mConnectionLock.unlock();
		throw Soy::AssertException("Failed to create socket");
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
		
		Soy::Winsock::HasError( Dbg.str() );
		mConnectionLock.unlock();
		Close();
		throw Soy::AssertException(Dbg.str());
	}

	mConnectionLock.unlock();
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
		//	the << on sockaddr clears the error so grab it now
		auto Error = Soy::Winsock::GetError();
		//	get error when socket has been closed
		Soy::Winsock::HasError( Soy::StreamToString(std::stringstream() << "Accept(" << Client << ")" ), false, Error );
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

void SoySocket::Bind(uint16 Port,SoySockAddr& outSockAddr)
{
	outSockAddr = SoySockAddr( INADDR_ANY, Port );
	
	if ( ::bind( mSocket, outSockAddr.GetSockAddr(), outSockAddr.GetSockAddrLength() ) != SOCKERROR_SUCCESS )
	{
		int Error = Soy::Winsock::GetError();
		std::stringstream ErrorStr;
		ErrorStr << "Failed to bind to " << Port;
		Soy::Winsock::HasError("", true, Error, &ErrorStr);
#if !defined(TARGET_WINDOWS)
		if ( Error == EACCES )
			ErrorStr << "Access denied binding to port " << Port << " only root user on OSX can have a port < 1024";
#endif
		Close();
		throw Soy::AssertException(ErrorStr.str());
	}
	
	//	find our socket (to get our port)
	if ( Port == PORT_ANY )
	{
		sockaddr SockAddr;
		socklen_t SockAddrLen = sizeof(SockAddr);
		auto Error = ::getsockname( mSocket, &SockAddr, &SockAddrLen );
		if ( Error != SOCKERROR_SUCCESS )
		{
			Error = Soy::Winsock::GetError();
			std::stringstream ErrorStr;
			ErrorStr << "Bind to " << Port << " getsockname()";
			Soy::Winsock::HasError("", true, Error, &ErrorStr );
			//throw Soy::AssertException(ErrorStr.str());
			std::Debug << ErrorStr.str() << std::endl;
		}
		else
		{
			//	save new address with corrected port
			SoySockAddr ResolvedAddr( SockAddr, SockAddrLen );
			std::Debug << "Resolved " << outSockAddr << " to " << ResolvedAddr << std::endl;
			outSockAddr = ResolvedAddr;
		}
	}
	
}

void SoySocket::ListenTcp(int Port)
{
	SoySockAddr SockAddr;
	Bind(Port, SockAddr);

	auto MaxConnections = SOMAXCONN;
	if ( ::listen(mSocket, MaxConnections) == SOCKET_ERROR)
	{
		std::stringstream Error;
		Error << "Failed to listen on port " << Port;
		Soy::Winsock::HasError(Error.str());
		Close();
		throw Soy::AssertException(Error.str());
	}

	mSocketAddr = SockAddr;

	
	std::Debug << "Socket bound on " << mSocketAddr << ", on interfaces ";
	auto DebugAddresses = [](std::string& InterfaceName,SoySockAddr& InterfaceAddr)
	{
		std::Debug << InterfaceName << ": " << InterfaceAddr << ", ";
	};
	this->GetSocketAddresses(DebugAddresses);
	std::Debug << std::endl;
}


void SoySocket::ListenUdp(int Port,bool SaveListeningConnection)
{
	Bind(Port, mSocketAddr);
	
	//	udp just binds
	std::Debug << "Socket UDP bound on " << mSocketAddr << std::endl;
	
	//	udp needs a socket to recieve from for any new clients
	//	gr: if we try and send to this peer, we get an error as it's our general listening address
	//		so if we're binding in order to send, don't add it
	if (SaveListeningConnection)
	{
		//	gr: maybe add to WaitForClient?
		SoySocketConnection PossibleClient;
		PossibleClient.mAddr = mSocketAddr;
		PossibleClient.mSocket = this->mSocket;
		OnConnection(PossibleClient);
	}
		
	std::Debug << "Socket bound on " << mSocketAddr << ", on interfaces ";
	auto DebugAddresses = [](std::string& InterfaceName,SoySockAddr& InterfaceAddr)
	{
		std::Debug << InterfaceName << ": " << InterfaceAddr << ", ";
	};
	this->GetSocketAddresses(DebugAddresses);
	std::Debug << std::endl;

}


bool SoySocket::IsConnected()
{
	bool Connected = !mConnections.empty();
	return Connected;
}

SoyRef SoySocket::Connect(const char* Hostname,uint16_t Port)
{
	if (mSocket == INVALID_SOCKET)
		throw Soy::AssertException("TCP Connect without creating socket first");

	SoySockAddr HostAddr( Hostname, Port );
	if ( !HostAddr.IsValid() )
	{
		std::stringstream Error;
		Error << "couldn't get sock address for " << Hostname;
		throw Soy::AssertException(Error);
	}

	//	gr: no connection lock here as this is blocking
	
	SoySocketConnection Connection;
	Connection.mSocket = mSocket;
	Connection.mAddr = HostAddr;
	
	//	try and connect
	std::Debug << "Connecting to " << Hostname << ":" << Port << "..." << std::endl;
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
			std::stringstream ErrorStr;
			ErrorStr << "connect(" << Hostname << ":" << Port << ") error: " << Platform::GetErrorString( Error );
			throw Soy::AssertException(ErrorStr);
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
			throw Soy::AssertException("Timed out waiting for connection (select)");
		}
		
		if (ReadyFileDescriptorCount != 1 )
		{
			std::stringstream Exception;
			Exception << "unexpected return from select() -> " << ReadyFileDescriptorCount << " (expected 1)";
			//	gr: what to do with socket if it's valid?
			Close();
			throw Soy::AssertException(Exception);
		}

		//	todo: double check state of socket here?
	}
	
	return OnConnection( Connection );
}

SoyRef SoySocket::UdpConnect(const char* Hostname,uint16 Port)
{
	SoySockAddr HostAddr( Hostname, Port );
	if ( !HostAddr.IsValid() )
	{
		std::stringstream Error;
		Error << "couldn't get sock address for " << Hostname;
		throw Soy::AssertException(Error);
	}
	
	return UdpConnect( HostAddr );
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
	try
	{
		ListenUdp(PORT_ANY,false);
		mConnectionLock.unlock();
		return OnConnection( Connection );
	}
	catch(...)
	{
		mConnectionLock.unlock();
		throw;
	}
}

SoyRef SoySocket::OnConnection(SoySocketConnection Connection)
{
	mConnectionLock.lock();
	
	SoyRef ConnectionRef = AllocConnectionRef();
	mConnections[ConnectionRef] = Connection;
	std::Debug << "New connection (" << ConnectionRef << ") established to " << Connection << std::endl;

	mConnectionLock.unlock();

	if ( mOnConnect != nullptr )
		mOnConnect( ConnectionRef );
	return ConnectionRef;
}


SoyRef SoySocket::GetConnectionRef(const SoySockAddr& SockAddr)
{
	//	if this doesn't exist, we allocate it
	{
		std::lock_guard<std::recursive_mutex> Lock( mConnectionLock );
		for ( auto& ConnectionElement : mConnections )
		{
			auto& ConnectionRef = ConnectionElement.first;
			auto& Connection = ConnectionElement.second;
			if ( Connection.mAddr == SockAddr )
				return ConnectionRef;
		}
	}
	
	//	new connection
	//	gr: for UDP, we need OUR socket here with the new address so when we grab it, we'll send over our own socket...
	//		maybe we need something more dynamic
	SoySocketConnection NewConnection;
	NewConnection.mAddr = SockAddr;
	NewConnection.mSocket = mSocket;
	return OnConnection( NewConnection );
}

void SoySocket::OnError(SoyRef ConnectionRef,const std::string& Reason)
{
	Disconnect( ConnectionRef, Reason );
}

void SoySocket::Disconnect(SoyRef ConnectionRef,const std::string& Reason)
{
	SoySocketConnection Connection;
	{
		std::lock_guard<std::recursive_mutex> Lock(mConnectionLock);

		auto Existing = mConnections.find(ConnectionRef);
		if (Existing == mConnections.end())
		{
			//	missing, already deleted?
			return;
		}

		Connection = Existing->second;

		bool ClosingSelf = (mSocket == Connection.mSocket);
		std::Debug << "Disconnect " << (ClosingSelf ? "self" : "client") << ConnectionRef << "(" << Connection << ") reason: " << Reason << std::endl;

		//	remove from list before callback? or after, so list is accurate for callbacks
		mConnections.erase(Existing);
	}

	//	do callback in case we want to try a hail mary
	//	gr: the old Disconnect() had this AFTER closing socket, OnError did NOT
	if ( mOnDisconnect != nullptr )
		mOnDisconnect(ConnectionRef,Reason);

	//	if the connection's socket is ourselves, (we are a client connecting to a server) close
	bool ClosingSelf = (mSocket == Connection.mSocket);
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
	#else
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
	std::lock_guard<std::recursive_mutex> Lock(mConnectionLock);
	
	//	accessing the map will create a new key, so look for it first
	auto Find = mConnections.find( ConnectionRef );
	if (Find == mConnections.end())
	{
		std::stringstream Error;
		Error << "No such connection ref " << ConnectionRef;
		throw Soy::AssertException(Error);
	}
	auto Connection = Find->second;
	
	return Connection;
}


void SoySocket::EnumConnections(std::function<void(SoyRef,SoySocketConnection)> EnumConnection)
{
	//	grab all the keys
	Array<SoyRef> ConnectionRefs;
	{
		std::lock_guard<std::recursive_mutex> Lock(mConnectionLock);
		for(auto const& Connection : mConnections)
			ConnectionRefs.PushBack(Connection.first);
	}
	
	//	run through them all, locking in-between
	for ( int i=0;	i<ConnectionRefs.GetSize();	i++ )
	{
		//	catch?
		auto ConnectionRef = ConnectionRefs[i];
		auto Connection = GetConnection(ConnectionRef);
		EnumConnection( ConnectionRef, Connection );
	}
}



void SoySocket::GetSocketAddresses(std::function<void(std::string& Name,SoySockAddr&)> EnumAdress) const
{
	//	https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c
	
	auto FamilyFilter = this->mSocketAddr.mAddr.ss_family;

	//	override port with our port
	auto Port = PORT_ANY;
	try
	{
		Port = mSocketAddr.GetPort();
	}
	catch(std::exception& e)
	{
		std::Debug << e.what() << std::endl;
	}

#if defined(TARGET_WINDOWS)
	//	ERROR_ADDRESS_NOT_ASSOCIATED
	//	ERROR_BUFFER_OVERFLOW
	//	ERROR_INVALID_PARAMETER
	//	ERROR_NOT_ENOUGH_MEMORY
	//	ERROR_NO_DATA
	//	example from https://docs.microsoft.com/en-us/windows/desktop/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
#define WORKING_BUFFER_SIZE 15000

	//	work out data size we need
	ULONG BufferSize = 0;
	auto Family = FamilyFilter;
	ULONG Flags = GAA_FLAG_INCLUDE_PREFIX;	//	see GAA_FLAG_SKIP_UNICAST etc
	void* Reserved = nullptr;
	while(true)
	{
		Array<uint8_t> Buffer( size_cast<size_t>(BufferSize) );
		auto* pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(Buffer.GetArray());
		auto Result = GetAdaptersAddresses(Family, Flags, Reserved, pAddresses, &BufferSize);
		//	buffersize now okay
		if ( Result == S_OK )
			break;
		//	try again with new size
		if ( Result == ERROR_BUFFER_OVERFLOW )
			continue;
		Platform::IsOkay( static_cast<int>(Result),"GetAdaptersAddresses");
		throw Soy::AssertException("Failed to get Adaptor addresses buffer size");
	}
	
	//	now grab data and iterate
	Array<uint8_t> AddressesBuffer( size_cast<size_t>(BufferSize) );
	auto* pAdaptorAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(AddressesBuffer.GetArray());
	auto Result = GetAdaptersAddresses(Family, Flags, Reserved, pAdaptorAddresses, &BufferSize);
	Platform::IsOkay( static_cast<int>(Result),"GetAdaptersAddresses");

	while ( pAdaptorAddresses )
	{
		auto& Adaptor = *pAdaptorAddresses;

		//	{XXXXX} guid
		auto AdaptorName = std::string(Adaptor.AdapterName);
		//	"Intel xyz ethernet adaptor"
		auto Description = Soy::WStringToString(Adaptor.Description);
		//	"Ethernet"
		auto FriendlyName = Soy::WStringToString(Adaptor.FriendlyName);
	
		//	gr: ip/sockaddr is the UnicastAddr (which is a link list, hence "first")
		//	https://social.msdn.microsoft.com/Forums/windows/en-US/3b6a92ac-93d3-4f59-a914-340c1ba41cff/how-to-retrieve-ip-addresses-of-the-network-cards-with-getadaptersaddresses?forum=windowssdk
		auto* UnicastAddr = Adaptor.FirstUnicastAddress;
		while ( UnicastAddr )
		{
			auto& AddrMeta = *UnicastAddr;
			auto& SockAddr = *AddrMeta.Address.lpSockaddr;
			auto SockAddrLen = AddrMeta.Address.iSockaddrLength;
			SoySockAddr Addr(SockAddr, SockAddrLen);
			if ( Port != PORT_ANY )
				Addr.SetPort(Port);

			EnumAdress(Description, Addr);

			UnicastAddr = UnicastAddr->Next;
		}
		pAdaptorAddresses = pAdaptorAddresses->Next;
	}

#else
	struct ifaddrs* Interfaces = nullptr;
	auto Success = getifaddrs( &Interfaces );
	if ( Success != 0 )
	{
		Soy::Winsock::HasError("getifaddrs()");
		return;
	}
	
	
	for ( auto* ifa = Interfaces; ifa; ifa = ifa->ifa_next)
	{
		auto& Interface = *ifa;
		if (Interface.ifa_addr->sa_family!=FamilyFilter)
			continue;
		
		try
		{
#if defined(TARGET_LINUX)
			socklen_t AddrLen = __SOCK_SIZE__;
#else
			socklen_t AddrLen = Interface.ifa_addr->sa_len;
#endif
			SoySockAddr Addr( *Interface.ifa_addr, AddrLen );
			if ( Port != PORT_ANY )
				Addr.SetPort( Port );
			
			std::string InterfaceName( Interface.ifa_name );
			EnumAdress( InterfaceName, Addr );
		}
		catch(std::exception& e)
		{
			std::Debug << "Error fetching interface address " << e.what() << std::endl;
		}
	}

	freeifaddrs(Interfaces);
#endif
}

bool SoySocketConnection::Recieve(ArrayBridge<char>&& Buffer)
{
	auto Sender = Recieve( Buffer, nullptr );
	return Sender.IsValid();
}

SoyRef SoySocketConnection::Recieve(ArrayBridge<char>&& Buffer,SoySocket& Parent)
{
	return Recieve( Buffer, &Parent );
}

SoyRef SoySocketConnection::Recieve(ArrayBridge<char>& Buffer,SoySocket* Parent)
{
	//	gr: don't try and recieve from an invalid socket.
	//		example case: dummy UDP client (who has no socket, just Ref to match a SockAddr)
	//		doesn't throw, just pretends is disconnected.
	if ( mSocket == INVALID_SOCKET )
		return SoyRef();
	
	//	gr: if you ask for 0 bytes in the buffer, we'll get 0 result, which we assume is "gracefully closed"
	if ( Buffer.IsEmpty() )
		Buffer.SetSize( 1024 );
	
	int Flags = 0;
	//auto Result = recv( mSocket, Buffer.GetArray(), size_cast<socket_data_size_t>(Buffer.GetDataSize()), Flags );
	SoySockAddr FromAddr;
	socklen_t FromAddrLen = FromAddr.GetSockAddrLength();
	auto Result = recvfrom( mSocket, Buffer.GetArray(), size_cast<socket_data_size_t>(Buffer.GetDataSize()), Flags, FromAddr.GetSockAddr(), &FromAddrLen);
	
	if ( Result == 0 )
	{
		//	socket closed gracefully
		//	gr: sometimes chrome seems to close the socket, but websocket is still waiting for it to connect... try forcing it to disconnect rather than assume its disconnected...
		//		maybe an issue with recv???
		//	no error to check!
		//	http://stackoverflow.com/questions/30257722/what-is-socket-accept-error-errno-316
		//	graceful close, don't throw
		return SoyRef();
	}
	else if ( Result == SOCKET_ERROR )
	{
		if ( Soy::Winsock::HasError("recv",false) )
		{
			std::stringstream Error;
			Error << "recv socket error.";
			Error << "SOCKET=" << (int)mSocket;
			throw Soy::AssertException( Error.str() );
		}
	
		//	error, must be non blocking, nowt to recieve so sleep a bit longer than normal
		Buffer.SetSize(0);
		return SoyRef();
	}
	
	//	on osx, using TCP server, this socket is client.
	//	recv() okay, this is zero, the from address should be us
	if ( FromAddrLen != 0 )
	{
		//	re-create in case length is different
		FromAddr = SoySockAddr( *FromAddr.GetSockAddr(), FromAddrLen );
	}
	else
	{
		FromAddr = this->mAddr;
	}
	
	//	if no parent provided, we can't return a valid ref!
	auto SenderRef = SoyRef("Unknown");
	if ( Parent )
	{
		SenderRef = Parent->GetConnectionRef( FromAddr );
	}
	
	//	resize buffer to reflect real contents size
	Buffer.SetSize( Result );
	return SenderRef;
}


void SoySocketConnection::Send(const ArrayBridge<char>& Data,bool IsUdp)
{
	//	gr: this code suggests, it HAS happened? or could happen.
	static int FailMax = 10;
	int FailCount = 0;
	
	auto BytesWritten = 0;
	
	//	todo: change this to an offset rather than re-allocating
	while ( BytesWritten < Data.GetDataSize() )
	{
		int Flags = 0;
		ssize_t Result = 0;
		
		//	cast away const so we can get an array we'll use readonly (don't really have a const FixedArray type)
		//	this is a tiny bit expensive, but stack alloc and safe!
		auto& MutableData = *const_cast<ArrayBridge<char>*>( &Data );
		/*
		auto* SendStart = MutableData.GetArray() + BytesWritten;
		size_t SendLength = MutableData.GetDataSize() - BytesWritten;
		//FixedRemoteArray<char> SendArray( SendStart, SendLength );
		Array<char> SendArray;
		for ( int i=0;	i<SendLength;	i++ )
			SendArray.PushBack(SendStart[i]);
		*/
		auto SendArray = MutableData.GetSubArray( BytesWritten );
		
		{
			auto DataSize = size_cast<socket_data_size_t>(SendArray.GetDataSize());
			auto* DataArray = SendArray.GetArray();
			
			if ( IsUdp )
			{
				//	UDP cannot send more than ~64kb
				//	find the proper limit
				if ( DataSize > 1033 )
					DataSize = 1033;
				
				Result = ::sendto( mSocket, DataArray, DataSize, Flags, mAddr.GetSockAddr(), mAddr.GetSockAddrLength() );
			}
			else
			{
				Result = ::send( mSocket, DataArray, DataSize, Flags );
			}
		}
		
		//	gr; not sure when this might happen, but don't get stuck in a loop
		if ( Result == 0 )
		{
			FailCount++;
			if ( FailCount > FailMax )
			{
				std::stringstream Error;
				Error << "Socket send(" << *this << ") failed too many (" << FailCount << ") times";
				throw Soy::AssertException( Error.str() );
			}
			continue;
		}
		//	error
		if ( Result == SOCKET_ERROR )
		{
			//	catch error in case any std stuff losees it
			auto Error = Soy::Winsock::GetError();
			
			std::stringstream SocketError;
			SocketError << "Send(" << *this << ", " << SendArray.GetDataSize() << "bytes)";
			if ( !Soy::Winsock::HasError("",false,Error,&SocketError) )
				SocketError << "Missing error(" << Error << ") but socket error, so failing anyway";
			throw Soy::AssertException( SocketError.str() );
		}
		
		auto DataToRemove = size_cast<size_t>( Result );
		BytesWritten += DataToRemove;
	}
}

