//=============================================================================
//
//	[ s2dbase_memory.h ]
//
//	the s2d local implement of CRT memory.
//
//	PLATFORM DEPENDENT!!!平台相关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_MEMORY_H__
#define __S2DBASE_MEMORY_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
// allocate memory blocks
void*	malloc( u32 size );
//---------------------------------------------------------------------------------------------------------
// allocate an array in memory with elements initialized to 0
void*	calloc( u32 count, u32 size );
//---------------------------------------------------------------------------------------------------------
// reallocate memory blocks, return the new address
void*	realloc( void* mem, u32 size );
//---------------------------------------------------------------------------------------------------------
// frees a memory block
void	free( void* mem );
//---------------------------------------------------------------------------------------------------------
// copy characters between buffers
void*	memcpy( void* dest, const void* src, u32 size );
//---------------------------------------------------------------------------------------------------------
// compare characters in two buffers.
s32		memcmp( const void* mem1, const void* mem2, u32 size );
//---------------------------------------------------------------------------------------------------------
// set buffers to a specified character
void*	memset( void* mem, s32 val, u32 size );
//---------------------------------------------------------------------------------------------------------
// find characters in a buffer
void*	memchr( const void* mem, s32 val, u32 cnt );
//--------------------------------------------------------------------------------------------------------
// moves one buffer to another
void*	memmove( void* dest, const void* src, u32 size );
//---------------------------------------------------------------------------------------------------------
// get total memory (in bytes), return -1 if failed to get
s32		memtotal( void );
//---------------------------------------------------------------------------------------------------------
// get remaining memory (in bytes), return -1 if failed to get
s32		memremain( void );
//---------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DBASE_MEMORY_H__
