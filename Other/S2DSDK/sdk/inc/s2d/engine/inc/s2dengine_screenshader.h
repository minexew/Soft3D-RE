//=============================================================================
//
//	[ s2dengine_screenshader.h ]
//
//	the s2d screen shader of engine proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DENGINE_SCREENSHADER_H__
#define __S2DENGINE_SCREENSHADER_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dengine_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class FadeShader : public BasicShader
{
public:
	FadeShader( Pixel* ptr, s32 wid, s32 hei );

	~FadeShader( void );

	/// start fade
	void			StartFade(
						s32 fade_in, 					//[in] is fade in
						s32 fadeSpeed,					//[in] fade speed, count in frame
						Pixel color					//[in] fade color
					);

	/// render
	void			Render( void );

	/// is active
	bool			IsActive( void );

private:
	s32				m_iPow;
	s32				m_iFrameCnt;
	s32				m_iMaxFrameCnt;
	bool			m_bFadeOut;
	bool			m_bStarted;
	bool			m_bCompleted;
	s16*			m_StepColor;
	s16*			m_CurColor;
	Pixel			m_FadeColor;
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DENGINE_SCREENSHADER_H__
