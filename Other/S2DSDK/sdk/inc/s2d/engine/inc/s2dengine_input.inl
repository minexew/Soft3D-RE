//=============================================================================
//
//	[ s2dengine_input.inl ]
//
//	the s2d input of engine proccess lib.
//
//	Copyright (c) 2006-2009, DingooGames. All rights reserved.
//
//=============================================================================
#ifndef __S2DENGINE_INPUT_INL__
#define __S2DENGINE_INPUT_INL__

//--------------------------------------------------------------------------------------------------------
namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### interfaces:
//--------------------------------------------------------------------------------------------------------
inline void	Input::MouseDown( MouseButton mb ,s32 x,s32 y )
{
	if( !(m_MouseTbl[ mb ] & MS_DOWN) ) {
		m_MouseTbl[ mb ] = MS_DOWN;
		m_MouseTbl[ mb ] |= MS_FIRSTDOWN;
	} else {
		m_MouseTbl[ mb ] = MS_DOWN;
	}
	m_MouseX=x;
	m_MouseY=y;
	//m_bIsAnyKeyDown = true;
}
//--------------------------------------------------------------------------------------------------------
//### interfaces:
//--------------------------------------------------------------------------------------------------------
inline void	Input::MouseMove( s32 x,s32 y )
{
 	m_MouseX=x;
	m_MouseY=y; 
	
}
//--------------------------------------------------------------------------------------------------------
inline void	Input::MouseUp( MouseButton mb  )
{
	if( m_MouseTbl[ mb ] & MS_DOWN ) {
		// it's a click
		m_MouseTbl[ mb ] = MS_CLICK;//so key down & press is cleared
	}
	//m_bIsAnyKeyDown = false;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsMouseDown( MouseButton mb )
{
	return (m_MouseTbl[ mb ] & MS_DOWN) != 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsMouseClick( MouseButton mb )
{
	return (m_MouseTbl[ mb ] & MS_CLICK) != 0;
}
inline s32 Input::GetMouseX()
{
	return m_MouseX;
}
inline s32 Input::GetMouseY()
{
	return m_MouseY;
}
//--------------------------------------------------------------------------------------------------------
//### interfaces:
//--------------------------------------------------------------------------------------------------------
inline void	Input::KeyDown( Keys key )
{
	if( !(m_KeyTbl[ key ] & KS_DOWN) ) {
		m_KeyTbl[ key ] = KS_DOWN;
		m_KeyTbl[ key ] |= KS_FIRSTDOWN;
	} else {
		m_KeyTbl[ key ] = KS_DOWN;
		m_KeyPressTbl[ key ] = m_KeyPressTbl[ key ] + 1;
		if( m_KeyPressTbl[ key ] >= 2 ) {
			m_KeyTbl[ key ] |= KS_PRESS;
			m_KeyPressTbl[ key ] = 0;
		}
	}
	m_bIsAnyKeyDown = true;
}
//--------------------------------------------------------------------------------------------------------
inline void	Input::KeyPress( Keys key )
{
	m_KeyTbl[ key ] |= KS_PRESS;
	m_bIsAnyKeyDown = true;
}
//--------------------------------------------------------------------------------------------------------
inline void	Input::KeyUp( Keys key )
{
	if( m_KeyTbl[ key ] & KS_DOWN ) {
		// it's a click
		m_KeyTbl[ key ] = KS_CLICK;//so key down & press is cleared
	}
	m_bIsAnyKeyDown = false;
	m_KeyPressTbl[ key ] = 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyFirstDown( Keys key )
{
	return (m_KeyTbl[ key ] & KS_FIRSTDOWN) != 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyDown( Keys key )
{
	return (m_KeyTbl[ key ] & KS_DOWN) != 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyPress( Keys key )
{
	return (m_KeyTbl[ key ] & KS_PRESS) != 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyUp( Keys key )
{
	return (m_KeyTbl[ key ] & KS_DOWN) == 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyClick( Keys key )
{
	return (m_KeyTbl[ key ] & KS_CLICK) != 0;
}
//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsKeyDownOrClick( Keys key )
{
	return	( IsKeyDown(key) || IsKeyClick(key) );
}

//--------------------------------------------------------------------------------------------------------
inline bool	Input::IsAnyKeyClick()
{
	return m_bIsAnyKeyDown;
}
//--------------------------------------------------------------------------------------------------------
};//namespace s2d
//--------------------------------------------------------------------------------------------------------

#endif//__S2DENGINE_INPUT_INL__
