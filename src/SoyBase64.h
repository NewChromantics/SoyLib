#pragma once

#include "array.hpp"

namespace Base64
{
	//	gr: change these inputs so it's char for encoded, uint8 for decoded
	//	http://www.adp-gmbh.ch/cpp/common/base64.html
	void			Encode(ArrayBridge<char>& Encoded,const ArrayBridge<char>& Decoded);
	void			Decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>& Decoded);
	inline void		Decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>&& Decoded)	{	Decode( Encoded, Decoded );	}
};


