//=============================================================================
//
//	[ s2dbase_fixpoint.h ]
//
//	the s2d encapsulation of FixedPoint data type.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_FIXPOINT_H__
#define __S2DBASE_FIXPOINT_H__

//--------------------------------------------------------------------------------------------------------
/// modify this to change [int:frac] bits
//WARN: MUST be EVEN( NOT ODD )
#define FP_FRACBITS			16
#define FP_HF_FRACBITS		(FP_FRACBITS>>1)
//--------------------------------------------------------------------------------------------------------
#define FP_ONE				((int)(1 << FP_FRACBITS))
#define FP_HALF				((int)(FP_ONE / 2))
#define FP_FRACMASK			((int)(FP_ONE - 1))
#define FP_INTMASK			((int)(0xFFFFFFFFL - FP_FRACMASK))
#define FP_FACTOR			((float)FP_ONE)
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class FixPoint
{
//--------------------------------------------------------------------------------------------------------
//### general:
public:
	FixPoint( void );
	//NOTE: why not simply give a FixPoint( int l ) as a constructor?
	//		because it produces a int->FixPoint conversion
	//		which should be int<<16, and gives an unexpected result.
	//		so I add a dummy param to constructor to avoid this conversion.
	//		further more, you should always use I2FP macro to do
	//		a int->FixPoint conversion for faster speed.
	//		so FixPoint( int l ) isn't given in FixPoint class.
	FixPoint( int l, const int dummy );

	FixPoint& operator = ( const FixPoint& fp );
//--------------------------------------------------------------------------------------------------------
//### conversions:
public:
	operator signed char	( void ) const;
	operator unsigned char	( void ) const;

	operator signed short	( void ) const;
	operator unsigned short	( void ) const;

	operator signed int		( void ) const;
	operator unsigned int	( void ) const;
//--------------------------------------------------------------------------------------------------------
//### operations:
public:
	/// positive
	FixPoint operator + ( void ) const;
	/// negative
	FixPoint operator - ( void ) const;

	FixPoint operator + ( const FixPoint& fp ) const;
	FixPoint operator - ( const FixPoint& fp ) const;
	FixPoint operator * ( const FixPoint& fp ) const;
	FixPoint operator / ( const FixPoint& fp ) const;

	void operator += ( const FixPoint& fp );
	void operator -= ( const FixPoint& fp );
	void operator *= ( const FixPoint& fp );
	void operator /= ( const FixPoint& fp );

	FixPoint operator <<  ( const int i ) const;
	void operator <<= ( const int i );
	FixPoint operator >>  ( const int i ) const;
	void operator >>= ( const int i );

	bool operator <  ( const FixPoint& fp ) const;
	bool operator <= ( const FixPoint& fp ) const;
	bool operator >  ( const FixPoint& fp ) const;
	bool operator >= ( const FixPoint& fp ) const;

	/// strict compare
	bool operator == ( const FixPoint& fp ) const;
	bool operator != ( const FixPoint& fp ) const;
//--------------------------------------------------------------------------------------------------------
//### data members:
public:
	int	data;
};
//--------------------------------------------------------------------------------------------------------
/// converts constant floats to fixpoints
//NOTE: non-constant floats should not exist as all calculations are based on fixpoints!
#define F2FP( f )			( s2d::FixPoint( (int)((f) * FP_FACTOR), 0 ) )
/// converts int16 to fixpoints
#define I2FP( i )			( s2d::FixPoint( (int)(i) << FP_FRACBITS, 0 ) )
/// converts fixpoints to constant floats
#define FP2F( fp )			( (float)(fp).data / FP_FACTOR )
/// converts fixpoints to int16
#define FP2I( fp )			( (fp).data >> FP_FRACBITS )
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_fixpoint.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_FIXPOINT_H__
