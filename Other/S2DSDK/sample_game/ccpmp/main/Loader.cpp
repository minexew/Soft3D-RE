#include ".\..\..\Source\inc\GameEngine.h"


typedef struct tagKEY_STATUS
{
	unsigned long pressed;
	unsigned long released;
	unsigned long status;
} KEY_STATUS;
extern "C" void kbd_get_status(KEY_STATUS* ks);

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



extern bool g_bGameRuning ;
extern Char g_FilePath[256];
extern Char g_AppFullName[256];
extern "C" int sys_judge_event(void * fun);
extern "C" char *__to_locale_ansi(wchar_t *);


KEY_STATUS ks;
KEY_STATUS oldks;

extern "C" void onkey( Engine* pEng )
{
	// 因为屏幕旋转了，所以按键映射也相应的进行了修改
	kbd_get_status( &ks );
	if( ks.status & SKEY_LEFT ){
		pEng->m_pInput->KeyDown( KEY_DN );
	}else{
		if( oldks.status & SKEY_LEFT ) 
			pEng->m_pInput->KeyUp( KEY_DN );
	}

	if( ks.status & SKEY_RIGHT ){
		pEng->m_pInput->KeyDown( KEY_UP  );
	}else{
		if( oldks.status & SKEY_RIGHT ) 
			pEng->m_pInput->KeyUp( KEY_UP );
	}

	if( ks.status & SKEY_UP ){
		pEng->m_pInput->KeyDown( KEY_LF );
	}else{
		if( oldks.status & SKEY_UP )
			pEng->m_pInput->KeyUp( KEY_LF );
	}

	if( ks.status & SKEY_DOWN ){
		pEng->m_pInput->KeyDown( KEY_RT );
	}else{
		if( oldks.status & SKEY_DOWN ) {
			pEng->m_pInput->KeyUp( KEY_RT );
		}
	}
	if( ks.status & SKEY_ESCAPE ){
		pEng->m_pInput->KeyDown( KEY_L1 );
	}else{
		if( oldks.status & SKEY_ESCAPE ) 
			pEng->m_pInput->KeyUp( KEY_L1 );
	}

	if( ks.status & SKEY_ENTER ){
		pEng->m_pInput->KeyDown( KEY_OK );
	}else{
		pEng->m_pInput->KeyUp( KEY_OK );
	}
	if( ks.status & 64 ){//Y键码 64
		pEng->m_pInput->KeyDown( KEY_R1 );
	}else{
		if( oldks.status & 64 ) 
			pEng->m_pInput->KeyUp( KEY_R1 );
	}

	if( ks.status &  SKEY_A_B){
		pEng->m_pInput->KeyDown( KEY_R1 );
	}else{
		if( oldks.status & SKEY_A_B ) 
			pEng->m_pInput->KeyUp( KEY_R1 );
	}
	//// SELECT
	if( ks.status & 0x00000400) {
		pEng->m_pInput->KeyDown( KEY_OK );
	}else{
		if( oldks.status & 0x00000400 ) 
			pEng->m_pInput->KeyUp( KEY_OK );
	}

	oldks = ks;
}


extern "C" int GameMain(char* respath)
{
	int ref = 1;
	int sysref = 0;
	GameEngine* pEng;
	Char AppFullName[256];
	char* fullpath;

	LOG_ON;
	  
	strcpy( (Char16*)AppFullName, (Char16*)respath );

	fullpath = __to_locale_ansi( (wchar_t*)respath );
	GetPathFromFilename( g_FilePath, fullpath );

	// 在这里修改屏幕分辨率
	int 	SCREEN_WIDTH = 240;
	int	SCREEN_HEIGHT = 320;
	// 需要旋转屏幕
	vidrotat( 1 );

	void* userdata[5];
	
	userdata[3] = (void*)&SCREEN_WIDTH;
	userdata[4] = (void*)&SCREEN_HEIGHT;

	pEng = new GameEngine( userdata );
	LOG(_LS("h"));
	pEng->Init();
	pEng->Start();

	g_bGameRuning = true;
	oldks.status = 0;
	 
	while( g_bGameRuning ){
		sysref = sys_judge_event( NULL);
		if( sysref  < 0 ){
			ref = sysref ;
			break;
		}
		onkey( pEng );

		pEng->OneFrame();
	}
	pEng->Stop();
	SAFE_DELETE( pEng );

	return ref;
}
