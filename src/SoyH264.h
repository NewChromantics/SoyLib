#pragma once

#include <SoyMedia.h>

namespace H264
{
	void		ResolveH264Format(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);
	void		ConvertToEs(SoyMediaFormat::Type& Format,ArrayBridge<uint8>&& Data);

	size_t		GetNaluLengthSize(SoyMediaFormat::Type Format);
	
	bool		IsNalu4(ArrayBridge<uint8>& Data);
	bool		IsNalu3(ArrayBridge<uint8>& Data);
	inline bool	IsNalu4(ArrayBridge<uint8>&& Data)	{	return IsNalu4( Data );	}
	inline bool	IsNalu3(ArrayBridge<uint8>&& Data)	{	return IsNalu3( Data );	}
}

