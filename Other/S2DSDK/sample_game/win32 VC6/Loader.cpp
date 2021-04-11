// s2dwin.cpp : 定义应用程序的入口点。
//

#include <windows.h>
#include <tchar.h>
#include "Loader.h"
#include "..\source\inc\GameEngine.h"

extern "C" long _ftol( double ); //defined by VC6 C libs
extern "C" long _ftol2( double dblSource ) { return _ftol( dblSource ); }

// 全局变量:
int					SCREEN_WIDTH = 240;
int					SCREEN_HEIGHT = 320;

HWND				g_hWnd = NULL;
Engine*				g_pEng = NULL;

BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	bool bRunning = true;


	if( !InitInstance( hInstance, nCmdShow ) ) {
		return FALSE;
	}

	void* userdata[5];
	HDC hDC		= GetDC( g_hWnd );
	HFONT hFont	= (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	userdata[0] = (void*)&g_hWnd;
	userdata[1] = (void*)&hDC;
	userdata[2] = (void*)&hFont;
	userdata[3] = (void*)&SCREEN_WIDTH;
	userdata[4] = (void*)&SCREEN_HEIGHT;

	g_pEng = new GameEngine( userdata );
	if( g_pEng->Init() != S2D_OK ) {
		SAFE_DELETE( g_pEng );
	}

	if( g_pEng ) {
		g_pEng->Start();

		while( bRunning ) {
			if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if ( msg.message == WM_QUIT ) { 
					bRunning = false; 
					::PostQuitMessage(0); 
					break; 
				}
			}
 			g_pEng->OneFrame();
		}
		g_pEng->Stop();
		//g_pEng->Free();
	}
	if(g_pEng&&g_pEng->IsRunning())
	{
		g_pEng->Stop();
	}
	SAFE_DELETE(g_pEng);
	return (int) msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASS WndClass;

	WndClass.style         = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc   = WndProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = hInstance;
	WndClass.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
	WndClass.hCursor       = NULL;
	WndClass.hbrBackground = NULL;
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = _T("s2dwin");
	RegisterClass(&WndClass);

	g_hWnd = CreateWindow( _T("s2dwin"), _T("s2dwin"), WS_OVERLAPPEDWINDOW,
			0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, hInstance, NULL );

	MoveWindow( g_hWnd, 400, 300, SCREEN_WIDTH, SCREEN_HEIGHT, TRUE );

	if( !g_hWnd ) {
		return FALSE;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	ShowCursor( true );

	HCURSOR   hc;

	hc=LoadCursor(NULL,IDC_ARROW);

	SetCursor(hc);


   RECT rcCli, rcWindow;
   int width, height;
   ::GetClientRect( g_hWnd, &rcCli );
   ::GetWindowRect( g_hWnd, &rcWindow );
   width = rcCli.right - rcCli.left + 1;
   if ( width < SCREEN_WIDTH )
		rcWindow.right += SCREEN_WIDTH - width;
   height = rcCli.bottom - rcCli.top + 1;
   if ( height < SCREEN_HEIGHT )
	   rcWindow.bottom += SCREEN_HEIGHT - height;

   ::MoveWindow( g_hWnd, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left + 1, rcWindow.bottom - rcWindow.top + 1, true );
	
	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Keys key;
	MouseButton mb;
	switch (message) 
	{
	case WM_MOUSEMOVE:
		g_pEng->m_pInput->MouseMove( LOWORD(lParam),HIWORD(lParam) );

		break;
	case WM_LBUTTONDOWN:
		mb= LBUTTON;
		g_pEng->m_pInput->MouseDown( mb,LOWORD(lParam),HIWORD(lParam) );
		
		break;
	case WM_LBUTTONUP
		:
		mb= LBUTTON;
		g_pEng->m_pInput->MouseUp( mb );

		break;
	case WM_KEYDOWN:
		key = TranslateKey( (s32)wParam );
		g_pEng->m_pInput->KeyDown( key );
		break;
	case WM_KEYUP:
		key = TranslateKey( (s32)wParam );
		g_pEng->m_pInput->KeyUp( key );
		break;
	case WM_PAINT:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
