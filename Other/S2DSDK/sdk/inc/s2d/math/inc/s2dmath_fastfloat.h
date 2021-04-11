//=============================================================================
//
//	[ s2dmath_fastfloat.inl ]
//
//	the s2d fastfloat proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#if !USE_FIXPOINT

#ifndef __S2DMATH_FASTFLOAT_H__
#define __S2DMATH_FASTFLOAT_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dmath_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
#if (USE_FASTRSQRT==1)
//NOTE: only apply for IEEE floats!
f32 rsqrt( f32 f );
//--------------------------------------------------------------------------------------------------------
//NOTE: only apply for IEEE floats!
inline f32 rsqrtf( f32 f )
{
	return rsqrt(f);
}
#endif
//--------------------------------------------------------------------------------------------------------
f32 frac( f32 f );
//--------------------------------------------------------------------------------------------------------
#define Deg2Ang( deg )		((short)((short)(deg * S2D_ANG_PI/180.0f) & S2D_ANG_FULLMASK))
#define Rad2Ang( rad )		((short)((short)(rad * S2D_ANG_PI/S2D_PI) & S2D_ANG_FULLMASK))
#define Ang2Deg( ang )		(F2FP((float)ang * 180.0f/S2D_ANG_PI))
#define Ang2Rad( ang )		(F2FP((float)ang * S2D_PI/S2D_ANG_PI))
#define Deg2Rad( deg )		(F2FP(deg * S2D_PI/180.0f))
#define Rad2Deg( rad )		(F2FP(rad * 180.0f/S2D_PI))

/// following func take Angles, use Rad/Deg2Ang() to ge Angles
f32 sin	( short ang );
f32 sinf( short ang );
f32 cos	( short ang );
f32 cosf( short ang );
f32 tan	( short ang );
f32 tanf( short ang );

/// following func return Angles
short asin	( f32 fp );
short asinf	( f32 fp );
short acos	( f32 fp );
short acosf	( f32 fp );
short atan	( f32 fp );
short atanf	( f32 fp );
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dmath_fastfloat.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_FASTFLOAT_H__

#endif//!USE_FIXPOINT
