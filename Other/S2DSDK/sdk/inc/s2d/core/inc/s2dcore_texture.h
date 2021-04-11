//=============================================================================
//
//	[ s2dcore_texture.h ]
//
//	the s2d core screen shader proccess lib.
//  *.s2dtex format:
//
//             |--bitdepth
//             |--format
//             |--width
//  |--header--|--height
//  |          |--colorkey
//  |--data
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================


#ifndef __S2DCORE_TEXTURE_H__
#define __S2DCORE_TEXTURE_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dcore_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Texture
{
//--------------------------------------------------------------------------------------------------------
//### interfaces:
public:
	Texture( void );
	~Texture( void );

	/// create texture
	//NOTE: we can create texture more than once, but previous data will be auto freed!
	result			Create(
						u16 format,						//[in] texture format
						u16 width,						//[in] texture width
						u16 height,						//[in] texture height
						u8 depth						//[in] texture bit-depth in bytes
					);

	/// create texture from *.s2dtex
	result			CreateFromFile(
						const Char* filename			//[in] texture filename
					);

	/// create texture from memory
	result			CreateFromMemory(
						u16 format,						//[in] texture format
						u16 width,						//[in] texture width
						u16 height,						//[in] texture height
						void* pdata,					//[in] texture data
						u8 depth,						//[in] texture bit-depth in bytes
						u32* pKey = NULL				//[in] color key (pass NULL if no color key)
					);

	/// create texture from *.s3dtex
	//result		CreateFromPak(
	//					const Char* filename,			//[in] texture filename
	//					File&	fp
	//				);

	/// free data
	void			Free( void );

	/// get texture data ptr
	void*			GetPtr( void );

	/// get width
	u16				GetWidth( void ) const;
	f32				GetWidthf( void ) const;

	/// get height
	u16				GetHeight( void ) const;
	f32				GetHeightf( void ) const;

	/// get bitdepth
	u8				GetBitDepth( void ) const;

	/// get format
	u16				GetFormat( void ) const;

	/// get colorkey
	u8				GetHasColorKey( void ) const;
	u32				GetColorKey( void ) const;

	/// get palette
	Pixel*			GetPalette( void ) const;

	/// get tpow
	s32				GetTPow( void ) const;

	/// fill texture with constant color
	void			Fill(
						u32 color,						//[in] fill color
						const Rect* rect = NULL,		//[in] fill rect (NULL : whole texture)
						bool fast = true				//[in] use fast fill method
					);
//--------------------------------------------------------------------------------------------------------
//### data members:
private:

	// texture size
	u16				m_iWidth;
	u16				m_iHeight;

	// for texture mapping
	s16				m_TPow;

	// texture format
	u16				m_Format;

	// palette entry count
	u16				m_PalEntryCnt;

	// bitdepth in bytes
	u8				m_Depth;

	// texture color key
	u8				m_bHasColorKey;
	u32				m_ColorKey;

	// texel data
	void*			m_pData;
	void*			m_pData_Aligned;//4-bytes aligned address
	
	// texture palette
	Pixel*			m_pPalette;
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dcore_texture.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DCORE_TEXTURE_H__
