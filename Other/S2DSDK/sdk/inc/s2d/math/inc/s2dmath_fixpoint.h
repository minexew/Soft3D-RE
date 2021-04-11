//=============================================================================
//
//	[ s2dmath_fixpoint.h ]
//
//	the s2d fixpoint proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_FIXPOINT_H__
#define __S2DMATH_FIXPOINT_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dmath_common.h"
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
//NOTE: these macros are for FLOAT type ONLY! so NEVER pass a FixPoint to them!
#define Deg2Ang( deg )		((short)((short)(deg * S2D_ANG_PI/180.0f) & S2D_ANG_FULLMASK))
#define Rad2Ang( rad )		((short)((short)(rad * S2D_ANG_PI/S2D_PI) & S2D_ANG_FULLMASK))
#define Ang2Deg( ang )		(F2FP((float)ang * 180.0f/S2D_ANG_PI))
#define Ang2Rad( ang )		(F2FP((float)ang * S2D_PI/S2D_ANG_PI))
#define Deg2Rad( deg )		(F2FP(deg * S2D_PI/180.0f))
#define Rad2Deg( rad )		(F2FP(rad * 180.0f/S2D_PI))
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
int round( const FixPoint& fp );

FixPoint fabs	( const FixPoint& fp );
FixPoint fabsf	( const FixPoint& fp );
int	 ceil		( const FixPoint& fp );
int	 ceilf		( const FixPoint& fp );
int	 floor		( const FixPoint& fp );
int	 floorf		( const FixPoint& fp );
FixPoint frac	( const FixPoint& fp );
FixPoint sqrt	( FixPoint fp );
FixPoint sqrtf	( const FixPoint& fp );
FixPoint rsqrt	( FixPoint fp );
FixPoint rsqrtf	( const FixPoint& fp );

/// following func take Angles, use Rad/Deg2Ang() to ge Angles
FixPoint sin	( short ang );
FixPoint sinf	( short ang );
FixPoint cos	( short ang );
FixPoint cosf	( short ang );
FixPoint tan	( short ang );
FixPoint tanf	( short ang );

/// following func return Angles
short asin	( const FixPoint& fp );
short asinf	( const FixPoint& fp );
short acos	( const FixPoint& fp );
short acosf	( const FixPoint& fp );
short atan	( const FixPoint& fp );
short atanf	( const FixPoint& fp );

/// faster than I2FP(1)/fp
FixPoint finv( const FixPoint& fp );

/// return: (1<<30) / data, using a Div LUT. data must be less than 16 bits.
long invsp( unsigned long data );

/// return: (1<<32) / data, using a Div LUT.
// no sign
long invsp_u16( unsigned long data );
long invsp_u17( unsigned long data );
long invsp_u18( unsigned long data );
long invsp_u19( unsigned long data );
long invsp_u20( unsigned long data );
long invsp_u21( unsigned long data );
long invsp_u22( unsigned long data );
long invsp_u23( unsigned long data );
long invsp_u24( unsigned long data );
long invsp_u25( unsigned long data );
long invsp_u26( unsigned long data );
long invsp_u27( unsigned long data );
long invsp_u28( unsigned long data );
long invsp_u29( unsigned long data );
long invsp_u30( unsigned long data );
long invsp_u31( unsigned long data );
long invsp_u32( unsigned long data );
// with sign
long invsp_s16( long data );
long invsp_s17( long data );
long invsp_s18( long data );
long invsp_s19( long data );
long invsp_s20( long data );
long invsp_s21( long data );
long invsp_s22( long data );
long invsp_s23( long data );
long invsp_s24( long data );
long invsp_s25( long data );
long invsp_s26( long data );
long invsp_s27( long data );
long invsp_s28( long data );
long invsp_s29( long data );
long invsp_s30( long data );
long invsp_s31( long data );
long invsp_s32( long data );
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dmath_fixpoint.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_FIXPOINT_H__
