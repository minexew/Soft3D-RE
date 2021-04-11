//=============================================================================
//
//	[ s2dbase_math.h ]
//
//	the s2d platform independent implement of common math functions.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Abs, pow, qsort, random
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_MATH_H__
#define __S2DBASE_MATH_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MAX			0x7fff
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
// calculates the absolute value
s32 Abs( s32 n );
//--------------------------------------------------------------------------------------------------------
// calculates x raised to the power of y
s32 pow( s32 x, s32 y );
//--------------------------------------------------------------------------------------------------------
// quick sort, num = number of elements, width = size of a single element, comp = compare func
void qsort( void* base, s32 num, s32 width, s32(*comp)(const void*, const void*) );
//--------------------------------------------------------------------------------------------------------
//NOTE: include a class to hold the seed, as on some platforms we can't use writeable static data
class RandomGenerator
{
public:
	// sets a random starting point
	void srand( u32 _seed );

	// generates a pseudorandom number
	s32 rand( void );

private:
	s32 seed;
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_math.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_MATH_H__
