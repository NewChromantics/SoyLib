#pragma once

#include <memory>
#include <string>
#include <SoyTypes.h>

#include <SoyPixels.h>
#include <SoyMediaFormat.h>

#include <Mfapi.h>
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"Mfuuid.lib")

#include <Mfidl.h>
#pragma comment(lib,"Mf.lib")

#include <Mfreadwrite.h>
#pragma comment(lib,"Mfreadwrite.lib")


std::ostream& operator<<(std::ostream& os, REFGUID guid);

class MfExtractor;
class TMediaPacket;


namespace MediaFoundation
{
	class TContext;

	std::shared_ptr<TContext>		GetContext();	//	singleton
	void							Shutdown();		//	cleanup singleton context

	inline bool						IsOkay(HRESULT Error,const std::string& Context,bool ThrowException=true)	{	return Platform::IsOkay( Error, Context, ThrowException );	}

	SoyMediaFormat::Type			GetFormat(GUID Format);
	SoyPixelsFormat::Type			GetPixelFormat(GUID Format);
	GUID							GetFormat(SoyPixelsFormat::Type Format);
	GUID							GetFormat(SoyMediaFormat::Type Format);
	AutoReleasePtr<IMFMediaType>	GetPlatformFormat(GUID MajorFormat,GUID MinorFormat);
	AutoReleasePtr<IMFMediaType>	GetPlatformFormat(SoyMediaFormat::Type Format);
	AutoReleasePtr<IMFMediaType>	GetPlatformFormat(SoyPixelsFormat::Type Format);
	DWORD							GetFourcc(SoyMediaFormat::Type Format);

	SoyMediaFormat::Type			GetFormat(GUID Major,GUID Minor,size_t H264NaluLengthSize);

	AutoReleasePtr<IMFMediaType>	CreateFormat(SoyMediaFormat::Type Format,size_t FrameRate,size_t BitRate,size_t Width,size_t Height);
	AutoReleasePtr<IMFSample>		CreatePixelBuffer(TMediaPacket& Packet);
}


class MediaFoundation::TContext
{
public:
	TContext();
	~TContext();
};

