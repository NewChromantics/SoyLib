#include "SoyMulticast.h"
#import <Foundation/Foundation.h>
#include <SoyTypes.h>
#include <SoyString.h>
#include <SoyDebug.h>
#include "HeapArray.hpp"


class TService
{
public:
	TService(NSNetService* Service) :
		mService	( Service )
	{
	}
	
	TServiceMeta			GetMeta();
	
public:
	ObjcPtr<NSNetService>	mService;
};



//	http://stackoverflow.com/questions/10101935/how-to-find-the-ip-address-of-the-active-airplay-device

@class TMulticastDelegate;

@interface TMulticastDelegate : NSObject <NSNetServiceBrowserDelegate,NSNetServiceDelegate>
{
	Platform::TMulticaster*	mParent;
}

- (id)initWithParent:(Platform::TMulticaster*)parent;


@end



class Platform::TMulticaster
{
public:
	TMulticaster(SoyMulticaster& Parent) :
		mParent	( Parent )
	{
	}
	
	void							UpdateService(NSNetService* Service);
	void							OnError(const std::string& Error);
	void							EnumServices(ArrayBridge<TServiceMeta>& Metas);
	
public:
	SoyMulticaster&					mParent;
	ObjcPtr<NSNetServiceBrowser>	mBrowser;
	ObjcPtr<TMulticastDelegate>		mDelgate;
	std::mutex						mServicesLock;
	Array<std::shared_ptr<TService>>	mServices;
};



@implementation TMulticastDelegate

- (id)initWithParent:(Platform::TMulticaster*)parent
{
	self = [super init];
	if (self)
	{
		mParent = parent;
	}
	return self;
}

- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)aNetServiceBrowser
{
	std::Debug << __func__ << std::endl;
}

- (void)netServiceBrowserDidStopSearch:(NSNetServiceBrowser *)aNetServiceBrowser
{
	std::Debug << __func__ << std::endl;
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didNotSearch:(NSDictionary *)errorDict
{
	std::Debug << __func__ << std::endl;

	Array<std::pair<std::string,std::string>> Errors;
	//Soy::NSDictionaryToStrings( GetArrayBridge(Errors), errorDict );

	std::stringstream Error;
	if ( Errors.IsEmpty() )
		Error << "Unkown error";
	for ( int e=0;	e<Errors.GetSize();	e++ )
	{
		Error << Errors[e].first << "=" << Errors[e].second << ", ";
	}

	mParent->OnError( Error.str() );
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didFindDomain:(NSString *)domainString moreComing:(BOOL)moreComing
{
	std::Debug << __func__ << std::endl;
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didFindService:(NSNetService *)aNetService moreComing:(BOOL)moreComing
{
	//	set delegate so we get it's events
	aNetService.delegate = self;
	
	mParent->UpdateService( aNetService );
	
	//	resolve hostnames
	[aNetService resolveWithTimeout:0.0];
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didRemoveDomain:(NSString *)domainString moreComing:(BOOL)moreComing
{
	std::Debug << __func__ << std::endl;
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didRemoveService:(NSNetService *)aNetService moreComing:(BOOL)moreComing
{
	std::Debug << __func__ << aNetService.name << std::endl;
}


- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
	//	resolved
	std::Debug << __func__ << sender.name << std::endl;
}

- (void)netService:(NSNetService *)sender didNotResolve:(NSDictionary *)errorDict
{
	std::Debug << __func__ << sender.name << std::endl;
}

- (void)netServiceDidStop:(NSNetService *)sender
{
	std::Debug << __func__ << sender.name << std::endl;
}

- (void)netService:(NSNetService *)sender didUpdateTXTRecordData:(NSData *)data
{
	std::Debug << __func__ << sender.name << std::endl;
}



@end

void Platform::TMulticaster::EnumServices(ArrayBridge<TServiceMeta>& Metas)
{
	std::lock_guard<std::mutex> Lock(mServicesLock);
	
	//	find existing service
	for ( int i=0;	i<mServices.GetSize();	i++ )
	{
		auto& Service = *mServices[i];

		auto Meta = Service.GetMeta();
		Metas.PushBack( Meta );
	}
}

void Platform::TMulticaster::UpdateService(NSNetService* NewService)
{
	std::lock_guard<std::mutex> Lock(mServicesLock);

	//	find existing service
	for ( int i=0;	i<mServices.GetSize();	i++ )
	{
		auto& Service = *mServices[i];
		if ( Service.mService.mObject != NewService )
			continue;
		
		//	already exists
		return;
	}
	
	//	make new one
	std::shared_ptr<TService> Service( new TService(NewService) );
	mServices.PushBack( Service );
}

void Platform::TMulticaster::OnError(const std::string& Error)
{
	mParent.mOnError.OnTriggered( Error );
}




SoyMulticaster::SoyMulticaster(const std::string& Protocol,const std::string& Domain) :
	mProtocol	( Protocol ),
	mDomain		( Domain ),
	mInternal	( new Platform::TMulticaster(*this) )
{
	//	alloc objects
	auto& mBrowser = mInternal->mBrowser;
	auto& mDelgate = mInternal->mDelgate;
	
	mDelgate.Retain( [[TMulticastDelegate alloc]initWithParent:mInternal.get() ] );
	
	auto DomainNs = Soy::StringToNSString(Domain);
	auto ProtocolNs = Soy::StringToNSString(Protocol);
	mBrowser.Retain( [[NSNetServiceBrowser alloc] init] );
	mBrowser.mObject.delegate = mDelgate.mObject;
	[mBrowser.mObject searchForServicesOfType:ProtocolNs inDomain:DomainNs];

}

void SoyMulticaster::EnumServices(ArrayBridge<TServiceMeta>&& Metas)
{
	mInternal->EnumServices( Metas );
}



TServiceMeta TService::GetMeta()
{
	NSNetService* Service = mService.mObject;
	
	TServiceMeta Meta;
	
	Meta.mIncludesPeerToPeer = Service.includesPeerToPeer;
	Meta.mName = Soy::NSStringToString( Service.name );
	Meta.mType = Soy::NSStringToString( Service.type );
	Meta.mDomain = Soy::NSStringToString( Service.domain );
	Meta.mHostName = Soy::NSStringToString( Service.hostName );
	Meta.mPort = (Service.port);
	
	//	The addresses of the service. This is an NSArray of NSData instances, each of which contains a single struct sockaddr suitable for use with connect(2). In the event that no addresses are resolved for the service or the service has not yet been resolved, an empty NSArray is returned.
	//@property (readonly, copy) NSArray *addresses;
	return Meta;
}






