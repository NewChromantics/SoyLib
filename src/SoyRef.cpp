#include "SoyRef.h"
#include "BufferArray.hpp"

namespace Private
{
	//	max number of unique refs; sizeof(gAlphabetRaw) * SoyRef::MaxStringLength (63 * 8)
	const char gAlphabetRaw[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
	BufferArray<char,100> gAlphabet;		//	initialising this seems to mess up a bit in release, size is right, but contents are zero.
	BufferArray<size_t,256> gAlphabetLookup;
};


struct uint64Chars
{
	uint64Chars() :
		m64	( 0 )
	{
	}
		
	union
	{
		uint64	m64;
		unsigned char	mChars[SoyRef::MaxStringLength];
	};
};


//	accessor for alphabet in case it's used BEFORE the global is constructed. Naughty globals
const BufferArray<char,100>& GetAlphabet()
{
	//	force initialisation
	auto& Alphabet = Private::gAlphabet;
	if ( Alphabet.IsEmpty() )
	{
		Alphabet.PushBackArray( Private::gAlphabetRaw );
	}

	return Private::gAlphabet;
}

const BufferArray<size_t,256>& GetAlphabetLookup()
{
	auto& AlphabetLookup = Private::gAlphabetLookup;

	//	generate
	if ( AlphabetLookup.IsEmpty() )
	{
		auto& Alphabet = GetAlphabet();
		AlphabetLookup.SetSize( 256 );
		for ( int i=0;	i<256;	i++ )
		{
			auto AlphabetIndex = Alphabet.FindIndex( static_cast<char>(i) );

			//	non-alphabet characters turn into #0 (default char)
			AlphabetLookup[i] = (AlphabetIndex < 0) ? 0 : AlphabetIndex;
		}
	}
	return AlphabetLookup;
}

void CorrectAlphabetChar(char& Char)
{
	auto& AlphabetLookup = GetAlphabetLookup();
	auto& Alphabet = GetAlphabet();
	Char = Alphabet[ AlphabetLookup[Char] ];
}

BufferArray<size_t,SoyRef::MaxStringLength> ToAlphabetIndexes(uint64 Ref64)
{
	uint64Chars Ref64Chars;
	Ref64Chars.m64 = Ref64;
	auto& AlphabetLookup = GetAlphabetLookup();
	
	BufferArray<size_t,SoyRef::MaxStringLength> Indexes;
	for ( int i=0;	i<SoyRef::MaxStringLength;	i++ )
	{
		auto& Char = Ref64Chars.mChars[i];
		Indexes.PushBack( AlphabetLookup[Char] );
	}
	return Indexes;
}
uint64 FromAlphabetIndexes(const BufferArray<size_t,SoyRef::MaxStringLength>& Indexes)
{
	//	turn back to chars and cram into a u64
	uint64Chars Ref64Chars;
	assert( Indexes.GetSize() == SoyRef::MaxStringLength );
	auto& Alphabet = GetAlphabet();

	for ( int i=0;	i<Indexes.GetSize();	i++ )
	{
		auto Index = Indexes[i];
		Ref64Chars.mChars[i] = Alphabet[Index];
	}
	return Ref64Chars.m64;
}

	

uint64 SoyRef::FromString(const std::string& String)
{
	//	make up indexes
	uint64Chars Ref64Chars;
	//auto& AlphabetLookup = GetAlphabetLookup();
	GetAlphabetLookup();
	auto& Alphabet = GetAlphabet();

	for ( int i=0;	i<SoyRef::MaxStringLength;	i++ )
	{
		char StringChar = i < String.length() ? String[i] : Alphabet[0];
		CorrectAlphabetChar( StringChar );
		Ref64Chars.mChars[i] = StringChar;
	}
	return Ref64Chars.m64;
}


std::string SoyRef::ToString() const
{
	//	turn integer into alphabet indexes
	auto AlphabetIndexes = ToAlphabetIndexes( mRef );
	auto& Alphabet = GetAlphabet();

	//	concat alphabet chars
	std::string String;
	for ( int i=0;	i<AlphabetIndexes.GetSize();	i++ )
	{
		auto Index = AlphabetIndexes[i];
		char Char = Alphabet[Index];
		String += Char;
	}
	return String;
}

void SoyRef::Increment()
{
	auto AlphabetIndexes = ToAlphabetIndexes( mRef );
	auto& Alphabet = GetAlphabet();

	//	start at the back and increment 
	auto MaxIndex = Alphabet.GetSize()-1;
	for ( ssize_t i=AlphabetIndexes.GetSize()-1;	i>=0;	i-- )
	{
		//	increment index
		AlphabetIndexes[i]++;
		if ( AlphabetIndexes[i] <= MaxIndex )
			break;

		//	rolled over...
		AlphabetIndexes[i] = 0;
		//	... let i-- and we increment previous char
	}

	//	if the "final integer" is reached, we'll end up with 000000000 (ie, invalid).
	//	if this is the case, we need to increment the alphabet, or start packing.
	//	gr: actually, this will never happen. 0000000 is _________ (not invalid!)
	assert( IsValid() );

	//	convert back
	mRef = FromAlphabetIndexes( AlphabetIndexes );
}

