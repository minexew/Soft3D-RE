//=============================================================================
//
//	[ s2dbase_fixpoint.inl ]
//
//	the s2d encapsulation of FixedPoint data type.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_FIXPOINT_INL__
#define __S2DBASE_FIXPOINT_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### general::
//--------------------------------------------------------------------------------------------------------
inline FixPoint::FixPoint( void ) : data(0)
{
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::FixPoint( int l, const int dummy ) : data(l)
{
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint& FixPoint::operator = ( const FixPoint& fp )
{
	data = fp.data;
	return (*this);
}
//--------------------------------------------------------------------------------------------------------
//### conversions:
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator signed char( void ) const
{
	return (signed char)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator unsigned char( void ) const
{
	return (unsigned char)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator signed short( void ) const
{
	return (signed short)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator unsigned short( void ) const
{
	return (unsigned short)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator signed int( void ) const
{
	return (signed int)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint::operator unsigned int( void ) const
{
	return (unsigned int)(data>>FP_FRACBITS);
}
//--------------------------------------------------------------------------------------------------------
//### operations:
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator + ( void ) const
{
	return (*this);
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator - ( void ) const
{
	return FixPoint( -data, 0 );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator + ( const FixPoint& fp ) const
{
#if FP_VALIDATE
	//range check
	Int64 big = Int64(data) + Int64(fp.data);
	assert( big <= 0xFFFFFFFF );

	//sign check
	int tmp = big.low;

	if( data>0 && fp.data>0 )	assert( tmp > 0 );
	if( data<0 && fp.data<0 )	assert( tmp < 0 );
#endif

	return FixPoint( data + fp.data, 0 );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator - ( const FixPoint& fp ) const
{
#if FP_VALIDATE
	//sign check
	int tmp = data - fp.data;

	if( data>0 && fp.data<0 )	assert( tmp > 0 );
	if( data<0 && fp.data>0 )	assert( tmp < 0 );
#endif

	return FixPoint( data - fp.data, 0 );
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator * ( const FixPoint& fp ) const
{
#ifdef _DEBUG
	int s1 = data >> 31;
	int s2 = fp.data >> 31;
	unsigned long n1 = (data ^ s1) - s1;
	unsigned long n2 = (fp.data ^ s2) - s2;
	unsigned short l1 = (unsigned short)(n1 & 0x0000FFFF);
	unsigned short h1 = (unsigned short)((n1 & 0xFFFF0000) >> 16);
	unsigned short l2 = (unsigned short)(n2 & 0x0000FFFF);
	unsigned short h2 = (unsigned short)((n2 & 0xFFFF0000) >> 16);

	unsigned long res =
		(((l1 * l2) & 0xFFFF0000) >> 16) +
		(l1 * h2) +
		(l2 * h1) +
		(((h1 * h2) & 0x0000FFFF) << 16);
	res &= 0x7FFFFFFF;

	long tmp = res;
	if( s1 ^ s2 )
		tmp = -tmp;

#else
    int tmp;

	asm volatile ( "SMULL r6, r7, %0, %1" : : "r"(data), "r"(fp.data) : "r6", "r7" );
	asm volatile ( "MOV r6, r6, LSR #16" : : : "r6" );
	asm volatile ( "ORR r6, r6, r7, LSL #16" : : : "r6" );
	asm volatile ( "STR r6, %0" : "=m"(tmp) );

#endif

#if FP_VALIDATE
	//sign check
	if( data>=0 && fp.data>=0 )	assert( tmp >= 0 );
	if( data>=0 && fp.data<=0 )	assert( tmp <= 0 );
	if( data<=0 && fp.data<=0 )	assert( tmp >= 0 );
	if( data<=0 && fp.data>=0 )	assert( tmp <= 0 );
#endif

	return FixPoint( tmp, 0 );

}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator / ( const FixPoint& fp ) const
{
	assert( fp.data );
	float dividend = FP2F(*this), divisor = FP2F(fp);

	return F2FP( dividend/divisor );

	//Int64 big = ((Int64((int)data))<<FP_FRACBITS) / Int64(((int)fp.data));
	//int tmp = big.low;

#if FP_VALIDATE
	//sign check
	if( data>=0 && fp.data>0 )	assert( tmp >= 0 );
	if( data>=0 && fp.data<0 )	assert( tmp <= 0 );
	if( data<=0 && fp.data<0 )	assert( tmp >= 0 );
	if( data<=0 && fp.data>0 )	assert( tmp <= 0 );
#endif

   // return FixPoint( tmp, 0 );
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator += ( const FixPoint& fp )
{
#if FP_VALIDATE
	//range check
	Int64 big = Int64(data) + Int64(fp.data);
	assert( big <= 0xFFFFFFFF );

	//sign check
	int tmp = big.low;

	if( data>0 && fp.data>0 )	assert( tmp > 0 );
	if( data<0 && fp.data<0 )	assert( tmp < 0 );
#endif

	data += fp.data;
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator -= ( const FixPoint& fp )
{
#if FP_VALIDATE
	//sign check
	int tmp = data - fp.data;

	if( data>0 && fp.data<0 )	assert( tmp > 0 );
	if( data<0 && fp.data>0 )	assert( tmp < 0 );
#endif

	data -= fp.data;
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator *= ( const FixPoint& fp )
{
#if FP_VALIDATE
	int tmp = data;
#endif

#ifdef _DEBUG
	int s1 = data >> 31;
	int s2 = fp.data >> 31;
	unsigned long n1 = (data ^ s1) - s1;
	unsigned long n2 = (fp.data ^ s2) - s2;
	unsigned short l1 = (unsigned short)(n1 & 0x0000FFFF);
	unsigned short h1 = (unsigned short)((n1 & 0xFFFF0000) >> 16);
	unsigned short l2 = (unsigned short)(n2 & 0x0000FFFF);
	unsigned short h2 = (unsigned short)((n2 & 0xFFFF0000) >> 16);

	unsigned long res =
		(((l1 * l2) & 0xFFFF0000) >> 16) +
		(l1 * h2) +
		(l2 * h1) +
		(((h1 * h2) & 0x0000FFFF) << 16);
	res &= 0x7FFFFFFF;

	data = res;
	if( s1 ^ s2 )
		data = -data;
#else
	asm volatile ( "SMULL r6, r7, %0, %1" : : "r"(data), "r"(fp.data) : "r6", "r7" );
	asm volatile ( "MOV r6, r6, LSR #16" : : : "r6" );
	asm volatile ( "ORR r6, r6, r7, LSL #16" : : : "r6" );
	asm volatile ( "STR r6, %0" : "=m"(data) );
#endif

#if FP_VALIDATE
	//sign check
	if( tmp>=0 && fp.data>=0 )	assert( data >= 0 );
	if( tmp>=0 && fp.data<=0 )	assert( data <= 0 );
	if( tmp<=0 && fp.data<=0 )	assert( data >= 0 );
	if( tmp<=0 && fp.data>=0 )	assert( data <= 0 );
#endif
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator /= ( const FixPoint& fp )
{
	assert( fp.data );

#if FP_VALIDATE
	int tmp = data;
#endif

	float dividend = FP2F(*this), divisor = FP2F(fp);
	FixPoint tempfp = F2FP( dividend/divisor );
	data =  tempfp.data;

	Int64 big = ((Int64((int)data))<<FP_FRACBITS) / Int64(((int)fp.data));
	data = big.low;
	//TInt64 big = ((TInt64((int)data))<<FP_FRACBITS) / TInt64(((int)fp.data));
	//data = big.Low();
#if FP_VALIDATE
	//sign check
	if( tmp>=0 && fp.data>0 )	assert( data >= 0 );
	if( tmp>=0 && fp.data<0 )	assert( data <= 0 );
	if( tmp<=0 && fp.data<0 )	assert( data >= 0 );
	if( tmp<=0 && fp.data>0 )	assert( data <= 0 );
#endif
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator <<  ( const int i ) const
{
	return FixPoint(data<<i, 0);
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator <<= ( const int i )
{
	data <<= i;
}
//--------------------------------------------------------------------------------------------------------
inline FixPoint FixPoint::operator >>  ( const int i ) const
{
	return FixPoint(data>>i, 0);
}
//--------------------------------------------------------------------------------------------------------
inline void FixPoint::operator >>= ( const int i )
{
	data >>= i;
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator <  ( const FixPoint& fp ) const
{
	return (data < fp.data);
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator <= ( const FixPoint& fp ) const
{
	return (data <= fp.data);
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator >  ( const FixPoint& fp ) const
{
	return (data > fp.data);
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator >= ( const FixPoint& fp ) const
{
	return (data >= fp.data);
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator == ( const FixPoint& fp ) const
{
	return (data == fp.data);
}
//--------------------------------------------------------------------------------------------------------
inline bool FixPoint::operator != ( const FixPoint& fp ) const
{
	return (data != fp.data);
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d

#endif//__S2DBASE_FIXPOINT_INL__
