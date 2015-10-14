#include "SoyMulticast.h"
#import <Foundation/Foundation.h>
#include <SoyTypes.h>
#include <SoyString.h>


//	http://stackoverflow.com/questions/10101935/how-to-find-the-ip-address-of-the-active-airplay-device

@class TMulticastDelegate;

@interface TMulticastDelegate : NSObject <NSNetServiceBrowserDelegate>
{
	Platform::TMulticaster*	mParent;
}

-(id)initWithParent:(Platform::TMulticaster*)parent;

-(void)_didFindService:(id)service moreComing:(BOOL)coming;

@end



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

-(void)_didFindService:(id)service moreComing:(BOOL)coming
{
	NSLog(@"Service Found. Attempting to resolve: %@ (%d more to come)", service, coming);
	[super _didFindService:service moreComing:coming];
	[[service retain] setDelegate:self];
	[service resolveWithTimeout:0.0f];
}

@end





class Platform::TMulticaster
{
public:
	ObjcPtr<NSNetServiceBrowser>	mBrowser;
	ObjcPtr<TMulticastDelegate>	mDelgate;
};



SoyMulticaster::SoyMulticaster(const std::string& Protocol,const std::string& Domain) :
	mProtocol	( Protocol ),
	mDomain		( Domain ),
	mInternal	( new Platform::TMulticaster )
{
	//	alloc objects
	auto& mBrowser = mInternal->mBrowser;
	auto& mDelgate = mInternal->mDelgate;
	
	mDelgate.Retain( [[TMulticastDelegate alloc]initWithParent:mInternal.get() ] );
	
	auto DomainNs = Soy::StringToNSString(Domain);
	mBrowser.Retain( [[NSNetServiceBrowser alloc] init] );
	mBrowser.mObject.delegate = mDelgate.mObject;
	[mBrowser.mObject searchForServicesOfType:@"_pdl-datastream._tcp" inDomain:DomainNs];

}









