//=============================================================================
//
//	[ s2dengine_input.h ]
//
//	the s2d input of engine proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DENGINE_INPUT_H__
#define __S2DENGINE_INPUT_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dengine_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
class Input
{
//--------------------------------------------------------------------------------------------------------
//### interfaces:
public:
	Input( void );

	/// clear all key status
	void	ClearKey( void );

	/// update key status
	void	Update( void );

	/// set key status
	void	KeyDown( Keys key );
	void	KeyPress( Keys key );
	void	KeyUp( Keys key );

	/// get key status
	bool	IsKeyFirstDown( Keys key );		// when key from up to down, return true
	bool	IsKeyDown( Keys key );			// when key is down, always return true
	bool	IsKeyPress( Keys key );			// when key is down, return true repeatly
	bool	IsKeyUp( Keys key );			// when key is up, always true
	bool	IsKeyClick( Keys key );			// return true when key from down to up
	bool	IsKeyDownOrClick( Keys key );//can respond to key down event of last frame
	bool	IsAnyKeyClick();			// when any key clicked ,return true;

	void	MouseDown( MouseButton mb ,s32 x,s32 y );
	void	MouseUp( MouseButton mb  );
	void	MouseMove( s32 x,s32 y );
	s32		GetMouseX();
	s32     GetMouseY();
	bool	IsMouseDown( MouseButton mb );
	bool	IsMouseClick( MouseButton mb );
//--------------------------------------------------------------------------------------------------------
//### data members:
private:
	u8		m_KeyTbl[ MAX_KEYS ];
	u8		m_KeyPressTbl[ MAX_KEYS ];
	bool	m_bIsAnyKeyDown;
	u8		m_MouseTbl[MAX_MBUTTON];
	s32		m_MouseX;
	s32		m_MouseY;
 

	 
};
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dengine_input.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DENGINE_INPUT_H__
