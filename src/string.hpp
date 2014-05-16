/*
	Twilight Prophecy SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2003 Twilight 3D Finland Oy Ltd.
*/
/*
	source:
		Expression Template string class

	revision history:
		Jun/10/2000 - Thomas Mannfred Carlsson - initial revision
		Nov/16/2001 - Mats Byggmastar - reported a memory leak in "+ operator"(s)
		Dec/01/2001 - Jukka Liimatta - rewrote the string class
		Feb/06/2002 - Francesco Montecuccoli - added SubString() method
		Nov/12/2002 - Jukka Liimatta - rewrote the string to use Soy::Array<char> as container
		Dec/26/2002 - Jukka Liimatta - rewrote the + operator to support expressions (christmas fun..)
		Jan/12/2003 - Nietje - fixed SubString() method
		Jan/12/2003 - Jukka Liimatta - added MetaSubString() method, and rewrote SubString() to use it
		Jan/13/2003 - Jukka Liimatta - fixed operator == for case when both strings are empty
		Jan/14/2003 - Jukka Liimatta - case conversion templates, made Soy::StringCompare() a template
*/
#pragma once


#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <iostream>
#include "SoyTypes.h"
#include "chartype.hpp"
#include "array.hpp"
//#include "heaparray.hpp"
#include "bufferarray.hpp"


namespace Soy
{
	//	cross platform, no warning, safe sprintf
	template<size_t BUFFERSIZE,typename TYPE>
	void		Sprintf(char (&Buffer)[BUFFERSIZE],const char* Format,const TYPE& Variable)
	{
#if defined(TARGET_WINDOWS)
		int Terminator = sprintf_s( Buffer, Format, Variable );
#else
		int Terminator = sprintf( Buffer, Format, Variable );
#endif
		//	force terminator in case of error/overflow
		Terminator = ofLimit<int>( Terminator, 0, BUFFERSIZE );
		Buffer[Terminator] = '\0';
		assert( strlen(Buffer) <= BUFFERSIZE );
	}


	inline int	StrCaseCmp( const char* a, const char* b );

	//	type independent version of strlen()
	template<typename S>
	int			StringLen(const S* a,int Max,int AllocMax)		
	{
		if ( AllocMax < Max && AllocMax > 0 )
			Max = AllocMax;

		if ( Max < 0 )
			Max = AllocMax;	//	gr: could be more clever?
		int i=0;	
		while ( a && *a && i<Max )	
		{	
			i++;	
			a++;	
		}	
		return i;	
	}

	//	return true if CHar is a commonly used spacer character to deliniate strings
	template<typename S>
	inline bool	IsWhitespaceChar(S Char)
	{
		return ( Char == ' ' ||
				Char == '\n' ||
				Char == '\t' ||
				Char == '\0' ||
				Char == ',' );
	}

	
	inline bool	IsUtf8Char(char Character)
	{
		switch ( Character )
		{
			case 0xC0:
			case 0xC1:
			case 0xF5:
			case 0xF6:
			case 0xF7:
			case 0xF8:
			case 0xF9:
			case 0xFA:
			case 0xFB:
			case 0xFC:
			case 0xFD:
			case 0xFE:
			case 0xFF:
				return false;
		}
		return true;
	}

	// string expression

	template <typename S, typename L, typename R>
	class StringExp
	{
		const L left;
		const R right;
		public:

		StringExp(const L& leftarg, const R& rightarg)
		: left(leftarg),right(rightarg)
		{
		}

		int size() const
		{
			return left.size() + right.size();
		}

		void append(S*& dest) const
		{
			left.append(dest);
			right.append(dest);
		}
	};

	// string container
	//	ARRAYTYPE is the storage class. Must include S;
	//	eg.
	//		Array<S>
	//		BufferArray<S>
	template <typename S,class ARRAYTYPE>
	class String2
	{
	public:
		String2()
		{
			mdata.PushBack(0);
		}

		String2(int size)
		: mdata(size+1)
		{
			mdata[size] = 0;
		}

		template<typename THATARRAYTYPE>
		String2(const String2<S,THATARRAYTYPE>& s)
		: mdata(s.GetArray())
		{
		}
		
		String2(const S* text)
		{
			*this = text;
		}

		String2(const std::string& String)
		{
			*this = String.c_str();
		}

		~String2()
		{
		}

		template <typename L, typename R>
		String2(const StringExp<S,L,R>& exp)
		: mdata(exp.size() + 1)
		{
			S* dest = mdata.GetArray();
			exp.append(dest);
			*dest = 0;
		}

		template <typename L, typename R>
		String2& operator = (const StringExp<S,L,R>& exp)
		{
			mdata.SetSize(exp.size() + 1);

			S* dest = mdata.GetArray();
			exp.append(dest);
			*dest = 0;

			return *this;
		}

		//	copy[part of] a string. if MaxLength < 0 we copy the whole string
		inline void CopyString(const S* text,int MaxLength=-1)
		{
			if ( !text )
			{
				mdata.ResetIndex();
				mdata.PushBack(0);
				return;
			}

			//	calc how much to copy
			int len = StringLen( text, MaxLength, mdata.MaxAllocSize()-1 );

			//	alloc block
			mdata.Clear(false);
			mdata.Reserve( len + 1, false );
			S* NewData = mdata.PushBlock( len );
			if ( !NewData )
				return;
			memcpy( NewData, text, len );
			
			//	add terminator
			mdata.PushBack(0);
		}

		template<class THATARRAYTYPE>
		String2& operator = (const String2<S,THATARRAYTYPE>& s)
		{
			mdata = s.GetArray();
			return *this;
		}

		//template <typename S2>
		String2& operator = (const S* text)
		{
			CopyString( text );
			return *this;
		}

		String2& operator += (const String2& s)
		{
			int count = s.GetLength();
			assert( count >= 0 );
			if ( count > 0 )
			{
				//	-1 so we start at existing terminator
				S* dest = mdata.PushBlock(count);

				//	failed to alloc (gr: may be due
				if ( dest == NULL )
					return *this;

				assert( dest );
				dest--;
				const S* text = s;

				//	<= to copy terminator
				for ( int i=0; i<=count; ++i )
					*dest++ = *text++;
			}
			return *this;
		}

		template <typename S2>
		String2& operator += (const S2* text)
		{
			if ( !text )
				return *this;

			mdata.PopBack();

			//	pre-alloc
			int len = StringLen( text, -1, mdata.MaxAllocSize() );
			mdata.Reserve( len + 1, false );

			for ( ; *text; ++text )
				mdata.PushBack(*text);

			mdata.PushBack(0);
			return *this;
		}

		bool operator == (const String2& s) const
		{
			if ( GetLength() != s.GetLength() )
				return false;

			const S* s1 = *this;
			const S* s2 = s;
			int count = GetLength();

			while ( count-- )
			{
				if ( *s1++ != *s2++ )
					return false;
			}

			return true;
		}
		
		template<class OTHERSTRINGARRAY>
		bool operator == (const String2<S,OTHERSTRINGARRAY>& s) const
		{
			if ( GetLength() != s.GetLength() )
				return false;

			const S* s1 = *this;
			const S* s2 = s;
			int count = GetLength();

			while ( count-- )
			{
				if ( *s1++ != *s2++ )
					return false;
			}

			return true;
		}

		bool operator == (const S* text) const
		{
			if ( !text )
				return (this->GetLength() == 0);

			const S* s = *this;
			while ( *s == *text++ )
			{
				if ( *s++ == 0 )
					return true;
			}

			return false;
		}

		bool operator < (const S* text) const
		{
			const S* s = *this;
			if ( !*s ) return true;
			if ( !*text ) return false; 
			while ( *s == *text )
			{
				if ( *s == NULL && *text == NULL)
				{
					// strings are identical
					return false;
				}
				
				++s;
				++text;
			}
			return (*s < *text);
		}
		
		bool operator > (const S* text) const
		{
			const S* s = *this;
			if ( !*s ) return true;
			if ( !*text ) return false; 
			while ( *s == *text )
			{
				if ( *s == NULL && *text == NULL)
				{
					// strings are identical
					return false;
				}

				++s;
				++text;
			}
			return (*s > *text);
		}
		
		template<typename OTHERSTRING>
		bool operator != (const OTHERSTRING& s) const
		{
			return !(*this == s);
		}

		S& operator [] (int index)
		{
			return mdata[index];
		}
		
		const S& operator [] (int index) const
		{
			return mdata[index];
		}

		const S*	c_str() const	{	return mdata.GetArray();	}

		operator S* ()
		{
			return mdata.GetArray();
		}

		operator const S* () const
		{
			return mdata.GetArray();
		}

		const ARRAYTYPE& GetArray() const
		{
			return mdata;
		}

		bool IsEmpty() const
		{
			return mdata.GetSize() <= 1;
		}

		int GetLength() const
		{
			return mdata.GetSize() - 1;
		}

		S* SetLength(int length)
		{
			mdata.SetSize(length + 1, true, true );
			mdata[length] = 0;
			return mdata.GetArray();
		}
		
		void QuickClear()
		{
			mdata.SetSize( 1, false, true );
			mdata[0] =0;
		}

		String2& operator << (const String2& s)
		{
			return operator += (s);
		}

		const String2& operator >> (String2& s)
		{
			s << *this;
			return *this;
		}

		template <typename S2>
		String2& operator << (const S2* text)
		{
			return operator += (text);
		}

		String2& operator << (const unsigned char v)
		{
			//	dont push if we're pushing a terminator
			if ( v == 0 )
				return *this;
			const int offset = mdata.GetSize() - 1;
			mdata[offset] = v;
			mdata.PushBack(0);
			return *this;
		}

		String2& operator << (const char v)
		{ 
			return operator << (static_cast<const unsigned char>(v));
		}

		String2& operator << (const signed char v)
		{ 
			return operator << (static_cast<const unsigned char>(v));
		}

		String2& operator << (const unsigned char* v)
		{ 
			return operator << (reinterpret_cast<const char*>(v));
		}

		String2& operator << (const signed char* v)
		{ 
			return operator << (reinterpret_cast<const char*>(v));
		}

		String2& operator << (const short v)
		{
			char text[8];
			Soy::Sprintf( text, "%d", static_cast<int>(v) );
			return operator += (text);
		}

		String2& operator << (const unsigned short v)
		{
			char text[8];
			Soy::Sprintf(text,"%u",static_cast<unsigned int>(v));
			return operator += (text);
		}

		String2& operator << (const int v)
		{
			char text[16];
			Soy::Sprintf( text, "%d", static_cast<int>(v) );
			return operator += (text);
		}

		String2& operator << (const unsigned int v)
		{
			char text[16];
			Soy::Sprintf(text,"%u",static_cast<unsigned int>(v));
			return operator += (text);
		}

		String2& operator << (const long v)
		{
			char text[16];
			Soy::Sprintf(text,"%d",static_cast<int>(v));
			return operator += (text);
		}

		String2& operator << (const unsigned long v)
		{
			char text[16];
			Soy::Sprintf(text,"%u",static_cast<unsigned int>(v));
			return operator += (text);
		}

		String2& operator << (const int64 v)
		{
			char text[32];
			Soy::Sprintf(text,"%lld",v);
			return operator += (text);
		}

		String2& operator << (const uint64 v)
		{
			char text[32];
			Soy::Sprintf(text,"%llu",v);
			return operator += (text);
		}

		String2& operator << (const float v)
		{
			char text[256];
			Soy::Sprintf(text,"%.3f",v);
			return operator += (text);
		}
	
		String2& operator << (const double v)
		{
			char text[256];
			Soy::Sprintf(text,"%.3f",v);
			return operator += (text);
		}

		int			FindIndex(const S* text,bool CaseSensitive=true,int StartPos=0) const;		//	find start of the occurance of this sub string. returns -1 if not found
		const S*	Find(const S* text,bool CaseSensitive=true,int StartPos=0) const			{	int Index = FindIndex( text, CaseSensitive, StartPos );	return (Index<0) ? NULL : &(*this)[Index];	}
		bool		Contains(const S* text,bool CaseSensitive=true,int StartPos=0) const		{	return FindIndex( text, CaseSensitive, StartPos ) >= 0;	}
		bool		StartsWith(const S* text,bool CaseSensitive=true,int StartPos=0) const		{	return FindIndex( text, CaseSensitive, StartPos ) == 0;	}
		bool		EndsWith(const S* text,bool CaseSensitive=true) const;

		//	copy string to a char Buffer[BUFFERSIZE] c-array. (lovely template syntax! :)
		template <typename BUFFERTYPE,unsigned int BUFFERSIZE>
		void		CopyToBuffer(BUFFERTYPE (& Buffer)[BUFFERSIZE]) const
		{
			int Length = ofMin<int>( GetLength(), static_cast<int>(BUFFERSIZE)-1 );
			for ( int i=0;	i<Length;		i++)
			{
				S Char = mdata[i];
				Buffer[i] = static_cast<BUFFERTYPE>( Char );
				//Buffer[i] &= 0x00ff;	//	memory wierdness, it happens.
			}

			//	force terminator
			Buffer[Length] = 0;
		}
		

		void PrintText(const char* text, ...)
		{
			if ( !text )
				return;

			va_list args;
			va_start(args,text);

			//	safer printf implemented. if the printf() operation doesn't fit in the buffer, the _TRUNCATE param means the string will clip at X chars
			//	if _TRUNCATE not provided, an empty buffer will be returned.
			char Buffer[512] = {0};
#if defined(TARGET_WINDOWS)
			const int BufferSize = sizeofarray(Buffer);
			bool Truncate = true;
			int Result = vsnprintf_s( Buffer, BufferSize, Truncate ? _TRUNCATE : BufferSize-1, text, args );
			//	if this is triggered, the string was truncated (with _TRUNCATE argument) and didn't fit in the buffer.
			//	the resulting string is still null-terminated, but probably not the expected/desired resulting string
			assert( Result != -1 );
#else
            //  C99/osx function does NOT null terminator, Result can be longer than buffer (prints what the length WOULD have been)
			int Length = vsnprintf( Buffer, sizeofarray(Buffer), text, args );
            Length = ofMin<int>( Length+1, sizeofarray(Buffer)-1 );
            Buffer[Length] = 0;
            assert( Buffer[Length] == 0 );
#endif
			va_end(args);


			*this = Buffer;
		}

		void ToLower()
		{
			int size = GetLength();
			for ( int i=0; i<size; ++i )
				mdata[i] = Soy::tolower(mdata[i]);
		}

		void ToUpper()
		{
			int size = GetLength();
			for ( int i=0; i<size; ++i )
				mdata[i] = Soy::toupper(mdata[i]);
		}

		void Reserve(int Size)	
		{
			mdata.Reserve( Size, false );	
		}

		void RemoveAt(int offset,int length=1)
		{
			int NewLength = GetLength() - length;
			mdata.RemoveBlock( offset, length );

			//	reset terminator 
			SetLength( NewLength );
		}

		void InsertAt(int Offset,const String2& String)
		{
			int Len = String.GetLength();
			char* Block = mdata.InsertBlock( Offset, Len );
			if ( !Block )
				return;
			memcpy( Block, String, Len );
		}

		void InsertAt(int Offset,const char* String)
		{
			int Len = StringLen( String );
			const char* Block = mdata.InsertBlock( Offset, Len );
			if ( !Block )
				return;
			memcpy( Block, String, Len );
		}

		//	extract the integer from this string 
		bool GetInteger(int32& Integer) const
		{
			if ( IsEmpty() )
			{
				Integer = 0;
				return false;
			}

			return ExtractInt( Integer, mdata.GetArray() , NULL );
		}

		//	extract the integer from this string 
		bool GetFloat(float& Float) const
		{
			if ( IsEmpty() )
			{
				Float = 0.f;
				return false;
			}

			bool PreviousInvalid;
			return ExtractFloat( Float, mdata.GetArray(), NULL, PreviousInvalid );
		}

		bool	GetBool(bool& Boolean) const;					//	extract boolean value from a string
		template<class FLOATARRAY>
		void	GetFloatArray(FLOATARRAY& Values) const;		//	read multiple floats from this string
		template<class INTARRAY>
		void	GetIntArray(INTARRAY& Values) const;			//	read multiple ints from this string

		//	extract all "chunks" of a string, split by a character
		template<class CHUNKARRAY>
		inline void	Split(CHUNKARRAY& Chunks,const S Delin,bool CullEmptyChunks=false,uint32 MaxSplits=0) const
		{
			BufferArray<S,1> Delins;	
			Delins.PushBack(Delin);	
			Split( Chunks, Delins, CullEmptyChunks, MaxSplits );	
		}


		//	extract all "chunks" of a string, split by any Delin[ination] character
		template<class CHUNKARRAY,class DELINARRAY>
		void Split(CHUNKARRAY& Chunks,const DELINARRAY& Delin,bool CullEmptyChunks=false,uint32 MaxSplits=0) const
		{
			int Length = GetLength();
			if ( Length == 0 )
				return;

			//	string to push into (un-assigned until needed)
            //  gr: to get around GCC name lookup (http://gcc.gnu.org/onlinedocs/gcc/Name-lookup.html) we auto get the type
			auto* NextString = Chunks.GetArray();
			NextString = nullptr;

			//	iterate through string...
			for ( int i=0;	i<Length;	i++ )
			{
				const S& Char = mdata[i];

				//	hit a Delininator? start new string
				if ( Delin.Find(Char) )
				{
					//	current chunk is empty so keep using it
					if ( CullEmptyChunks )
					{
						if ( !NextString || (NextString&&NextString->IsEmpty()) )
							continue;
					}

					//	if we've hit our limit of splits, then just keep adding to the last one
					if ( MaxSplits > 0 && Chunks.GetSize() >= static_cast<int>(MaxSplits) )
					{
						//	gr: still add this delinination character so that
						//	"One.Two.Three." split('.',2) retults in
						//	[One],[Two.Three.] and not [One],[TwoThree]
						if ( !NextString )
							NextString = &Chunks.PushBack();

						if ( NextString )	//	extra safe
							(*NextString) << Char;
						continue;
					}

					//	new chunk at delininator
					NextString = &Chunks.PushBack();
					continue;
				}
	
				//	add to current string (or if we have no current string, get one)
				if ( !NextString )	
					NextString = &Chunks.PushBack();

				if ( NextString )	//	extra safe
					(*NextString) << Char;
			}

			//	might end up with an empty string on the end if [splitting by ,] we have a , on the end like: "a,b,c,"
			//	gr: should no longer end up with an empty NextString here any more.
			if ( CullEmptyChunks && NextString && NextString->IsEmpty() )
				Chunks.PopBack();
		}

		void	Replace(const char& Match,const char& Replacement);			//	replace characters in the string

		int		GetNextNonWhitespaceCharacterIndex(uint32 From=0) const;	//	to aid splitting strings, iterate from From until we hit a non-whitespace character. Will return terminator, and returns -1 if starting at terminator
		int		GetNextWhitespaceCharacterIndex(uint32 From=0) const;		//	to aid splitting strings, iterate from From until we hit a whitespace character. Will return terminator, and returns -1 if starting at terminator
		int		GetNextChar(const S& Char,int32 From=0) const;				//	return index of next occurrence of this character. returns -1 if no more
		int		GetLastChar(const S& Char,int32 From=0) const;				//	return index of the last occurrence of this character. returns -1 if no more
		void	Trim(const S& Char=0)										{	TrimLeft( Char );	TrimRight( Char );	}
		void	TrimRight(const S& Char=0);									//	trim whitespace (or specific char) from the end of the string
		void	TrimLeft(const S& Char=0);									//	trim whitespace (or specific char) from the start of the string


		protected:
			static bool		ExtractFloat(float& Value,const char* Start,const char** End,bool& PreviousInvalid);
			static bool		ExtractInt(int& Value,const char* Start,const char** End);

		protected:
			ARRAYTYPE	mdata;
	};



	// global case sensitive string compare

	template <typename S,class ARRAYTYPE1,class ARRAYTYPE2>
	inline bool StringCompare(const String2<S,ARRAYTYPE1>& s0, const String2<S,ARRAYTYPE2>& s1, bool case_sensitive = true)
	{
		// first test for equal length
		int size = s0.GetLength();
		if ( size != s1.GetLength() )
			return false;

		const S* v0 = s0;
		const S* v1 = s1;

		if ( case_sensitive )
		{
			for ( int i=0; i<size; ++i )
				if ( v0[i] != v1[i] )
					return false;
		}
		else
		{
			for ( int i=0; i<size; ++i )
				if ( Soy::toupper(v0[i]) != Soy::toupper(v1[i]) )
					return false;
		}
		
		return true;
	}

};

//	gr: BufferString CAN be implemented thusly, but it's a bit complicated for a typename, especially if we want to commonly have varying lengths...
//		typedef Soy::String2<char,BufferArray<char,255> >	BufferString;
//	instead, I've derived a class so we explicitly specify the length (I've NOT used a default... but we could for simplicity)
//	so that we have nice typenames; BufferString<255> BufferString<10> instead of String2<char,BufferArray<char,X> >
//	NO virtuals, so can still use DECLARE_NONCOMPLEX_TYPE on Bufferstrings and structs containing them.
template<uint32 SIZE>	//	SIZE INCLUDES the terminator!
class BufferString : public Soy::String2<char,BufferArray<char,SIZE> >
{
private:
	typedef char S;
    typedef class Soy::String2<char,BufferArray<char,SIZE> > SUPER;
	//	re-implement functions lost with inheritance
public:
	BufferString()	{}
	//BufferString(int size) : Soy::String2(size)	{}
	template<typename THATARRAYTYPE>
	BufferString(const Soy::String2<S,THATARRAYTYPE>& s) :	SUPER( s )	{}
	BufferString(const S* text) : SUPER( text )	{}
	BufferString(const std::string& String) : SUPER( String )	{}

	template<class THATARRAYTYPE>
	BufferString<SIZE>& operator = (const Soy::String2<char,THATARRAYTYPE>& s)
	{
		this->mdata = s.GetArray();
		return *this;
	}

	operator S* ()					{	return this->mdata.GetArray();		}
	operator const S* () const		{	return this->mdata.GetArray();	}
};
	


template <typename S,class ARRAYTYPE>
bool Soy::String2<S,ARRAYTYPE>::ExtractFloat(float& Value,const char* Start,const char** EndPtr,bool& PreviousInvalid)
{
	const char* DummyEnd = NULL;
	const char*& End = EndPtr ? *const_cast<const char**>( EndPtr ) : DummyEnd;

	//	if no end provided, fix it
	if ( !EndPtr )
	{
		DummyEnd = Start + strlen( Start );
	}

	//	strtod takes End with the wrong constness
	PreviousInvalid = false;

	bool DoStrToD = true;

	//	gr: pre-emptive failure check. I've added this as the GetFloatArray of QNAN's in VTMASTER RELEASE ONLY fails... 
	//		it's optimised so the best I can work out is that this function is returning FALSE, but Value==-1.QNAN0 (correct)
	if ( (*Start)=='#' || (*Start)=='Q' )
	{
		DoStrToD = false;
		End = Start;
	}

	//	gr: strtod is slow on long buffers (where strlen(Start) would be very large). 
	//		This has been corrected for GetFloatArray. if you find this function slow outside of GetFloatArray, 
	//		then maybe use GetFloatArray and limit to 1 or crop your string!
	if ( DoStrToD )
	{
		char*& MutableEnd = const_cast<char*&>( End );	//	strtod uses a non-const char* for some C backwards compatibility/incompatbility
		Value = static_cast<float>( strtod( Start, &MutableEnd ) );
	}

	//	if the end hasn't moved on, then strtod couldn't decode the string to a float
	if ( End == Start )
	{
		//	check for special case
		static const char* QNanString = "QNAN0";
		static const size_t QNanStringLen = strlen(QNanString);

		//	in some cases we get #QNAN0, some we get QNAN0 (these are just because of processing differences in GetFloatArray and ExtractFloat)
		int StartOffset = (*Start=='#') ? 1 : 0;

		//	special cases, catch invalid floats
		if ( memcmp( Start+StartOffset, QNanString, QNanStringLen-StartOffset ) == 0 )
		{
			//	previous value was invalid
			PreviousInvalid = true;
			//	dont forget to move past the string
			End += QNanStringLen + StartOffset;
			Value = std::numeric_limits<float>::quiet_NaN();
		}
		else
		{
			//	not a special value, not parseable
			return false;
		}
	}

	//	failed if float can't be parsed
	if ( Value == HUGE_VAL )
		return false;

	return true;
}


template <typename S,class ARRAYTYPE>
bool Soy::String2<S,ARRAYTYPE>::ExtractInt(int& Value,const char* Start,const char** EndPtr)
{
	const char* DummyEnd = NULL;
	const char*& End = EndPtr ? *const_cast<const char**>( EndPtr ) : DummyEnd;

	//	if no end provided, fix it
	if ( !EndPtr )
	{
		DummyEnd = Start + strlen( Start );
	}

	//	strtod can tell us if we failed to extract
	char*& MutableEnd = const_cast<char*&>( End );
	Value = strtol( Start, &MutableEnd, 10 );

	//	if the end hasn't moved on, then strtod couldn't decode the string to a float
	if ( End == Start )
	{
		//	not a special value, not parseable
		return false;
	}

	return true;
}

//------------------------------------------------
//	read multiple floats from this string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
template<class FLOATARRAY>
void Soy::String2<S,ARRAYTYPE>::GetFloatArray(FLOATARRAY& Values) const
{
	const S* Start = mdata.GetArray();
	const S* End = Start;
	while ( Start && *Start != '\0' )
	{
		float ExtractedValue;
		bool PreviousInvalid;

		//	string buffer to speed up strtod - see;
		//		http://www.ryanjuckett.com/programming/c-cplusplus/25-optimizing-atof-and-strtod
		//	rather than re-write strtod or find an alternative (note; C99 has strtof which is apparently much faster, but I can't find source)
		//	we use strtod on a small buffer to eliminate the cost of strlen() on our giant float-string's (ie. stadium's collada vertex lists of thousands of floats)
		String2<S,BufferArray<S,50>> Buffer;
		Buffer.CopyString( Start, Buffer.GetArray().MaxSize()-1 );	//	remember -1 for terminator
		const S* BufferStart = Buffer.GetArray().GetArray();
		const S* BufferEnd = BufferStart;
		if ( !ExtractFloat( ExtractedValue, BufferStart, &BufferEnd, PreviousInvalid ) )
			break;

		//	move along End as much as ExtractFloat would have
		int CharsRead = static_cast<int>( BufferEnd - BufferStart );
		End += CharsRead;

		//	if we stopped at an f (eg. 1.0f) explicitly step over it
		if ( *End == 'f' || *End == 'F' )
			End++;

		//	+1 to step over the character we stopped at (unless it's the terminator)
		if ( *End != '\0' )
			End++;

		//	previous value was invalid, so remove it and add this one to replace it
		if ( PreviousInvalid )
			Values.PopBack();

		//	add float to array
		Values.PushBack( ExtractedValue );

		//	move to next
		Start = End;
	}
}		

//------------------------------------------------
//	to aid splitting strings, iterate from From until we hit a non-whitespace character. Will return terminator, and returns -1 if starting at terminator
//------------------------------------------------
template <typename S,class ARRAYTYPE>
int Soy::String2<S,ARRAYTYPE>::GetNextNonWhitespaceCharacterIndex(uint32 From) const
{
	int LastIndex = GetLength();	//	should be terminator
	if ( LastIndex <= 0 )
		return -1;

	//	searching from terminator results in no more matches
	if ( From >= static_cast<uint32>(LastIndex) )
		return -1;

	//	iterate till non-whitespace
	for ( int i=From;	i<=LastIndex;	i++ )
	{
		if ( !Soy::IsWhitespaceChar( mdata[i] ) )
			return i;
	}

	//	should never reach here, unless From is >LastIndex?
	return -1;
}

//------------------------------------------------
//	to aid splitting strings, iterate from From until we hit a whitespace character. Will return terminator, and returns -1 if starting at terminator
//------------------------------------------------
template <typename S,class ARRAYTYPE>
int Soy::String2<S,ARRAYTYPE>::GetNextWhitespaceCharacterIndex(uint32 From) const
{
	int LastIndex = GetLength();	//	should be terminator
	if ( LastIndex <= 0 )
		return -1;

	//	searching from terminator results in no more matches
	if ( From >= static_cast<uint32>(LastIndex) )
		return -1;

	//	iterate till non-whitespace
	for ( int i=From;	i<=LastIndex;	i++ )
	{
		if ( Soy::IsWhitespaceChar( mdata[i] ) )
			return i;
	}

	//	should never reach here, unless From is >LastIndex?
	return -1;
}


//------------------------------------------------
//	reutnr index of next occurrance of this character. returns -1 if no more
//------------------------------------------------
template <typename S,class ARRAYTYPE>
int Soy::String2<S,ARRAYTYPE>::GetNextChar(const S& Char,int32 From) const
{
	while ( From < GetLength() )
	{
		if ( mdata[From] == Char )
			return From;
		From++;
	}

	//	reached end without finding desired char
	return -1;
}

//------------------------------------------------
//	return index of the last occurrence of this character. returns -1 if non exist
//------------------------------------------------
template <typename S,class ARRAYTYPE>
int Soy::String2<S,ARRAYTYPE>::GetLastChar(const S& Char,int32 From) const
{
	int32 iCandidate(-1);
	while ( From < GetLength() )
	{
		if ( mdata[From] == Char )
			iCandidate = From;
		From++;
	}

	//	reached end without finding desired char
	return iCandidate;
}

//------------------------------------------------
//	trim whitespace (or specific char) from the end of the string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
void Soy::String2<S,ARRAYTYPE>::TrimRight(const S& Char)
{
	int NewLength = GetLength();
	
	//	keep trimming last
	while ( NewLength > 0 )
	{
		const S& NextChar = mdata[NewLength-1];
		if ( Char != 0 )
		{
			if ( NextChar != Char )
				break;
		}
		else
		{
			if ( !IsWhitespaceChar( NextChar ) )
				break;
		}

		//	cull this char
		NewLength--;
	}
	
	SetLength( NewLength );
}		


//------------------------------------------------
//	trim whitespace (or specific char) from the start of the string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
void Soy::String2<S,ARRAYTYPE>::TrimLeft(const S& Char)
{
	//	keep trimming last
	while ( !IsEmpty() )
	{
		const S& NextChar = mdata[0];
		if ( Char != 0 )
		{
			if ( NextChar != Char )
				break;
		}
		else
		{
			if ( !IsWhitespaceChar( NextChar ) )
				break;
		}

		//	cull this char
		RemoveAt(0,1);
	}
}		

//------------------------------------------------
//	extract boolean value from a string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
bool Soy::String2<S,ARRAYTYPE>::GetBool(bool& Boolean) const
{
	//	treat an empty string as false, as HTML forms submit checkbox values as checkbox=&
	if ( IsEmpty() )
	{
		Boolean = false;
		return true;
	}

	switch ( mdata[0] )
	{
		case '1':
			if ( mdata[1] != '\0' )
				return false;
		case 't':
		case 'T':
			Boolean = true;
			return true;

		case '0':
			if ( mdata[1] != '\0' )
				return false;
		case 'f':
		case 'F':
			Boolean = false;
			return true;
	}

	//	unhandled char
	return false;
}

//------------------------------------------------
//	read multiple integers from this string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
template<class INTARRAY>
void Soy::String2<S,ARRAYTYPE>::GetIntArray(INTARRAY& Values) const
{
	//	move to first non-whitespace char, strtol only walks over spaces, not commas etc
	int Index = GetNextNonWhitespaceCharacterIndex(0);
	while ( Index >= 0 && Index < GetLength() )
	{
		const S* Start = &mdata[Index];
		S* End = const_cast<S*>( Start );
		long ExtractedValue = strtol( Start, &End, 10 );

		//	if end == start, we failed to parse here
		if ( End == Start )
			break;

		//	add float to array
		Values.PushBack( ExtractedValue );

		//	move along to next non-deliniator
		int EndIndex = static_cast<int>( End - (&mdata[0]) );	//	64->32
		Index = GetNextNonWhitespaceCharacterIndex( EndIndex );
	}
}



//------------------------------------------------
//	replace characters in the string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
void Soy::String2<S,ARRAYTYPE>::Replace(const char& Match,const char& Replacement)
{
	if ( IsEmpty() )
		return;

	S* s = &(*this)[0];
	while ( *s )
	{
		if ( *s == Match )
			*s = Replacement;
		s++;
	}
}

//------------------------------------------------
//	find start of the occurance of this sub string. returns -1 if not found
//------------------------------------------------
template <typename S,class ARRAYTYPE>
int Soy::String2<S,ARRAYTYPE>::FindIndex(const S* text,bool CaseSensitive,int StartPos) const
{
	if ( !text )
		return -1;

	StartPos = ofMin( StartPos, GetLength() );
	StartPos = ofMax( StartPos, 0 );

	for ( int Index=StartPos;	Index<GetLength();	Index++ )
	{
		const S* s = &(*this)[Index];

		//	oddly early...
		if ( *s == '\0' )
			break;

		const S* SubS = s;
		const S* SubText = text;

		while ( true )
		{
			bool Match;
			if ( !CaseSensitive )
				Match = Soy::toupper(*SubText) == Soy::toupper(*SubS);
			else
				Match = (*SubText) == (*SubS);

			if ( !Match )
				break;
		
			SubText++;
			SubS++;

			//	matched all the way up to this point, and out of text, so we have succeeded
			if ( !*SubText )
				return Index;

			//	matched up to now, but ran out of this, we cannot possibly fit another string of text in, this must have been our last chance, and this ended before text
			if ( !*SubS )
				return -1;
		}
	}

	//	ran through the entire string(this) with no matches
	return -1;
}


//------------------------------------------------
//	match the end of this string with another
//------------------------------------------------
template <typename S,class ARRAYTYPE>
bool Soy::String2<S,ARRAYTYPE>::EndsWith(const S* text,bool CaseSensitive) const
{
	if ( !text )
		return false;

	int len = StringLen( text, -1, mdata.MaxAllocSize() );

	//	won't fit or no string to match
	if ( len > GetLength() || len == 0 )
		return false;

	const S* EndStart = &(*this)[GetLength()-len];

	for ( int i=0;	i<len;	i++ )
	{
		bool Match;
		if ( !CaseSensitive )
			Match = Soy::toupper(EndStart[i]) == Soy::toupper(text[i]);
		else
			Match = (EndStart[i] == text[i]);

		if ( !Match )
			return false;
	}

	return true;
}

inline int Soy::StrCaseCmp( const char* a, const char* b )
{
#if defined WIN32 || defined _WIN32
	return( _stricmp( a, b ) );
#else
	return( strcasecmp( a, b ) );
#endif

}


