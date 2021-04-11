//=============================================================================
//
//	[ s2dmath_color3.h ]
//
//	the s2d color3 proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_COLOR3_H__
#define __S2DMATH_COLOR3_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dmath_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Color3
{
//--------------------------------------------------------------------------------------------------------
//### general:
public:
	Color3( void );
	Color3( const f32* data );
	Color3( f32 _r, f32 _g, f32 _b);

	Color3& operator = ( const Color3& c );
//--------------------------------------------------------------------------------------------------------
//### conversions:
public:
	operator f32* ( void );
	operator const f32* ( void ) const;
//--------------------------------------------------------------------------------------------------------
//### operations:
public:
	Color3 operator + ( void ) const;
	/// negative color
	Color3 operator - ( void ) const;

	const Color3& operator += ( const Color3& c );
	const Color3& operator -= ( const Color3& c );
	const Color3& operator *= ( const Color3& c );
	const Color3& operator *= ( f32 f );
	const Color3& operator /= ( f32 f );

	Color3 operator + ( const Color3& c ) const;
	Color3 operator - ( const Color3& c ) const;
	Color3 operator * ( const Color3& c ) const;
	Color3 operator * ( f32 f ) const;
	Color3 operator / ( f32 f ) const;

    /// strict compare
	bool operator == ( const Color3& c ) const;
	bool operator != ( const Color3& c ) const;
//--------------------------------------------------------------------------------------------------------
//### data members:
public:
	f32 r, g, b;
};
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
Color3 operator * ( f32 f, const Color3& c );
//--------------------------------------------------------------------------------------------------------
/// compare with epsilon
bool		Color3Equal(
				Color3& cIn1,					//[in]
				Color3& cIn2,					//[in]
				f32 epsilon = S2D_EPSILON		//[in] user defined epsilon
			);
//--------------------------------------------------------------------------------------------------------
/// clamp rgb to [0.0, 1.0]
Color3&		Color3Clamp(
				Color3& cInOut					//[in/out]
			);
//--------------------------------------------------------------------------------------------------------
/// negative
Color3&		Color3Negative(
				Color3& cOut,					//[out]
				const Color3& cIn				//[in]
			);
//--------------------------------------------------------------------------------------------------------
/// adjust contrast
Color3&		Color3AdjustContrast(
				Color3& cOut,					//[out]
				const Color3& cIn,				//[in]
				f32 con							//[in] contrast
			);
//--------------------------------------------------------------------------------------------------------
/// adjust saturation
Color3&		Color3AdjustSaturation(
				Color3& cOut,					//[out]
				const Color3& cIn,				//[in]
				f32 sat							//[in] saturation
			);
//--------------------------------------------------------------------------------------------------------
/// linear interpolation, cA + s*(cB-cA)
Color3&		Color3Lerp(
				Color3& cOut,					//[out]
				const Color3& cA,				//[in]
				const Color3& cB,				//[in]
				const f32 s						//[in] interpolate factor
			);
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dmath_color3.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DMATH_COLOR3_H__
