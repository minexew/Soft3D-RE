//=============================================================================
//
//	[ s2dbase_io.h ]
//
//	the s2d local implement of CRT I/O stream.
//
//	PLATFORM DEPENDENT!!!平台相关
//	MUST be compiled as C++ code
//	Char8(Char16) version is only a proxy for Unicode(ANSI) mode
//	NOT supported: tmpnam, ferror, setbuf, setvbuf, ungetc, gets, getws,
//	               getchar, getwchar, scanf
//	ADD: fgetsize, CompleteFilenameWithAppPath
//	WARNING: for fopen: "w", "a" is the same as "w+', "a+"
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================

#ifndef __S2DBASE_IO_H__
#define __S2DBASE_IO_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dbase_common.h"
#include "s2dbase_arg.h"
//--------------------------------------------------------------------------------------------------------
#define BUFSIZ			512
//--------------------------------------------------------------------------------------------------------
#define EOF				(-1)
//--------------------------------------------------------------------------------------------------------
#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//---------------------------------------------------------------------------------------------------------
typedef struct FILEHandle
{
	void* impl;//point to platform dependent implement of s2dFILE
} FILE;

//---------------------------------------------------------------------------------------------------------
// init resource
void	InitDLRes( void );
//---------------------------------------------------------------------------------------------------------
// release resource
void	ReleaseDLRes( void );
//---------------------------------------------------------------------------------------------------------
// get resource handle
void*	GetDLRes( const Char* name );
//---------------------------------------------------------------------------------------------------------
// close resource handle
void	CloseDLRes(  void* hres );
//---------------------------------------------------------------------------------------------------------
// get resource length
s32		GetDLResSize(  void* hres );
//---------------------------------------------------------------------------------------------------------
// read resource data
s32		ReadDLResData(  void* hres, void* buf, s32 bufflen, s32 readlen );
//---------------------------------------------------------------------------------------------------------
// complete filename with app path, filename buffer is both for in/out, no overflow checking
Char*	CompleteFilenameWithAppPath( Char* filename );
//---------------------------------------------------------------------------------------------------------
// get path from filename, no overflow checking
void	GetPathFromFilename( Char* path, const Char* filename );
//---------------------------------------------------------------------------------------------------------
// Char8( ANSI ):
//---------------------------------------------------------------------------------------------------------
// removes a single file
s32		remove( const Char8* filename );
//---------------------------------------------------------------------------------------------------------
// renames a single file
s32		rename( const Char8* oldname, const Char8* newname );
//---------------------------------------------------------------------------------------------------------
// creates a temporary file
FILE*	tmpfile( void );
//---------------------------------------------------------------------------------------------------------
// generate names you can use to create temporary files
Char8*	tmpnam( Char8* generated );
//---------------------------------------------------------------------------------------------------------
// tests for an error on a stream
s32		ferror( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// open a file
// "r"  Opens for reading. The file must exist.
// "w"  Opens an empty file for writing. If the file exists, its contents are destroyed.
// "a"  Opens for appending. Creates the file first if it doesn’t exist.
// "r+" Opens for both reading and writing. The file must exist.
// "w+" Opens an empty file for both reading and writing. If the file exists, its contents are destroyed.
// "a+" Opens for reading and appending. Creates the file first if it doesn’t exist.
// "b"  Binary mode
// "t"  Text mode (assume default)
FILE*	fopen( const Char8* filename, const Char8* mode );
//---------------------------------------------------------------------------------------------------------
// reassign a file pointer
FILE*	freopen( const Char8* filename, const Char8* mode, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// close a file
s32		fclose( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// get a file size
s32		fgetsize( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// sets the stream-position indicator
s32		fsetpos( FILE* fp, const s32* pos );
//---------------------------------------------------------------------------------------------------------
// gets the stream-position indicator
s32		fgetpos( FILE* fp, s32* pos );
//---------------------------------------------------------------------------------------------------------
// moves the file pointer to a specified location
s32		fseek( FILE* fp, s32 offset, s32 reference );
//---------------------------------------------------------------------------------------------------------
// repositions the file pointer to the beginning of a file
void	rewind( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// gets the current position of a file pointer
s32		ftell( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// tests for end-of-file on a stream
s32		feof( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// flushes a stream
s32		fflush( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// controls stream buffering
void	setbuf( FILE* fp, s8* buf );
//---------------------------------------------------------------------------------------------------------
// controls stream buffering and buffer size
s32		setvbuf( FILE* fp, s8* buf, s32 mode, s32 size );
//---------------------------------------------------------------------------------------------------------
// reads data from a stream
s32		fread( void* buffer, s32 size, s32 count, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// reads data from a stream
s32		fread( void* buffer, s32 totalsize, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes data to a stream
s32		fwrite( const void* buffer, s32 size, s32 count, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes data to a stream
s32		fwrite( const void* buffer, s32 totalsize, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// read a character from a stream
Char8	fgetc( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// pushes a character back onto the stream
s32		ungetc( Char8 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes a character to a stream 
s32		fputc( Char8 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// get a string from a stream
Char8*	fgets( Char8* outstr, s32 count, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes a string to a stream
s32		fputs( const Char8* str, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// read a character from a stream 
Char8	getc( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// write a character to a stream 
s32		putc( Char8 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// get a character from stdin
Char8	getchar( void );
//---------------------------------------------------------------------------------------------------------
// write a character to stdout
s32		putchar( Char8 c );
//---------------------------------------------------------------------------------------------------------
// get a line from stdin
Char8*	gets( Char8* outstr );
//---------------------------------------------------------------------------------------------------------
// write a string to stdout
s32		puts( const Char8* str );
//---------------------------------------------------------------------------------------------------------
// print formatted output to stdout
s32		printf( const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from stdin
s32		scanf( const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// print an error message to stderr
void	perror( const Char8* msg );
//---------------------------------------------------------------------------------------------------------
// print formatted data to a stream
s32		fprintf( FILE* fp, const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from a stream
s32		fscanf( FILE* fp, const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// write formatted data to a string
s32		sprintf( Char8* str, const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from a string
s32		sscanf( const Char8* str, const Char8* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to stdout
s32		vprintf( const Char8* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to a stream
s32		vfprintf( FILE* fp, const Char8* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to a string
s32		vsprintf( Char8* str, const Char8* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// Char16( UNICODE ):
//---------------------------------------------------------------------------------------------------------
// removes a single file
s32		remove( const Char16* filename );
//---------------------------------------------------------------------------------------------------------
// renames a single file
s32		rename( const Char16* oldname, const Char16* newname );
//---------------------------------------------------------------------------------------------------------
// generate names you can use to create temporary files
Char16*	tmpnam( Char16* generated );
//---------------------------------------------------------------------------------------------------------
// open a file
FILE*	fopen( const Char16* filename, const Char16* mode );
//---------------------------------------------------------------------------------------------------------
// reassign a file pointer
FILE*	freopen( const Char16* filename, const Char16* mode, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// read a character from a stream
Char16	fgetwc( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// pushes a character back onto the stream
s32		ungetwc( Char16 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes a character to a stream 
s32		fputwc( Char16 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// get a string from a stream
Char16*	fgetws( Char16* outstr, s32 count, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// writes a string to a stream
s32		fputws( const Char16* str, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// read a character from a stream 
Char16	getwc( FILE* fp );
//---------------------------------------------------------------------------------------------------------
// write a character to a stream 
s32		putwc( Char16 c, FILE* fp );
//---------------------------------------------------------------------------------------------------------
// get a character from stdin
Char16	getwchar( void );
//---------------------------------------------------------------------------------------------------------
// write a character to stdout
s32		putwchar( Char16 c );
//---------------------------------------------------------------------------------------------------------
// get a line from stdin
Char16*	getws( Char16* outstr );
//---------------------------------------------------------------------------------------------------------
// write a string to stdout
s32		putws( const Char16* str );
//---------------------------------------------------------------------------------------------------------
// print formatted output to stdout
s32		printf( const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from stdin
s32		scanf( const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// print an error message to stderr
void	perror( const Char16* msg );
//---------------------------------------------------------------------------------------------------------
// print formatted data to a stream
s32		fprintf( FILE* fp, const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from a stream
s32		fscanf( FILE* fp, const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// write formatted data to a string
s32		sprintf( Char16* str, const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// read formatted data from a string
s32		sscanf( const Char16* str, const Char16* fmt, .../*arg*/ );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to stdout
s32		vprintf( const Char16* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to a stream
s32		vfprintf( FILE* fp, const Char16* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// write formatted output using a pointer to a list of arguments to a string
s32		vsprintf( Char16* str, const Char16* fmt, va_list arg );
//---------------------------------------------------------------------------------------------------------
// translate key from hardware scancode to s2d::Keys
Keys	TranslateKey( s32 key );
//---------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dbase_io.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DBASE_IO_H__
