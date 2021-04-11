//=============================================================================
//
//	[ s2dbase_pixel.h ]
//
//	the s2d color format independent encapsulation of Pixel.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	color format independent.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_PIXEL_H__
#define __S2DBASE_PIXEL_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Pixel
{
public:
	Pixel( void );
	Pixel( u8 _r, u8 _g, u8 _b );

	// get r,g,b
	u8		r( void ) const;
	u8		g( void ) const;
	u8		b( void ) const;

	// set r,g,b
	void	r( u8 _r );
	void	g( u8 _g );
	void	b( u8 _b );

	// set r,g,b, more accurate, but much slower than r(u8)/g(u8)/b(u8)
	void	r_slow( u8 _r );
	void	g_slow( u8 _g );
	void	b_slow( u8 _b );

	// set rgb, faster than set r,g,b seperately
	void	rgb( u8 _r, u8 _g, u8 _b );
	void	rgb_slow( u8 _r, u8 _g, u8 _b );

	// operators
	Pixel	operator + ( const Pixel& pix ) const;
	Pixel	operator - ( const Pixel& pix ) const;

	void	operator += ( const Pixel& pix );
	void	operator -= ( const Pixel& pix );

	bool	operator == ( const Pixel& pix ) const;
	bool	operator != ( const Pixel& pix ) const;

	// blend with given alpha level [0-31]
	void	Blend_32( const Pixel& pix, u8 level );

	// blend with src 25%, src 50%, src 75%
	void	Blend25( const Pixel& pix );
	void	Blend50( const Pixel& pix );
	void	Blend75( const Pixel& pix );

public:
#if (COLOR_FORMAT==RGB332)
	u8		m_rgb;
#elif (COLOR_FORMAT==RGB444) || (COLOR_FORMAT==RGB555) || (COLOR_FORMAT==RGB565)
	u16		m_rgb;
#elif (COLOR_FORMAT==RGB666) || (COLOR_FORMAT==RGB888)
	u8		m_r;
	u8		m_g;
	u8		m_b;
#endif
} ALIGN(1);
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_pixel.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_PIXEL_H__
