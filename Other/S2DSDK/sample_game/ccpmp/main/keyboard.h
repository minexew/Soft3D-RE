/******************************************************************************
 * Copyright (c) 2007 by China Chip Tech.
 *
 * File Name   : keyboard.h
 * Description : head file of keyboard.
 *
 * History :
 *   Version      Date      Author     Memo
 *     0.1     2007-05-23   fanoble    Created.
 *
 *****************************************************************************/

#ifndef		_KEYBOARD_H_
#define		_KEYBOARD_H_

#ifdef		__cplusplus
extern "C" {
#endif

//physical key code
#define		SKEY_NONE		0x00000000

#define		SKEY_LEFT		0x10000000
#define		SKEY_RIGHT		0x00040000
#define		SKEY_UP			0x00100000
#define		SKEY_DOWN		0x08000000


#define     SKEY_ESCAPE     SKEY_MENU
#define		SKEY_MENU		0x00010000

#define		SKEY_ENTER		0x80000000
#define     SKEY_CAMERA		0x40000000
#define 	SKEY_A_B 		0x00200000
#define     SKEY_E_Q		0x00400000

#define     SKEY_REC        0x00020000

#define		SKEY_POWER		0x00000080



typedef struct tagKEY_STATUS
{
	unsigned long pressed;
	unsigned long released;
	unsigned long status;
} KEY_STATUS;

int kbd_init(void);

unsigned long kbd_get_key(void);

void kbd_get_status(KEY_STATUS* ks);

#ifdef __cplusplus
}
#endif

#endif		/* _KEYBOARD_H_ */

/******************************************************************************
                               END  OF  FILE
******************************************************************************/
