//=============================================================================
//
//	[ s2dmath_color4.inl ]
//
//	the s2d color4 proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_COLOR4_INL__
#define __S2DMATH_COLOR4_INL__

//--------------------------------------------------------------------------------------------------------
#define CLAMPCOL4												\
		if( r > F2FP(1.0f) )	r = F2FP(1.0f);		else		\
		if( r < F2FP(0.0f) )	r = F2FP(0.0f);					\
		if( g > F2FP(1.0f) )	g = F2FP(1.0f);		else		\
		if( g < F2FP(0.0f) )	g = F2FP(0.0f);					\
		if( b > F2FP(1.0f) )	b = F2FP(1.0f);		else		\
		if( b < F2FP(0.0f) )	b = F2FP(0.0f);					\
		if( a > F2FP(1.0f) )	a = F2FP(1.0f);		else		\
		if( a < F2FP(0.0f) )	a = F2FP(0.0f);
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### general:
//--------------------------------------------------------------------------------------------------------
inline Color4::Color4( void )
{
}
//--------------------------------------------------------------------------------------------------------
inline Color4::Color4( const f32* data )
{
	// clamp color value
	if( data[0] > F2FP(1.0f) )	r = F2FP(1.0f);		else
	if( data[0] < F2FP(0.0f) )	r = F2FP(0.0f);		else
								r = data[0];

	if( data[1] > F2FP(1.0f) )	g = F2FP(1.0f);		else
	if( data[1] < F2FP(0.0f) )	g = F2FP(0.0f);		else
								g = data[1];

	if( data[2] > F2FP(1.0f) )	b = F2FP(1.0f);		else
	if( data[2] < F2FP(0.0f) )	b = F2FP(0.0f);		else
								b = data[2];

	if( data[3] > F2FP(1.0f) )	a = F2FP(1.0f);		else
	if( data[3] < F2FP(0.0f) )	a = F2FP(0.0f);		else
								a = data[3];
}
//--------------------------------------------------------------------------------------------------------
inline Color4::Color4( f32 _r, f32 _g, f32 _b, f32 _a )
{
	// clamp color value
	if( _r > F2FP(1.0f) )	r = F2FP(1.0f);		else
	if( _r < F2FP(0.0f) )	r = F2FP(0.0f);		else
							r = _r;

	if( _g > F2FP(1.0f) )	g = F2FP(1.0f);		else
	if( _g < F2FP(0.0f) )	g = F2FP(0.0f);		else
							g = _g;

	if( _b > F2FP(1.0f) )	b = F2FP(1.0f);		else
	if( _b < F2FP(0.0f) )	b = F2FP(0.0f);		else
							b = _b;

	if( _a > F2FP(1.0f) )	a = F2FP(1.0f);		else
	if( _a < F2FP(0.0f) )	a = F2FP(0.0f);		else
							a = _a;
}
//--------------------------------------------------------------------------------------------------------
inline Color4& Color4::operator = ( const Color4& c )
{
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
//### conversions:
//--------------------------------------------------------------------------------------------------------
inline	Color4::operator f32* ( void )
{
	return (f32*)this;
}
//--------------------------------------------------------------------------------------------------------
inline	Color4::operator const f32* ( void ) const
{
	return (const f32*)this;
}
//--------------------------------------------------------------------------------------------------------
//### operations:
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator + ( void ) const
{
	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator - ( void ) const
{
	return	Color4(
				F2FP(1.0f) - r,
				F2FP(1.0f) - g,
				F2FP(1.0f) - b,
				F2FP(1.0f) - a
			);
}
//--------------------------------------------------------------------------------------------------------
inline const Color4& Color4::operator += ( const Color4& c )
{
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;

	// clamp color value
	CLAMPCOL4;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color4& Color4::operator -= ( const Color4& c )
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;

	// clamp color value
	CLAMPCOL4;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color4& Color4::operator *= ( const Color4& c )
{
	r *= c.r;
	g *= c.g;
	b *= c.b;
	a *= c.a;

	// clamp color value
	CLAMPCOL4;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color4& Color4::operator *= ( f32 f )
{
	r *= f;
	g *= f;
	b *= f;
	a *= f;

	// clamp color value
	CLAMPCOL4;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color4& Color4::operator /= ( f32 f )
{
	const f32 fInv = F2FP(1.0f) / f;

	r *= fInv;
	g *= fInv;
	b *= fInv;
	a *= fInv;

	// clamp color value
	CLAMPCOL4;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator + ( const Color4& c ) const
{
	return Color4( r+c.r, g+c.g, b+c.b, a+c.a );
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator - ( const Color4& c ) const
{
	return Color4( r-c.r, g-c.g, b-c.b, a-c.a );
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator * ( const Color4& c ) const
{
	return Color4( r*c.r, g*c.g, b*c.b, a*c.a );
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator * ( f32 f ) const
{
	return Color4( r*f, g*f, b*f, a*f );
}
//--------------------------------------------------------------------------------------------------------
inline Color4 Color4::operator / ( f32 f ) const
{
	const f32 fInv = F2FP(1.0f) / f;

	return Color4( r*fInv, g*fInv, b*fInv, a*fInv );
}
//--------------------------------------------------------------------------------------------------------
inline bool Color4::operator == ( const Color4& c ) const
{
	return ( r == c.r && g == c.g && b == c.b && a == c.a );
}
//--------------------------------------------------------------------------------------------------------
inline bool Color4::operator != ( const Color4& c ) const
{
	return ( r != c.r || g != c.g || b != c.b || a != c.a );
}
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
inline Color4 operator * ( f32 f, const Color4& c )
{
	return Color4( c.r*f, c.g*f, c.b*f, c.a*f );
}
//--------------------------------------------------------------------------------------------------------
/// compare with epsilon
inline bool		Color4Equal(
					Color4& cIn1,					//[in]
					Color4& cIn2,					//[in]
					f32 epsilon						//[in] user defined epsilon
				)
{
	if( fabsf( cIn1.r - cIn2.r ) > epsilon )
		return false;
	if( fabsf( cIn1.g - cIn2.g ) > epsilon )
		return false;
	if( fabsf( cIn1.b - cIn2.b ) > epsilon )
		return false;
	if( fabsf( cIn1.a - cIn2.a ) > epsilon )
		return false;

	return true;
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_COLOR4_INL__
