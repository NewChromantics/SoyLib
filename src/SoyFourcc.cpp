#include "SoyFourcc.h"
#include <string>
#include "SoyString.h"


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
	//	for safety, we detect non ascii and make a more paletteable representation
	auto IsChar = [](char x)
	{
		if ( x >='A' && x <= 'Z' )	return true;
		if ( x >='a' && x <= 'z' )	return true;
		if ( x >='0' && x <= '9' )	return true;
		if ( x >=' ' )	return true;
		return false;
	};
	bool IsAscii = true;
	IsAscii &= IsChar(mFourccChar[0]);
	IsAscii &= IsChar(mFourccChar[1]);
	IsAscii &= IsChar(mFourccChar[2]);
	IsAscii &= IsChar(mFourccChar[3]);
	
	if ( !IsAscii )
	{
		char String[] = "{0xZZ,0xZZ,0xZZ,0xZZ}";
		Soy::ByteToHex( mFourcc8[0], String[3], String[4] );
		Soy::ByteToHex( mFourcc8[0], String[8], String[9] );
		Soy::ByteToHex( mFourcc8[0], String[13], String[14] );
		Soy::ByteToHex( mFourcc8[0], String[18], String[19] );
		std::string StringString( String );
		return StringString;
	}
	
	std::string FourccString( mFourccChar, 4 );
	return FourccString;
}

std::ostream& Soy::operator<<(std::ostream &out,const Soy::TFourcc& in)
{
	out << in.mFourccChar[0];
	out << in.mFourccChar[1];
	out << in.mFourccChar[2];
	out << in.mFourccChar[3];
	return out;
}
