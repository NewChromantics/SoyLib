#pragma once

#include <SoyMedia.h>

namespace H264
{
	void		ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);
	void		ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);

	bool		IsNalu4(ArrayBridge<uint8>& Data);
	bool		IsNalu3(ArrayBridge<uint8>& Data);
}

