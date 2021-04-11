//=============================================================================
//
//	[ s2dbase_common.h ]
//
//	the s2d base module common defines.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_COMMON_H__
#define __S2DBASE_COMMON_H__

//--------------------------------------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning (disable:4786)//too long class name
#endif
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_options.h"
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_limits.h"
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_platform.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
typedef signed char				s8;
typedef signed short			s16;
typedef signed int				s32;

typedef unsigned char			u8;
typedef unsigned short			u16;
typedef unsigned int			u32;
//--------------------------------------------------------------------------------------------------------
//NOTE: character data type
typedef char					Char8;
typedef unsigned short			Char16;

#if defined(_UNICODE)
typedef Char16					Char;
#else
typedef Char8					Char;
#endif
//--------------------------------------------------------------------------------------------------------
//NOTE: const characters convert to Char
#if defined(_UNICODE)
#define _LS(a)					((Char*)L ## a)	// for string
#define _LC(a)					(L##a)			// for character
#else
#define _LS(a)					((Char*)a)
#define _LC(a)					(a)
#endif
//--------------------------------------------------------------------------------------------------------
enum result
{
	S2D_OK = 0,					// No error.

	S2D_MATH,					// Math exception.
	S2D_PARAM,					// Invalid parameters.
	S2D_FILE,					// File not exist.
	S2D_MEMORY,					// Out of memory.
	S2D_ACCESS,					// Invalid memory access.
	S2D_FORMAT,					// Invalid data format.
	S2D_STATE,					// Invalid render state.

	S2D_UNKNOWN					// Unknown error.
};
//--------------------------------------------------------------------------------------------------------
enum Keys
{
	KEY_NOTUSED = 0,			//not used

	KEY_LF,						//left
	KEY_RT,						//right
	KEY_UP,						//up
	KEY_DN,						//down
	KEY_L1,						//L1
	KEY_R1,						//R1
	KEY_L2,						//L2
	KEY_R2,						//R2
	KEY_OK,						//ok
	KEY_0,						//0
	KEY_1,						//1
	KEY_2,						//2
	KEY_3,						//3
	KEY_4,						//4
	KEY_5,						//5
	KEY_6,						//6
	KEY_7,						//7
	KEY_8,						//8
	KEY_9,						//9

	MAX_KEYS
};
enum MouseButton
{
		 

	LBUTTON = 0,						//left
	MBUTTON, 						//mid
	RBUTTON,

	MAX_MBUTTON
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#ifndef NULL
#define NULL					0
#endif

#define SAFE_FREE( p )			{ if( p ) { free(p);    p = 0; } }
#define SAFE_DELETE( p )		{ if( p ) { delete p;   p = 0; } }
#define SAFE_DELETE_ARRAY( p )	{ if( p ) { delete[] p; p = 0; } }

#ifdef _DEBUG
#define assert(p)				{ if( !(p) ) { int i = 0; i = i / i; } }
#else
#define assert(p)
#endif

#define TODO					assert(0);

#define GET_ALIGN( a, align )	(((a) + ((align)-1)) & ~((align)-1))
#define GET_ALIGN_ADDR( a )		(((long)&(a)[0] + CPU_CACHE_ALIGN - 1) & ~(CPU_CACHE_ALIGN - 1))
//-------------------------------------------------------------------------------------------------------
// structure alignment
#ifdef _DEBUG
#define ALIGN(x)
#else
#define ALIGN(x)				__attribute__((packed, aligned(x)))
#endif
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_int64.h"
//--------------------------------------------------------------------------------------------------------
#if USE_FIXPOINT
	#include "s2dbase_fixpoint.h"
#endif
#include <stdlib.h>
#include <memory.h>
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
#if USE_FIXPOINT
	typedef FixPoint			f32;
	typedef FixPoint			f64;
#else
	typedef float				f32;
	typedef double				f64;

	#define F2FP(f)				(f)
	#define FP2F(f)				(f)
	#define I2FP(i)				((f32)(i))
#endif
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_COMMON_H__
