//=============================================================================
//
//	[ s2dbase_string.inl ]
//
//	the s2d local platform independent implement of CRT string.
//
//	PLATFORM INDEPENDENT!!!ƽ̨�޹�
//
//	MUST be compiled as C++ code
//	character sets are limited to Char8(ANSI) & Char16(UNICODE).
//	NOT supported: strcoll, strxfrm, strerror, strtok.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_STRING_INL__
#define __S2DBASE_STRING_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
// CTYPE:
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a space
inline s32		isspace( s32 c )
{
	return	(c == ' ') ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a character
inline s32		isalpha( s32 c )
{
	return	((c >= 'a' && c <= 'z')||(c >= 'A' && c <= 'Z')) ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a digit
inline s32		isdigit( s32 c )
{
	return	(c >= '0' && c <= '9') ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a charactor or a digit
inline s32		isalnum( s32 c )
{
	return (((c >= 'a' && c <= 'z')||(c >= 'A' && c <= 'Z')) || (c >= '0' && c <= '9')) ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is in upper case
inline s32		isupper( s32 c )
{
	return (c >= 'A' && c <= 'Z') ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is in lower case
inline s32		islower( s32 c )
{
	return (c >= 'a' && c <= 'z') ? 1 : 0;
}
//---------------------------------------------------------------------------------------------------------
// convert to upper case
inline s32		toupper( s32 c )
{

	return (c >= 'a' && c <= 'z') ? (c + 'A' - 'a') : c;
}
//---------------------------------------------------------------------------------------------------------
// convert to lower case
inline s32		tolower( s32 c )
{
	return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}
//---------------------------------------------------------------------------------------------------------
// Char8( ANSI ):
//---------------------------------------------------------------------------------------------------------
// convert a string to an integer value using number base 10
inline s32		atoi( const Char8* str )
{
	return atol(str);
}
//---------------------------------------------------------------------------------------------------------
// Char16( UNICODE ):
//---------------------------------------------------------------------------------------------------------
// convert a string to an integer value using number base 10
inline s32		atoi( const Char16* str )
{
	return atol(str);
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_STRING_INL__
