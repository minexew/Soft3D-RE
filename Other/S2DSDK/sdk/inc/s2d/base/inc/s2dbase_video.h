//=============================================================================
//
//	[ s2dbase_video.h ]
//
//	the s2d basic interface of video display.
//
//	PLATFORM DEPENDENT!!!平台相关
//
//	use an offscreen backbuffer as the render target,
//	then update it to screen and refresh the display every frame.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_VIDEO_H__
#define __S2DBASE_VIDEO_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
struct VID
{
	void* impl;
};
//--------------------------------------------------------------------------------------------------------
class Pixel;
//--------------------------------------------------------------------------------------------------------
// called once when the engine inits, create all the resources such as backbuffer, font.
// usage: VID* vid = vidinit();
VID*		vidinit( void* userdata [] );
//--------------------------------------------------------------------------------------------------------
// start video rendering, called once when the engine starts running
void		vidstart( VID* vid );
//--------------------------------------------------------------------------------------------------------
// stop video rendering, called once when the engine stops running
void		vidstop( VID* vid );
//--------------------------------------------------------------------------------------------------------
// called once when the engine exits, free up all the resources.
void		videxit( VID* vid );
//--------------------------------------------------------------------------------------------------------
// get the starting address of the backbuffer to write in.
Pixel*		vidptr( VID* vid );
//--------------------------------------------------------------------------------------------------------
// update the backbuffer to screen and refresh the display
void		vidupdate( VID* vid );
//--------------------------------------------------------------------------------------------------------
// draw text at given pos.
void		vidtext( VID* vid, const Char* text, s32 xpos, s32 ypos );
//--------------------------------------------------------------------------------------------------------
// set text color.
void		vidtextcolor( VID* vid, s32 r, s32 g, s32 b );
//--------------------------------------------------------------------------------------------------------
// set display rotate state
void		vidrotat( s32 botat );
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_VIDEO_H__
