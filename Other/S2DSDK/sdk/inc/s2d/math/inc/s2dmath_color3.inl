//=============================================================================
//
//	[ s2dmath_color3.inl ]
//
//	the s2d color3 proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_COLOR3_INL__
#define __S2DMATH_COLOR3_INL__

//--------------------------------------------------------------------------------------------------------
#define CLAMPCOL3												\
		if( r > F2FP(1.0f) )	r = F2FP(1.0f);		else		\
		if( r < F2FP(0.0f) )	r = F2FP(0.0f);					\
		if( g > F2FP(1.0f) )	g = F2FP(1.0f);		else		\
		if( g < F2FP(0.0f) )	g = F2FP(0.0f);					\
		if( b > F2FP(1.0f) )	b = F2FP(1.0f);		else		\
		if( b < F2FP(0.0f) )	b = F2FP(0.0f);
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### general:
//--------------------------------------------------------------------------------------------------------
inline Color3::Color3( void )
{
}
//--------------------------------------------------------------------------------------------------------
inline Color3::Color3( const f32* data )
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
}
//--------------------------------------------------------------------------------------------------------
inline Color3::Color3( f32 _r, f32 _g, f32 _b )
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
}
//--------------------------------------------------------------------------------------------------------
inline Color3& Color3::operator = ( const Color3& c )
{
	r = c.r;
	g = c.g;
	b = c.b;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
//### conversions:
//--------------------------------------------------------------------------------------------------------
inline	Color3::operator f32* ( void )
{
	return (f32*)this;
}
//--------------------------------------------------------------------------------------------------------
inline	Color3::operator const f32* ( void ) const
{
	return (const f32*)this;
}
//--------------------------------------------------------------------------------------------------------
//### operations:
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator + ( void ) const
{
	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator - ( void ) const
{
	return	Color3(
				F2FP(1.0f) - r,
				F2FP(1.0f) - g,
				F2FP(1.0f) - b
			);
}
//--------------------------------------------------------------------------------------------------------
inline const Color3& Color3::operator += ( const Color3& c )
{
	r += c.r;
	g += c.g;
	b += c.b;

	// clamp color value
	CLAMPCOL3;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color3& Color3::operator -= ( const Color3& c )
{
	r -= c.r;
	g -= c.g;
	b -= c.b;

	// clamp color value
	CLAMPCOL3;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color3& Color3::operator *= ( const Color3& c )
{
	r *= c.r;
	g *= c.g;
	b *= c.b;

	// clamp color value
	CLAMPCOL3;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color3& Color3::operator *= ( f32 f )
{
	r *= f;
	g *= f;
	b *= f;

	// clamp color value
	CLAMPCOL3;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline const Color3& Color3::operator /= ( f32 f )
{
	const f32 fInv = F2FP(1.0f) / f;

	r *= fInv;
	g *= fInv;
	b *= fInv;

	// clamp color value
	CLAMPCOL3;

	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator + ( const Color3& c ) const
{
	return Color3( r+c.r, g+c.g, b+c.b );
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator - ( const Color3& c ) const
{
	return Color3( r-c.r, g-c.g, b-c.b );
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator * ( const Color3& c ) const
{
	return Color3( r*c.r, g*c.g, b*c.b );
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator * ( f32 f ) const
{
	return Color3( r*f, g*f, b*f );
}
//--------------------------------------------------------------------------------------------------------
inline Color3 Color3::operator / ( f32 f ) const
{
	const f32 fInv = F2FP(1.0f) / f;

	return Color3( r*fInv, g*fInv, b*fInv );
}
//--------------------------------------------------------------------------------------------------------
inline bool Color3::operator == ( const Color3& c ) const
{
	return ( r == c.r && g == c.g && b == c.b );
}
//--------------------------------------------------------------------------------------------------------
inline bool Color3::operator != ( const Color3& c ) const
{
	return ( r != c.r || g != c.g || b != c.b );
}
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
inline Color3 operator * ( f32 f, const Color3& c )
{
	return Color3( c.r*f, c.g*f, c.b*f );
}
//--------------------------------------------------------------------------------------------------------
/// compare with epsilon
inline bool		Color3Equal(
					Color3& cIn1,					//[in]
					Color3& cIn2,					//[in]
					f32 epsilon						//[in] user defined epsilon
				)
{
	if( fabsf( cIn1.r - cIn2.r ) > epsilon )
		return false;
	if( fabsf( cIn1.g - cIn2.g ) > epsilon )
		return false;
	if( fabsf( cIn1.b - cIn2.b ) > epsilon )
		return false;

	return true;
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_COLOR3_INL__
