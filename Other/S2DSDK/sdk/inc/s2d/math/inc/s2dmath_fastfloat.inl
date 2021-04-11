//=============================================================================
//
//	[ s2dmath_fastfloat.h ]
//
//	the s2d fastfloat proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#if !USE_FIXPOINT

#ifndef __S2DMATH_FASTFLOAT_INL__
#define __S2DMATH_FASTFLOAT_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
#if (USE_FASTRSQRT==1)
//NOTE: only apply for IEEE floats!
inline f32 rsqrt( f32 f )
{
    f32 fHalf = 0.5f * f;

    u32 i = *(u32*)&f;
    i = 0x5f3759df - (i >> 1);

    f = *(f32*)&i;
    f = f * (1.5f - fHalf*f*f);

    return f;
}
#endif
//--------------------------------------------------------------------------------------------------------
inline f32 frac( f32 f )
{
	return (f - (u32)f);
}
//--------------------------------------------------------------------------------------------------------
extern const f32 SinLUT[ S2D_ANG_HALFPI+1 ];
inline f32 sin( int ang )
{
#ifdef FP_VALIDATE
	assert( ang < S2D_ANGRANGE );
#endif

	if( ang & S2D_ANG_HALFPI ) {
        if ( ang & S2D_ANG_PI )
            return -SinLUT[ S2D_ANG_HALFPI - (ang & S2D_ANG_QUADMASK) ];//sin(3PI/2+a) = -cos(a)	IV
        else
        	return SinLUT[ S2D_ANG_HALFPI - (ang & S2D_ANG_QUADMASK) ];	//sin(PI/2+a) = cos(a)		II
    }else{
        if ( ang & S2D_ANG_PI )
            return -SinLUT[ ang & S2D_ANG_QUADMASK ];					//sin(PI+a) = -sin(a)		III
        else
            return SinLUT[ ang & S2D_ANG_QUADMASK ];					//sin(a)					I
    }
}
//--------------------------------------------------------------------------------------------------------
inline f32 sinf( s32 ang )
{
	return sin( ang );
}
//--------------------------------------------------------------------------------------------------------
inline f32 cos( s32 ang )
{
	return sin( ang + S2D_ANG_HALFPI );
}
//--------------------------------------------------------------------------------------------------------
inline f32 cosf( s32 ang )
{
	return cos( ang );
}
//--------------------------------------------------------------------------------------------------------
extern const f32 TanLUT[ S2D_ANG_HALFPI+1 ];
inline f32 tan( s32 ang )
{
#ifdef FP_VALIDATE
	assert( ang < S2D_ANGRANGE );
#endif

	if( ang & S2D_ANG_HALFPI ){
        return -TanLUT[S2D_ANG_HALFPI - (ang & S2D_ANG_QUADMASK)];	//tg(PI/2+a) = -ctg(a)		II
																	//tg(3PI/2+a) = -ctg(a)		IV
    }else{
        return TanLUT[ang & S2D_ANG_QUADMASK];						//tg(a)						I
																	//tg(PI+a) = tg(a)			III
    }
}
//--------------------------------------------------------------------------------------------------------
inline f32 tanf( s32 ang )
{
	return tan( ang );
}
//--------------------------------------------------------------------------------------------------------
inline s32 asinf	( f32 fp )
{
	return asin( fp );
}
//--------------------------------------------------------------------------------------------------------
inline s32 acosf	( f32 fp )
{
	return acos( fp );
}
//--------------------------------------------------------------------------------------------------------
inline s32 atanf	( f32 fp )
{
	return atan( fp );
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DMATH_FASTFLOAT_INL__

#endif//!USE_FIXPOINT

