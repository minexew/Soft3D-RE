//=============================================================================
//
//	[ s2dmath_color4.h ]
//
//	the s2d color4 proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_COLOR4_H__
#define __S2DMATH_COLOR4_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dmath_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Color4
{
//--------------------------------------------------------------------------------------------------------
//### general:
public:
	Color4( void );
	Color4( const f32* data );
	Color4( f32 _r, f32 _g, f32 _b, f32 _a );

	Color4& operator = ( const Color4& c );
//--------------------------------------------------------------------------------------------------------
//### conversions:
public:
	operator f32* ( void );
	operator const f32* ( void ) const;
//--------------------------------------------------------------------------------------------------------
//### operations:
public:
	Color4 operator + ( void ) const;
	/// negative color
	Color4 operator - ( void ) const;

	const Color4& operator += ( const Color4& c );
	const Color4& operator -= ( const Color4& c );
	const Color4& operator *= ( const Color4& c );
	const Color4& operator *= ( f32 f );
	const Color4& operator /= ( f32 f );

	Color4 operator + ( const Color4& c ) const;
	Color4 operator - ( const Color4& c ) const;
	Color4 operator * ( const Color4& c ) const;
	Color4 operator * ( f32 f ) const;
	Color4 operator / ( f32 f ) const;

    /// strict compare
	bool operator == ( const Color4& c ) const;
	bool operator != ( const Color4& c ) const;
//--------------------------------------------------------------------------------------------------------
//### data members:
public:
	f32 r, g, b, a;
};
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
Color4 operator * ( f32 f, const Color4& c );
//--------------------------------------------------------------------------------------------------------
/// compare with epsilon
bool		Color4Equal(
				Color4& cIn1,					//[in]
				Color4& cIn2,					//[in]
				f32 epsilon = S2D_EPSILON		//[in] user defined epsilon
			);
//--------------------------------------------------------------------------------------------------------
/// clamp rgba to [0.0, 1.0]
Color4&		Color4Clamp(
				Color4& cInOut					//[in/out]
			);
//--------------------------------------------------------------------------------------------------------
/// negative
Color4&		Color4Negative(
				Color4& cOut,					//[out]
				const Color4& cIn				//[in]
			);
//--------------------------------------------------------------------------------------------------------
/// adjust contrast
Color4&		Color4AdjustContrast(
				Color4& cOut,					//[out]
				const Color4& cIn,				//[in]
				f32 con							//[in] contrast
			);
//--------------------------------------------------------------------------------------------------------
/// adjust saturation
Color4&		Color4AdjustSaturation(
				Color4& cOut,					//[out]
				const Color4& cIn,				//[in]
				f32 sat							//[in] saturation
			);
//--------------------------------------------------------------------------------------------------------
/// linear interpolation, cA + s*(cB-cA)
Color4&		Color4Lerp(
				Color4& cOut,					//[out]
				const Color4& cA,				//[in]
				const Color4& cB,				//[in]
				const f32 s						//[in] interpolate factor
			);
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dmath_color4.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_COLOR4_H__
