//=============================================================================
//
//	[ s2dengine_common.h ]
//
//	the s2d engine module common defines.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DENGINE_COMMON_H__
#define __S2DENGINE_COMMON_H__

//--------------------------------------------------------------------------------------------------------
#include "../../core/inc/s2dcore.h"
//--------------------------------------------------------------------------------------------------------
//NOTE: max stateset size
#define MAX_STATESET_SIZE			256
//--------------------------------------------------------------------------------------------------------
//NOTE: max textures
#define	MAX_TEXTURES				512		//148B * 512 = 74K
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Engine;
//--------------------------------------------------------------------------------------------------------
enum KeyStates
{
	KS_UP        = 0,			//key up
	KS_DOWN      = 1,			//key down
	KS_FIRSTDOWN = 2,			//key first down
	KS_PRESS     = 4,			//key press
	KS_CLICK     = 8,			//key click

	MAX_KEYSTATES
};
enum MouseStates
{
	MS_UP        = 0,			//mouse up
	MS_DOWN      = 1,			//mouse down
	MS_FIRSTDOWN = 2,			//mouse first down
	MS_PRESS     = 4,			//mouse press
	MS_CLICK     = 8,			//mouse click

	MAX_MOUSESTATES
};
//--------------------------------------------------------------------------------------------------------
enum ANCHOR
{
	ANCHOR_LEFT = 0x1,
	ANCHOR_RIGHT = 0x2,
	ANCHOR_HCENTER = 0x4,

	ANCHOR_TOP = 0x10,
	ANCHOR_BOTTOM = 0x20,
	ANCHOR_VCENTER = 0x40,
};

//--------------------------------------------------------------------------------------------------------
}//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DENGINE_COMMON_H__
