#include "SoyFourcc.h"
#include <string>


Soy::TFourcc::TFourcc(const char* abcd) :
	TFourcc	( reinterpret_cast<const uint8_t*>(abcd) )
{
}

Soy::TFourcc::TFourcc(const uint8_t* abcd)
{
	mFourcc8[0] = abcd[0];
	mFourcc8[1] = abcd[1];
	mFourcc8[2] = abcd[2];
	mFourcc8[3] = abcd[3];
}

Soy::TFourcc::TFourcc(uint8_t a,uint8_t b,uint8_t c,uint8_t d)
{
	mFourcc8[0] = a;
	mFourcc8[1] = b;
	mFourcc8[2] = c;
	mFourcc8[3] = d;
}

Soy::TFourcc::TFourcc(uint32_t abcd) :
	mFourcc32	( abcd )
{
}

std::string Soy::TFourcc::GetString() const
{
	std::string FourccString( mFourccChar, 4 );
	return FourccString;
}

std::ostream& operator<<(std::ostream &out,const Soy::TFourcc& in)
{
	out << in.mFourccChar[0];
	out << in.mFourccChar[1];
	out << in.mFourccChar[2];
	out << in.mFourccChar[3];
	return out;
}
