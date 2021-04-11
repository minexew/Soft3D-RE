//=============================================================================
//
//	[ s2dbase_pixel.inl ]
//
//	the s2d color format independent encapsulation of Pixel.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	color format independent
//	TODO: RGB888 needs optimize
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_PIXEL_INL__
#define __S2DBASE_PIXEL_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
inline Pixel::Pixel( void ) :
#if (COLOR_FORMAT==RGB666) || (COLOR_FORMAT==RGB888)
	m_r(0), m_g(0), m_b(0)
#elif (COLOR_FORMAT==RGB332) || (COLOR_FORMAT==RGB444) || (COLOR_FORMAT==RGB555) || (COLOR_FORMAT==RGB565)
	m_rgb(0)
#endif
{
}
//---------------------------------------------------------------------------------------------------------
inline Pixel::Pixel( u8 _r, u8 _g, u8 _b )
#if (COLOR_FORMAT==RGB888)
	: m_r(_r), m_g(_g), m_b(_b)
#endif
{
#if (COLOR_FORMAT==RGB666)
	m_r = _r >> 2;
	m_g = _g >> 2;
	m_b = _b >> 2;
#elif (COLOR_FORMAT==RGB332)
	m_rgb = ((_r>>5)<<5) | ((_g>>5)<<2) | (_b>>6);
#elif (COLOR_FORMAT==RGB444)
	m_rgb = ((_r>>4)<<8) | ((_g>>4)<<4) | (_b>>4);
#elif (COLOR_FORMAT==RGB555)
	m_rgb = ((_r>>3)<<10) | ((_g>>3)<<5) | (_b>>3);
#elif (COLOR_FORMAT==RGB565)
	m_rgb = ((_r>>3)<<11) | ((_g>>2)<<5) | (_b>>3);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline u8		Pixel::r( void ) const
{
#if (COLOR_FORMAT==RGB888)
	return m_r;
#elif (COLOR_FORMAT==RGB666)
	return m_r << 2;
#elif (COLOR_FORMAT==RGB332)
	return (m_rgb >> 5) << 5;
#elif (COLOR_FORMAT==RGB444)
	return (m_rgb >> 8) << 4;
#elif (COLOR_FORMAT==RGB555)
	return (m_rgb >> 10) << 3;
#elif (COLOR_FORMAT==RGB565)
	return (m_rgb >> 11) << 3;
#endif
}
//---------------------------------------------------------------------------------------------------------
inline u8		Pixel::g( void ) const
{
#if (COLOR_FORMAT==RGB888)
	return m_g;
#elif (COLOR_FORMAT==RGB666)
	return m_g << 2;
#elif (COLOR_FORMAT==RGB332)
	return ((m_rgb >> 2) & 0x7) << 5;
#elif (COLOR_FORMAT==RGB444)
	return ((m_rgb >> 4) & 0xF) << 4;
#elif (COLOR_FORMAT==RGB555)
	return ((m_rgb >> 5) & 0x1F) << 3;
#elif (COLOR_FORMAT==RGB565)
	return ((m_rgb >> 5) & 0x3F) << 2;
#endif
}
//---------------------------------------------------------------------------------------------------------
inline u8		Pixel::b( void ) const
{
#if (COLOR_FORMAT==RGB888)
	return m_b;
#elif (COLOR_FORMAT==RGB666)
	return m_b << 2;
#elif (COLOR_FORMAT==RGB332)
	return (m_rgb & 0x3) << 6;
#elif (COLOR_FORMAT==RGB444)
	return (m_rgb & 0xF) << 4;
#elif (COLOR_FORMAT==RGB555)
	return (m_rgb & 0x1F) << 3;
#elif (COLOR_FORMAT==RGB565)
	return (m_rgb & 0x1F) << 3;
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::r( u8 _r )
{
#if (COLOR_FORMAT==RGB888)
	m_r = _r;
#elif (COLOR_FORMAT==RGB666)
	m_r = _r >> 2;
#elif (COLOR_FORMAT==RGB332)
	m_rgb |= ((_r>>5)<<5);
#elif (COLOR_FORMAT==RGB444)
	m_rgb |= ((_r>>4)<<8);
#elif (COLOR_FORMAT==RGB555)
	m_rgb |= ((_r>>3)<<10);
#elif (COLOR_FORMAT==RGB565)
	m_rgb |= ((_r>>3)<<11);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::g( u8 _g )
{
#if (COLOR_FORMAT==RGB888)
	m_g = _g;
#elif (COLOR_FORMAT==RGB666)
	m_g = _g >> 2;
#elif (COLOR_FORMAT==RGB332)
	m_rgb |= ((_g>>5)<<2);
#elif (COLOR_FORMAT==RGB444)
	m_rgb |= ((_g>>4)<<4);
#elif (COLOR_FORMAT==RGB555)
	m_rgb |= ((_g>>3)<<5);
#elif (COLOR_FORMAT==RGB565)
	m_rgb |= ((_g>>2)<<5);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::b( u8 _b )
{
#if (COLOR_FORMAT==RGB888)
	m_b = _b;
#elif (COLOR_FORMAT==RGB666)
	m_b = _b >> 2;
#elif (COLOR_FORMAT==RGB332)
	m_rgb |= (_b>>6);
#elif (COLOR_FORMAT==RGB444)
	m_rgb |= (_b>>4);
#elif (COLOR_FORMAT==RGB555)
	m_rgb |= (_b>>3);
#elif (COLOR_FORMAT==RGB565)
	m_rgb |= (_b>>3);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::r_slow( u8 _r )
{
#if (COLOR_FORMAT==RGB888)
	m_r = _r;
#elif (COLOR_FORMAT==RGB666)
	m_r = _r >> 2;
	if( m_r != 0x3F )
		m_r += ((_r & 0x2) >> 1);
#elif (COLOR_FORMAT==RGB332)
	u8 tmp_r = _r >> 5;
	if( tmp_r != 0x7 )
		tmp_r += ((_r & 0x10) >> 4);
	m_rgb |= (tmp_r<<5);
#elif (COLOR_FORMAT==RGB444)
	u8 tmp_r = _r >> 4;
	if( tmp_r != 0xF )
		tmp_r += ((_r & 0x8) >> 3);
	m_rgb |= (tmp_r<<8);
#elif (COLOR_FORMAT==RGB555)
	u8 tmp_r = _r >> 3;
	if( tmp_r != 0x1F )
		tmp_r += ((_r & 0x4) >> 2);
	m_rgb |= (tmp_r<<10);
#elif (COLOR_FORMAT==RGB565)
	u8 tmp_r = _r >> 3;
	if( tmp_r != 0x1F )
		tmp_r += ((_r & 0x4) >> 2);
	m_rgb |= (tmp_r<<11);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::g_slow( u8 _g )
{
#if (COLOR_FORMAT==RGB888)
	m_g = _g;
#elif (COLOR_FORMAT==RGB666)
	m_g = _g >> 2;
	if( m_g != 0x3F )
		m_g += ((_g & 0x2) >> 1);
#elif (COLOR_FORMAT==RGB332)
	u8 tmp_g = _g >> 5;
	if( tmp_g != 0x7 )
		tmp_g += ((_g & 0x10) >> 4);
	m_rgb |= (tmp_g<<2);
#elif (COLOR_FORMAT==RGB444)
	u8 tmp_g = _g >> 4;
	if( tmp_g != 0xF )
		tmp_g += ((_g & 0x8) >> 3);
	m_rgb |= (tmp_g<<4);
#elif (COLOR_FORMAT==RGB555)
	u8 tmp_g = _g >> 3;
	if( tmp_g != 0x1F )
		tmp_g += ((_g & 0x4) >> 2);
	m_rgb |= (tmp_g<<5);
#elif (COLOR_FORMAT==RGB565)
	u8 tmp_g = _g >> 2;
	if( tmp_g != 0x3F )
		tmp_g += ((_g & 0x2) >> 1);
	m_rgb |= (tmp_g<<5);
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::b_slow( u8 _b )
{
#if (COLOR_FORMAT==RGB888)
	m_b = _b;
#elif (COLOR_FORMAT==RGB666)
	m_b = _b >> 2;
	if( m_b != 0x3F )
		m_b += ((_b & 0x2) >> 1);
#elif (COLOR_FORMAT==RGB332)
	u8 tmp_b = _b >> 6;
	if( tmp_b != 0x3 )
		tmp_b += ((_b & 0x20) >> 5);
	m_rgb |= tmp_b;
#elif (COLOR_FORMAT==RGB444)
	u8 tmp_b = _b >> 4;
	if( tmp_b != 0xF )
		tmp_b += ((_b & 0x8) >> 3);
	m_rgb |= tmp_b;
#elif (COLOR_FORMAT==RGB555)
	u8 tmp_b = _b >> 3;
	if( tmp_b != 0x1F )
		tmp_b += ((_b & 0x4) >> 2);
	m_rgb |= tmp_b;
#elif (COLOR_FORMAT==RGB565)
	u8 tmp_b = _b >> 3;
	if( tmp_b != 0x1F )
		tmp_b += ((_b & 0x4) >> 2);
	m_rgb |= tmp_b;
#endif
}
//---------------------------------------------------------------------------------------------------------
inline void		Pixel::rgb( u8 _r, u8 _g, u8 _b )
{
#if (COLOR_FORMAT==RGB888)
	m_r = _r;
	m_g = _g;
	m_b = _b;
#elif (COLOR_FORMAT==RGB666)
	m_r = _r >> 2;
	m_g = _g >> 2;
	m_b = _b >> 2;
#elif (COLOR_FORMAT==RGB332)
	m_rgb = ((_r>>5)<<5) | ((_g>>5)<<2) | (_b>>6);
#elif (COLOR_FORMAT==RGB444)
	m_rgb = ((_r>>4)<<8) | ((_g>>4)<<4) | (_b>>4);
#elif (COLOR_FORMAT==RGB555)
	m_rgb = ((_r>>3)<<10) | ((_g>>3)<<5) | (_b>>3);
#elif (COLOR_FORMAT==RGB565)
	m_rgb = ((_r>>3)<<11) | ((_g>>2)<<5) | (_b>>3);
#endif
}
//--------------------------------------------------------------------------------------------------------
inline void		Pixel::rgb_slow( u8 _r, u8 _g, u8 _b )
{
#if (COLOR_FORMAT==RGB888)
	m_r = _r;
	m_g = _g;
	m_b = _b;
#elif (COLOR_FORMAT==RGB666)
	m_r = _r >> 2;
	if( m_r != 0x3F )
		m_r += ((_r & 0x2) >> 1);
	m_g = _g >> 2;
	if( m_g != 0x3F )
		m_g += ((_g & 0x2) >> 1);
	m_b = _b >> 2;
	if( m_b != 0x3F )
		m_b += ((_b & 0x2) >> 1);
#elif (COLOR_FORMAT==RGB444)
	u8 tmp_r = _r >> 5;
	if( tmp_r != 0x7 )
		tmp_r += ((_r & 0x10) >> 4);
	u8 tmp_g = _g >> 5;
	if( tmp_g != 0x7 )
		tmp_g += ((_g & 0x10) >> 4);
	u8 tmp_b = _b >> 6;
	if( tmp_b != 0x3 )
		tmp_b += ((_b & 0x20) >> 5);
	m_rgb = (tmp_r<<5) | (tmp_g<<2) | (tmp_b);
#elif (COLOR_FORMAT==RGB444)
	u8 tmp_r = _r >> 4;
	if( tmp_r != 0xF )
		tmp_r += ((_r & 0x8) >> 3);
	u8 tmp_g = _g >> 4;
	if( tmp_g != 0xF )
		tmp_g += ((_g & 0x8) >> 3);
	u8 tmp_b = _b >> 4;
	if( tmp_b != 0xF )
		tmp_b += ((_b & 0x8) >> 3);
	m_rgb = (tmp_r<<8) | (tmp_g<<4) | (tmp_b);
#elif (COLOR_FORMAT==RGB555)
	u8 tmp_r = _r >> 3;
	if( tmp_r != 0x1F )
		tmp_r += ((_r & 0x4) >> 2);
	u8 tmp_g = _g >> 3;
	if( tmp_g != 0x1F )
		tmp_g += ((_g & 0x4) >> 2);
	u8 tmp_b = _b >> 3;
	if( tmp_b != 0x1F )
		tmp_b += ((_b & 0x4) >> 2);
	m_rgb = (tmp_r<<10) | (tmp_g<<5) | (tmp_b);
#elif (COLOR_FORMAT==RGB565)
	u8 tmp_r = _r >> 3;
	if( tmp_r != 0x1F )
		tmp_r += ((_r & 0x4) >> 2);
	u8 tmp_g = _g >> 2;
	if( tmp_g != 0x3F )
		tmp_g += ((_g & 0x2) >> 1);
	u8 tmp_b = _b >> 3;
	if( tmp_b != 0x1F )
		tmp_b += ((_b & 0x4) >> 2);
	m_rgb = (tmp_r<<11) | (tmp_g<<5) | (tmp_b);
#endif
}
//--------------------------------------------------------------------------------------------------------
inline Pixel	Pixel::operator + ( const Pixel& pix ) const
{
#if (COLOR_FORMAT==RGB888)
	Pixel r;
	u16 tmp;
	tmp = m_r + pix.m_r;
	if( tmp > 0xFF )	tmp = 0xFF;
	r.m_r = tmp;
	tmp = m_g + pix.m_g;
	if( tmp > 0xFF )	tmp = 0xFF;
	r.m_g = tmp;
	tmp = m_b + pix.m_b;
	if( tmp > 0xFF )	tmp = 0xFF;
	r.m_b = tmp;
	return r;
#elif (COLOR_FORMAT==RGB666)
	Pixel r;
	u16 tmp;
	tmp = m_r + pix.m_r;
	if( tmp > 0x3F )	tmp = 0x3F;
	r.m_r = tmp;
	tmp = m_g + pix.m_g;
	if( tmp > 0x3F )	tmp = 0x3F;
	r.m_g = tmp;
	tmp = m_b + pix.m_b;
	if( tmp > 0x3F )	tmp = 0x3F;
	r.m_b = tmp;
	return r;
#elif (COLOR_FORMAT==RGB332)
	Pixel r;
	u8 tmp;
	tmp = (m_rgb & 0x03) + (pix.m_rgb & 0x03);
	if( tmp > 0x03 )	tmp = 0x03;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x1C) + (pix.m_rgb & 0x1C);
	if( tmp > 0x1C )	tmp = 0x1C;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0xE0) + (pix.m_rgb & 0xE0);
	if( tmp > 0xE0 )	tmp = 0xE0;
	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB444)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x000F) + (pix.m_rgb & 0x000F);
	if( tmp > 0x000F )	tmp = 0x000F;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x00F0) + (pix.m_rgb & 0x00F0);
	if( tmp > 0x00F0 )	tmp = 0x00F0;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x0F00) + (pix.m_rgb & 0x0F00);
	if( tmp > 0x0F00 )	tmp = 0x0F00;
	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB555)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x001F) + (pix.m_rgb & 0x001F);
	if( tmp > 0x001F )	tmp = 0x001F;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x03E0) + (pix.m_rgb & 0x03E0);
	if( tmp > 0x03E0 )	tmp = 0x03E0;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x7C00) + (pix.m_rgb & 0x7C00);
	if( tmp > 0x7C00 )	tmp = 0x7C00;
	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB565)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x001F) + (pix.m_rgb & 0x001F);
	if( tmp > 0x001F )	tmp = 0x001F;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x07E0) + (pix.m_rgb & 0x07E0);
	if( tmp > 0x07E0 )	tmp = 0x07E0;
	r.m_rgb |= tmp;
	tmp = (m_rgb & 0xF800) + (pix.m_rgb & 0xF800);
	if( tmp > 0xF800 )	tmp = 0xF800;
	r.m_rgb |= tmp;
	return r;
#endif
}
//--------------------------------------------------------------------------------------------------------
inline Pixel	Pixel::operator - ( const Pixel& pix ) const
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	Pixel r;
	s16 tmp;
	tmp = m_r - pix.m_r;
	if( tmp < 0 )	tmp = 0;
	r.m_r = tmp;
	tmp = m_g - pix.m_g;
	if( tmp < 0 )	tmp = 0;
	r.m_g = tmp;
	tmp = m_b - pix.m_b;
	if( tmp < 0 )	tmp = 0;
	r.m_b = tmp;
	return r;
#elif (COLOR_FORMAT==RGB332)
	Pixel r;
	u8 tmp;
	tmp = (m_rgb & 0x03) - (pix.m_rgb & 0x03);
	if( !(tmp & 0xF6) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x1C) - (pix.m_rgb & 0x1C);
	if( !(tmp & 0xE3) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0xE0) - (pix.m_rgb & 0xE0);
	if( !(tmp & 0x1F) )	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB444)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x000F) - (pix.m_rgb & 0x000F);
	if( !(tmp & 0xFFF0) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x00F0) - (pix.m_rgb & 0x00F0);
	if( !(tmp & 0xFF0F) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x0F00) - (pix.m_rgb & 0x0F00);
	if( !(tmp & 0xF0FF) )	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB555)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x001F) - (pix.m_rgb & 0x001F);
	if( !(tmp & 0xFFE0) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x03E0) - (pix.m_rgb & 0x03E0);
	if( !(tmp & 0xFC1F) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x7C00) - (pix.m_rgb & 0x7C00);
	if( !(tmp & 0x83FF) )	r.m_rgb |= tmp;
	return r;
#elif (COLOR_FORMAT==RGB565)
	Pixel r;
	u16 tmp;
	tmp = (m_rgb & 0x001F) - (pix.m_rgb & 0x001F);
	if( !(tmp & 0xFFE0) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0x07E0) - (pix.m_rgb & 0x07E0);
	if( !(tmp & 0xF81F) )	r.m_rgb |= tmp;
	tmp = (m_rgb & 0xF800) - (pix.m_rgb & 0xF800);
	if( !(tmp & 0x03FF) )	r.m_rgb |= tmp;
	return r;
#endif
}
//--------------------------------------------------------------------------------------------------------
inline void		Pixel::operator += ( const Pixel& pix )
{
#if (COLOR_FORMAT==RGB888)
	m_r += pix.m_r;
	if( m_r > 0xFF )	m_r = 0xFF;
	m_g += pix.m_g;
	if( m_g > 0xFF )	m_g = 0xFF;
	m_b += pix.m_b;
	if( m_b > 0xFF )	m_b = 0xFF;
#elif (COLOR_FORMAT==RGB666)
	m_r += pix.m_r;
	if( m_r > 0x3F )	m_r = 0x3F;
	m_g += pix.m_g;
	if( m_g > 0x3F )	m_g = 0x3F;
	m_b += pix.m_b;
	if( m_b > 0x3F )	m_b = 0x3F;
#elif (COLOR_FORMAT==RGB332)
	u8 rgb = 0;
	u8 tmp;
	tmp = (m_rgb & 0x03) + (pix.m_rgb & 0x03);
	if( tmp > 0x03 )	tmp = 0x03;
	rgb |= tmp;
	tmp = (m_rgb & 0x1C) + (pix.m_rgb & 0x1C);
	if( tmp > 0x1C )	tmp = 0x1C;
	rgb |= tmp;
	tmp = (m_rgb & 0xE0) + (pix.m_rgb & 0xE0);
	if( tmp > 0xE0 )	tmp = 0xE0;
	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB444)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x000F) + (pix.m_rgb & 0x000F);
	if( tmp > 0x000F )	tmp = 0x000F;
	rgb |= tmp;
	tmp = (m_rgb & 0x00F0) + (pix.m_rgb & 0x00F0);
	if( tmp > 0x00F0 )	tmp = 0x00F0;
	rgb |= tmp;
	tmp = (m_rgb & 0x0F00) + (pix.m_rgb & 0x0F00);
	if( tmp > 0x0F00 )	tmp = 0x0F00;
	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB555)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x001F) + (pix.m_rgb & 0x001F);
	if( tmp > 0x001F )	tmp = 0x001F;
	rgb |= tmp;
	tmp = (m_rgb & 0x03E0) + (pix.m_rgb & 0x03E0);
	if( tmp > 0x03E0 )	tmp = 0x03E0;
	rgb |= tmp;
	tmp = (m_rgb & 0x7C00) + (pix.m_rgb & 0x7C00);
	if( tmp > 0x7C00 )	tmp = 0x7C00;
	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB565)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x001F) + (pix.m_rgb & 0x001F);
	if( tmp > 0x001F )	tmp = 0x001F;
	rgb |= tmp;
	tmp = (m_rgb & 0x07E0) + (pix.m_rgb & 0x07E0);
	if( tmp > 0x07E0 )	tmp = 0x07E0;
	rgb |= tmp;
	tmp = (m_rgb & 0xF800) + (pix.m_rgb & 0xF800);
	if( tmp > 0xF800 )	tmp = 0xF800;
	rgb |= tmp;
	m_rgb = rgb;
#endif
}
//--------------------------------------------------------------------------------------------------------
inline void		Pixel::operator -= ( const Pixel& pix )
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	m_r -= pix.m_r;
	if( m_r < 0 )	m_r = 0;
	m_g -= pix.m_g;
	if( m_g < 0 )	m_g = 0;
	m_b -= pix.m_b;
	if( m_b < 0 )	m_b = 0;
#elif (COLOR_FORMAT==RGB332)
	u8 rgb = 0;
	u8 tmp;
	tmp = (m_rgb & 0x03) - (pix.m_rgb & 0x03);
	if( !(tmp & 0xF6) )	rgb |= tmp;
	tmp = (m_rgb & 0x1C) - (pix.m_rgb & 0x1C);
	if( !(tmp & 0xE3) )	rgb |= tmp;
	tmp = (m_rgb & 0xE0) - (pix.m_rgb & 0xE0);
	if( !(tmp & 0x1F) )	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB444)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x000F) - (pix.m_rgb & 0x000F);
	if( !(tmp & 0xFFF0) )	rgb |= tmp;
	tmp = (m_rgb & 0x00F0) - (pix.m_rgb & 0x00F0);
	if( !(tmp & 0xFF0F) )	rgb |= tmp;
	tmp = (m_rgb & 0x0F00) - (pix.m_rgb & 0x0F00);
	if( !(tmp & 0xF0FF) )	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB555)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x001F) - (pix.m_rgb & 0x001F);
	if( !(tmp & 0xFFE0) )	rgb |= tmp;
	tmp = (m_rgb & 0x03E0) - (pix.m_rgb & 0x03E0);
	if( !(tmp & 0xFC1F) )	rgb |= tmp;
	tmp = (m_rgb & 0x7C00) - (pix.m_rgb & 0x7C00);
	if( !(tmp & 0x83FF) )	rgb |= tmp;
	m_rgb = rgb;
#elif (COLOR_FORMAT==RGB565)
	u16 rgb = 0;
	u16 tmp;
	tmp = (m_rgb & 0x001F) - (pix.m_rgb & 0x001F);
	if( !(tmp & 0xFFE0) )	rgb |= tmp;
	tmp = (m_rgb & 0x07E0) - (pix.m_rgb & 0x07E0);
	if( !(tmp & 0xF81F) )	rgb |= tmp;
	tmp = (m_rgb & 0xF800) - (pix.m_rgb & 0xF800);
	if( !(tmp & 0x03FF) )	rgb |= tmp;
	m_rgb = rgb;
#endif
}
//--------------------------------------------------------------------------------------------------------
inline bool		Pixel::operator == ( const Pixel& pix ) const
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	return (m_r==pix.m_r) && (m_g==pix.m_g) && (m_b==pix.m_b);
#elif (COLOR_FORMAT==RGB332) || (COLOR_FORMAT==RGB444) || (COLOR_FORMAT==RGB555) || (COLOR_FORMAT==RGB565)
	return (m_rgb == pix.m_rgb);
#endif
}
//--------------------------------------------------------------------------------------------------------
inline bool		Pixel::operator != ( const Pixel& pix ) const
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	return (m_r!=pix.m_r) || (m_g!=pix.m_g) || (m_b!=pix.m_b);
#elif (COLOR_FORMAT==RGB332) || (COLOR_FORMAT==RGB444) || (COLOR_FORMAT==RGB555) || (COLOR_FORMAT==RGB565)
	return (m_rgb != pix.m_rgb);
#endif
}
//--------------------------------------------------------------------------------------------------------
// blend with given alpha level [0-31]
inline void		Pixel::Blend_32( const Pixel& pix, u8 level )
{
	assert( level >= 0 && level <= 31 );

#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	if( level == 0 ) {
		return;
	}
	if( level == 31 ) {
		m_r = pix.m_r;
		m_g = pix.m_g;
		m_b = pix.m_b;
		return;
	}
#elif (COLOR_FORMAT==RGB332) || (COLOR_FORMAT==RGB444) || (COLOR_FORMAT==RGB555) || (COLOR_FORMAT==RGB565)
	if( level == 0 ) {
		return;
	}
	if( level == 31 ) {
		m_rgb = pix.m_rgb;
		return;
	}
#endif

#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	#error
#elif (COLOR_FORMAT==RGB332)
	s32 src = ((pix.m_rgb & 0xE0) << 10) | ((pix.m_rgb & 0x1C) << 5) | (pix.m_rgb & 0x03);
	s32 dst = ((m_rgb & 0xE0) << 10) | ((m_rgb & 0x1C) << 5) | (m_rgb & 0x03);
	dst = ( dst << 5 ) + ( src - dst ) * level;
	m_rgb = ((dst & 0x700000) >> 15) | ((dst & 0x7000) >> 10) | ((dst & 0x60) >> 5);
#elif (COLOR_FORMAT==RGB444)
	s32 src = ((pix.m_rgb & 0x0F00) << 10) | ((pix.m_rgb & 0x00F0) << 5) | (pix.m_rgb & 0x000F);
	s32 dst = ((m_rgb & 0x0F00) << 10) | ((m_rgb & 0x00F0) << 5) | (m_rgb & 0x000F);
	dst = ( dst << 5 ) + ( src - dst ) * level;
	m_rgb = ((dst & 0x7800000) >> 15) | ((dst & 0x3C000) >> 10) | ((dst & 0x1E0) >> 5);
#elif (COLOR_FORMAT==RGB555)
	s32 src = ((pix.m_rgb & 0x7C00) << 10) | ((pix.m_rgb & 0x03E0) << 5) | (pix.m_rgb & 0x001F);
	s32 dst = ((m_rgb & 0x7C00) << 10) | ((m_rgb & 0x03E0) << 5) | (m_rgb & 0x001F);
	dst = ( dst << 5 ) + ( src - dst ) * level;
	m_rgb = ((dst & 0x3E000000) >> 15) | ((dst & 0xF8000) >> 10) | ((dst & 0x3E0) >> 5);
#elif (COLOR_FORMAT==RGB565)
	s32 src = ((pix.m_rgb & 0xF800) << 10) | ((pix.m_rgb & 0x07E0) << 5) | (pix.m_rgb & 0x001F);
	s32 dst = ((m_rgb & 0xF800) << 10) | ((m_rgb & 0x07E0) << 5) | (m_rgb & 0x001F);
	dst = ( dst << 5 ) + ( src - dst ) * level;
	m_rgb = ((dst & 0x7C000000) >> 15) | ((dst & 0x1F8000) >> 10) | ((dst & 0x3E0) >> 5);
#endif
}
//--------------------------------------------------------------------------------------------------------
// blend with src 25%
inline void		Pixel::Blend25( const Pixel& pix )
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	m_r = (m_r>>2) + (pix.m_r>>1) + (pix.m_r>>2);
	m_g = (m_g>>2) + (pix.m_g>>1) + (pix.m_g>>2);
	m_b = (m_b>>2) + (pix.m_b>>1) + (pix.m_b>>2);
#elif (COLOR_FORMAT==RGB332)
	m_rgb = ((m_rgb & 0x90) >> 2) + ((pix.m_rgb & 0xDA) >> 1) + ((pix.m_rgb & 0x90) >> 2);
#elif (COLOR_FORMAT==RGB444)
	m_rgb = ((m_rgb & 0x0CCC) >> 2) + ((pix.m_rgb & 0x0EEE) >> 1) + ((pix.m_rgb & 0x0CCC) >> 2);
#elif (COLOR_FORMAT==RGB555)
	m_rgb = ((m_rgb & 0x739C) >> 2) + ((pix.m_rgb & 0x7BDE) >> 1) + ((pix.m_rgb & 0x739C) >> 2);
#elif (COLOR_FORMAT==RGB565)
	m_rgb = ((m_rgb & 0xE79C) >> 2) + ((pix.m_rgb & 0xF7DE) >> 1) + ((pix.m_rgb & 0xE79C) >> 2);
#endif
}
//--------------------------------------------------------------------------------------------------------
// blend with src 50%
inline void		Pixel::Blend50( const Pixel& pix )
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	m_r = (m_r>>1) + (pix.m_r>>1);
	m_g = (m_g>>1) + (pix.m_g>>1);
	m_b = (m_b>>1) + (pix.m_b>>1);
#elif (COLOR_FORMAT==RGB332)
	m_rgb = ((m_rgb & 0xDA) >> 1) + ((pix.m_rgb & 0xDA) >> 1);
#elif (COLOR_FORMAT==RGB444)
	m_rgb = ((m_rgb & 0x0EEE) >> 1) + ((pix.m_rgb & 0x0EEE) >> 1);
#elif (COLOR_FORMAT==RGB555)
	m_rgb = ((m_rgb & 0x7BDE) >> 1) + ((pix.m_rgb & 0x7BDE) >> 1);
#elif (COLOR_FORMAT==RGB565)
	m_rgb = ((m_rgb & 0xF7DE) >> 1) + ((pix.m_rgb & 0xF7DE) >> 1);
#endif
}
//--------------------------------------------------------------------------------------------------------
// blend with src 75%
inline void		Pixel::Blend75( const Pixel& pix )
{
#if (COLOR_FORMAT==RGB888) || (COLOR_FORMAT==RGB666)
	m_r = (m_r>>1) + (m_r>>2) + (pix.m_r>>1);
	m_g = (m_g>>1) + (m_g>>2) + (pix.m_g>>1);
	m_b = (m_b>>1) + (m_b>>2) + (pix.m_b>>1);
#elif (COLOR_FORMAT==RGB332)
	m_rgb = ((m_rgb & 0xDA) >> 1) + ((m_rgb & 0x90) >> 2) + ((pix.m_rgb & 0x90) >> 2);
#elif (COLOR_FORMAT==RGB444)
	m_rgb = ((m_rgb & 0x0EEE) >> 1) + ((m_rgb & 0x0CCC) >> 2) + ((pix.m_rgb & 0x0CCC) >> 2);
#elif (COLOR_FORMAT==RGB555)
	m_rgb = ((m_rgb & 0x7BDE) >> 1) + ((m_rgb & 0x739C) >> 2) + ((pix.m_rgb & 0x739C) >> 2);
#elif (COLOR_FORMAT==RGB565)
	m_rgb = ((m_rgb & 0xF7DE) >> 1) + ((m_rgb & 0xE79C) >> 2) + ((pix.m_rgb & 0xE79C) >> 2);
#endif
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_PIXEL_INL__
