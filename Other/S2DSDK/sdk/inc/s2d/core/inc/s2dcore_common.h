//=============================================================================
//
//	[ s2dcore_common.h ]
//
//	the s2d core module common defines.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DCORE_COMMON_H__
#define __S2DCORE_COMMON_H__

//---------------------------------------------------------------------------------------------------------
#include "../../math/inc/s2dmath.h"
//---------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
// texture format
const u16 TF_ALPHA		= 0x8000;	// alpha texture
const u16 TF_COLOR		= 0;		// color texture

// color format
const u16 TF_TRUECOLOR	= 0x4000;	// true color
const u16 TF_PALLETE	= 0;		// pallete

// pallete format
const u16 TF_G_I8		= 0x2000;	// 8-bits indexed color ( global pallete )
const u16 TF_L_I8		= 0;		// 8-bits indexed color ( local pallete )

// pixel format
const u16 TF_RGB332		= RGB332;	// RGB332 ( 1 byte  )
const u16 TF_RGB444		= RGB444;	// RGB444 ( 2 bytes )
const u16 TF_RGB555		= RGB555;	// RGB555 ( 2 bytes )
const u16 TF_RGB565		= RGB565;	// RGB565 ( 2 bytes )
const u16 TF_RGB666		= RGB666;	// RGB666 ( 3 bytes )
const u16 TF_RGB888		= RGB888;	// RGB888 ( 3 bytes )
//---------------------------------------------------------------------------------------------------------
enum OverlayBlendMode
{
	OBM_NONE = 0,
	OBM_ADD,
	OBM_SUB,
	OBM_SRC25,
	OBM_SRC50,
	OBM_SRC75,

	MAX_OBMS
};
//---------------------------------------------------------------------------------------------------------
struct Rect
{
	s32			left;
	s32			top;
	s32			right;
	s32			bottom;
};
//---------------------------------------------------------------------------------------------------------
struct Point
{
	Point( void ) { x = 0; y = 0; }
	Point( s32 _x, s32 _y ) { x = _x; y = _y; }
	s32			x;
	s32			y;
};
//--------------------------------------------------------------------------------------------------------
enum FLIPTYPE
{
	FLIP_NONE = 0,
	FLIP_VERTICAL,
	FLIP_HORIZONTAL,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270,

	MAX_FLIPTYPE
};

#define MAX_DRAWFUNC	((MAX_OBMS*MAX_FLIPTYPE)<<2)
//---------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DCORE_COMMON_H__
