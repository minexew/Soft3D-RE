//=============================================================================
//
//	[ s2dbase_string.h ]
//
//	the s2d local platform independent implement of CRT string.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	MUST be compiled as C++ code
//	character sets are limited to Char8(ANSI) & Char16(UNICODE).
//	NOT supported: strcoll, strxfrm, strerror, strtok.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_STRING_H__
#define __S2DBASE_STRING_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
// CTYPE:
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a space
s32		isspace( s32 c );
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a character
s32		isalpha( s32 c );
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a digit
s32		isdigit( s32 c );
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is a charactor or a digit
s32		isalnum( s32 c );
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is in upper case
s32		isupper( s32 c );
//---------------------------------------------------------------------------------------------------------
// return non-zero value if c is in lower case
s32		islower( s32 c );
//---------------------------------------------------------------------------------------------------------
// convert to upper case
s32		toupper( s32 c );
//---------------------------------------------------------------------------------------------------------
// convert to lower case
s32		tolower( s32 c );
//---------------------------------------------------------------------------------------------------------
// Conversion:
//---------------------------------------------------------------------------------------------------------
// cast from an ansi string to an unicode string, NO overflow checking
Char16*	strcast( Char16* dest, const Char8* src );
//---------------------------------------------------------------------------------------------------------
// cast from an unicode string to an ansi string, NO overflow checking
Char8*	strcast( Char8* dest, const Char16* src );
//---------------------------------------------------------------------------------------------------------
// Char8( ANSI ):
//---------------------------------------------------------------------------------------------------------
// get the length of a string, '\0' NOT included
u32		strlen( const Char8* str );
//---------------------------------------------------------------------------------------------------------
// copy one string to overwrite another, NO overflow checking
Char8*	strcpy( Char8* dest, const Char8* src );
//---------------------------------------------------------------------------------------------------------
// copy a certain amount of characters of one string to overwrite another, NO overflow checking
Char8*	strncpy( Char8* dest, const Char8* src, u32 len );
//---------------------------------------------------------------------------------------------------------
// append one string to the tail of another, NO overflow checking
Char8*	strcat( Char8* dest, const Char8* src );
//---------------------------------------------------------------------------------------------------------
// append a certain amount of characters of one string to the tail of another, NO overflow checking
Char8*	strncat( Char8* dest, const Char8* src, u32 len );
//---------------------------------------------------------------------------------------------------------
// compare two strings, return 0 if identical
s32		strcmp( const Char8* str1, const Char8* str2 );
//---------------------------------------------------------------------------------------------------------
// compare two strings in lowercase, return 0 if identical
s32		stricmp( const Char8* str1, const Char8* str2 );
//---------------------------------------------------------------------------------------------------------
// compare a certain amount of characters of two strings, return 0 if identical
s32		strncmp( const Char8* str1, const Char8* str2, u32 len );
//---------------------------------------------------------------------------------------------------------
// compare a certain amount of characters of two strings in lowercase, return 0 if identical
s32		strnicmp( const Char8* str1, const Char8* str2, u32 len );
//---------------------------------------------------------------------------------------------------------
// find a character in a string, return a pointer to the FIRST occurrence
Char8*	strchr( const Char8* str, s32 c );
//---------------------------------------------------------------------------------------------------------
// find a character in a string, return a pointer to the LAST occurrence
Char8*	strrchr( const Char8* str, s32 c );
//---------------------------------------------------------------------------------------------------------
// find a entire substring match in a string, return a pointer to the first occurrence, NULL if not found
Char8*	strstr( const Char8* str, const Char8* sub );
//---------------------------------------------------------------------------------------------------------
// 1st char NOT in set stops search, return an offset to the first occurrence, 0 if not found
u32		strspn( const Char8* str, const Char8* set );
//---------------------------------------------------------------------------------------------------------
// 1st char in set stops search, return an offset to the first occurrence, 0 if not found
u32		strcspn( const Char8* str, const Char8* set );
//---------------------------------------------------------------------------------------------------------
// 1st char in set stops search, return a pointer to the first occurrence, NULL if not found
Char8*	strpbrk( const Char8* str, const Char8* set );
//---------------------------------------------------------------------------------------------------------
// convert a string of given number base to a long-integer value
s32		strtol( const Char8* str, Char8** pend, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert a string of given number base to an unsigned-long value
u32		strtoul( const Char8* str, Char8** pend, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert a string to a double-precision value
f64		strtod( const Char8* str, Char8** pend );
//---------------------------------------------------------------------------------------------------------
// convert a string to an integer value using number base 10
s32		atoi( const Char8* str );
//---------------------------------------------------------------------------------------------------------
// convert a string to a long-integer value using number base 10
s32		atol( const Char8* str );
//---------------------------------------------------------------------------------------------------------
// convert a string to a double-precision value
f64		atof( const Char8* str );
//---------------------------------------------------------------------------------------------------------
// convert an integer value of given number base to a string
Char8*	itoa( s32 val, Char8* str, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert an long-integer value of given number base to a string
Char8*	ltoa( s32 val, Char8* str, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert an unsigned-long value of given number base to a string
Char8*	ultoa( u32 val, Char8* str, s32 base );
//---------------------------------------------------------------------------------------------------------
// Char16( UNICODE ):
//---------------------------------------------------------------------------------------------------------
// get the length of a string, '\0' NOT included
u32		strlen( const Char16* str );
//---------------------------------------------------------------------------------------------------------
// copy one string to overwrite another, NO overflow checking
Char16*	strcpy( Char16* dest, const Char16* src );
//---------------------------------------------------------------------------------------------------------
// copy a certain amount of characters of one string to overwrite another, NO overflow checking
Char16*	strncpy( Char16* dest, const Char16* src, u32 len );
//---------------------------------------------------------------------------------------------------------
// append one string to the tail of another, NO overflow checking
Char16*	strcat( Char16* dest, const Char16* src );
//---------------------------------------------------------------------------------------------------------
// append a certain amount of characters of one string to the tail of another, NO overflow checking
Char16*	strncat( Char16* dest, const Char16* src, u32 len );
//---------------------------------------------------------------------------------------------------------
// compare two strings, return 0 if identical
s32		strcmp( const Char16* str1, const Char16* str2 );
//---------------------------------------------------------------------------------------------------------
// compare two strings in lowercase, return 0 if identical
s32		stricmp( const Char16* str1, const Char16* str2 );
//---------------------------------------------------------------------------------------------------------
// compare a certain amount of characters of two strings, return 0 if identical
s32		strncmp( const Char16* str1, const Char16* str2, u32 len );
//---------------------------------------------------------------------------------------------------------
// compare a certain amount of characters of two strings in lowercase, return 0 if identical
s32		strnicmp( const Char16* str1, const Char16* str2, u32 len );
//---------------------------------------------------------------------------------------------------------
// find a character in a string, return a pointer to the FIRST occurrence
Char16*	strchr( const Char16* str, s32 c );
//---------------------------------------------------------------------------------------------------------
// find a character in a string, return a pointer to the LAST occurrence
Char16*	strrchr( const Char16* str, s32 c );
//---------------------------------------------------------------------------------------------------------
// find a entire substring match in a string, return a pointer to the first occurrence, NULL if not found
Char16*	strstr( const Char16* str, const Char16* sub );
//---------------------------------------------------------------------------------------------------------
// 1st char NOT in set stops search, return an offset to the first occurrence, 0 if not found
u32		strspn( const Char16* str, const Char16* set );
//---------------------------------------------------------------------------------------------------------
// 1st char in set stops search, return an offset to the first occurrence, 0 if not found
u32		strcspn( const Char16* str, const Char16* set );
//---------------------------------------------------------------------------------------------------------
// 1st char in set stops search, return a pointer to the first occurrence, NULL if not found
Char16*	strpbrk( const Char16* str, const Char16* set );
//---------------------------------------------------------------------------------------------------------
// convert a string of given number base to a long-integer value
s32		strtol( const Char16* str, Char16** pend, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert a string of given number base to an unsigned-long value
u32		strtoul( const Char16* str, Char16** pend, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert a string to a double-precision value
f64		strtod( const Char16* str, Char16** pend );
//---------------------------------------------------------------------------------------------------------
// convert a string to an integer value using number base 10
s32		atoi( const Char16* str );
//---------------------------------------------------------------------------------------------------------
// convert a string to a long-integer value using number base 10
s32		atol( const Char16* str );
//---------------------------------------------------------------------------------------------------------
// convert a string to a double-precision value
f64		atof( const Char16* str );
//---------------------------------------------------------------------------------------------------------
// convert an integer value of given number base to a string
Char16*	itoa( s32 val, Char16* str, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert an long-integer value of given number base to a string
Char16*	ltoa( s32 val, Char16* str, s32 base );
//---------------------------------------------------------------------------------------------------------
// convert an unsigned-long value of given number base to a string
Char16*	ultoa( u32 val, Char16* str, s32 base );
//---------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_string.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_STRING_H__
