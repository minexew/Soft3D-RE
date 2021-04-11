//=============================================================================
//
//	[ s2dbase_math.inl ]
//
//	the s2d platform independent implement of common math functions
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	abs, pow, srand, rand
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_MATH_INL__
#define __S2DBASE_MATH_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
// calculates the absolute value
inline s32 Abs( s32 n )
{
#ifdef _DEBUG
   s32 s = n >> 31;
   return ( ( n ^ s ) - s );
#else
   s32 tmp;
   asm	volatile("LDR r6, %0" : : "m"(n) : "r6");
   asm	volatile("MOVS r7, r6, ASR #31" : : : "r7");
   asm	volatile("EORNE r6, r6, r7" : : : "r6");
   asm	volatile("SUBNE r6, r6, r7" : : : "r6");
   asm			("STR r6, %0" : "=m"(tmp));
   return tmp;
#endif
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_MATH_INL__
