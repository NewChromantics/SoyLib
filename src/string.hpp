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
#include <limits>
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
	int			StringLen(const S* a,ssize_t Max,size_t AllocMax)
	{
		if ( size_cast<ssize_t>(AllocMax) < Max && AllocMax > 0 )
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

	

		//	copy[part of] a string. if MaxLength < 0 we copy the whole string
		inline void CopyString(const S* text,ssize_t MaxLength=-1)
		{
			if ( !text )
			{
				mdata.ResetIndex();
				mdata.PushBack(0);
				return;
			}

			//	calc how much to copy
			auto len = StringLen( text, MaxLength, mdata.MaxSize()-1 );

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
			auto count = s.GetLength();
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
			auto len = StringLen( text, -1, mdata.MaxSize() );
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
			auto count = GetLength();

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

		size_t GetLength() const
		{
			return mdata.GetSize() - 1;
		}

		S* SetLength(int length)
		{
			mdata.SetSize(length + 1, true, true );
			mdata[length] = 0;
			return mdata.GetArray();
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
	auto ValueLong = strtol( Start, &MutableEnd, 10 );
	assert( ValueLong <= std::numeric_limits<int>::max() );
	Value = static_cast<int>( ValueLong );

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
//	read multiple integers from this string
//------------------------------------------------
template <typename S,class ARRAYTYPE>
template<class INTARRAY>
void Soy::String2<S,ARRAYTYPE>::GetIntArray(INTARRAY& Values) const
{
	/*
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
	 */
}



