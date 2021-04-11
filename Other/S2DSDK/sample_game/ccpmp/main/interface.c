/******************************************************************************
	针对不同的 LCD 屏分辨率、按键数量、按键转换，是否使用触摸屏，
	本文件有 4 处需要修改，每处需要修改的地方都在注视里面以 !!! 打头，
	所以只需要查找 "!!!" 就可以找到要修改的地方
******************************************************************************/

/* 版权声明 */

/* 头文件包含 */
#include	<stdlib.h>
#include	<string.h>

/* 宏定义 */

/* 按键定义 */
#define		KYE_TRANS_NUMBER	6				/* !!!在此处修改键盘映射表总数 */

/* LCD 分辨率定义 */
#define		LCM_320X240			1
#define		LCM_400X240			2
#define		LCM_480X272			4

#define		LCM_RESOLUTION		LCM_400X240		/* !!!在此处修改 LCM 定义，不同的项目要做相应的修改 */

#if (LCM_RESOLUTION == LCM_400X240)

//#define		LCM_USE_TOUCH_KEY					/* !!!在此处修改 触摸屏 定义，不同的项目要做相应的修改 */

#endif

#ifdef		TRUE
	#undef	TRUE
#endif
#define		TRUE			1

#ifdef		FALSE
	#undef	FALSE
#endif
#define		FALSE			0

#define		GAME_DECODE_BUF_SIZE	(320 * 240 * 2)	/* !!!此处不需要修改，如果确实修改了，请同时修改 main.c 里对应的宏定义 */

/* GAME 内部按键定义，如果 GAME 里面的库对这些定义做了修改，
	这里也要做相应的修改，所以 GAME 库还需要在这些方面进一步完善 */
#define		GAME_SKEY_NONE		0x00000000		/* 未用 */
#define		GAME_SKEY_LEFT		0x10000000		/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_RIGHT		0x00040000		/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_UP		0x00100000		/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_DOWN		0x08000000		/* 不同界面操作不一样，根据实际情况操作 */
#define     GAME_SKEY_ESCAPE	GAME_SKEY_MENU	/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_MENU		0x00010000		/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_ENTER		0x80000000		/* 不同界面操作不一样，根据实际情况操作 */
#define     GAME_SKEY_PLAY		0x20000000		/* 不同界面操作不一样，根据实际情况操作 */
#define     GAME_SKEY_CAMERA	0x40000000		/* 不同界面操作不一样，根据实际情况操作 */
#define 	GAME_SKEY_A_B		0x00200000		/* 不同界面操作不一样，根据实际情况操作 */
#define     GAME_SKEY_E_Q		0x00400000		/* 不同界面操作不一样，根据实际情况操作 */
#define     GAME_SKEY_REC		0x00020000		/* 不同界面操作不一样，根据实际情况操作 */
#define		GAME_SKEY_POWER		0x00000080		/* 关机 */

#define		GAME_TP_UP			0x00
#define		GAME_TP_DOWN		0x01
#define		GAME_TP_MOVE		0x02

/* 自定义类型定义 */
typedef	char			I8;
typedef	short			I16;
typedef	int				I32;
typedef	long			I64;
typedef	unsigned char	U8;
typedef	unsigned short	U16;
typedef	unsigned int	U32;
typedef	unsigned long	U64;
typedef	U8				BOOL;

typedef struct tagKeyStatus
{
	U32	pressed;
	U32	released;
	U32	status;
}tKeyStatus;

typedef struct tagKeyTrans
{
	U32	srcKey;
	U32	transKey;
}tKeyTrans;

/* 外部函数、变量声明 */
void					lcd_set_frame(void);

extern void				sprintf(char *, const char *, ...);
extern unsigned short	*_lcd_get_frame(void);
extern void				_lcd_set_frame(void);
extern unsigned int		_kbd_get_key(void);
extern int				_sys_judge_event(void *);
extern void				*_waveout_open(void *);
extern int				_waveout_close(void *);
extern void				_waveout_close_at_once(unsigned int);
#ifdef LCM_USE_TOUCH_KEY
extern int				Tp_Get_Pos(int *, int *);
#endif
extern void				_kbd_get_status(tKeyStatus *);

extern const unsigned short c_TouchKeyMap[];

/* 静态全局变量定义 */
#ifdef	LCM_USE_TOUCH_KEY

static U32	g_KeyPressed;							/* 触摸屏按下标志(一个键对应一个位) */
static U32	g_KeyReleased;							/* 触摸屏释放标志(一个键对应一个位) */
static U32	g_KeyStatusLast;						/* 触摸屏上一次键值 */

#endif

/* !!!在此处修改键盘映射关系 */
static const tKeyTrans c_GameTransKeys[KYE_TRANS_NUMBER] =
{
	{GAME_SKEY_LEFT, GAME_SKEY_LEFT},
	{GAME_SKEY_RIGHT, GAME_SKEY_RIGHT},
	{GAME_SKEY_UP, GAME_SKEY_UP},
	{GAME_SKEY_DOWN, GAME_SKEY_DOWN},
	{GAME_SKEY_ENTER, GAME_SKEY_ENTER},
	{GAME_SKEY_ESCAPE, GAME_SKEY_ESCAPE}
};

/* 全局变量定义 */
U16 *g_pGameDecodeBuf;		/* GAME 库解码缓冲区 */

/* 静态函数实现 */

/* 函数说明 */
#if ((LCM_RESOLUTION != LCM_320X240) && (!defined(LCM_USE_TOUCH_KEY)))

static inline U16	InterpolatePixel(U16 color1, U16 color2)
{
	if (color1 == color2)
	{
		(void)color2;
		
		return	color1;
	}
	else
	{
		return	((color1 & 0xF7DF) + (color2 & 0xF7DF)) >> 1;
	}
}

#endif

/* 函数说明 */
#ifdef	LCM_USE_TOUCH_KEY

static I32	GetTouchKey(int x, int y)
{
	I32	key = GAME_SKEY_NONE;

	if (x < 320)
	{
		key = GAME_SKEY_ENTER;
		
		return	key;
	}
	
	if (y < 47)
	{
		key = GAME_SKEY_UP;
	}
	else if (y < 117)
	{
		if (x < 320 + 40)
		{
			key = GAME_SKEY_LEFT;
		}
		else
		{
			key = GAME_SKEY_RIGHT;
		}
	}
	else if (y < 178)
	{
		key = GAME_SKEY_DOWN;
	}
	else
	{
		key = GAME_SKEY_ESCAPE;
	}
	
	return	key;
}

#endif

/* 全局函数实现 */

/* 函数说明 */
I8	*lcd_get_frame(void)
{
	#if 0
	return	(I8*)g_pGameDecodeBuf;
	#endif
	
	return _lcd_get_frame();
}

/* 函数说明 */
void	lcd_set_frame(void)
{
	U16	*pLcdBuf;
	U16	*pTmp;
	#if 0
#if	(LCM_RESOLUTION != LCM_320X240)

	#ifndef	LCM_USE_TOUCH_KEY
	
	I32  x;
	
	#endif
	
	I32  y;

#endif

#if	(LCM_RESOLUTION == LCM_480X272)

	U16	*pTmp1;

#endif
	
	pLcdBuf = _lcd_get_frame();
	pTmp = g_pGameDecodeBuf;

#if	(LCM_RESOLUTION == LCM_320X240)

	memcpy((void*)pLcdBuf, (void*)pTmp, GAME_DECODE_BUF_SIZE);
	
#elif (LCM_RESOLUTION == LCM_400X240)

	#ifdef	LCM_USE_TOUCH_KEY
	
	for (y = 0; y < 240; y++)
	{
		memcpy((void*)pLcdBuf, (void*)pTmp, 320 * sizeof(U16));
		memcpy((void*)(pLcdBuf + 320), (void*)&c_TouchKeyMap[80 * y], 80 * sizeof(unsigned short));
		
		pLcdBuf += 400;
		pTmp += 320;
	}
	
	#else
	
	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 320; x++)
		{
			*pLcdBuf++ = *pTmp++;
			
			if (2 == (x & 0x03))
			{
				*pLcdBuf++ = InterpolatePixel(*(pTmp - 1), *pTmp);
			}
		}
	}
	
	#endif

#elif (LCM_RESOLUTION == LCM_480X272)
	
	memset((void*)pLcdBuf, 0x00, 480 * 2);
	pLcdBuf += 480;
	for (y = 1; y < (272 - 1); y++)
	{
		for (x = 0; x < 480; x++)
		{
			*pLcdBuf++ = *pTmp++;
			
			if (1 == (x % 2))
			{
				*pLcdBuf++ = InterpolatePixel(*(pTmp - 1), *pTmp);
			}
		}
		
		if (3 == (y & 0x07))
		{
			pLcdBuf += 480;
		}
		
		if (4 == (y & 0x07))
		{
			pTmp1 = pLcdBuf - 960;
			
			for (x = 0; x < 480; x++)
			{
				*pTmp1 = InterpolatePixel(*(pTmp1 - 480), *(pTmp1 + 480));
				pTmp1++;
			}
		}
	}
	memset((void*)pLcdBuf, 0x00, 480 * 2);

#else

#error("!!! PLEASE DEFINE LCM !!!")

#endif
	#endif
	_lcd_set_frame();
}

/* 函数说明 */
U32	kbd_get_key(void)
{
	I32	i, keyVal;
	
	printf("kbd_get_key\n");
	keyVal = _kbd_get_key();

#if 0
	if (GAME_SKEY_NONE != keyVal)
	{
		for (i = 0; i < KYE_TRANS_NUMBER; i++)
		{
			if (c_GameTransKeys[i].srcKey == keyVal)
			{
				keyVal = c_GameTransKeys[i].transKey;
				
				break;
			}
		}
		
		return	keyVal;
	}
#else
		return	keyVal;
#endif
	
#ifdef	LCM_USE_TOUCH_KEY

	else
	{
		return	g_KeyStatusLast;
	}

#endif

	return	GAME_SKEY_NONE;
}
/* 函数说明 */

void kbd_get_status(tKeyStatus *pKeyStatus)
{
	
	//printf("kbd_get_status\n");
	_kbd_get_status(pKeyStatus);
	
#ifdef	LCM_USE_TOUCH_KEY

	if ((!pKeyStatus->pressed) && (!pKeyStatus->released) && (!pKeyStatus->status))
	{
		pKeyStatus->pressed = g_KeyPressed;
		pKeyStatus->released = g_KeyReleased;
		pKeyStatus->status = g_KeyStatusLast;
	}
	
#endif

	//printf("pKeyStatus->pressed: %X, pKeyStatus->released: %X, pKeyStatus->status :%X\n", pKeyStatus->pressed, pKeyStatus->released, pKeyStatus->status);
}

/* 函数说明 */
I32	sys_judge_event(void *pFun)
{
#ifdef	LCM_USE_TOUCH_KEY

	I32	x = 0, y = 0;
	U32	keyVal, keyStatusNew = GAME_SKEY_NONE, keyStatusChanged;
	

#endif
	I32	r, ret;

	ret = _sys_judge_event(pFun);
	
#ifdef	LCM_USE_TOUCH_KEY

	r = Tp_Get_Pos(&x, &y);
	
	if (-1 != r)
	{
		keyVal = GetTouchKey(x,  y);
		keyStatusNew = keyVal;
		keyStatusChanged = g_KeyStatusLast ^ keyStatusNew;
    	
		if (keyStatusChanged)
		{
			switch (r)
			{
				case	GAME_TP_UP:
					g_KeyReleased = ~keyStatusNew & keyStatusChanged;
					
					break;
				case	GAME_TP_DOWN:
					g_KeyPressed = keyStatusNew & keyStatusChanged;
				case	GAME_TP_MOVE:				/* 按照目前的逻辑，该 CASE 语句永远进不来，不过现在的版本并不影响 */
					g_KeyStatusLast = keyStatusNew;
					
					break;
				default:
					break;
			}
		}
		else
		{
			g_KeyPressed = FALSE;
			g_KeyReleased = FALSE;
		}
	}
	else
	{
		g_KeyPressed = FALSE;
		g_KeyReleased = FALSE;
		g_KeyStatusLast = GAME_SKEY_NONE;
	}

#endif

	return	ret;
}
