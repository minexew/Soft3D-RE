//=============================================================================
//
//	[ s2dbase_log.h ]
//
//	the s2d platform independent implement of log files.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	you must turn on LOG_OUT macro to support log file,
//	then use: LOG_ON to enable log, and LOG to output messages.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================


#ifndef __S2DBASE_LOG_H__
#define __S2DBASE_LOG_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//---------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
struct Log
{
	/// create output file
	static void		On( void );

	/// output debug message to file
	static void		Output( const Char* msg, ... );
};
//---------------------------------------------------------------------------------------------------------
#if LOG_OUT
#define LOG_ON			Log::On();
#define LOG				Log::Output
#else
	#define LOG_ON
	#ifdef __GNUC__
		inline void NullLog( const Char* msg, ... )
		{
		}
		#define LOG	NullLog
	#else
		#define LOG
	#endif
#endif
//---------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DBASE_LOG_H__
