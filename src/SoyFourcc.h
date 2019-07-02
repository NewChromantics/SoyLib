#pragma once

#include <iosfwd>
#include <stdint.h>

namespace Soy
{
	class TFourcc;
}


class Soy::TFourcc
{
public:
	TFourcc(const char* abcd);
	TFourcc(const uint8_t* abcd);
	//TFourcc(char a,char b,char c,char d);	//	be careful with char->u8
	TFourcc(uint8_t a,uint8_t b,uint8_t c,uint8_t d);	//	be careful with char->u8
	TFourcc(uint32_t abcd);
	
	std::string	GetString() const;
	bool		operator==(const TFourcc& That) const	{	return mFourcc32 == That.mFourcc32;	}
	bool		operator!=(const TFourcc& That) const	{	return mFourcc32 != That.mFourcc32;	}
	
	union
	{
		uint8_t		mFourcc8[4];
		char		mFourccChar[4];
		uint32_t	mFourcc32;
	};
};

std::ostream& operator<<(std::ostream &out,const Soy::TFourcc& in);



