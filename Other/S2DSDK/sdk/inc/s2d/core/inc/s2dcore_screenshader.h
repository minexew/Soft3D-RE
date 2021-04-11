//=============================================================================
//
//	[ s2dcore_screenshader.h ]
//
//	the s2d core screen shader proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DCORE_SCREENSHADER_H__
#define __S2DCORE_SCREENSHADER_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dcore_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
// BasicShader:
//--------------------------------------------------------------------------------------------------------
class BasicShader
{
public:
	BasicShader( Pixel* ptr, s32 wid, s32 hei ) : m_pVRAM(ptr), m_iScreenWidth(wid), m_iScreenHeight(hei) {}
	virtual ~BasicShader( void ) {}

	/// render
	virtual void	Render( void ) = 0;

protected:
	Pixel*			m_pVRAM;

	s32				m_iScreenWidth;
	s32				m_iScreenHeight;
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DCORE_SCREENSHADER_H__
