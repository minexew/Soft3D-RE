//=============================================================================
//
//	[ s2dcore_draw2d.inl ]
//
//	the s2d core draw proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DCORE_DRAW2D_INL__
#define __S2DCORE_DRAW2D_INL__

//--------------------------------------------------------------------------------------------------------
#include "s2dcore_texture.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
/// set display mode
inline void		Draw2D::SetDisplayMode(
					s32 wid,						//[in] screen width
					s32 hei							//[in] screen height
				)
{
	m_iScreenWidth = wid;
	m_iScreenHeight = hei;
	SAFE_FREE( m_pFillBuffer );
	m_pFillBuffer = (u8*)malloc( m_iScreenWidth * m_iScreenHeight );
}
//--------------------------------------------------------------------------------------------------------
/// set vram ptr
inline void		Draw2D::SetVRAMPtr(
					Pixel* pVRAM					//[in] video-ram ptr
				)
{
	m_pVRAM = pVRAM;
}
//--------------------------------------------------------------------------------------------------------
/// get vram ptr
inline Pixel*	Draw2D::GetVRAMPtr( void )
{
	return m_pVRAM;
}
//--------------------------------------------------------------------------------------------------------
/// set current texture
inline void		Draw2D::SetTexture(
					Texture* ptex					//[in] texture ptr
				)
{
	if(ptex->GetFormat() & TF_ALPHA){
		m_pTexture = NULL;
		return;	
	}
	m_pTexture = ptex;

	// set colorkey
	if( ptex ) {
		EnableColorKey( ptex->GetHasColorKey() );
		u32 ckey = ptex->GetColorKey();
		SetColorKey( *(Pixel*)&ckey );
	}
}

//----------------------------------------------------------------------------------------------------
/// set alpha channels texture
inline void		Draw2D::SetAlphaChannels(
							 Texture* ptex					//[in] alpha texture ptr
							 )
{
	if(ptex!=NULL){

		if(!(ptex->GetFormat()&TF_ALPHA)){
			m_pAlphaChannel = NULL;
			return;	
		}
		if(ptex->GetWidth()!=m_pTexture->GetWidth() || 
			ptex->GetHeight()!=m_pTexture->GetHeight())
		{
			m_pAlphaChannel = NULL;
			return;	
		}
	}
	m_pAlphaChannel = ptex;
}

//--------------------------------------------------------------------------------------------------------
/// enable color key
inline void		Draw2D::EnableColorKey(
					s32 enable						//[in] enable color key
				)
{
	m_bHasColorKey = enable;
}
//--------------------------------------------------------------------------------------------------------
/// set color key value
inline void		Draw2D::SetColorKey(
					Pixel col						//[in] color key value
				)
{
	m_ColorKey = col;
}
//--------------------------------------------------------------------------------------------------------
/// set overlay blend mode
inline void		Draw2D::SetOverlayBlendMode(
					OverlayBlendMode obm			//[in] overlay blend mode
				)
{
	m_OverlayBlendMode = obm;
}
//----------------------------------------------------------------------------------------------------
/// set flip type
inline void		Draw2D::SetFilpType(
						FLIPTYPE ft						//[in] overlay blend mode
						)
{
	m_FilpType = ft;
}

//----------------------------------------------------------------------------------------------------
/// count current draw function
inline void		Draw2D::CounDrawFunc( 
								bool bStretch				// is stretch
							)
{
	/// have alpha channels
	s32 TrueColorIdx = 0;
	s32 ColorKeyIdx = 0;

	/// true color or palette
	if( ! (m_pTexture->GetFormat()&TF_TRUECOLOR) ){
		TrueColorIdx = 1;
	}

	/// color key
	if( m_bHasColorKey ){
		ColorKeyIdx = 1;
	}else{
		ColorKeyIdx = 0;
	}

	if(m_pAlphaChannel){
		if(bStretch){
			m_CurFunc = m_AlphaFunc[0][TrueColorIdx][m_FilpType];
		}else{
			m_CurFunc = m_AlphaFunc[1][TrueColorIdx][m_FilpType];
		}
	}else{
		if(bStretch){
			m_CurFunc = m_StrFunc[TrueColorIdx][ColorKeyIdx][m_FilpType][m_OverlayBlendMode];
		}else{
			m_CurFunc = m_NoStrFunc[TrueColorIdx][ColorKeyIdx][m_FilpType][m_OverlayBlendMode];
		}
	}

}	//----------------------------------------------------------------------------------------------------
/// Translation
inline void		Draw2D::Translation(
						s32 tx,							// [in] Translation on x coor
						s32 ty							// [in] Translation on y coor
						)
{
	m_TranX = tx;
	m_TranY = ty;
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DCORE_DRAW2D_INL__
