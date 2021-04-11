//=============================================================================
//
//	[ s2dcore_draw2d.h ]
//
//	the s2d core draw proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DCORE_DRAW2D_H__
#define __S2DCORE_DRAW2D_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dcore_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Texture;
//--------------------------------------------------------------------------------------------------------
class Draw2D
{
//--------------------------------------------------------------------------------------------------------
public:
	Draw2D( void );
	~Draw2D(void)
	{
		SAFE_FREE( m_pFillBuffer );
	}
//--------------------------------------------------------------------------------------------------------
//### interfaces:
public:
	//----------------------------------------------------------------------------------------------------
	/// set display mode
	void		SetDisplayMode(
					s32 wid,						//[in] screen width
					s32 hei							//[in] screen height
				);
	//----------------------------------------------------------------------------------------------------
	/// set vram ptr
	void		SetVRAMPtr(
					Pixel* pVRAM					//[in] video-ram ptr
				);
	//----------------------------------------------------------------------------------------------------
	/// get vram ptr
	Pixel*		GetVRAMPtr( void );
	//----------------------------------------------------------------------------------------------------
	/// set current texture
	void		SetTexture(
					Texture* ptex					//[in] texture ptr
				);
	//----------------------------------------------------------------------------------------------------
	/// set alpha channels texture
	void		SetAlphaChannels(
					Texture* ptex					//[in] alpha texture ptr
				);
	//----------------------------------------------------------------------------------------------------
	/// enable color key
	void		EnableColorKey(
					s32 enable						//[in] enable color key
				);
	//----------------------------------------------------------------------------------------------------
	/// set color key value
	void		SetColorKey(
					Pixel col						//[in] color key value
				);
	//----------------------------------------------------------------------------------------------------
	/// set overlay blend mode
	void		SetOverlayBlendMode(
					OverlayBlendMode obm			//[in] overlay blend mode
				);

	//----------------------------------------------------------------------------------------------------
	/// set flip type
	void		SetFilpType(
					FLIPTYPE ft						//[in] overlay blend mode
				);

	//----------------------------------------------------------------------------------------------------
	/// fill vram with given color
	void		Clear(
					u8 r,							//[in] fill color
					u8 g,							//[in] ..
					u8 b							//[in] ..
				);
	//----------------------------------------------------------------------------------------------------
	/// draw 2d true color overlay after entire scene was rendered (NO stretch)
	void		DrawOverlay(
					s32 x,							//[in] left-up corner x-pos in screen space
					s32 y,							//[in] left-up corner y-pos in screen space
					Rect* pRect = NULL				//[in] source rect (NULL:whole texture)
				);
	//----------------------------------------------------------------------------------------------------
	/// draw 2d true color overlay after entire scene was rendered (stretch)
	void		DrawStretchOverlay(
					Rect& destR,					// [in] screen rect
					Rect* pRect = NULL				// [in] texture rect (NULL:whole texture)
				);

	//----------------------------------------------------------------------------------------------------
	/// Translation
	void		Translation(
					s32 tx,							// [in] Translation on x coor
					s32 ty							// [in] Translation on y coor
				);

	//----------------------------------------------------------------------------------------------------
	void		FillRect( Rect &rect, Pixel &color, OverlayBlendMode obm );

	//----------------------------------------------------------------------------------------------------
	void		FillCircle( Point &centre, s32 radius, Pixel &color, OverlayBlendMode obm );

	//----------------------------------------------------------------------------------------------------
	void		FillTriangle( Point *pPoint, Pixel &color, OverlayBlendMode obm );

//--------------------------------------------------------------------------------------------------------
//### data members:
private:
	/// display mode
	s32					m_iScreenWidth;
	s32					m_iScreenHeight;

	/// vram ptr
	Pixel*				m_pVRAM;

	/// z buffer
	u8*					m_pFillBuffer;

	/// current texture
	Texture*			m_pTexture;

	/// alpha channels
	Texture*			m_pAlphaChannel;

	/// color key
	s32					m_bHasColorKey;
	Pixel				m_ColorKey;

	/// blend mode
	OverlayBlendMode	m_OverlayBlendMode;

	/// filp type 
	FLIPTYPE			m_FilpType;

	/// 2d rendering params
	Pixel*				m_pSrcOverlay;
	Pixel*				m_pDstOverlay;
	u8*					m_pAlphaOverlay;
	u8*					m_pPalIdxOverlay;

	Rect				m_SrcRect;
	Rect				m_DestRect;

	f32					m_fScaleX;
	f32					m_fScaleY;
	
	s32					m_TranX;
	s32					m_TranY;

	Pixel				m_FillColor;

	/// function table
	typedef void	(Draw2D::* DrawFunc) ( void );

	DrawFunc			m_NoStrFunc[2][2][MAX_FLIPTYPE][MAX_OBMS];
	DrawFunc			m_StrFunc[2][2][MAX_FLIPTYPE][MAX_OBMS];
	DrawFunc			m_AlphaFunc[2][2][MAX_FLIPTYPE];
	DrawFunc			m_CurFunc;

//--------------------------------------------------------------------------------------------------------
//### draw overlay functions:
private:
	
	//----------------------------------------------------------------------------------------------------
	/// count current draw function
	void		CounDrawFunc( 
								bool bStretch				// is stretch
							);

	//----------------------------------------------------------------------------------------------------
	//  no stretch function
	//----------------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , no flip
	void		DrawOL_True_NoStretch_NoKey_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , no flip
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , no flip
	void		DrawOL_True_NoStretch_NoKey_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , no flip
	void		DrawOL_True_NoStretch_NoKey_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , no flip
	void		DrawOL_True_NoStretch_NoKey_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , no flip
	void		DrawOL_True_NoStretch_NoKey_Blend75_FilpNone( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , vertical flip
	void		DrawOL_True_NoStretch_NoKey_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , vertical flip
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , vertical flip
	void		DrawOL_True_NoStretch_NoKey_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , vertical flip
	void		DrawOL_True_NoStretch_NoKey_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , vertical flip
	void		DrawOL_True_NoStretch_NoKey_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , vertical flip
	void		DrawOL_True_NoStretch_NoKey_Blend75_FilpVer( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_Blend75_FilpHor( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , rate 90 
	void		DrawOL_True_NoStretch_NoKey_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , rate 90
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , rate 90
	void		DrawOL_True_NoStretch_NoKey_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , rate 90
	void		DrawOL_True_NoStretch_NoKey_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , rate 90
	void		DrawOL_True_NoStretch_NoKey_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , rate 90
	void		DrawOL_True_NoStretch_NoKey_Blend75_Rate90( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , rate 180
	void		DrawOL_True_NoStretch_NoKey_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , rate 180
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , rate 180
	void		DrawOL_True_NoStretch_NoKey_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , rate 180
	void		DrawOL_True_NoStretch_NoKey_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , rate 180
	void		DrawOL_True_NoStretch_NoKey_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , rate 180
	void		DrawOL_True_NoStretch_NoKey_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image, no stretch, no color key, no blend , rate 270
	void		DrawOL_True_NoStretch_NoKey_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode add , rate 270
	void		DrawOL_True_NoStretch_NoKey_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode sub , rate 270
	void		DrawOL_True_NoStretch_NoKey_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 25% , rate 270
	void		DrawOL_True_NoStretch_NoKey_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 50% , rate 270
	void		DrawOL_True_NoStretch_NoKey_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode 75% , rate 270
	void		DrawOL_True_NoStretch_NoKey_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , no flip
	void		DrawOL_True_NoStretch_Key_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , no flip
	void		DrawOL_True_NoStretch_Key_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , no flip
	void		DrawOL_True_NoStretch_Key_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , no flip
	void		DrawOL_True_NoStretch_Key_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , no flip
	void		DrawOL_True_NoStretch_Key_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , no flip
	void		DrawOL_True_NoStretch_Key_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , vertical flip
	void		DrawOL_True_NoStretch_Key_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , vertical flip
	void		DrawOL_True_NoStretch_Key_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , vertical flip
	void		DrawOL_True_NoStretch_Key_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , vertical flip
	void		DrawOL_True_NoStretch_Key_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , vertical flip
	void		DrawOL_True_NoStretch_Key_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , vertical flip
	void		DrawOL_True_NoStretch_Key_Blend75_FilpVer( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , horizontal flip
	void		DrawOL_True_NoStretch_Key_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , horizontal flip
	void		DrawOL_True_NoStretch_Key_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , horizontal flip
	void		DrawOL_True_NoStretch_Key_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , horizontal flip
	void		DrawOL_True_NoStretch_Key_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , horizontal flip
	void		DrawOL_True_NoStretch_Key_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , horizontal flip
	void		DrawOL_True_NoStretch_Key_Blend75_FilpHor( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , rate 90
	void		DrawOL_True_NoStretch_Key_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , rate 90
	void		DrawOL_True_NoStretch_Key_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , rate 90
	void		DrawOL_True_NoStretch_Key_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , rate 90
	void		DrawOL_True_NoStretch_Key_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , rate 90
	void		DrawOL_True_NoStretch_Key_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , rate 90
	void		DrawOL_True_NoStretch_Key_Blend75_Rate90( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , rate 180
	void		DrawOL_True_NoStretch_Key_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , rate 180
	void		DrawOL_True_NoStretch_Key_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , rate 180
	void		DrawOL_True_NoStretch_Key_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , rate 180
	void		DrawOL_True_NoStretch_Key_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , rate 180
	void		DrawOL_True_NoStretch_Key_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , rate 180
	void		DrawOL_True_NoStretch_Key_Blend75_Rate180( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, no blend , rate 270
	void		DrawOL_True_NoStretch_Key_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode add , rate 270
	void		DrawOL_True_NoStretch_Key_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode sub , rate 270
	void		DrawOL_True_NoStretch_Key_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 25% , rate 270
	void		DrawOL_True_NoStretch_Key_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 50% , rate 270
	void		DrawOL_True_NoStretch_Key_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, color key, blend mode 75% , rate 270
	void		DrawOL_True_NoStretch_Key_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , no flip
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , no flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , no flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , no flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , no flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , no flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_FilpVer( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_FilpHor( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, no blend , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode add , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode sub , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 25% , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 50% , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode 75% , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_Blend75_Rate270( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , no flip
	void		DrawOL_Pal_NoStretch_Key_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , no flip
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , no flip
	void		DrawOL_Pal_NoStretch_Key_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , no flip
	void		DrawOL_Pal_NoStretch_Key_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , no flip
	void		DrawOL_Pal_NoStretch_Key_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , no flip
	void		DrawOL_Pal_NoStretch_Key_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , vertical flip
	void		DrawOL_Pal_NoStretch_Key_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , vertical flip
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , vertical flip
	void		DrawOL_Pal_NoStretch_Key_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , vertical flip
	void		DrawOL_Pal_NoStretch_Key_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , vertical flip
	void		DrawOL_Pal_NoStretch_Key_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , vertical flip
	void		DrawOL_Pal_NoStretch_Key_Blend75_FilpVer( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , horizontal flip
	void		DrawOL_Pal_NoStretch_Key_Blend75_FilpHor( void );



	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , rate 90
	void		DrawOL_Pal_NoStretch_Key_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , rate 90
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , rate 90
	void		DrawOL_Pal_NoStretch_Key_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , rate 90
	void		DrawOL_Pal_NoStretch_Key_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , rate 90
	void		DrawOL_Pal_NoStretch_Key_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , rate 90
	void		DrawOL_Pal_NoStretch_Key_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , rate 180
	void		DrawOL_Pal_NoStretch_Key_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , rate 180
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , rate 180
	void		DrawOL_Pal_NoStretch_Key_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , rate 180
	void		DrawOL_Pal_NoStretch_Key_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , rate 180
	void		DrawOL_Pal_NoStretch_Key_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , rate 180
	void		DrawOL_Pal_NoStretch_Key_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, no blend , rate 270
	void		DrawOL_Pal_NoStretch_Key_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode add , rate 270
	void		DrawOL_Pal_NoStretch_Key_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode sub , rate 270
	void		DrawOL_Pal_NoStretch_Key_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 25% , rate 270
	void		DrawOL_Pal_NoStretch_Key_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 50% , rate 270
	void		DrawOL_Pal_NoStretch_Key_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, color key, blend mode 75% , rate 270
	void		DrawOL_Pal_NoStretch_Key_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	//  stretch function
	//----------------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , no flip
	void		DrawOL_True_Stretch_NoKey_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , no flip
	void		DrawOL_True_Stretch_NoKey_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , no flip
	void		DrawOL_True_Stretch_NoKey_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , no flip
	void		DrawOL_True_Stretch_NoKey_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , no flip
	void		DrawOL_True_Stretch_NoKey_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , no flip
	void		DrawOL_True_Stretch_NoKey_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , vertical flip
	void		DrawOL_True_Stretch_NoKey_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , vertical flip
	void		DrawOL_True_Stretch_NoKey_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , vertical flip
	void		DrawOL_True_Stretch_NoKey_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , vertical flip
	void		DrawOL_True_Stretch_NoKey_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , vertical flip
	void		DrawOL_True_Stretch_NoKey_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , vertical flip
	void		DrawOL_True_Stretch_NoKey_Blend75_FilpVer( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , horizontal flip
	void		DrawOL_True_Stretch_NoKey_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , horizontal flip
	void		DrawOL_True_Stretch_NoKey_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , horizontal flip
	void		DrawOL_True_Stretch_NoKey_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , horizontal flip
	void		DrawOL_True_Stretch_NoKey_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , horizontal flip
	void		DrawOL_True_Stretch_NoKey_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , horizontal flip
	void		DrawOL_True_Stretch_NoKey_Blend75_FilpHor( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , rate 90
	void		DrawOL_True_Stretch_NoKey_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , rate 90
	void		DrawOL_True_Stretch_NoKey_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , rate 90
	void		DrawOL_True_Stretch_NoKey_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , rate 90
	void		DrawOL_True_Stretch_NoKey_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , rate 90
	void		DrawOL_True_Stretch_NoKey_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , rate 90
	void		DrawOL_True_Stretch_NoKey_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , rate 180
	void		DrawOL_True_Stretch_NoKey_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , rate 180
	void		DrawOL_True_Stretch_NoKey_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , rate 180
	void		DrawOL_True_Stretch_NoKey_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , rate 180
	void		DrawOL_True_Stretch_NoKey_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , rate 180
	void		DrawOL_True_Stretch_NoKey_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , rate 180
	void		DrawOL_True_Stretch_NoKey_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, no blend , rate 270
	void		DrawOL_True_Stretch_NoKey_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode Add , rate 270
	void		DrawOL_True_Stretch_NoKey_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode sub , rate 270
	void		DrawOL_True_Stretch_NoKey_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 25% , rate 270
	void		DrawOL_True_Stretch_NoKey_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 50% , rate 270
	void		DrawOL_True_Stretch_NoKey_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode 75% , rate 270
	void		DrawOL_True_Stretch_NoKey_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , no flip
	void		DrawOL_True_Stretch_Key_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , no flip
	void		DrawOL_True_Stretch_Key_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , no flip
	void		DrawOL_True_Stretch_Key_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , no flip
	void		DrawOL_True_Stretch_Key_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , no flip
	void		DrawOL_True_Stretch_Key_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , no flip
	void		DrawOL_True_Stretch_Key_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , vertical flip
	void		DrawOL_True_Stretch_Key_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , vertical flip
	void		DrawOL_True_Stretch_Key_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , vertical flip
	void		DrawOL_True_Stretch_Key_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , vertical flip
	void		DrawOL_True_Stretch_Key_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , vertical flip
	void		DrawOL_True_Stretch_Key_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , vertical flip
	void		DrawOL_True_Stretch_Key_Blend75_FilpVer( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , horizontal flip
	void		DrawOL_True_Stretch_Key_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , horizontal flip
	void		DrawOL_True_Stretch_Key_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , horizontal flip
	void		DrawOL_True_Stretch_Key_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , horizontal flip
	void		DrawOL_True_Stretch_Key_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , horizontal flip
	void		DrawOL_True_Stretch_Key_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , horizontal flip
	void		DrawOL_True_Stretch_Key_Blend75_FilpHor( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , rate 90
	void		DrawOL_True_Stretch_Key_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , rate 90
	void		DrawOL_True_Stretch_Key_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , rate 90
	void		DrawOL_True_Stretch_Key_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , rate 90
	void		DrawOL_True_Stretch_Key_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , rate 90
	void		DrawOL_True_Stretch_Key_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , rate 90
	void		DrawOL_True_Stretch_Key_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , rate 180
	void		DrawOL_True_Stretch_Key_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , rate 180
	void		DrawOL_True_Stretch_Key_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , rate 180
	void		DrawOL_True_Stretch_Key_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , rate 180
	void		DrawOL_True_Stretch_Key_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , rate 180
	void		DrawOL_True_Stretch_Key_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , rate 180
	void		DrawOL_True_Stretch_Key_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, no blend , rate 270
	void		DrawOL_True_Stretch_Key_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode Add , rate 270
	void		DrawOL_True_Stretch_Key_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode sub , rate 270
	void		DrawOL_True_Stretch_Key_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 25% , rate 270
	void		DrawOL_True_Stretch_Key_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 50% , rate 270
	void		DrawOL_True_Stretch_Key_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, color key, blend mode 75% , rate 270
	void		DrawOL_True_Stretch_Key_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , no flip
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , no flip
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , no flip
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , no flip
	void		DrawOL_Pal_Stretch_NoKey_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , no flip
	void		DrawOL_Pal_Stretch_NoKey_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , no flip
	void		DrawOL_Pal_Stretch_NoKey_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_Blend75_FilpVer( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_Blend75_FilpHor( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , rate 90
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , rate 90
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , rate 90
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , rate 90
	void		DrawOL_Pal_Stretch_NoKey_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , rate 90
	void		DrawOL_Pal_Stretch_NoKey_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , rate 90
	void		DrawOL_Pal_Stretch_NoKey_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , rate 180
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , rate 180
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , rate 180
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , rate 180
	void		DrawOL_Pal_Stretch_NoKey_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , rate 180
	void		DrawOL_Pal_Stretch_NoKey_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , rate 180
	void		DrawOL_Pal_Stretch_NoKey_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, no blend , rate 270
	void		DrawOL_Pal_Stretch_NoKey_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode Add , rate 270
	void		DrawOL_Pal_Stretch_NoKey_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode sub , rate 270
	void		DrawOL_Pal_Stretch_NoKey_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , rate 270
	void		DrawOL_Pal_Stretch_NoKey_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , rate 270
	void		DrawOL_Pal_Stretch_NoKey_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , rate 270
	void		DrawOL_Pal_Stretch_NoKey_Blend75_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , no flip
	void		DrawOL_Pal_Stretch_Key_NoBlend_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , no flip
	void	    DrawOL_Pal_Stretch_Key_BlendAdd_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , no flip
	void		DrawOL_Pal_Stretch_Key_BlendSub_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 25% , no flip
	void		DrawOL_Pal_Stretch_Key_Blend25_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 50% , no flip
	void		DrawOL_Pal_Stretch_Key_Blend50_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode 75% , no flip
	void		DrawOL_Pal_Stretch_Key_Blend75_FilpNone( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , vertical flip
	void		DrawOL_Pal_Stretch_Key_NoBlend_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , vertical flip
	void		DrawOL_Pal_Stretch_Key_BlendAdd_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , vertical flip
	void		DrawOL_Pal_Stretch_Key_BlendSub_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 25% , vertical flip
	void		DrawOL_Pal_Stretch_Key_Blend25_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 50% , vertical flip
	void		DrawOL_Pal_Stretch_Key_Blend50_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 75% , vertical flip
	void		DrawOL_Pal_Stretch_Key_Blend75_FilpVer( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , horizontal flip
	void		DrawOL_Pal_Stretch_Key_NoBlend_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , horizontal flip
	void		DrawOL_Pal_Stretch_Key_BlendAdd_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , horizontal flip
	void		DrawOL_Pal_Stretch_Key_BlendSub_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 25% , horizontal flip
	void		DrawOL_Pal_Stretch_Key_Blend25_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 50% , horizontal flip
	void		DrawOL_Pal_Stretch_Key_Blend50_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 75% , horizontal flip
	void		DrawOL_Pal_Stretch_Key_Blend75_FilpHor( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , rate 90
	void		DrawOL_Pal_Stretch_Key_NoBlend_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , rate 90
	void		DrawOL_Pal_Stretch_Key_BlendAdd_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , rate 90
	void		DrawOL_Pal_Stretch_Key_BlendSub_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 25% , rate 90
	void		DrawOL_Pal_Stretch_Key_Blend25_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 50% , rate 90
	void		DrawOL_Pal_Stretch_Key_Blend50_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 75% , rate 90
	void		DrawOL_Pal_Stretch_Key_Blend75_Rate90( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , rate 180
	void		DrawOL_Pal_Stretch_Key_NoBlend_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , rate 180
	void		DrawOL_Pal_Stretch_Key_BlendAdd_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , rate 180
	void		DrawOL_Pal_Stretch_Key_BlendSub_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 25% , rate 180
	void		DrawOL_Pal_Stretch_Key_Blend25_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 50% , rate 180
	void		DrawOL_Pal_Stretch_Key_Blend50_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 75% , rate 180
	void		DrawOL_Pal_Stretch_Key_Blend75_Rate180( void );


	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, no blend , rate 270
	void		DrawOL_Pal_Stretch_Key_NoBlend_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode Add , rate 270
	void		DrawOL_Pal_Stretch_Key_BlendAdd_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode sub , rate 270
	void		DrawOL_Pal_Stretch_Key_BlendSub_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 25% , rate 270
	void		DrawOL_Pal_Stretch_Key_Blend25_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 50% , rate 270
	void		DrawOL_Pal_Stretch_Key_Blend50_Rate270( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, color key, blend mode 75% , rate 270
	void		DrawOL_Pal_Stretch_Key_Blend75_Rate270( void );


	//----------------------------------------------------------------------------------------------------
	//  alpha draw function
	//----------------------------------------------------------------------------------------------------
	
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , no flip
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , vertical flip
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , horizontal flip
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 90
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 180
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 270
	void		DrawOL_True_NoStretch_NoKey_Blend_Channel_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw true color image,stretch, no color key, blend mode alpha Channels 5bit(32 level) , no flip
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , vertical flip
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , horizontal flip
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 90
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 180
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 270
	void		DrawOL_True_Stretch_NoKey_Blend_Channel_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , no flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , vertical flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , horizontal flip
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 90
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 180
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image,no stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 270
	void		DrawOL_Pal_NoStretch_NoKey_Blend_Channel_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode alpha Channels 5bit(32 level) , no flip
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_FilpNone( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image,stretch, no color key, blend mode alpha Channels 5bit(32 level) , vertical flip
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_FilpVer( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , horizontal flip
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_FilpHor( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 90
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_Rate90( void );
	//----------------------------------------------------------------------------------------------------
	/// draw palette color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 180
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_Rate180( void );
	//----------------------------------------------------------------------------------------------------
	/// draw true color image, stretch, no color key, blend mode alpha Channels 5bit(32 level) , rate 270
	void		DrawOL_Pal_Stretch_NoKey_Blend_Channel_Rate270( void );

	//----------------------------------------------------------------------------------------------------
	/// fill rect no blend
	void		FillRect_NoBlend( void );
	//----------------------------------------------------------------------------------------------------
	/// fill rect blend mode Add
	void	    FillRect_BlendAdd( void );
	//----------------------------------------------------------------------------------------------------
	/// fill rect blend mode sub
	void		FillRect_BlendSub( void );
	//----------------------------------------------------------------------------------------------------
	/// fill rect blend mode 25%
	void		FillRect_Blend25( void );
	//----------------------------------------------------------------------------------------------------
	/// fill rect blend mode 50%
	void		FillRect_Blend50( void );
	//----------------------------------------------------------------------------------------------------
	/// fill rect blend mode 75%
	void		FillRect_Blend75( void );
	//----------------------------------------------------------------------------------------------------
	/// fill circle no blend
	void		FillCircle_NoBlend( Point &centre,s32 x, s32 y );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode Add
	void	    FillCircle_BlendAdd( Point &centre,s32 x, s32 y );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode sub
	void		FillCircle_BlendSub( Point &centre,s32 x, s32 y );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 25%
	void		FillCircle_Blend25( Point &centre,s32 x, s32 y );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 50%
	void		FillCircle_Blend50( Point &centre,s32 x, s32 y );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 75%
	void		FillCircle_Blend75( Point &centre,s32 x, s32 y );

	//----------------------------------------------------------------------------------------------------
	/// fill circle no blend
	void		FillTriangle_NoBlend( Point p1, Point p2, Point p3 );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode Add
	void	    FillTriangle_BlendAdd( Point p1, Point p2, Point p3 );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode sub
	void		FillTriangle_BlendSub( Point p1, Point p2, Point p3 );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 25%
	void		FillTriangle_Blend25( Point p1, Point p2, Point p3 );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 50%
	void		FillTriangle_Blend50( Point p1, Point p2, Point p3 );
	//----------------------------------------------------------------------------------------------------
	/// fill circle blend mode 75%
	void		FillTriangle_Blend75( Point p1, Point p2, Point p3 );
	void		swap(Point &p1,Point &p2);
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dcore_draw2d.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DCORE_DRAW2D_H__
