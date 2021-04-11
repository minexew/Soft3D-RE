//=============================================================================
//
//	[ s2dbase_exit.h ]
//
//	the s2d local implement of process/thread termination.
//
//	PLATFORM DEPENDENT!!!平台相关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_EXIT_H__
#define __S2DBASE_EXIT_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
// terminate the current process (or thread, depend on your platform implement)
// reason is typically set to 0 to indicate a normal exit, any other value to indicate an error.
void	Exit( s32 reason );
//--------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DBASE_EXIT_H__
