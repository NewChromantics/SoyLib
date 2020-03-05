#pragma once

#include "SoyPixels.h"
#include <tuple>

class SoyPixelsMeta;
class SoyPixelsImpl;
class TStreamBuffer;


namespace Soy
{
	//	detect presence of a[stb supported] image from a buffer
	SoyPixelsMeta	IsImage(const ArrayBridge<uint8_t>&& Buffer);
	void			DecodeImage(SoyPixelsImpl& Pixels, const ArrayBridge<uint8_t>&& Buffer);
}

//	stb interfaces which haven't yet had any specific Soy stuff yet
namespace Png
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer,std::function<void(const std::string& Section,const ArrayBridge<uint8_t>& MetaData)> OnMeta=nullptr);
	static const char*	FileExtensions[] = {".png"};
}

namespace Jpeg
{
	class TMeta
	{
	public:
		Array<uint8>	mExif;
		std::string		mXmp;	//	xml
	};
	
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	void		ReadMeta(Jpeg::TMeta& Meta,TStreamBuffer& Buffer);

	static const char*	FileExtensions[] = {".jpg",".jpeg"};
}

namespace Gif
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".gif"};
}

namespace Tga
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".tga"};
}

namespace Bmp
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".bmp"};
}

namespace Psd
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".psd"};
}


