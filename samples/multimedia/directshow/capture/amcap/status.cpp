//------------------------------------------------------------------------------
// File: Status.cpp
//
// Desc: Status Bar Window Code
//
// Copyright (c) 1999 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <streams.h>
//#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "status.h"


// class names for status bar and static text windows
char	szStatusClass[] = "StatusClass";
char	szText[]   = "SText";
int gStatusStdHeight;   // based on font metrics

static HANDLE ghFont;
static HBRUSH ghbrHL, ghbrShadow;




//------------------------------------------------------------------------------
// Local Function Prototypes
//------------------------------------------------------------------------------
LRESULT CALLBACK statusWndProc(HWND hwnd, UINT msg,
						WPARAM wParam, LONG lParam);
LRESULT CALLBACK fnText(HWND, UINT, WPARAM, LONG);
static void PaintText(HWND hwnd, HDC hdc);




/*
 * create the brushes we need
 */
void
statusCreateTools(void)
{
    HDC hdc;
    TEXTMETRIC tm;
    HFONT hfont;

    ghbrHL = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    ghbrShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));

    /* Create the font we'll use for the status bar - use system as default */
    ghFont = CreateFont(12, 0,		// height, width
                0, 0,			// escapement, orientation
                FW_NORMAL,		// weight,
                FALSE, FALSE, FALSE,	// attributes
                ANSI_CHARSET,		// charset
                OUT_DEFAULT_PRECIS,	// output precision
                CLIP_DEFAULT_PRECIS,	// clip precision
                DEFAULT_QUALITY,	// quality
                VARIABLE_PITCH | FF_MODERN,
                "Helv");

    if (ghFont == NULL) {
        ghFont = GetStockObject(SYSTEM_FONT);
    }

    // find the char size to calc standard status bar height
    hdc = GetDC(NULL);
    hfont = (HFONT)SelectObject(hdc, ghFont);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hfont);
    ReleaseDC(NULL, hdc);

    gStatusStdHeight = tm.tmHeight * 3 / 2;

}

void
statusDeleteTools(void)
{
    DeleteObject(ghbrHL);
    DeleteObject(ghbrShadow);

    DeleteObject(ghFont);
}




/*--------------------------------------------------------------+
| statusInit - initialize for status window, register the	|
|	       Window's class.					|
|								|
+--------------------------------------------------------------*/
//#pragma alloc_text(INIT_TEXT, statusInit)
BOOL statusInit(HANDLE hInst, HANDLE hPrev)
{
  WNDCLASS  cls;

  statusCreateTools();

  if (!hPrev){
	  cls.hCursor		= LoadCursor(NULL, IDC_ARROW);
	  cls.hIcon		= NULL;
	  cls.lpszMenuName	= NULL;
	  cls.lpszClassName	= szStatusClass;
	  cls.hbrBackground     = (HBRUSH) (COLOR_BTNFACE + 1);
	  cls.hInstance		= (HINSTANCE)hInst;
	  cls.style		= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	  cls.lpfnWndProc	= (WNDPROC)statusWndProc;
	  cls.cbClsExtra	= 0;
	  cls.cbWndExtra	= 0;
	
	  if (!RegisterClass(&cls))
		  return FALSE;

	  cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
	  cls.hIcon          = NULL;
	  cls.lpszMenuName   = NULL;
	  cls.lpszClassName  = (LPSTR)szText;
	  cls.hbrBackground  = (HBRUSH) (COLOR_BTNFACE + 1);
	  cls.hInstance      = (HINSTANCE)hInst;
	  cls.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	  cls.lpfnWndProc    = (WNDPROC)fnText;
	  cls.cbClsExtra     = 0;
	  cls.cbWndExtra     = 0;
	  if (!RegisterClass(&cls))
		return FALSE;
  }


  return TRUE;
}

/*
 * returns the recommended height for a status bar based on the
 * character dimensions of the font used
 */
int
statusGetHeight(void)
{
    return(gStatusStdHeight);
}


/*--------------------------------------------------------------+
| statusUpdateStatus - update the status line			|
|								|
| The argument can either be NULL, or a string,                 |
+--------------------------------------------------------------*/
void statusUpdateStatus(HWND hwnd, LPCTSTR lpsz)
{
    HWND hwndtext;

    hwndtext = GetDlgItem(hwnd, 1);
    if (!lpsz || *lpsz == '\0') {
		SetWindowText(hwndtext,"");
    } 
	else {
		SetWindowText(hwndtext, lpsz);
    }
}

/*--------------------------------------------------------------+
| statusWndProc - window proc for status window			|
|								|
+--------------------------------------------------------------*/
LRESULT CALLBACK
statusWndProc(HWND hwnd, unsigned msg, UINT wParam, LONG lParam)
{
  PAINTSTRUCT	ps;
  HDC		hdc;
  HWND          hwndSText;

  switch(msg){
    case WM_CREATE:
	
	    /* we need to create the static text control for the status bar */
	    hwndSText = CreateWindow(
                            szText,
                            "",
                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		            0, 0, 0, 0,
                            hwnd,
                            (HMENU) 1,  // child id
                            GetWindowInstance(hwnd),
                            NULL);

	    if (!hwndSText) {
		    return -1;
            }
	    break;
	
    case WM_DESTROY:
            statusDeleteTools();
	    break;
	
    case WM_SIZE:
        {
            RECT rc;

            GetClientRect(hwnd, &rc);

            MoveWindow(
                GetDlgItem(hwnd, 1),    // get child window handle
                2, 1,                   // xy just inside
                rc.right - 4,
                rc.bottom - 2,
                TRUE);

	    break;
        }

    case WM_PAINT:
	    hdc = BeginPaint(hwnd, &ps);

            // only the background and the child window need painting

	    EndPaint(hwnd, &ps);
	    break;

    case WM_SYSCOLORCHANGE:
        statusDeleteTools();
        statusCreateTools();
        break;

    case WM_ERASEBKGND:
        break;

  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
 * window proc for static text window
 */
LRESULT CALLBACK
fnText(HWND hwnd, UINT msg, UINT wParam, LONG lParam )
{
	PAINTSTRUCT ps;

	switch (msg) {
	case WM_SETTEXT:
		DefWindowProc(hwnd, msg, wParam, lParam);
		InvalidateRect(hwnd,NULL,FALSE);
		UpdateWindow(hwnd);
		return 0L;

	case WM_ERASEBKGND:
		return 0L;

	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		PaintText(hwnd, ps.hdc);
		EndPaint(hwnd, &ps);
		return 0L;
        }

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*--------------------------------------------------------------+
| PaintText - paint the shadowed static text field.		|
|								|
+--------------------------------------------------------------*/
void
PaintText(HWND hwnd, HDC hdc)
{
  RECT rc;
  char        ach[128];
  int  len;
  int	dx, dy;
  RECT	rcFill;
  HFONT	hfontOld;
  HBRUSH hbrSave;

  GetClientRect(hwnd, &rc);

  len = GetWindowText(hwnd,ach,sizeof(ach));

  SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
  SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

  hfontOld = (HFONT)SelectObject(hdc, ghFont);

  rcFill.left = rc.left + 1;
  rcFill.right = rc.right - 1;
  rcFill.top = rc.top + 1;
  rcFill.bottom = rc.bottom - 1;

  /* move in some and do background and text in one swoosh */
  ExtTextOut(hdc,4,1,ETO_OPAQUE,&rcFill,ach,len,NULL);

  dx = rc.right - rc.left;
  dy = rc.bottom - rc.top;

  hbrSave = (HBRUSH)SelectObject(hdc, ghbrShadow);
  PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
  PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);

  SelectObject(hdc, ghbrHL);
  PatBlt(hdc, rc.right-1, rc.top+1, 1, dy-1, PATCOPY);
  PatBlt(hdc, rc.left+1, rc.bottom -1, dx-1, 1,  PATCOPY);

  if (hfontOld)
	  SelectObject(hdc, hfontOld);
  if (hbrSave)
	  SelectObject(hdc, hbrSave);
  return ;
}
