
#ifndef __S2DENGINE_STRINGRES_H__
#define __S2DENGINE_STRINGRES_H__

//--------------------------------------------------------------------------------------------------------
#include "s2dengine_common.h"
//--------------------------------------------------------------------------------------------------------
namespace s2d
{
class Font;
class StringManager;
enum STRKEY
{
	STR_NEWLINE = 0,
	STR_COLOR,
	STR_COLOREND,
	STR_BLANK,
	MAX_STRKEY
};
//--------------------------------------------------------------------------------------------------------
// VisString:
//--------------------------------------------------------------------------------------------------------
class VisString
{
//--------------------------------------------------------------------------------------------------------
//### interfaces:
public:
	VisString(
	);

	VisString(
		u32 aLength						//[in] length of string
	);

	VisString(
		Char* aStr							//[in] input string, can only use 'a-z', 'A-Z', '0-9', and ' '
	);

	~VisString( void );

	/// clear string data
	void	Clear( void );

	/// get original string length
	u32		GetLength( void );

	/// get original string buffer
	u32*	GetBuffer( void );

	/// append a string to current string
	void    Append(
				Char* aStr					//[in] append string, can only use 'a-z', 'A-Z', '0-9', and ' '
			);		

	/// append a number to current string
	void    Append(
				s32	aNumber					//[in] append number
			);

	/// find next blank index
	s32		GetNextBlankIndex( 
				s32 aStrInd					//[in] start index to find
			);

private:
	/// set string
	void	SetString(
				u32* aStr,					//[in] string code buffer
				u32 aLength					//[in] string length
			);
//--------------------------------------------------------------------------------------------------------
//### data members:
private:

	// string code buffer
	u32*			m_pBuffer;

	// string length
	u32				m_iLength;

	// size of memory (in byte)
	u32				m_iSize;

	friend class StringManager;
};
//--------------------------------------------------------------------------------------------------------
// StringManager:
//--------------------------------------------------------------------------------------------------------
class StringManager
{
//--------------------------------------------------------------------------------------------------------
//### interfaces:
public:
	StringManager( void );
	~StringManager( void );
	
	/// init VisString and font
	bool			Init( 
						LanguageType laugage ,
						Char*		path
					);
		/// set render target
	void	SetVRAMPtr( 
						Pixel* aTarg,			//[in] display target
						s32	 iSrcWid,			//[in] screen width
						s32	 iSrcHig			//[in] screen width
					);

	/// load VisString res
	result		Load(
						Char* afilename		//[in] file name
					);

	/// clear all VisString
	void			Clear( void );

	/// get VisString count
	u32				GetCount( void );

	/// get VisString
	VisString*		GetString(
						u32 aStrInd			//[in] string index
					);

	/// create new VisString with input string
	VisString*		NewString(
								Char* aInput		//[in] input string, can only use 'a-z', 'A-Z', '0-9', and ' '
							);

	/// create new VisString of given length
	VisString*		NewString(
								u32 aLength			//[in] string length
							);

	
	/// count width of string 
	u32				CountTextWidth(
						   VisString* aStr,		//[in] to be count string
						   u32 aStarPos			//[in] string star point
						);
	
	/// count line of split string on width
	u32			    CountLineOnWidth(
						VisString* aStr,		//[in] to be count string
						u32 aStarPos,			//[in] string star point
						u32 aMaxWidth			//[in] max width in pixel
					);

	/// count height of text 
	u32				GetTextHeight();

	/// draw string
	void			DrawString(
						u32 aStrInd,				//[in] string index
						Pixel aColor,			//[in] string color
						u32 anchor,					//[in] anchor type
						Point& aPoint				//[in] point 
					);

	/// draw string
	void			DrawString(
						u32 aStrInd,				//[in] string index
						Pixel aColor,			//[in] string color
						Rect& aRect					//[in] rect 
					);	

	/// draw string
	void			DrawString(
						u32 aStrInd,				//[in] string index
						Pixel aColor,			//[in] string color
						u32	start,					//[in] start draw line
						s32	end,					//[in] end draw line
						Rect& aRect					//[in] rect 
					);	

	/// draw string
	void			DrawString(
						VisString* aVStr,			//[in] string
						Pixel aColor,			//[in] string color
						Rect& aRect					//[in] rect 
					);

	/// draw string
	void			DrawString(
						VisString* str,			//[in] string
						Pixel aColor,			//[in] string color
						u32 anchor,				//[in] anchor type
					    Point& aPoint			//[in] point 
					);

	/// draw string
	void			DrawString(
						u32 aStrInd,				//[in] string index
						Pixel aColor,			//[in] string color
						u32 anchor,				//[in] anchor type
						Rect& aRect					//[in] rect 
					);

private:
	
	/// draw character
	void			DrawChar(
						u32 aCode,					//[in] string code
						Pixel aColor,				//[in] string color
						Point& aPoint				//[in] point 
					);

	//anchor point
	void			Anchor(
						u32 type,					//[in] anchor type
						VisString *str,				//[in] string
						Point& aPoint				//[in\out] point
					);
	//anchor rect
	void			Anchor(
						u32 type,					//[in] anchor type
						VisString *str,				//[in] string
						s32   pos,					//[in] cur pos
						Point& aPoint,				//[out] point
						Rect& aRect					//[in] rect
					);
//--------------------------------------------------------------------------------------------------------
//### data members:
private:

	// VisString count
	u32				m_iStrCnt;
	// VisString buffer
	VisString**		m_pStrList;

	// font resource
	Font*			m_pFontRes;
	

	// render target
	Pixel*			m_pVRAM;

	/// screen width and height
	s32				m_iScreenWidth;
	s32				m_iScreenHeight;
	
	// 
	LanguageType	m_eCurLanguage;
};

}; // namespace s2d
//--------------------------------------------------------------------------------------------------------
#include "s2dengine_stringres.inl"
//--------------------------------------------------------------------------------------------------------

#endif//__S2DENGINE_STRINGRES_H__
