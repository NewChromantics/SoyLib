#include "SoyBase64.h"



namespace Base64
{
	const std::string Alphabet =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	const char PaddingChar = '=';

	
	inline bool IsBase64Char(unsigned char c)
	{
		//	gr: faster than using alphabet
		if ( c>='A' && c<='Z' )	return true;
		if ( c>='a' && c<='z' )	return true;
		if ( c>='0' && c<='9' )	return true;
		if ( c == '+' )			return true;
		if ( c == '/' )			return true;
		return false;
	}
	
	inline bool IsPadding(unsigned char c)
	{
		return c==PaddingChar;
	}

}



void Base64::Encode(ArrayBridge<char>& Encoded,const ArrayBridge<char>& Decoded)
{
	auto& base64_chars = Alphabet;
	
	//	gr: pre-alloc. base64 turns 3 chars into 4...
	//	just double what we take in.
	Encoded.Reserve( Decoded.GetSize()*2 );
		
	auto in_len = Decoded.GetDataSize();
	auto* bytes_to_encode = Decoded.GetArray();
	if ( !bytes_to_encode )
		return;
	
	auto& ret = Encoded;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; (i <4) ; i++)
				ret.PushBack( base64_chars[char_array_4[i]] );
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++)
			ret.PushBack( base64_chars[char_array_4[j]] );
		
		while((i++ < 3))
			ret.PushBack( PaddingChar );
		
	}
	
}

void Base64::Decode(const ArrayBridge<char>& Encoded,ArrayBridge<char>& Decoded)
{
	auto& base64_chars = Alphabet;
	
	
	//	gr: pre-alloc. base64 turns 3 chars into 4...
	//		so decoding... we only need 3/4 of the data
	//	...	just alloc same amount, it'll be smaller
	Decoded.Reserve( Encoded.GetSize() );
	
	ssize_t in_len = Encoded.GetSize();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	auto& ret = Decoded;

	//	gr: this should inline when optimised (-O1 etc)
	auto Decode4to3 = [&ret,&char_array_4,&char_array_3](int Length)
	{
		for ( int j = 0; j < 4; j++)
		{
			auto Index = base64_chars.find(char_array_4[j]);
			//	gr: what should this be if padding??
			if ( Index == base64_chars.npos )
				Index = 0;
			char_array_4[j] = size_cast<uint8>(Index);
		}
		
		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
		
		for ( int j = 0; (j < Length - 1); j++)
			ret.PushBack( char_array_3[j] );
	};

	
	while (in_len-- )
	{
		if ( Encoded[in_] == PaddingChar )
			continue;
		
		if ( !IsBase64Char( Encoded[in_] ) )
			throw Soy::AssertException("Character not base64");
		
		char_array_4[i++] = Encoded[in_];
		in_++;
		if (i==4)
		{
			Decode4to3(i);
			i = 0;
		}
	}
	
	if (i)
	{
		//	clear old
		for (j = i; j <4; j++)
			char_array_4[j] = 0;
		Decode4to3(i);
	}
}
