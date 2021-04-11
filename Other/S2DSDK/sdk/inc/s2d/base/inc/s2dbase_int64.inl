//=============================================================================
//
//	[ s2dbase_int64.inl ]
//
//	the s2d local platform independent implement of 64bits integer.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_INT64_INL__
#define __S2DBASE_INT64_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
inline Int64::Int64(void)
{
	low = high = 0;
}
//--------------------------------------------------------------------------------------------------------
inline Int64::~Int64(void)
{
}
//--------------------------------------------------------------------------------------------------------
inline Int64::Int64( const Int64& data )
{
	low = data.low;
	high = data.high;
}
//--------------------------------------------------------------------------------------------------------
inline Int64::Int64( const s32 val )
{
	if ( val >= 0 )
	{
		low = val;
		high = 0;
	}
	else
	{
		low = (u32)(val);
		high = ~0;
	}
}
//--------------------------------------------------------------------------------------------------------
inline Int64::Int64( const u32 val )
{
	low = val;
	high = 0;
}
//--------------------------------------------------------------------------------------------------------
inline void Int64::Not( void )
{
	low = ~low;
	high = ~high;
}
//--------------------------------------------------------------------------------------------------------
inline void Int64::Make64(const s32 _high, const u32 _low)
{
	low = _low;
	high = _high;
}
//--------------------------------------------------------------------------------------------------------
inline void Int64::negate( void )
{
	Not();

	this->operator ++();
}
//--------------------------------------------------------------------------------------------------------
inline void Int64::Zero( void )
{
	high = 0;
	low  = 0;
}
//--------------------------------------------------------------------------------------------------------
inline s32  Int64::IsZero( void ) const
{
	if ( high == 0 && low == 0 )
		return true;
	else
		return false;
}
//--------------------------------------------------------------------------------------------------------
inline Int64& Int64::operator = ( const s32 val )
{
	if ( val >= 0 ) {
		low = val;
		high = 0;
	} else {
		low = (u32)(val);
		high = ~0;
	}
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Int64& Int64::operator=  (const u32 val)
{
	low = val;
	high = 0;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Int64& Int64::operator = ( const Int64& val )
{
	low  = val.low;
	high = val.high;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Int64& Int64::operator++ (void)
{
	u32 tmp = low;
    low += 1;
    if ( low < tmp )
        high++;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Int64 Int64::operator++ (s32)
{
	Int64 temp;
	temp = *this;
	this->operator ++();
	return temp;
}
//--------------------------------------------------------------------------------------------------------
inline Int64& Int64::operator --(void)
{
    u32    tmp;
    tmp = low;
    low -= 1;
    if ( low >= tmp )
        high--;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Int64 Int64::operator --(s32)
{
	Int64 temp;
	temp = *this;
	this->operator --();
	return temp;
}
//--------------------------------------------------------------------------------------------------------
inline	void	Int64::operator>>=( u32 val )
{
	*this = this->operator >> ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void	Int64::operator>>=( s32 val )
{
	*this = this->operator >> ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void	Int64::operator<<=( u32 val )
{
	*this = this->operator << ( val );
}
//--------------------------------------------------------------------------------------------------------
inline  void	Int64::operator<<=( s32 val )
{
	*this = this->operator << ( val );
}
//--------------------------------------------------------------------------------------------------------
inline Int64 Int64::operator-  (void)
{
	Int64 tmp = *this;
	tmp.negate();
	return tmp;
}
//--------------------------------------------------------------------------------------------------------
inline bool  Int64::operator>  (const Int64& val) const
{
	if( high > val.high ) {
		return true;
	} else
	if( high < val.high ) {
		return false;
	} else {
		if( low > val.low ) {
			return true;
		} else {
			return false;
		}
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator>  (const s32 val) const
{
	Int64 iTmp( val );

	return this->operator > ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator>  (const u32 val) const
{
	Int64 iTmp( val );

	return this->operator > ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<  (const Int64& val) const
{
	if( high < val.high ) {
		return true;
	} else
	if( high > val.high ) {
		return false;
	} else {
		if( low < val.low ) {
			return true;
		} else {
			return false;
		}
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<  (const s32 val) const
{
	Int64 iTmp( val );

	return this->operator < ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<  (const u32 val) const
{
	Int64 iTmp( val );

	return this->operator < ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator>= (const Int64& val) const
{
	if( high > val.high ) {
		return true;
	} else
	if( high < val.high ) {
		return false;
	} else {
		if( low >= val.low ) {
			return true;
		} else {
			return false;
		}
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator>= (const s32 val) const
{
	Int64 iTmp( val );

	return this->operator >= ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator>= (const u32 val) const
{
	Int64 iTmp( val );

	return this->operator >= ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<= (const Int64& val) const
{
	if( high < val.high ) {
		return true;
	} else
	if( high > val.high ) {
		return false;
	} else {
		if( low <= val.low ) {
			return true;
		} else {
			return false;
		}
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<= (const s32 val) const
{
	Int64 iTmp( val );

	return this->operator <= ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator<= (const u32 val) const
{
	Int64 iTmp( val );

	return this->operator <= ( iTmp );
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator== (const Int64& val) const
{
	if ( low == val.low && high == val.high )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator== (const s32 val) const
{
	Int64 tmp = val;
	if ( high == tmp.high && low == tmp.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator== (const u32 val) const
{
	Int64 tmp = val;
	if ( high == tmp.high && low == tmp.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator!= (const Int64& val) const
{
	if ( high != val.high || low != val.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator!= (const s32 val) const
{
	Int64 tmp = val;
	if ( high != tmp.high || low != tmp.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   Int64::operator!= (const u32 val) const
{
	Int64 tmp = val;
	if ( high != tmp.high || low != tmp.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
// Uint64 Functions
//--------------------------------------------------------------------------------------------------------
inline Uint64::Uint64(void)
{
	Zero();
}
//--------------------------------------------------------------------------------------------------------
inline Uint64::Uint64( u32 _low, u32 _high )
{
	low  = _low;
	high = _high;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64::Uint64( const u32 val )
{
	low  = val;
	high = 0;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64::Uint64( const Uint64& u64 )
{
	low  = u64.low;
	high = u64.high;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64::~Uint64(void)
{
}
//--------------------------------------------------------------------------------------------------------
inline void Uint64::Make64( u32 _high, u32 _low )
{
	high = _high;
	low = _low;
}
//--------------------------------------------------------------------------------------------------------
inline void Uint64::Zero()
{
    low = 0;
    high = 0;
}
//--------------------------------------------------------------------------------------------------------
inline s32 Uint64::IsZero( void ) const
{
    if ((low == 0) && (high == 0))
        return (true);
    else
        return (false);
}
//--------------------------------------------------------------------------------------------------------
inline Uint64& Uint64::operator = ( const Uint64& val )
{
	low = val.low;
	high = val.high;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64& Uint64::operator = ( const u32 val )
{
	low = val;
	high = 0;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64& Uint64::operator = ( const s32 val )
{
	low = val;
	high = 0;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64& Uint64::operator-- (void)
{
    u32    tmp;
    tmp = low;
    low -= 1;
    if ( low >= tmp )
        high--;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64  Uint64::operator-- (s32)
{
	Uint64 temp;
	temp = *this;
	this->operator --();
	return temp;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64& Uint64::operator++ (void)
{
	u32 tmp = low;
    low += 1;
    if ( low < tmp )
        high++;
	return *this;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64  Uint64::operator++ (s32)
{
	Uint64 temp;
	temp = *this;
	this->operator ++();
	return temp;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator == ( const Uint64& val ) const
{
	if ( val.high != high )
		return false;
	if ( val.low != low )
		return false;

	return true;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator == ( const u32 val ) const
{
	if ( val == low && high == 0 )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator == ( const s32 val ) const
{
	if( val == (s32)low && high == 0 )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator != ( const Uint64& val ) const
{
	if ( low != val.high || high != val.high )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator != ( const u32 val ) const
{
	if ( val != low || high != 0 )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator != ( const s32 val ) const
{
	if ( val != (s32)low || high != 0 )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator > ( const Uint64& val ) const
{
	if ( high > val.high )
		return true;
	else if ( high < val.high )
		return false;
	else
	{
		if ( low > val.low )
			return true;
		else
			return false;
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator > ( const u32 val ) const
{
	if( high > 0 || low > val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator > ( const s32 val ) const
{
	if( high > 0 || low > ( const u32 )val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator >=( const Uint64& val ) const
{
	if ( high > val.high )
		return true;
	else if ( high < val.high )
		return false;
	else
	{
		if ( low >= val.low )
			return true;
		else
			return false;
	}
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator >= ( const u32 val ) const
{
	if ( high > 0 || low >= val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator >= ( const s32 val ) const
{
	if ( high > 0 || low >= ( const u32 )val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator < ( const Uint64& val ) const
{
	if ( high < val.high )
		return true;
	else if ( high == val.high )
	{
		if ( low < val.low )
			return true;
	}
	
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator < ( const u32 val ) const
{
	if ( high == 0 && low < val )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator < ( const s32 val ) const
{
	if ( high == 0 && low < ( const u32 )val )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator <=( const Uint64& val ) const
{
	if( high < val.high )
		return true;
	else if ( high == val.high )
	{
		if ( low <= val.low )
			return true;
	}

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator <=( const u32 val ) const
{
	if ( high == 0 && low <= val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool Uint64::operator <=( const s32 val ) const
{
	if ( high == 0 && low <= ( const u32 )val )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 Uint64::operator * ( const s32 val1 ) const
{
	return this->operator* ( ( const u32 )val1 );
}
//--------------------------------------------------------------------------------------------------------
inline void Uint64::operator *= ( const s32 val )
{
	*this = this->operator* ( val );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator + ( const u32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator+( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator + ( const s32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator+( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator - ( const u32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator-( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator - ( const s32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator-( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator * ( const u32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator*( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator * ( const s32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator*( rval );
}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator / ( const u32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator/( rval );

}
//--------------------------------------------------------------------------------------------------------
inline Uint64 operator / ( const s32 lval, const Uint64& rval )
{
	Uint64 tmp( lval );

	return tmp.operator/( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool operator == ( const u32 lval, const Uint64 rval )
{
	Uint64 tmp( lval );

	return tmp.operator==( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool operator == ( const s32 lval, const Uint64 rval )
{
	Uint64 tmp( lval );

	return tmp.operator==( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool operator != ( const u32 lval, const Uint64 rval ) 
{
	Uint64 tmp( lval );

	return tmp.operator!=( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool operator != ( const s32 lval, const Uint64 rval ) 
{
	Uint64 tmp( lval );

	return tmp.operator!=( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator > ( const u32 lval, const Uint64& rval )
{
	if( rval.high == 0 && lval > rval.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator > ( const s32 lval, const Uint64& rval )
{
	if( rval.high == 0 && ( const u32 )lval > rval.low )
		return true;
	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator >= ( const u32 lval, const Uint64& rval ) 
{
	if ( rval.high == 0 && lval >= rval.low )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator >= ( const s32 lval, const Uint64& rval ) 
{
	if ( rval.high == 0 && ( const u32 )lval >= rval.low )
		return true;

	return false;
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator < ( const u32 val, const Uint64& val2 ) 
{
	return ( val2 > val );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator < ( const s32 val, const Uint64& val2 ) 
{
	return ( val2 > ( const u32 )val );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator <=( const u32 val1, const Uint64& val2 )
{
	return !( val2 > val1 );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator <=( const s32 val1, const Uint64& val2 )
{
	return !( val2 > (const u32)val1 );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator-= (const Int64& val) 
{
	*this = this->operator -( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator-= (const s32 val)
{
	*this = this->operator- ( val );
}
//--------------------------------------------------------------------------------------------------------
inline void  Int64::operator-= (const u32 val)
{
	*this = this->operator- ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator+= (const Int64& val)
{
	*this = this->operator+ ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator+= (const s32 val)
{
	*this = this->operator+ ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator+= (const u32 val)
{
	*this = this->operator+ ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator*= (const Int64& val)
{
	*this = this->operator* ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator*= (const s32 val)
{
	*this = this->operator* ( val );
}
//--------------------------------------------------------------------------------------------------------
inline	void  Int64::operator*= (const u32 val)
{
	*this = this->operator* ( val );
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator-  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator- ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator-  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator- ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator+  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator+ ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator+  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator+ ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator*  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator* ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator*  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator* ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator/  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator/ ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator/  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator/ ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator%  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator% ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline Int64  operator%  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	li64.operator% ( rval );
	return li64;
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator>  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return li64.operator> ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator>  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator> ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator<  (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator< ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator<  (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return li64.operator< ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator>= (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator>= ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator>= (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator>= ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator<= (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator<= ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator<= (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator<= ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator== (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator== ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator== (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator== ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator!= (const s32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator!= ( rval );
}
//--------------------------------------------------------------------------------------------------------
inline bool   operator!= (const u32 lval, const Int64& rval)
{
	Int64 li64(lval);

	return 	li64.operator!= ( rval );
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_INT64_INL__
