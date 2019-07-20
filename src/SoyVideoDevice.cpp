#include "SoyVideoDevice.h"
#include "SoyString.h"
#include "SortArray.h"

bool TVideoDeviceMeta::operator==(const std::string& Serial) const
{
	if ( mSerial == Serial )
		return true;
	
	//	gr: allow loose match by name
	if ( Soy::StringContains( GetName(), Serial, false ) )
		return true;
	
	return false;
}


std::ostream& operator<< (std::ostream &out,const TVideoDeviceMeta &in)
{
	auto& Meta = in;
	out << Meta.GetName() << "[" << Meta.mSerial << "]";
	if ( Meta.mVideo )	out << " +Video";
	if ( Meta.mAudio )	out << " +Audio";
	if ( Meta.mText )	out << " +Text";
	if ( Meta.mClosedCaption )	out << " +ClosedCaption";
	if ( Meta.mSubtitle )	out << " +Subtitle";
	if ( Meta.mTimecode )	out << " +Timecode";
	if ( Meta.mTimedMetadata )	out << " +TimedMetadata";
	if ( Meta.mMetadata )	out << " +Metadata";
	if ( Meta.mMuxed )	out << " +Muxed";
	if ( Meta.mDepth )	out << " +Depth";
	return out;
}



SoyPixelsImpl& TVideoFrameImpl::GetPixels()
{
	auto Pixels = GetPixelsShared();
	if ( Pixels == nullptr )
		throw Soy::AssertException("Pixels expected");
	return *Pixels;
}



TVideoDevice::TVideoDevice(const TVideoDeviceMeta& Meta) :
//	mLastFrame		( Meta.mSerial, Soy::StreamToString(std::stringstream() << "frame_" << Meta.mSerial), true ),
	mLastFrame		( Meta.mSerial ),
	mLastError		( "waiting for first frame" ),
	mFrameCount		( 0 )
{
}

TVideoDevice::~TVideoDevice()
{
}


TVideoFrame& TVideoDevice::GetLastFrame()
{
	//	there's an error, throw instead of returning frame
	if ( !mLastError.empty() )
	{
		throw Soy::AssertException( mLastError );
	}

	return mLastFrame;
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

SoyPixelsImpl& TVideoDevice::LockNewFrame()
{
	mLastError.clear();
	return *mLastFrame.mPixels;
}

void TVideoDevice::UnlockNewFrame(SoyTime Timecode)
{
	//	gr: might want to reject earlier timecodes here
	//	gr^^ nope! this class is dumb
	mLastFrame.mTimecode = Timecode;
	
	//	update frame/rate counting
	mFrameCount++;
	mLastFrameTime = SoyTime(true);
	if ( !mFirstFrameTime.IsValid() )
		mFirstFrameTime = mLastFrameTime;
	
	if ( mOnNewFrame )
		mOnNewFrame( *this );
}


void TVideoDevice::OnNewFrame(const SoyPixelsImpl& Pixels,SoyTime Timecode)
{
	if ( !mLastFrame.mPixels )
	{
		mLastError = "Missing pixels";
		return;
	}
	
	
	mLastFrame.mPixels->Copy( Pixels );

	//if ( !mLastFrame.mPixels->Copy( Pixels ) )
	{
		mLastError = "Failed to copy pixels";
		return;
	}
	
	mLastError.clear();
	UnlockNewFrame( Timecode );
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
	TSortPolicy_BestVideoMeta(const std::string& Serial) :
		mMatchSerial		( Serial ),
		mMatchSerialLower	( Soy::StringToLower( Serial ) )
	{
	}
	
	template<typename TYPEB>
	static int			Compare(const TVideoDeviceMeta& a,const TYPEB& b,const TSortPolicy_BestVideoMeta& This)
	{
		bool aExactSerial = a.mSerial == This.mMatchSerial;
		bool bExactSerial = b.mSerial == This.mMatchSerial;
		if ( aExactSerial && !bExactSerial )	return -1;
		if ( !aExactSerial && bExactSerial )	return 1;
		
		std::string a_mSerial_Lower = Soy::StringToLower( a.mSerial );
		std::string b_mSerial_Lower = Soy::StringToLower( b.mSerial );
		bool aExactSerialLower = a_mSerial_Lower == This.mMatchSerialLower;
		bool bExactSerialLower = b_mSerial_Lower == This.mMatchSerialLower;
		if ( aExactSerialLower && !bExactSerialLower )	return -1;
		if ( !aExactSerialLower && bExactSerialLower )	return 1;
		
		bool aExactName = a.mName == This.mMatchSerial;
		bool bExactName = b.mName == This.mMatchSerial;
		if ( aExactName && !bExactName )	return -1;
		if ( !aExactName && bExactName )	return 1;
		
		bool aSerialStartsWith = Soy::StringBeginsWith( a.mSerial, This.mMatchSerial, false );
		bool bSerialStartsWith = Soy::StringBeginsWith( b.mSerial, This.mMatchSerial, false );
		if ( aSerialStartsWith && !bSerialStartsWith )	return -1;
		if ( !aSerialStartsWith && bSerialStartsWith )	return 1;
		
		bool aNameStartsWith = Soy::StringBeginsWith( a.mName, This.mMatchSerial, false );
		bool bNameStartsWith = Soy::StringBeginsWith( b.mName, This.mMatchSerial, false );
		if ( aNameStartsWith && !bNameStartsWith )	return -1;
		if ( !aNameStartsWith && bNameStartsWith )	return 1;

		std::stringstream Error;
		Error << "Need some more sorting comparisons for [" << This.mMatchSerial << "] with [" << a.mSerial << "/" << a.mName << "] and [" << b.mSerial << "/" << b.mName << "]";
		Soy::Assert( false, Error );
		return 0;
	}
	
public:
	std::string		mMatchSerial;	//	gr: need a better way of having variables in sort policies
	std::string		mMatchSerialLower;
};


TVideoDeviceMeta SoyVideoCapture::GetDeviceMeta(std::string Serial)
{
	Array<TVideoDeviceMeta> Metas;
	GetDevices( GetArrayBridge(Metas) );
	
	//	filter non-video devices
	for ( ssize_t i=Metas.GetSize()-1;	i>=0;	i-- )
	{
		if ( Metas[i].mVideo )
			continue;
		if ( Metas[i].mDepth )
			continue;
		Metas.RemoveBlock( i, 1 );
	}
	
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
	for ( ssize_t m=Metas.GetSize()-1;	m>=0;	m-- )
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
	
	//	in case we've provided the name, or only part of the serial, find the proper serial (and it's meta)
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
		Device = Container.AllocDevice( Meta, InitError );
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
