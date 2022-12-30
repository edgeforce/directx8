//------------------------------------------------------------------------------
// File: Bdasampl.cpp
//
// Desc: Sample code implementing BDA graph building.
//
// Copyright (c) 2000, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "resource.h"
#include "graph.h"
#include <initguid.h>

// globals
HWND                hwndMain;
HWND                hwndDlg;
HINSTANCE           hInst;
TCHAR               szAppName[]  = TEXT("BDASampl");
TCHAR               szAppTitle[] = TEXT("BDA Sample");

CBDAFilterGraph*    pfg         = NULL;


INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine,
    INT nCmdShow)
{
    MSG         msg;
    HWND        hwnd;
    WNDCLASS    wndclass;
    HACCEL      hAccel;
    HDC         hdc;

    hdc     = CreateCompatibleDC(NULL);
    hInst   = hInstance;

    wndclass.style         = 0; //CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInst;
    wndclass.hIcon         = LoadIcon(hInst, "BDASAMPLICON");
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
    wndclass.lpszMenuName  = szAppName;
    wndclass.lpszClassName = szAppName;
    RegisterClass(&wndclass);

    hwnd = CreateWindow(szAppName, szAppTitle, WS_OVERLAPPEDWINDOW | 
                WS_CLIPCHILDREN, 200, 200, 500, 280, NULL, NULL, hInst, NULL);

    ASSERT(hwnd);

    // Create the BDA filter graph and initialize its components
    pfg = new CBDAFilterGraph();

    ASSERT(pfg);
    
    ShowWindow(hwnd, nCmdShow);
    hwndMain = hwnd;

    hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(ACC_GRAPH));

    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if(!TranslateAccelerator(hwnd, hAccel, &msg))
        {
            if(!IsDialogMessage(hwndDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    // Release the BDA components and clean up
    delete pfg;
	DeleteDC(hdc);
    
    return msg.wParam;
}


// WndProc                                                                    
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
        return 0;

    case WM_SIZE:
		if(pfg->m_fGraphBuilt)
			pfg->SetVideoWindow(hwndMain);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDM_BUILD_ATSC:
            if(FAILED(pfg->BuildGraph(ATSC)))
            {
                ErrorMessageBox("Could not Build the ATSC BDA FilterGraph");
            }
            else
            {
                pfg->SetVideoWindow(hwndMain);
            }
            
            EnableMenuItem((HMENU)wParam, IDM_BUILD_ATSC, MF_GRAYED);
            // DVB is not implemented for DX8 RC0
            //EnableMenuItem((HMENU)wParam, IDM_BUILD_DVB, MF_GRAYED);
            
            break;

		/* DVB is not implemented for DX8 RC0

        case IDM_BUILD_DVB:
            if(FAILED(pfg->BuildGraph(DVB)))
            {
                ErrorMessageBox("Could not Build the DVB BDA FilterGraph");
            }
            else
            {
                pfg->SetVideoWindow(hwndMain);
            }
            EnableMenuItem((HMENU)wParam, IDM_BUILD_ATSC, MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDM_BUILD_DVB, MF_GRAYED);

            break;
		*/

        case IDM_RUN_GRAPH:
            if(pfg->m_fGraphBuilt)
            {   
                if(!pfg->m_fGraphRunning)
                {
                    if(FAILED(pfg->RunGraph()))
                    {
                        ErrorMessageBox("Could not Play the FilterGraph");
                    }
                }
            }
            else
            {
                ErrorMessageBox("The FilterGraph is not built yet");
            }

            break;

        case IDM_STOP_GRAPH:
            if(pfg->m_fGraphBuilt)
            {
                if(pfg->m_fGraphRunning)
                {
                    if(FAILED(pfg->StopGraph()))
                    {
                        ErrorMessageBox("Could not Stop the FilterGraph");
                    }
                }
            }
            else
            {
                ErrorMessageBox("The FilterGraph is not Built Yet");
            }
            
            break;

        case IDM_SELECT_CHANNEL:
            if(pfg->m_fGraphBuilt)
            {
                hwndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SELECT_CHANNEL),
                    hwnd, reinterpret_cast<DLGPROC>(SelectChannelDlgProc));
            }
            else
            {
                ErrorMessageBox("The FilterGraph is not built yet");
            }
            
            break;

        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, 
                reinterpret_cast<DLGPROC>(AboutDlgProc));
            break;

        case IDM_EXIT:
            DbgTerminate();
            PostQuitMessage(0);
            break;

        default:
            break;
        }

    case WM_PAINT:
        break;

    case WM_ERASEBKGND:
        break;

    case WM_INITMENU:
        if(pfg->m_fGraphFailure)
        {
            EnableMenuItem((HMENU)wParam, IDM_BUILD_ATSC,       MF_GRAYED);
            // DVB is not implemented for DX8 RC0 
            //EnableMenuItem((HMENU)wParam, IDM_BUILD_DVB,        MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDM_SELECT_CHANNEL,   MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDM_RUN_GRAPH,        MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDM_STOP_GRAPH,       MF_GRAYED);
        }
        else
        {
            EnableMenuItem((HMENU)wParam, IDM_RUN_GRAPH, 
                pfg->m_fGraphRunning ? MF_GRAYED : MF_ENABLED);

            EnableMenuItem((HMENU)wParam, IDM_BUILD_ATSC, 
                pfg->m_fGraphBuilt ? MF_GRAYED : MF_ENABLED);
        
            // DVB is not implemented for DX8 RC0 
            //EnableMenuItem((HMENU)wParam, IDM_BUILD_DVB, MF_GRAYED
            //    pfg->m_fGraphBuilt ? MF_GRAYED : MF_ENABLED);

            // we can stop viewing if it's currently viewing
            EnableMenuItem((HMENU)wParam, IDM_STOP_GRAPH, 
                (pfg->m_fGraphRunning) ? MF_ENABLED : MF_GRAYED);

            EnableMenuItem((HMENU)wParam, IDM_SELECT_CHANNEL, MF_ENABLED);
        }

        break;

    case WM_CLOSE:
    case WM_DESTROY:
        DbgTerminate();
        PostQuitMessage(0);
        break;

    case WM_ACTIVATE:
        if(LOWORD(wParam) != WA_INACTIVE)
        {
            HWND hwndPrevious = (HWND)lParam;

            if((GetParent(hwndPrevious) == hwnd) && IsWindow(hwndPrevious) &&
                IsWindowVisible(hwndPrevious))
            {
                if(!SetActiveWindow(hwndPrevious))
                {
                }
            }
            return 0;
        }
    default:
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}


// AboutDlgProc
//
// Dialog Procedure for the "about" dialog box.
//
BOOL CALLBACK 
AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) 
    {
    case WM_COMMAND:
        EndDialog(hwnd, TRUE);
        return TRUE;
    
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}


// SelectChannelDlgProc
// Dialog Procedure for the "about" dialog box.
//                                                                              
BOOL CALLBACK
SelectChannelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG    ch_maj  = pfg->m_MajorChannel;
    LONG    ch_min  = pfg->m_MinorChannel;
    BOOL    bRet    = FALSE;
    
    switch(message)
    {
    case WM_INITDIALOG:
        EnableWindow(hwndMain, FALSE); //disable parent
        pfg->Refresh(hDlg);
        SetFocus(hDlg);
        
        return FALSE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_ENTER:
            ch_maj = (LONG) GetDlgItemInt(hDlg, IDC_MAJOR_CHANNEL, &bRet, FALSE);
            ch_min = (LONG) GetDlgItemInt(hDlg, IDC_MINOR_CHANNEL, &bRet, TRUE);
            
            pfg->ChangeChannel(ch_maj, ch_min);

            // update dialog box
            pfg->Refresh(hDlg);

            SetFocus(hDlg);
            break;

        case IDOK:
            ch_maj = (LONG) GetDlgItemInt(hDlg, IDC_MAJOR_CHANNEL, &bRet, FALSE);
            ch_min = (LONG) GetDlgItemInt(hDlg, IDC_MINOR_CHANNEL, &bRet, TRUE);
            
            pfg->ChangeChannel(ch_maj, ch_min);

            // update dialog box
            pfg->Refresh(hDlg);

            // fall through
            
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;

        case IDC_MAJOR_CHANNEL_UP:
            ch_min = (LONG) GetDlgItemInt(hDlg, IDC_MINOR_CHANNEL, &bRet, TRUE);
            pfg->ChangeChannel(CHANNEL_UP, ch_min); 
            
            // update dialog box
            pfg->Refresh(hDlg);

            SetFocus(hDlg);

            break;

        case IDC_MAJOR_CHANNEL_DOWN:
            ch_min = (LONG) GetDlgItemInt(hDlg, IDC_MINOR_CHANNEL, &bRet, TRUE);
            pfg->ChangeChannel(CHANNEL_DOWN, ch_min);

            // update dialog box
            pfg->Refresh(hDlg);

            SetFocus(hDlg);
            break;

        case IDC_MINOR_CHANNEL_UP:
            ch_maj = (LONG) GetDlgItemInt(hDlg, IDC_MAJOR_CHANNEL, &bRet, FALSE);
            pfg->ChangeChannel(ch_maj, CHANNEL_UP); 
            
            // update dialog box
            pfg->Refresh(hDlg);

            SetFocus(hDlg);

            break;

        case IDC_MINOR_CHANNEL_DOWN:
            ch_maj = (LONG) GetDlgItemInt(hDlg, IDC_MAJOR_CHANNEL, &bRet, FALSE);
            pfg->ChangeChannel(ch_maj, CHANNEL_DOWN);

            // update dialog box
            pfg->Refresh(hDlg);

            SetFocus(hDlg);
            break;

        default:
            EnableWindow(hwndMain, TRUE); //reenable parent
            return TRUE;
        }
    }

    return FALSE;
}


// ErrorMessageBox
//
// Opens a Message box with a error message in it.  The user can     
// select the OK button to continue.
//
VOID
ErrorMessageBox(LPTSTR sz,...)
{
    static TCHAR    ach[2000];
    va_list         va;

    va_start(va, sz);
    wvsprintf(ach, sz, va);
    va_end(va);

    MessageBox(hwndMain, ach, NULL, MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
}

