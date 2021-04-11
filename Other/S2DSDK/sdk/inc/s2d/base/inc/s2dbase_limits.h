//=============================================================================
//
//	[ s2dbase_limits.h ]
//
//	defines for date range limits.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_LIMITS_H__
#define __S2DBASE_LIMITS_H__

//---------------------------------------------------------------------------------------------------------
// characters
#define CHAR_BIT		8				// number of bits in a char
#define SCHAR_MIN		(-128)			// minimum signed char value
#define SCHAR_MAX		127				// maximum signed char value
#define UCHAR_MAX		0xFF			// maximum unsigned char value

#ifdef	_CHAR_UNSIGNED
#define CHAR_MIN		0				// minimum char value
#define	CHAR_MAX		UCHAR_MAX		// maximum char value
#else
#define CHAR_MIN		SCHAR_MIN		// minimum char value
#define	CHAR_MAX		SCHAR_MAX		// maximum char value
#endif
//---------------------------------------------------------------------------------------------------------
// intergers
#define SHRT_MIN		(-32768)		// minimum (signed) short value
#define SHRT_MAX		32767			// maximum (signed) short value
#define USHRT_MAX		0xFFFF			// maximum unsigned short value
#define INT_MIN			(-2147483647-1)	// minimum (signed) int value
#define INT_MAX			2147483647		// maximum (signed) int value
#define UINT_MAX		0xFFFFFFFF		// maximum unsigned int value
#define LONG_MIN		(-2147483647L-1)// minimum (signed) long value
#define LONG_MAX		2147483647L		// maximum (signed) long value
#define ULONG_MAX		0xffffffffUL	// maximum unsigned long value
//---------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_STRING_H__
