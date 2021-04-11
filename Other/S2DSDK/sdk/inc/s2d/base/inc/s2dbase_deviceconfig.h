//=============================================================================
//
//	[ s2dbase_crtwrapper.h ]
//
//	the S3D crt wrapper.
//
//	if you want to add crt based code into s3d, include this file at the
//	beginning of all the source files.
//
//	2006 @ Team S3D, China
//	contact: SilverSmoke2000@hotmail.com
//
//=============================================================================

#ifndef __S2DBASE_DEVICECONFIG_H__
#define __S2DBASE_DEVICECONFIG_H__
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"

namespace s2d
{
enum LanguageType
{
	SimplifiedChinese,
	TraditionalChinese,
	English,
};

/// get current language set
LanguageType GetCurLanguage( void );

};
#endif//__S2DBASE_DEVICECONFIG_H__
