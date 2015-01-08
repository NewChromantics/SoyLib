#include "SoyVideoDevice.h"
#include <SoyString.h>
#include <SortArray.h>

bool TVideoDeviceMeta::operator==(const std::string& Serial) const
{
	if ( mSerial == Serial )
		return true;
	
	//	gr: allow loose match by name
	if ( Soy::StringContains( mName, Serial, false ) )
		return true;
	
	return false;
}


TVideoDevice::TVideoDevice(std::string Serial,std::stringstream& Error) :
mLastFrame		( (std::stringstream() << "frame_" << Serial).str(), true ),
mLastError		( "waiting for first frame" ),
mFrameCount		( 0 )
{
}

TVideoDevice::~TVideoDevice()
{
}

float TVideoDevice::GetFps() const
{
	uint64 TotalMs = mLastFrameTime.GetTime() - mFirstFrameTime.GetTime();
	if ( TotalMs == 0 )
		return 0.f;
	
	float TotalSecs = TotalMs / 1000.f;
	return mFrameCount / TotalSecs;
}

int TVideoDevice::GetFrameMs() const
{
	uint64 TotalMs = mLastFrameTime.GetTime() - mFirstFrameTime.GetTime();
	if ( TotalMs == 0 )
		return 0;
	uint64 AverageMs = TotalMs / mFrameCount;
	//	cast, this shouldn't be massive
	if ( !Soy::Assert( AverageMs < 0xffffffff, "very large avergage ms" ) )
		return -1;
	return static_cast<int>(AverageMs);
}

void TVideoDevice::ResetFrameCounter()
{
	mLastFrameTime = SoyTime();
	mFirstFrameTime = SoyTime();
	mFrameCount = 0;
}


void TVideoDevice::OnFailedFrame(const std::string &Error)
{
	mLastError = Error;
}

void TVideoDevice::OnNewFrame(const SoyPixelsImpl& Pixels,SoyTime Timecode)
{
	mLastError.clear();
	
	if ( !mLastFrame.mPixels.Copy( Pixels ) )
	{
		mLastError = "Failed to copy pixels";
		return;
	}
	
	//	gr: might want to reject earlier timecodes here
	mLastFrame.mTimecode = Timecode;
	
	//	update frame/rate counting
	mFrameCount++;
	mLastFrameTime = SoyTime(true);
	if ( !mFirstFrameTime.IsValid() )
		mFirstFrameTime = mLastFrameTime;
	
	mOnNewFrame.OnTriggered( *this );
}





SoyVideoCapture::SoyVideoCapture()
{
}

SoyVideoCapture::~SoyVideoCapture()
{
	//	kill all devices
	while ( !mDevices.IsEmpty() )
	{
		auto Device = mDevices.PopBack();
		Device.reset();
	}
}


void SoyVideoCapture::GetDevices(ArrayBridge<TVideoDeviceMeta>&& Metas)
{
	for ( int c=0;	c<mContainers.GetSize();	c++ )
	{
		auto& Container = *mContainers[c];
		Container.GetDevices( Metas );
	}
}



class TSortPolicy_BestVideoMeta : public TSortPolicy<TVideoDeviceMeta>
{
public:
	TSortPolicy_BestVideoMeta(const std::string& Serial)
	{
		mMatchSerial = Serial;
	}
	
	template<typename TYPEB>
	static int		Compare(const TVideoDeviceMeta& a,const TYPEB& b)
	{
		bool aExactSerial = a.mSerial == mMatchSerial;
		bool bExactSerial = b.mSerial == mMatchSerial;
		if ( aExactSerial && !bExactSerial )	return -1;
		if ( !aExactSerial && bExactSerial )	return 1;
		
		bool aExactName = a.mName == mMatchSerial;
		bool bExactName = b.mName == mMatchSerial;
		if ( aExactName && !bExactName )	return -1;
		if ( !aExactName && bExactName )	return 1;
		
		bool aSerialStartsWith = Soy::StringBeginsWith( a.mSerial, mMatchSerial, false );
		bool bSerialStartsWith = Soy::StringBeginsWith( b.mSerial, mMatchSerial, false );
		if ( aSerialStartsWith && !bSerialStartsWith )	return -1;
		if ( !aSerialStartsWith && bSerialStartsWith )	return 1;
		
		bool aNameStartsWith = Soy::StringBeginsWith( a.mName, mMatchSerial, false );
		bool bNameStartsWith = Soy::StringBeginsWith( b.mName, mMatchSerial, false );
		if ( aNameStartsWith && !bNameStartsWith )	return -1;
		if ( !aNameStartsWith && bNameStartsWith )	return 1;
		
		std::stringstream Error;
		Error << "Need some more sorting comparisons for [" << mMatchSerial << "] with [" << a.mSerial << "/" << a.mName << "] and [" << b.mSerial << "/" << b.mName << "]";
		Soy::Assert( false, Error );
		return 0;
	}
	
private:
	static std::string		mMatchSerial;	//	gr: need a better way of having variables in sort policies
};
std::string TSortPolicy_BestVideoMeta::mMatchSerial;


TVideoDeviceMeta SoyVideoCapture::GetDeviceMeta(std::string Serial)
{
	Array<TVideoDeviceMeta> Metas;
	GetDevices( GetArrayBridge(Metas) );
	return GetBestDeviceMeta( Serial, GetArrayBridge(Metas) );
}



int DoTest()
{
	TVideoDeviceMeta MetasDef[] =
	{
		TVideoDeviceMeta("123456789", "Camera" ),
		TVideoDeviceMeta("123456780", "Camera 2" ),
		TVideoDeviceMeta("cameraX", "Left Camera" ),
	};
	BufferArray<TVideoDeviceMeta,100> Metas;
	
	Metas = BufferArray<TVideoDeviceMeta,100>(MetasDef);
	TVideoDeviceMeta BestMetaA = SoyVideoCapture::GetBestDeviceMeta("Camera", GetArrayBridge(Metas) );
	Soy::Assert( BestMetaA.mSerial == MetasDef[0].mSerial, "Wrong match" );
	
	Metas = BufferArray<TVideoDeviceMeta,100>(MetasDef);
	TVideoDeviceMeta BestMetaB = SoyVideoCapture::GetBestDeviceMeta("Camera 2", GetArrayBridge(Metas) );
	Soy::Assert( BestMetaB.mSerial == MetasDef[1].mSerial, "Wrong match" );
	
	Metas = BufferArray<TVideoDeviceMeta,100>(MetasDef);
	TVideoDeviceMeta BestMetaC = SoyVideoCapture::GetBestDeviceMeta("left", GetArrayBridge(Metas) );
	Soy::Assert( BestMetaC.mSerial == MetasDef[2].mSerial, "Wrong match" );
	
	Metas = BufferArray<TVideoDeviceMeta,100>(MetasDef);
	TVideoDeviceMeta BestMetaD = SoyVideoCapture::GetBestDeviceMeta("123456789", GetArrayBridge(Metas) );
	Soy::Assert( BestMetaD.mSerial == MetasDef[0].mSerial, "wrong match" );
	
	return 0;
	
}
int gfgfdg = DoTest();



TVideoDeviceMeta SoyVideoCapture::GetBestDeviceMeta(std::string Serial,ArrayBridge<TVideoDeviceMeta>&& Metas)
{
	//	get all meta's first and filter until we find the best
	//	this way we can match serial, name, and odd names (eg. "Camera" and "Camera 2") properly
	//	"Cam" will find "Camera" and "Camera2" (so whichever is first), but "Camera" will find camera
	
	//	standard meta==string filter
	for ( int m=Metas.GetSize()-1;	m>=0;	m-- )
	{
		auto& Meta = Metas[m];
		if ( Meta == Serial )
			continue;
		Metas.RemoveBlock( m, 1 );
	}
	
	if ( Metas.IsEmpty() )
		return TVideoDeviceMeta();
	
	auto SortArray = GetSortArray( Metas, TSortPolicy_BestVideoMeta(Serial) );
	SortArray.Sort();
	
	Soy::Assert( SortArray[0] == Serial, "Error in Meta==String" );
	
	return SortArray[0];
}



std::shared_ptr<TVideoDevice> SoyVideoCapture::GetDevice(std::string Serial,std::stringstream& Error)
{
	//	see if device already exists
	for ( int i=0;	i<mDevices.GetSize();	i++ )
	{
		auto& Device = *mDevices[i];
		if ( Device == Serial )
			return mDevices[i];
	}
	
	//	in case we've provided the name, find the proper serial
	auto Meta = GetDeviceMeta( Serial );
	if ( !Meta.IsValid() )
	{
		Error << "Unknown device " << Serial << " (meta not found)";
		return nullptr;
	}
	
	//	create new device
	std::stringstream InitError;

	//	gr: meta should say which factory/container it's from instead of this double-search?
	std::shared_ptr<TVideoDevice> Device;
	for ( int c=0;	c<mContainers.GetSize();	c++ )
	{
		auto& Container = *mContainers[c];
		if ( !Container.HasDevice( Meta.mSerial ) )
			continue;
		Device = Container.AllocDevice( Meta.mSerial, InitError );
	}

	if ( !Device )
	{
		Error << "Known device " << Serial << ", but failed to allocate";
		return nullptr;
	}
	
	//	gr: require meta to be valid immediately, otherwise we assume the device failed to be created
	if ( !Device->GetMeta().IsValid() )
	{
		Error << "Failed to initialise device " << Serial << " " << InitError.str();
		return nullptr;
	}
	
	//	if there was some error/debug at init... include it
	Error << InitError.str();
	
	mDevices.PushBack( Device );
	return Device;
}

void SoyVideoCapture::AddContainer(std::shared_ptr<SoyVideoContainer> Container)
{
	mContainers.PushBack( Container );
}

bool SoyVideoContainer::HasDevice(const std::string& Serial)
{
	Array<TVideoDeviceMeta> Metas;
	GetDevices( GetArrayBridge(Metas) );
	if ( !Metas.Find( Serial ) )
		return false;
	return true;
}
