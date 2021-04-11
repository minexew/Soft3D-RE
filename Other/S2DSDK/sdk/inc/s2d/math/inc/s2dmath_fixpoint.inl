//=============================================================================
//
//	[ s2dmath_fixpoint.inl ]
//
//	the s2d fixpoint proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DMATH_FIXPOINT_INL__
#define __S2DMATH_FIXPOINT_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### helper functions:
//--------------------------------------------------------------------------------------------------------
inline int round( const FixPoint& fp )
{
	return ((fp.data + FP_HALF) >> FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint fabs( const FixPoint& fp )
{
	return FixPoint( Abs(fp.data), 0 );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint fabsf( const FixPoint& fp )
{
	return fabs( fp );
}
//--------------------------------------------------------------------------------------------------------
inline int ceil( const FixPoint& fp )
{
	return (fp.data + FP_FRACMASK) >> FP_FRACBITS;
}
//--------------------------------------------------------------------------------------------------------
inline int ceilf( const FixPoint& fp )
{
	return ceil( fp );
}
//--------------------------------------------------------------------------------------------------------
inline int floor( const FixPoint& fp )
{
	return fp.data >> FP_FRACBITS;
}
//--------------------------------------------------------------------------------------------------------
inline int floorf( const FixPoint& fp )
{
	return floor( fp );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint frac( const FixPoint& fp )
{
	return FixPoint( fp.data & FP_FRACMASK, 0 );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint sqrtf( const FixPoint& fp )
{
	return sqrt( fp );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint rsqrtf( const FixPoint& fp )
{
	return rsqrt( fp );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint sinf( short ang )
{
	return sin( ang );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint cos( short ang )
{
	return sin( (ang + S2D_ANG_HALFPI) & S2D_ANG_FULLMASK );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint cosf( short ang )
{
	return cos( ang );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint tanf( short ang )
{
	return tan( ang );
}
//--------------------------------------------------------------------------------------------------------
inline short asinf	( const FixPoint& fp )
{
	return asin( fp );
}
//--------------------------------------------------------------------------------------------------------
inline short acosf	( const FixPoint& fp )
{
	return acos( fp );
}
//--------------------------------------------------------------------------------------------------------
inline short atanf	( const FixPoint& fp )
{
	return atan( fp );
}
//--------------------------------------------------------------------------------------------------------
/// faster than I2FP(1)/fp
inline FixPoint finv( const FixPoint& fp )
{
	assert( (fp.data>>2) != 0 );
	int tmp = (1<<30) / (fp.data>>2);

	return FixPoint( tmp, 0 );
}
//--------------------------------------------------------------------------------------------------------
extern const long InvLUT[ 0x10000 ];
//--------------------------------------------------------------------------------------------------------
inline long invsp( unsigned long data )
{
	return InvLUT[ data ];
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u16( unsigned long data )
{
		data = invsp( data ) << 2;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u17( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
		data = invsp( data>>1 ) << 1;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u18( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
		data = invsp( data>>2 );
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u19( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
		data = invsp( data>>3 ) >> 1;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u20( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
		data = invsp( data>>4 ) >> 2;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u21( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
		data = invsp( data>>5 ) >> 3;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u22( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
		data = invsp( data>>6 ) >> 4;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u23( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
	if( data < (32<<FP_FRACBITS) ) {
		data = invsp( data>>5 ) >> 3;
		return data;
	}
	//if( data < (64<<FP_FRACBITS) ) {
	//	data = invsp( data>>6 ) >> 4;
	//	return data;
	//}
		data = invsp( data>>7 ) >> 5;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u24( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
	if( data < (64<<FP_FRACBITS) ) {
		data = invsp( data>>6 ) >> 4;
		return data;
	}
	//if( data < (128<<FP_FRACBITS) ) {
	//	data = invsp( data>>7 ) >> 5;
	//	return data;
	//}
		data = invsp( data>>8 ) >> 6;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u25( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
	if( data < (32<<FP_FRACBITS) ) {
		data = invsp( data>>5 ) >> 3;
		return data;
	}
	//if( data < (64<<FP_FRACBITS) ) {
	//	data = invsp( data>>6 ) >> 4;
	//	return data;
	//}
	if( data < (128<<FP_FRACBITS) ) {
		data = invsp( data>>7 ) >> 5;
		return data;
	}
	//if( data < (256<<FP_FRACBITS) ) {
	//	data = invsp( data>>8 ) >> 6;
	//	return data;
	//}
		data = invsp( data>>9 ) >> 7;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u26( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
	if( data < (64<<FP_FRACBITS) ) {
		data = invsp( data>>6 ) >> 4;
		return data;
	}
	//if( data < (128<<FP_FRACBITS) ) {
	//	data = invsp( data>>7 ) >> 5;
	//	return data;
	//}
	if( data < (256<<FP_FRACBITS) ) {
		data = invsp( data>>8 ) >> 6;
		return data;
	}
	//if( data < (512<<FP_FRACBITS) ) {
	//	data = invsp( data>>9 ) >> 7;
	//	return data;
	//}
		data = invsp( data>>10 ) >> 8;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u27( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
	if( data < (32<<FP_FRACBITS) ) {
		data = invsp( data>>5 ) >> 3;
		return data;
	}
	//if( data < (64<<FP_FRACBITS) ) {
	//	data = invsp( data>>6 ) >> 4;
	//	return data;
	//}
	if( data < (128<<FP_FRACBITS) ) {
		data = invsp( data>>7 ) >> 5;
		return data;
	}
	//if( data < (256<<FP_FRACBITS) ) {
	//	data = invsp( data>>8 ) >> 6;
	//	return data;
	//}
	if( data < (512<<FP_FRACBITS) ) {
		data = invsp( data>>9 ) >> 7;
		return data;
	}
	//if( data < (1024<<FP_FRACBITS) ) {
	//	data = invsp( data>>10 ) >> 8;
	//	return data;
	//}
		data = invsp( data>>11 ) >> 9;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u28( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
	if( data < (64<<FP_FRACBITS) ) {
		data = invsp( data>>6 ) >> 4;
		return data;
	}
	//if( data < (128<<FP_FRACBITS) ) {
	//	data = invsp( data>>7 ) >> 5;
	//	return data;
	//}
	if( data < (256<<FP_FRACBITS) ) {
		data = invsp( data>>8 ) >> 6;
		return data;
	}
	//if( data < (512<<FP_FRACBITS) ) {
	//	data = invsp( data>>9 ) >> 7;
	//	return data;
	//}
	if( data < (1024<<FP_FRACBITS) ) {
		data = invsp( data>>10 ) >> 8;
		return data;
	}
	//if( data < (2048<<FP_FRACBITS) ) {
	//	data = invsp( data>>11 ) >> 9;
	//	return data;
	//}
		data = invsp( data>>12 ) >> 10;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u29( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
	if( data < (32<<FP_FRACBITS) ) {
		data = invsp( data>>5 ) >> 3;
		return data;
	}
	//if( data < (64<<FP_FRACBITS) ) {
	//	data = invsp( data>>6 ) >> 4;
	//	return data;
	//}
	if( data < (128<<FP_FRACBITS) ) {
		data = invsp( data>>7 ) >> 5;
		return data;
	}
	//if( data < (256<<FP_FRACBITS) ) {
	//	data = invsp( data>>8 ) >> 6;
	//	return data;
	//}
	if( data < (512<<FP_FRACBITS) ) {
		data = invsp( data>>9 ) >> 7;
		return data;
	}
	//if( data < (1024<<FP_FRACBITS) ) {
	//	data = invsp( data>>10 ) >> 8;
	//	return data;
	//}
	if( data < (2048<<FP_FRACBITS) ) {
		data = invsp( data>>11 ) >> 9;
		return data;
	}
	//if( data < (4096<<FP_FRACBITS) ) {
	//	data = invsp( data>>12 ) >> 10;
	//	return data;
	//}
		data = invsp( data>>13 ) >> 11;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u30( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
	if( data < (64<<FP_FRACBITS) ) {
		data = invsp( data>>6 ) >> 4;
		return data;
	}
	//if( data < (128<<FP_FRACBITS) ) {
	//	data = invsp( data>>7 ) >> 5;
	//	return data;
	//}
	if( data < (256<<FP_FRACBITS) ) {
		data = invsp( data>>8 ) >> 6;
		return data;
	}
	//if( data < (512<<FP_FRACBITS) ) {
	//	data = invsp( data>>9 ) >> 7;
	//	return data;
	//}
	if( data < (1024<<FP_FRACBITS) ) {
		data = invsp( data>>10 ) >> 8;
		return data;
	}
	//if( data < (2048<<FP_FRACBITS) ) {
	//	data = invsp( data>>11 ) >> 9;
	//	return data;
	//}
	if( data < (4096<<FP_FRACBITS) ) {
		data = invsp( data>>12 ) >> 10;
		return data;
	}
	//if( data < (8192<<FP_FRACBITS) ) {
	//	data = invsp( data>>13 ) >> 11;
	//	return data;
	//}
		data = invsp( data>>14 ) >> 12;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u31( unsigned long data )
{
	//if( data < (1<<FP_FRACBITS) ) {
	//	data = invsp( data ) << 2;
	//	return data;
	//}
	if( data < (2<<FP_FRACBITS) ) {
		data = invsp( data>>1 ) << 1;
		return data;
	}
	//if( data < (4<<FP_FRACBITS) ) {
	//	data = invsp( data>>2 );
	//	return data;
	//}
	if( data < (8<<FP_FRACBITS) ) {
		data = invsp( data>>3 ) >> 1;
		return data;
	}
	//if( data < (16<<FP_FRACBITS) ) {
	//	data = invsp( data>>4 ) >> 2;
	//	return data;
	//}
	if( data < (32<<FP_FRACBITS) ) {
		data = invsp( data>>5 ) >> 3;
		return data;
	}
	//if( data < (64<<FP_FRACBITS) ) {
	//	data = invsp( data>>6 ) >> 4;
	//	return data;
	//}
	if( data < (128<<FP_FRACBITS) ) {
		data = invsp( data>>7 ) >> 5;
		return data;
	}
	//if( data < (256<<FP_FRACBITS) ) {
	//	data = invsp( data>>8 ) >> 6;
	//	return data;
	//}
	if( data < (512<<FP_FRACBITS) ) {
		data = invsp( data>>9 ) >> 7;
		return data;
	}
	//if( data < (1024<<FP_FRACBITS) ) {
	//	data = invsp( data>>10 ) >> 8;
	//	return data;
	//}
	if( data < (2048<<FP_FRACBITS) ) {
		data = invsp( data>>11 ) >> 9;
		return data;
	}
	//if( data < (4096<<FP_FRACBITS) ) {
	//	data = invsp( data>>12 ) >> 10;
	//	return data;
	//}
	if( data < (8192<<FP_FRACBITS) ) {
		data = invsp( data>>13 ) >> 11;
		return data;
	}
	//if( data < (16384<<FP_FRACBITS) ) {
	//	data = invsp( data>>14 ) >> 12;
	//	return data;
	//}
		data = invsp( data>>15 ) >> 13;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_u32( unsigned long data )
{
	if( data < (1<<FP_FRACBITS) ) {
		data = invsp( data ) << 2;
		return data;
	}
	//if( data < (2<<FP_FRACBITS) ) {
	//	data = invsp( data>>1 ) << 1;
	//	return data;
	//}
	if( data < (4<<FP_FRACBITS) ) {
		data = invsp( data>>2 );
		return data;
	}
	//if( data < (8<<FP_FRACBITS) ) {
	//	data = invsp( data>>3 ) >> 1;
	//	return data;
	//}
	if( data < (16<<FP_FRACBITS) ) {
		data = invsp( data>>4 ) >> 2;
		return data;
	}
	//if( data < (32<<FP_FRACBITS) ) {
	//	data = invsp( data>>5 ) >> 3;
	//	return data;
	//}
	if( data < (64<<FP_FRACBITS) ) {
		data = invsp( data>>6 ) >> 4;
		return data;
	}
	//if( data < (128<<FP_FRACBITS) ) {
	//	data = invsp( data>>7 ) >> 5;
	//	return data;
	//}
	if( data < (256<<FP_FRACBITS) ) {
		data = invsp( data>>8 ) >> 6;
		return data;
	}
	//if( data < (512<<FP_FRACBITS) ) {
	//	data = invsp( data>>9 ) >> 7;
	//	return data;
	//}
	if( data < (1024<<FP_FRACBITS) ) {
		data = invsp( data>>10 ) >> 8;
		return data;
	}
	//if( data < (2048<<FP_FRACBITS) ) {
	//	data = invsp( data>>11 ) >> 9;
	//	return data;
	//}
	if( data < (4096<<FP_FRACBITS) ) {
		data = invsp( data>>12 ) >> 10;
		return data;
	}
	//if( data < (8192<<FP_FRACBITS) ) {
	//	data = invsp( data>>13 ) >> 11;
	//	return data;
	//}
	if( data < (16384<<FP_FRACBITS) ) {
		data = invsp( data>>14 ) >> 12;
		return data;
	}
	//if( data < (32768<<FP_FRACBITS) ) {
	//	data = invsp( data>>15 ) >> 13;
	//	return data;
	//}
		data = invsp( data>>16 ) >> 14;
		return data;
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s16( long data )
{
	if( data > 0 ) {
		return invsp_u16( data );
	}
	return -invsp_u16( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s17( long data )
{
	if( data > 0 ) {
		return invsp_u17( data );
	}
	return -invsp_u17( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s18( long data )
{
	if( data > 0 ) {
		return invsp_u18( data );
	}
	return -invsp_u18( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s19( long data )
{
	if( data > 0 ) {
		return invsp_u19( data );
	}
	return -invsp_u19( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s20( long data )
{
	if( data > 0 ) {
		return invsp_u20( data );
	}
	return -invsp_u20( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s21( long data )
{
	if( data > 0 ) {
		return invsp_u21( data );
	}
	return -invsp_u21( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s22( long data )
{
	if( data > 0 ) {
		return invsp_u22( data );
	}
	return -invsp_u22( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s23( long data )
{
	if( data > 0 ) {
		return invsp_u23( data );
	}
	return -invsp_u23( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s24( long data )
{
	if( data > 0 ) {
		return invsp_u24( data );
	}
	return -invsp_u24( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s25( long data )
{
	if( data > 0 ) {
		return invsp_u25( data );
	}
	return -invsp_u25( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s26( long data )
{
	if( data > 0 ) {
		return invsp_u26( data );
	}
	return -invsp_u26( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s27( long data )
{
	if( data > 0 ) {
		return invsp_u27( data );
	}
	return -invsp_u27( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s28( long data )
{
	if( data > 0 ) {
		return invsp_u28( data );
	}
	return -invsp_u28( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s29( long data )
{
	if( data > 0 ) {
		return invsp_u29( data );
	}
	return -invsp_u29( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s30( long data )
{
	if( data > 0 ) {
		return invsp_u30( data );
	}
	return -invsp_u30( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s31( long data )
{
	if( data > 0 ) {
		return invsp_u31( data );
	}
	return -invsp_u31( -data );
}
//--------------------------------------------------------------------------------------------------------
inline long invsp_s32( long data )
{
	if( data > 0 ) {
		return invsp_u32( data );
	}
	return -invsp_u32( -data );
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DMATH_FIXPOINT_INL__
