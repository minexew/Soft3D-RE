//=============================================================================
//
//	[ s2dmath_common.h ]
//
//	the s2d math module common defines.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_COMMON_H__
#define __S2DMATH_COMMON_H__

//--------------------------------------------------------------------------------------------------------
#define S2D_PI					3.141592654f
//--------------------------------------------------------------------------------------------------------
//NOTE: we use angle system, range = [0, S2D_ANGRANGE)
//		S2D_ANGRANGE/4 is PI/2, S2D_ANGRANGE/2 is PI, S2D_ANGRANGE is 2*PI
//		so range is same as [0, 2*PI)
//		we only store [0, PI/2] in the LUT,
//		so a LUT size = (range/4) * 4 / 1024(KB)
//WARN:	S2D_ANGRANGE must <= 0x10000, and must be (1<<SHIFT) style!
#define S2D_ANGRANGE			0x2000	//LUT(sin + tan) = 16K

#define	S2D_ANG_TWOPI			(S2D_ANGRANGE)
#define	S2D_ANG_PI				(S2D_ANG_TWOPI / 2)
#define	S2D_ANG_HALFPI			(S2D_ANG_PI / 2)
#define	S2D_ANG_HHPI			(S2D_ANG_HALFPI / 2)
#define S2D_TWOPI_SHIFT			13
#define S2D_PI_SHIFT			12
#define S2D_HALFPI_SHIFT		11
#define S2D_HHPI_SHIFT			10
#define S2D_ANG_FULLMASK		(S2D_ANG_TWOPI - 1)
#define	S2D_ANG_QUADMASK		(S2D_ANG_HALFPI - 1)
//--------------------------------------------------------------------------------------------------------
#define	AND						&
#define	OR						|
#define	XOR						^
//--------------------------------------------------------------------------------------------------------
//NOTE: use these macros maybe troublesome, use them carefully!
#define SQUARE(a)				((a) * (a))
#define	CUBE(a)					((a)*(a)*(a))
#define LERP(a, b, x)			((a) + (x) * ((b) - (a)))
#define SWAP(a, b, t)			{ t = a; a = b; b = t; }
#define	MIN(a, b)				((a) < (b) ? (a) : (b))
#define	MAX(a, b)				((a) > (b) ? (a) : (b))
#define MID(a, b, c)			MAX((a), MIN((b), (c)))
#define	MAXMAX(a, b, c)			((a) > (b) ? MAX (a,c) : MAX (b,c))
#define	MINMIN(a, b, c)			((a) < (b) ? MIN (a,c) : MIN (b,c))
#define SIGN(x)					(((x) >= 0) ? 1 : -1)
#define CLAMP( a, min, max )	if((a) < (min)) (a) = (min); if((a) > (max)) (a) = (max);
//--------------------------------------------------------------------------------------------------------
#include "../../base/inc/s2dbase.h"
//--------------------------------------------------------------------------------------------------------
//epsilon
#if (USE_FIXPOINT == 0)
#define S2D_EPSILON				1.192092896e-07f
#else
#define S2D_EPSILON				(f32(655,0))
#endif
//--------------------------------------------------------------------------------------------------------
//fast rsqrt (error may up to 0.001f)
#if (USE_FIXPOINT == 0)  
#define USE_FASTRSQRT			0
#else
#define USE_FASTRSQRT			1
#endif
//--------------------------------------------------------------------------------------------------------
//fast floating point math functions
#if USE_FIXPOINT
	#include "s2dmath_fixpoint.h"
#else
	#include "s2dmath_fastfloat.h"
#endif
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_COMMON_H__

