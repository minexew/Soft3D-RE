//=============================================================================
//
//	[ s2dbase_int64.h ]
//
//	the s2d local platform independent implement of 64bits integer.
//
//	PLATFORM INDEPENDENT!!!平台无关
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_INT64_H__
#define	__S2DBASE_INT64_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
//------------------------------------------- -------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
// Int64:
//--------------------------------------------------------------------------------------------------------
class Int64
{
public:
    s32	high;
	u32	low;

public:
	Int64( void );
	Int64( s32 val );
	Int64( u32 val );
	Int64( s32 high, u32 low );
	Int64( const Int64& data );
	~Int64( void );

public:
	void Zero( void );
	s32  IsZero( void ) const;
	void Multiply( s32 val1, s32 val2 );

public:
	Int64& operator=  (s32);
	Int64& operator=  (u32);
	Int64& operator=  (const Int64&);
	Int64& operator-- (void);
	Int64  operator-- (s32);
	Int64& operator++ (void);
	Int64  operator++ (s32);
	Int64  operator-  (void);
	Int64  operator-  (const Int64&) const;
	Int64  operator-  (s32) const;
	Int64  operator-  (u32) const;
	Int64  operator+  (const Int64&) const;
	Int64  operator+  (s32) const;
	Int64  operator+  (u32) const;
	Int64  operator*  (const Int64&) const;
	Int64  operator*  (s32) const;
	Int64  operator*  (u32) const;
	Int64  operator/  (const Int64) const;
	Int64  operator/  (u32) const;
	Int64  operator/  (s32) const;
	Int64  operator%  (const Int64&) const;
	Int64  operator%  (s32) const;
	Int64  operator%  (u32) const;
	Int64  operator>> (u32) const;
	Int64  operator>> (s32) const;
	Int64  operator<< (u32) const;
	Int64  operator<< (s32) const;
	bool   operator>  (const Int64&) const;
	bool   operator>  (s32) const;
	bool   operator>  (u32) const;
	bool   operator<  (const Int64&) const;
	bool   operator<  (s32) const;
	bool   operator<  (u32) const;

	void	operator-= (const Int64&);
	void	operator-= (s32);
	void	operator-= (u32);
	void	operator+= (const Int64&);
	void	operator+= (s32);
	void	operator+= (u32);
	void	operator*= (const Int64&);
	void	operator*= (s32);
	void	operator*= (u32);
	void	operator/= (const Int64);
	void	operator/= (u32);
	void	operator/= (s32);
	void	operator>>=( u32 val );
	void	operator>>=( s32 val );
	void	operator<<=( u32 val );
	void	operator<<=( s32 val );
	void	operator%= (const Int64&);
	void	operator%= (s32);
	void	operator%= (u32);

	bool   operator>= (const Int64&) const;
	bool   operator>= (s32) const;
	bool   operator>= (u32) const;
	bool   operator<= (const Int64&) const;
	bool   operator<= (s32) const;
	bool   operator<= (u32) const;
	bool   operator== (const Int64&) const;
	bool   operator== (s32) const;
	bool   operator== (u32) const;
	bool   operator!= (const Int64&) const;
	bool   operator!= (s32) const;
	bool   operator!= (u32) const;

private:
	void negate( void );
	void Not( void );
	void Make64(s32 high, u32 low);

#ifdef _DEBUG
public:
	void PrintI64(char* buf) const;
	s32  Read64(char* buf);
#endif
};
//--------------------------------------------------------------------------------------------------------
// Uint64:
//--------------------------------------------------------------------------------------------------------
class Uint64
{
public:
	u32 high;
	u32 low;

public:
	Uint64( void );
	Uint64( u32 val );
	Uint64( u32 low, u32 high );
	Uint64( const Uint64& u64 );
	~Uint64( void );

public:
	void Zero( void );
	s32  IsZero( void ) const;
	void Multiply( u32 val1, u32 val2 );

public:
	Uint64& operator = ( const Uint64& val );
	Uint64& operator = ( u32 val );
	Uint64& operator = ( s32 val );
	Uint64& operator-- (void);
	Uint64  operator-- (s32);
	Uint64& operator++ (void);
	Uint64  operator++ (s32);
	Uint64  operator + ( const Uint64& val ) const;
	Uint64  operator + ( u32 val ) const;
	Uint64  operator + ( s32 val ) const;
	Uint64  operator - ( const Uint64& val ) const;
	Uint64  operator - ( u32 val ) const;
	Uint64  operator - ( s32 val ) const;
	Uint64  operator * ( u32 val ) const;
	Uint64  operator * ( const Uint64& val ) const;
	Uint64  operator * ( s32 val ) const;
	Uint64	operator / ( const Uint64& val ) const;
	Uint64  operator / ( u32 val ) const;
	Uint64  operator / ( s32 val ) const;
	Uint64  operator >>( u32 val ) const;
	Uint64  operator <<( u32 val ) const;
	Uint64  operator >>( s32 val ) const;
	Uint64  operator <<( s32 val ) const;
	Uint64  operator % ( const Uint64& val ) const;
	Uint64  operator % ( u32 val ) const;
	Uint64  operator % ( s32 val ) const;

	void	operator += ( const Uint64& val );
	void	operator += ( u32 val );
	void	operator += ( s32 val );
	void	operator -= ( const Uint64& val );
	void	operator -= ( u32 val );
	void	operator -= ( s32 val );
	void	operator *= ( u32 val );
	void	operator *= ( const Uint64& val );
	void	operator *= ( s32 val );
	void	operator /= ( const Uint64& val );
	void	operator /= ( u32 val );
	void	operator /= ( s32 val );
	void	operator >>=( u32 val );
	void	operator >>=( s32 val );
	void	operator <<=( u32 val );
	void	operator <<=( s32 val );
	void	operator %= ( const Uint64& val );
	void	operator %= ( u32 val );
	void	operator %= ( s32 val );

	bool	operator ==( const Uint64& val ) const;
	bool	operator !=( const Uint64& val ) const;
	bool	operator > ( const Uint64& val ) const;
	bool	operator >=( const Uint64& val ) const;
	bool	operator < ( const Uint64& val ) const;
	bool	operator <=( const Uint64& val ) const;
	bool	operator ==( u32 val ) const;
	bool	operator !=( u32 val ) const;
	bool	operator > ( u32 val ) const;
	bool	operator >=( u32 val ) const;
	bool	operator < ( u32 val ) const;
	bool	operator <=( u32 val ) const;
	bool	operator ==( s32 val ) const;
	bool	operator !=( s32 val ) const;
	bool	operator > ( s32 val ) const;
	bool	operator >=( s32 val ) const;
	bool	operator < ( s32 val ) const;
	bool	operator <=( s32 val ) const;

private:
	void IncrByU32( u32 );
	void Make64( u32 high, u32 low );

#ifdef _DEBUG
public:
	void Div10( u32* );
	void Mult10( void );
    void PrintU64( char* );
	s32  Read64( const char* );
#endif
};
//--------------------------------------------------------------------------------------------------------
};

#include "s2dbase_int64.inl"

#endif//__S2DBASE_INT64_H__
