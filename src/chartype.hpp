/*
	Twilight Prophecy SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2003 Twilight 3D Finland Oy Ltd.
*/
#ifndef Soy_CHARTYPE_HPP
#define Soy_CHARTYPE_HPP


#if defined(_MSC_VER) && (_MSC_VER <= 1200) && !defined(_STLPORT_VERSION)
#define Soy_CHARTYPE_DISABLE1
#endif
#define Soy_CHARTYPE_DISABLE1

#include <cwctype>


namespace Soy
{

	// case conversion templates

	template <typename S>
	inline S tolower(S v)
	{
		return v;
	}

	template <typename S>
	inline S toupper(S v)
	{
		return v;
	}

	template <>
	inline char tolower(char v)
	{
		return (v >= 'A' && v <= 'Z') ? static_cast<char>(v | 0x20) : v;
	}

	template <>
	inline char toupper(char v)
	{
		return (v >= 'a' && v <= 'z') ? static_cast<char>(v & 0xdf) : v;
	}

	#ifndef Soy_CHARTYPE_DISABLE1

	template <>
	inline uchar16 tolower(uchar16 v)
	{
		return ::towlower(v);
	}

	template <>
	inline uchar16 toupper(uchar16 v)
	{
		return ::towupper(v);
	}

	#endif

} // namespace Soy


#ifdef Soy_CHARTYPE_DISABLE1
#undef Soy_CHARTYPE_DISABLE1
#endif


#endif
