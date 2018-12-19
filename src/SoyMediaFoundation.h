#pragma once

#include <memory>
#include <string>
#include <SoyTypes.h>

#include <SoyPixels.h>
#include <SoyMediaFormat.h>
#include <SoyMedia.h>
#include <SoyAutoReleasePtr.h>

#include <Mfapi.h>
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"Mfuuid.lib")

#include <Mfidl.h>
#pragma comment(lib,"Mf.lib")

#include <Mfreadwrite.h>
#pragma comment(lib,"Mfreadwrite.lib")


std::ostream& operator<<(std::ostream& os, REFGUID guid);
std::ostream& operator<<(std::ostream& os,MFVideoTransferMatrix Mode);

class MfExtractor;
class TMediaPacket;


namespace MediaFoundation
{
	class TContext;

	std::shared_ptr<TContext>		GetContext();	//	singleton
	void							Shutdown();		//	cleanup singleton context

	void							IsOkay(HRESULT Error,const std::string& Context);

	SoyMediaFormat::Type			GetFormat(GUID Format);
	SoyPixelsFormat::Type			GetPixelFormat(GUID Format);
	SoyMediaFormat::Type			GetFormat(GUID Major,GUID Minor,size_t H264NaluLengthSize);
	GUID							GetFormat(SoyPixelsFormat::Type Format);
	GUID							GetFormat(SoyMediaFormat::Type Format);

	Soy::AutoReleasePtr<IMFMediaType>	GetPlatformFormat(GUID MajorFormat,GUID MinorFormat);
	Soy::AutoReleasePtr<IMFMediaType>	GetPlatformFormat(SoyMediaFormat::Type Format);
	Soy::AutoReleasePtr<IMFMediaType>	GetPlatformFormat(SoyMediaFormat::Type Format,size_t Width,size_t Height);
	Soy::AutoReleasePtr<IMFMediaType>	GetPlatformFormat(SoyPixelsFormat::Type Format);
	Soy::AutoReleasePtr<IMFMediaType>	GetPlatformFormat(TMediaEncoderParams Params,size_t Width,size_t Height);

	DWORD							GetFourcc(SoyMediaFormat::Type Format);
	Soy::AutoReleasePtr<IMFSample>		CreatePixelBuffer(TMediaPacket& Packet);
}


class MediaFoundation::TContext
{
public:
	TContext();
	~TContext();
};

