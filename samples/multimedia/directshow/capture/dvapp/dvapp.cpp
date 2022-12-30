//------------------------------------------------------------------------------
// File: DVApp.cpp
//
// Desc: DirectShow sample code - DV control/capture example.
//
// Copyright (c) 1993 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include "dvapp.h"

/*-------------------------------------------------------------------------
Routine:        WinMain
Purpose:        Program entry point      
Arguments:    Usual
Returns:        Usual
Notes:          Sets up window capabilities, initializes & creates the required DirectShow interface & filters.
------------------------------------------------------------------------*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
    static TCHAR    szAppName[] = APPNAME ;
    MSG                 msg ;
    WNDCLASSEX wndclass ;
    HMENU           hmenu;

    // Set the Window Parameters   
    wndclass.cbSize        = sizeof (wndclass) ;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = DV_WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (hInstance, TEXT("DVICON")) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = szAppName ;
    wndclass.hIconSm       = LoadIcon(hInstance, TEXT("DVICON"));
    // create & setup the window
    RegisterClassEx (&wndclass) ;
    hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    g_hwndApp = CreateWindow  (szAppName,            // window class name
                            DV_APPTITLE,             // window caption
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,     // window style
                            CW_USEDEFAULT,           // initial x position
                            CW_USEDEFAULT,           // initial y position
                            g_iAppWidth,             // initial x size
                            g_iAppHeight,            // initial y size
                            NULL,                    // parent window handle
                            hmenu,                   // window menu handle
                            hInstance,               // program instance handle
                            NULL) ;                   // creation parameters
    
    ShowWindow (g_hwndApp, iCmdShow) ;
    UpdateWindow (g_hwndApp) ;

    // Initialize UI & Logging levels
    SetWindowText(g_hwndApp, TEXT("Initializing..."));
    CheckMenuItem(hmenu, IDM_LEVEL_SUCCINCT, MF_CHECKED);
    CheckMenuItem(hmenu, IDM_PRIORITY_ERROR, MF_CHECKED);

    // Register for device add/remove notifications.
    DEV_BROADCAST_DEVICEINTERFACE filterData;
    ZeroMemory(&filterData, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
    
    filterData.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    filterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filterData.dbcc_classguid = AM_KSCATEGORY_CAPTURE;

    g_pUnregisterDeviceNotification = NULL;
    g_pRegisterDeviceNotification = NULL;
    // dynload device removal APIs
    {
        HMODULE hmodUser = GetModuleHandle(TEXT("user32.dll"));
        ASSERT(hmodUser);       // we link to user32
        g_pUnregisterDeviceNotification = (PUnregisterDeviceNotification)
            GetProcAddress(hmodUser, "UnregisterDeviceNotification");

        // m_pRegisterDeviceNotification is prototyped differently in unicode
        g_pRegisterDeviceNotification = (PRegisterDeviceNotification)
            GetProcAddress(hmodUser,
#ifdef UNICODE
                           "RegisterDeviceNotificationW"
#else
                           "RegisterDeviceNotificationA"
#endif
                           );
        // failures expected on older platforms.
        ASSERT(g_pRegisterDeviceNotification && g_pUnregisterDeviceNotification ||
               !g_pRegisterDeviceNotification && !g_pUnregisterDeviceNotification);
    }

    g_hDevNotify = NULL;

    if (g_pRegisterDeviceNotification)
    {
        g_hDevNotify = g_pRegisterDeviceNotification(g_hwndApp, &filterData, DEVICE_NOTIFY_WINDOW_HANDLE);
        ASSERT(g_hDevNotify != NULL);
	}

    // Setup & Initialize the device & filtergraph
    DV_AppSetup();

    // Message processing loop
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }
    return (int) msg.wParam;
}


/*-------------------------------------------------------------------------
Routine:        DV_TransportCommand_WndProc 
Purpose:        Message Handler to control transport state of the device & the filtergraph state
Arguments:    Usual message processing parameters
Returns:        Usual
Notes:          Handles for the Toolbar button controls
------------------------------------------------------------------------*/
void CALLBACK DV_TransportCommand_WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HMENU       hmenu;   
    switch (iMsg)
    {
    case WM_COMMAND :
        hmenu = GetMenu(hwnd);
        switch (LOWORD (wParam))
        {
    /*
        The VCR Tool bar commands need to behave differently depending on the 
        current graph.  

        In Preview (default) mode, the VCR commands should simply
        control the VCR functions on the vcr device.

        In DV To File mode, or File To DV mode the commands should start and stop the graph, 
        as  well as control the vcr mode (although we will disable some buttons to avoid confusion)
    */
        case IDM_PLAY :
        {
            //check if the current graph is a transmit graph
            if (GRAPH_FILE_TO_DV == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE == g_iCurrentGraph || 
                GRAPH_FILE_TO_DV_TYPE2 == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE_TYPE2 == g_iCurrentGraph)
            {
                // update the toolbar accordingly
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                //update the status bar with the dropped frames information
                g_CapStartTime = GetTickCount();
                SetTimer(hwnd, DV_TIMER_FRAMES,  DV_TIMERFREQ, (TIMERPROC) DV_DroppedFrameProc);
                // run the filter graph & wait for dshow events
                DV_StartGraph();
                IMediaEventEx *pMe = NULL;
                if (SUCCEEDED(g_pGraphBuilder->QueryInterface(IID_IMediaEventEx, reinterpret_cast<PVOID *>(&pMe))))
                {
                    pMe->SetNotifyWindow((LONG_PTR) g_hwndApp, WM_FGNOTIFY, 0);
                    SAFE_RELEASE(pMe);
                } 
            } 
            else
            {
                // play the tape for the capture or preview graph
                DV_PutVcrMode(ED_MODE_PLAY);

                // run the filter graph too if it is preview
                if(GRAPH_PREVIEW == g_iCurrentGraph)
                    DV_StartGraph();
                //do we want to display timecodes?
                if (IsDlgButtonChecked(g_hwndTBar, IDC_TCCHECKBOX))
                {
                    SetTimer(hwnd, DV_TIMER_ATN,  DV_TIMERFREQ, (TIMERPROC) DV_TimecodeTimerProc);
                    g_bUseAtnTimer = TRUE;
                } 
            } 
            break;
        }

        /*
            The record button starts the *entire* graph, so preview (if selected), won't
            start until the recording starts.  
        */
        case IDM_RECORD :
            // update the toolbar accordingly
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));

            // check to see if it is a capture graph
            if (GRAPH_DV_TO_FILE == g_iCurrentGraph || GRAPH_DV_TO_FILE_NOPRE == g_iCurrentGraph ||
                GRAPH_DV_TO_FILE_TYPE2 == g_iCurrentGraph || GRAPH_DV_TO_FILE_NOPRE_TYPE2 == g_iCurrentGraph)
            {
                //do something here to record to an avi file on the disk - or to start recording on the vcr.
                switch (g_dwCaptureLimit)
                {
                    case DV_CAPLIMIT_NONE :
                        break;
                    case DV_CAPLIMIT_TIME :
                        SetTimer(hwnd, DV_TIMER_CAPLIMIT, g_dwTimeLimit * 1000, (TIMERPROC) DV_StopRecProc);
                        break;
                    case DV_CAPLIMIT_SIZE :
                        //rather than monitor disk usage, we'll just do the math and set a timer
                        SetTimer(hwnd, DV_TIMER_CAPLIMIT, ((g_dwDiskSpace * 100000) / DV_BYTESPERMSEC), (TIMERPROC) DV_StopRecProc);
                        break;
                    default :
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Bad value for g_dwCaptureLimit (%d)"), g_dwCaptureLimit);
                        break;
                }
                //update the status bar with the dropped frames information
                g_CapStartTime = GetTickCount();
                SetTimer(hwnd, DV_TIMER_FRAMES,  DV_TIMERFREQ, (TIMERPROC) DV_DroppedFrameProc);
                //run the graph - assume that the camera is already playing if in Vcr mode
                DV_StartGraph();             
            } 
            else if (GRAPH_FILE_TO_DV == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE == g_iCurrentGraph ||
                     GRAPH_FILE_TO_DV_TYPE2 == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE_TYPE2 == g_iCurrentGraph)
            {
                // if transmit graph then record on tape of the device
                DV_PutVcrMode(ED_MODE_RECORD);
            } 
            else
            {
                //we shouldn't get here
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Undefined graph mode (maybe GRAPH_PREVIEW) in IDM_RECORD message"));
            } 
            break;

        case IDM_STOP :
            if (GRAPH_PREVIEW != g_iCurrentGraph)
            {   // for transmit & capture graphs we need stats if any
                DV_GetFinalDroppedFramesStats(GetTickCount());
            } 
    
            // handle the timers accordingly
            if (g_bUseAtnTimer)
                KillTimer(hwnd, DV_TIMER_ATN);
            //if we're here, these timers were set
            KillTimer(hwnd, DV_TIMER_CAPLIMIT);
            KillTimer(hwnd, DV_TIMER_FRAMES);

            if (GRAPH_PREVIEW != g_iCurrentGraph)
            {   // for transmit & capture graphs we need stop the graph, preview graph is running all the time
                DV_StopGraph();
            } 
            // Stop the transport on the device
            DV_PutVcrMode(ED_MODE_STOP);
            // update the toolbar 
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY, MAKELONG(TBSTATE_ENABLED, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            break;

        case IDM_PAUSE :
            if (GRAPH_FILE_TO_DV == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE == g_iCurrentGraph ||
                GRAPH_FILE_TO_DV_TYPE2 == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE_TYPE2 == g_iCurrentGraph)
            {   // transmit graph
                DV_PauseGraph();
            } 
            else
            {   // capture or preview graph
                DV_PutVcrMode(ED_MODE_FREEZE);
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
            } 
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY, MAKELONG(TBSTATE_ENABLED, 0L));
            break;    

        case IDM_FF :
            // all graphs just forward the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_FF);
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            break;

        case IDM_REW :
            // all graphs just rewind the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_REW);
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            break;

        case IDM_PLAY_FAST_FF :
            // all graphs just forward the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_PLAY_FASTEST_FWD);
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            break;

        case IDM_PLAY_FAST_REV :
            // all graphs just rewind the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_PLAY_FASTEST_REV);
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
            break;            

        case IDM_STEP_FWD :
            // all graphs just forward the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_STEP_FWD);
            g_bUseAtnTimer = FALSE;
            DV_DisplayTimecode();
            break;

        case IDM_STEP_REV :
            // all graphs just rewind the tape & update the toolbar
            DV_PutVcrMode(ED_MODE_STEP_REV);
            g_bUseAtnTimer = FALSE;
            DV_DisplayTimecode();
            break;

        case IDM_SEEKTIMECODE :
            // ATN Seek & display on the toolbar
             DV_SeekATN();
             DV_DisplayTimecode();
             break;

        } // switch (LOWORD(wParam))
    } // switch (iMsg)
}




/*-------------------------------------------------------------------------
Routine:        DV_WndProc
Purpose:        Message Handler
Arguments:    Usual
Returns:        Usual
Notes:          Handles Windows UI & Command Messages, Menu Messages et al
------------------------------------------------------------------------*/
LRESULT CALLBACK DV_WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc ;
    PAINTSTRUCT ps ;
    RECT        rect;
    HMENU       hmenu;
    static HWND hWndTT;
    
    switch (iMsg)
    {
    case WM_CREATE :
        // Initializes & loads the Controls like toolbar buttons etc
        if (!DV_InitControls(hwnd, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE)))
        {
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("DV_InitControls failed to create one of the control windows"));
        }
        break;

    case WM_PAINT :
        hdc = BeginPaint (hwnd, &ps) ;
        GetClientRect (hwnd, &rect) ;
        EndPaint (hwnd, &ps) ;
        // not calling default message handler
        return 0;

    case WM_SIZE:
    {
        RECT  rcAppWin, rcClient, rcTB;
        //re-size the App window
        int cxBorder = GetSystemMetrics(SM_CXBORDER);
        int cyBorder = GetSystemMetrics(SM_CYBORDER);
        GetWindowRect(g_hwndApp, &rcAppWin);
        GetWindowRect(g_hwndTBar, &rcTB);
        MoveWindow(g_hwndApp, rcAppWin.left, rcAppWin.top, g_iAppWidth, g_iAppHeight, TRUE);

        // Tell the toolbar to resize itself to fill the top of the window.
        SendMessage(g_hwndTBar, TB_AUTOSIZE, 0L, 0L);

        //handle the status bar height
        GetClientRect(g_hwndApp, &rcClient);
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        MoveWindow(g_hwndStatus, -cxBorder, rcClient.bottom - (g_statusHeight + cyBorder), 
                   rcClient.right + (2 * cxBorder), (g_statusHeight + (2 * cyBorder)), TRUE);  
        DV_StatusParts(rcClient.right);
        
        //re-size the video window
        GetWindowRect(g_hwndTBar, &rcTB);
        if (g_pVideoWindow)
        {
            g_pVideoWindow->SetWindowPosition(0, rcTB.bottom - rcTB.top, g_iVWWidth, g_iVWHeight);
        }
        break;
    }

	case WM_CLOSE:
		return SendMessage(hwnd, WM_DESTROY, 0,0);

    case WM_DESTROY :
        // Unregister device notifications
        if (g_hDevNotify != NULL)
        {
            ASSERT(g_pUnregisterDeviceNotification);
            g_pUnregisterDeviceNotification(g_hDevNotify);
            g_hDevNotify = NULL;
        }
        // Time to Kill All
        DV_CleanUp();
        hmenu = GetMenu(hwnd);
        DestroyMenu(hmenu);
        DestroyWindow(g_hwndTBar);
        DestroyWindow(g_hwndStatus);
        PostQuitMessage (0) ;
        // not calling default message handler
        return 0;     

    case WM_NOTIFY :
    {   
        //Windows Events Notify Message handler
        switch (((LPNMHDR)lParam)->code)
        {
            case TTN_NEEDTEXT :
                LPTOOLTIPTEXT lpttt;
                TCHAR         ttBuff[128];
                lpttt = (LPTOOLTIPTEXT) lParam; 
                LoadString ((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), (UINT) lpttt->hdr.idFrom, ttBuff, 128);
                lpttt->lpszText = ttBuff;           
                break;
        } 
        break;   
    } 

    case WM_DEVICECHANGE:
    {
        // We are interested in only device arrival events
        if (DBT_DEVICEARRIVAL != wParam)
            break;
        PDEV_BROADCAST_HDR pdbh = (PDEV_BROADCAST_HDR) lParam;
        if (pdbh->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
            break;

        // Check for capture devices.
        PDEV_BROADCAST_DEVICEINTERFACE pdbi = (PDEV_BROADCAST_DEVICEINTERFACE) lParam;
        if (pdbi->dbcc_classguid != AM_KSCATEGORY_CAPTURE)
            break;

        // Check for device arrival.
        if (g_bDeviceFound == FALSE)
        {
            MessageBox(g_hwndApp, TEXT("There is a new capture device on this system..."), APPNAME, MB_OK);
            DV_CleanUp();
            DV_AppSetup();
        }
        break;
    }
    
    case WM_FGNOTIFY :
    {
        //Filter Graph Events Notify Message handler
        IMediaEventEx *pMe = NULL;
        if(g_pGraphBuilder == NULL)
            break;
        else if (SUCCEEDED(g_pGraphBuilder->QueryInterface(IID_IMediaEventEx, reinterpret_cast<PVOID *>(&pMe))))
        {
            long event;
            LONG_PTR l1, l2;
            // Get the corresponding filtergraph directshow media event to handle
            while (SUCCEEDED(pMe->GetEvent(&event, &l1, &l2, 0L)))
            {
                switch (event)
                {
                    case EC_USERABORT :
                    case EC_COMPLETE :
                        // filtergraph is done, time to stop the filtergraph or transport state
                        SendMessage(g_hwndApp, WM_COMMAND, IDM_STOP, 0);  
                        break;
                    
                    case EC_ERRORABORT :
                        //something bad happened during capture
                        MBOX(TEXT("Error during preview, capture or transmit"));
                        break;
                    case EC_DEVICE_LOST :
                        // Check if we have lost a capture filter being used.
                        // lParam2 of EC_DEVICE_LOST event == 0 indicates device removed
                        // lParam2 of EC_DEVICE_LOST event == 1 indicates removed device added again
                        if (l2 == 0)
                        {
                            IBaseFilter *pf;
                            IUnknown *punk = (IUnknown *) l1;
                            if (S_OK == punk->QueryInterface(IID_IBaseFilter, (void **) &pf))
                            {
                                if (::IsEqualObject(g_pDVCamera, pf))
                                {
                                    pMe->FreeEventParams(event, l1, l2);

									// handle the timers accordingly
									if (g_bUseAtnTimer)
										KillTimer(hwnd, DV_TIMER_ATN);
									KillTimer(hwnd, DV_TIMER_CAPLIMIT);
									KillTimer(hwnd, DV_TIMER_FRAMES);
                                    // Cleanup the old device's filtergraph & 
                                    // Search for other DV camcorders or the same DV camcorder being plugged in again, 
                                    // if found set it up else exit the app
                                    MBOX("DV Camcorder Device in use was removed");
                                    g_bDeviceFound = FALSE;
                                    DV_CleanUp();
                                    DV_AppSetup();
                                }
								pf->Release();
                            }
                        }
                        break;

                    default :
                        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("Unhandled Event Code (0x%x)"), event);
                        break;
                }
                pMe->FreeEventParams(event, l1, l2);
            } 
            SAFE_RELEASE(pMe);
        } 
        SAFE_RELEASE(pMe);       
        break;
    }

    
    case WM_COMMAND :
        // Handle menu commands
        hmenu = GetMenu(hwnd);
        switch (LOWORD (wParam))
        {
            // Help Menu
        case IDM_ABOUT:
            DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC)DV_AboutDlgProc);
            break;

        // The File Menu 
        /*  
            The globals for input file and output file are TCHAR.  When compiled ANSI,
            GetOpenFileName (obviously) return ANSI buffers.  DShow functions use Unicode
            names exclusively, so these are converted to Unicode within the functions 
            that need to use these variables (DV_Make...To... functions).
        */
        case IDM_SETINPUT :    //fall through
        case IDM_SETOUTPUT :
        case IDM_OPTIONS_SAVEGRAPH :
        {
            OPENFILENAME ofn = {0};
            OSVERSIONINFO osi = {0};
            //need to adjust the ofn struct size if we are running on win98 vs. nt5
            osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osi);
            if (osi.dwMajorVersion >=5 && osi.dwPlatformId == VER_PLATFORM_WIN32_NT)
            {
                ofn.lStructSize = sizeof (OPENFILENAME);
            } 
            else
            {
                ofn.lStructSize = sizeof(OPENFILENAME);
            }   
            
            ofn.hwndOwner = g_hwndApp;
            ofn.nMaxFile = _MAX_PATH;
            if (IDM_OPTIONS_SAVEGRAPH == LOWORD (wParam))
            {
                ofn.lpstrFilter = TEXT("Filter Graph\0*.grf\0\0");
                ofn.lpstrTitle = TEXT("Set FilterGraph File Name");
                ofn.lpstrFile = g_FilterGraphFileName;
            } 
            else if (IDM_SETOUTPUT == LOWORD (wParam))
            {
                ofn.lpstrFilter = TEXT("Microsoft AVI\0*.avi\0\0");
                ofn.lpstrTitle = TEXT("Set output File Name");
                ofn.lpstrFile = g_OutputFileName;
            } 
            else
            {
                ofn.lpstrFilter = TEXT("Microsoft AVI\0*.avi\0\0");
                ofn.lpstrTitle = TEXT("Set input File Name");
                ofn.lpstrFile = g_InputFileName;
            } 

            if (GetOpenFileName(&ofn))
            {
                if (IDM_OPTIONS_SAVEGRAPH == LOWORD (wParam))
                {
                    lstrcpy(g_FilterGraphFileName, ofn.lpstrFile);
                    // Save the current built filter graph to a *.grf file
                    DV_SaveGraph(g_FilterGraphFileName);
                } 
                else if (IDM_SETOUTPUT == LOWORD (wParam))
                {
                    lstrcpy(g_OutputFileName, ofn.lpstrFile);
                } 
                else
                {
                    lstrcpy(g_InputFileName, ofn.lpstrFile);
                } 
            }
            break;    
        } 

        case IDM_CAPSIZE:
            DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_DIALOG_CAPSIZE), hwnd, (DLGPROC)DV_CapSizeDlgProc);
            break;

        case IDM_EXIT :
            return SendMessage(hwnd, WM_DESTROY, 0,0);

        // Graph Mode Menu
        //Change the Current Graph Mode
        case IDM_PREVIEW :
        case IDM_FILETODV :
        case IDM_FILETODV_NOPRE :
        case IDM_DVTOFILE :
        case IDM_DVTOFILE_NOPRE :
        case IDM_FILETODV_TYPE2 :
        case IDM_FILETODV_NOPRE_TYPE2 :
        case IDM_DVTOFILE_TYPE2 :
        case IDM_DVTOFILE_NOPRE_TYPE2 :
            DV_GraphModeCommand_WndProc (hwnd, iMsg, wParam, lParam);
            break;

        // Toolbar Button Commands
        // Transport State & Graph State Control
        case IDM_PLAY :
        case IDM_RECORD :
        case IDM_STOP :
        case IDM_PAUSE :
        case IDM_FF :
        case IDM_REW :
        case IDM_PLAY_FAST_FF :
        case IDM_PLAY_FAST_REV :
        case IDM_STEP_FWD :
        case IDM_STEP_REV :
        case IDM_SEEKTIMECODE :
            DV_TransportCommand_WndProc(hwnd, iMsg, wParam, lParam);
            break;

        // The Options Menu
        case IDM_DECODESIZE :
            DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_DIALOG_DECODESIZE), hwnd, (DLGPROC)DV_ChangeDecodeSizeProc);
            break;
            
        case IDM_CHECKTAPE :
            if (S_OK != DV_GetTapeInfo())
            {
                MBOX(TEXT("Tape is not inserted, or has an improper format.\nReinsert the tape and select Options - Check Tape again"));
            } 
            break;

        case IDM_REFRESHMODE :
            DV_RefreshMode();
            break;
                
        // Log Menu :: Set logging Priority & level 
        case IDM_LEVEL_SUCCINCT :
             CheckMenuItem(hmenu, IDM_LEVEL_SUCCINCT, MF_CHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_MEDIUM, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_VERBOSE, MF_UNCHECKED);
             g_iLogLevel = LOG_LEVEL_SUCCINCT;
             break;
        case IDM_LEVEL_MEDIUM :
             CheckMenuItem(hmenu, IDM_LEVEL_SUCCINCT, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_MEDIUM, MF_CHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_VERBOSE, MF_UNCHECKED);
             g_iLogLevel = LOG_LEVEL_MEDIUM;
             break;
        case IDM_LEVEL_VERBOSE :
             CheckMenuItem(hmenu, IDM_LEVEL_SUCCINCT, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_MEDIUM, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_LEVEL_VERBOSE, MF_CHECKED);
             g_iLogLevel = LOG_LEVEL_VERBOSE;
             break;

        case IDM_PRIORITY_ERROR :
             CheckMenuItem(hmenu, IDM_PRIORITY_ERROR, MF_CHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_WARN, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_INFO, MF_UNCHECKED);
             g_iLogPriority = LOG_PRIORITY_ERROR;
             break;
        case IDM_PRIORITY_WARN :
             CheckMenuItem(hmenu, IDM_PRIORITY_ERROR, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_WARN, MF_CHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_INFO, MF_UNCHECKED);
             g_iLogPriority = LOG_PRIORITY_WARN;
             break;
        case IDM_PRIORITY_INFO :
             CheckMenuItem(hmenu, IDM_PRIORITY_ERROR, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_WARN, MF_UNCHECKED);
             CheckMenuItem(hmenu, IDM_PRIORITY_INFO, MF_CHECKED);
             g_iLogPriority = LOG_PRIORITY_INFO;
             break;

        } // switch (LOWORD(wParam))
        
               
    } // switch (iMsg)
    return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}


/*-------------------------------------------------------------------------
Routine:        DV_GraphModeCommand_WndProc
Purpose:        Message Handler for Graph Mode menu items
Arguments:    Usual message processing parameters
Returns:        Usual
Notes:          Builds the various kinds of Graph types preview/capture/transmit
------------------------------------------------------------------------*/
void CALLBACK DV_GraphModeCommand_WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HMENU       hmenu;    
    switch (iMsg)
    {
    case WM_COMMAND :
        hmenu = GetMenu(hwnd);
        switch (LOWORD (wParam))
        {   
        case IDM_PREVIEW :
            // Call the Special DV Graph Builder Method 
            if (DV_MakeSpecialGraph(GRAPH_PREVIEW))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_PREVIEW;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built Preview Graph"));

                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                //re-enable everything else
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));
                
                CheckMenuItem(hmenu, IDM_PREVIEW, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);
            } 
            break;

        // Type 1 file (transmit & playback)    
        case IDM_FILETODV :
            if (DV_MakeSpecialGraph(GRAPH_FILE_TO_DV))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_FILE_TO_DV;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built File to DV Graph (Type1)"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

                //disable everything except for play, pause, and stop
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_INDETERMINATE, 0L));

                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);
            } 
            break;

        // Type 1 file (transmit)   
        case IDM_FILETODV_NOPRE :
            if (DV_MakeSpecialGraph(GRAPH_FILE_TO_DV_NOPRE))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_FILE_TO_DV_NOPRE;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built File to DV Graph (Type1) without Preview"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

                //disable everything except for play, pause, and stop
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_INDETERMINATE, 0L));

                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);

            } 
            break;
            
        // Type 1 file (capture & preview)  
        case IDM_DVTOFILE :
            if (DV_MakeSpecialGraph(GRAPH_DV_TO_FILE))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_DV_TO_FILE;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built DV to File Graph (Type1)"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));
                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);
                //re-enable everything 
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

            } 
            break;

        // Type 1 file (capture)    
        case IDM_DVTOFILE_NOPRE :
            if (DV_MakeSpecialGraph(GRAPH_DV_TO_FILE_NOPRE))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_DV_TO_FILE_NOPRE;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built DV to File Graph (Type1) without Preview"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));
                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_CHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);

                //re-enable everything 
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));
            } 
            break;

        // Type 2 File (transmit & playback)
        case IDM_FILETODV_TYPE2 :
            if (DV_MakeSpecialGraph(GRAPH_FILE_TO_DV_TYPE2))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_FILE_TO_DV_TYPE2;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built File to DV Graph (Type2)"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

                //disable everything except for play, pause, and stop
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_INDETERMINATE, 0L));

                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);
            } 
            break;

        // Type 2 File (transmit)
        case IDM_FILETODV_NOPRE_TYPE2 :
            if (DV_MakeSpecialGraph(GRAPH_FILE_TO_DV_NOPRE_TYPE2))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_FILE_TO_DV_NOPRE_TYPE2;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built File to DV Graph (Type2) without Preview"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

                //disable everything except for play, pause, and stop
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_INDETERMINATE, 0L));

                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);               
            } 
            break;
            
        // Type 2 File (capture & preview)
        case IDM_DVTOFILE_TYPE2 :
            if (DV_MakeSpecialGraph(GRAPH_DV_TO_FILE_TYPE2))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_DV_TO_FILE_TYPE2;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built DV to File Graph (Type2)"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));
                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_CHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_UNCHECKED);
                //re-enable everything 
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

            } 
            break;

        // Type 2 File (capture)
        case IDM_DVTOFILE_NOPRE_TYPE2 :
            if (DV_MakeSpecialGraph(GRAPH_DV_TO_FILE_NOPRE_TYPE2))
            {
                // update globals, log, toolbar, menu items
                g_iCurrentGraph = GRAPH_DV_TO_FILE_NOPRE_TYPE2;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built DV to File Graph (Type2) without Preview"));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));
                CheckMenuItem(hmenu, IDM_PREVIEW, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_UNCHECKED);

                CheckMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_UNCHECKED);
                CheckMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_CHECKED);

                //re-enable everything 
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_ENABLED, 0L));

            } 
            break;
        } // switch (LOWORD(wParam))
    } // switch (iMsg)
}

/*-------------------------------------------------------------------------
Routine:        DV_AppSetup
Purpose:        look for a DV device, initialize it, get its subunit mode, Create the filtergraph and instantiate the filters
Arguments:    None
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void DV_AppSetup(void)
{
    
    // COM initialization
    CoInitialize(NULL);

    HMENU hmenu = GetMenu(g_hwndApp);   
    /*
        look for a DV device and initialize it,
        or show an error if one does not exist
    */  
    if (!DV_InitDevice())
    {
        g_bDeviceFound = FALSE;
        int iOption = MessageBox(g_hwndApp, TEXT("There are no DV Camcorder devices on this system\n\nDo you want to exit the app?"), APPNAME, MB_YESNO);
        if(iOption == IDYES) 
            SendMessage(g_hwndApp, WM_DESTROY, 0,0);
        else
        {
            DV_CleanUp();
            ShowWindow(g_hwndTBar, SW_HIDE);

            EnableMenuItem(hmenu, IDM_REFRESHMODE, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_CHECKTAPE, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_DECODESIZE, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_PREVIEW, MF_GRAYED);

            EnableMenuItem(hmenu, IDM_DVTOFILE, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_FILETODV, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_GRAYED);

            EnableMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_GRAYED);
            EnableMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_GRAYED);
        }
    }
    else
    {
            g_bDeviceFound = TRUE;
            ShowWindow(g_hwndTBar, SW_SHOWNORMAL);
            EnableMenuItem(hmenu, IDM_REFRESHMODE, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_CHECKTAPE, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_DECODESIZE, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_PREVIEW, MF_ENABLED);

            EnableMenuItem(hmenu, IDM_DVTOFILE, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_DVTOFILE_NOPRE, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_FILETODV, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_ENABLED);

            EnableMenuItem(hmenu, IDM_DVTOFILE_TYPE2, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_DVTOFILE_NOPRE_TYPE2, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_ENABLED);
            EnableMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_ENABLED);

        //if we are here, all of our interfaces should be valid
        ASSERT(g_pBuilder);
        ASSERT(g_pGraphBuilder);
        ASSERT(g_pDVCamera);

        //get directshow interfaces for DV
        g_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, g_pDVCamera, IID_IAMExtTransport, reinterpret_cast<PVOID*>(&g_pExtTrans));
        g_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, g_pDVCamera, IID_IAMTimecodeReader, reinterpret_cast<PVOID*>(&g_pTimeReader));

        //determine if we are in camera mode or vcr mode and disable unavailable menu items
        //  Use GetDVMode to *not* use AVC commands

        switch (DV_GetDVMode())
        {
            case CameraMode :          
                EnableMenuItem(hmenu, IDM_FILETODV, MF_GRAYED);
                EnableMenuItem(hmenu, IDM_FILETODV_NOPRE, MF_GRAYED);
                EnableMenuItem(hmenu, IDM_FILETODV_TYPE2, MF_GRAYED);
                EnableMenuItem(hmenu, IDM_FILETODV_NOPRE_TYPE2, MF_GRAYED);
                EnableMenuItem(hmenu, IDM_CHECKTAPE, MF_GRAYED);
                DV_StatusText(TEXT("Camera Mode"), 2);
                break;                

            case VcrMode :
               //check information about the tape
                if (S_OK != DV_GetTapeInfo())
                {
                    MBOX(TEXT("Tape is not inserted, or has an improper format.\nReinsert the tape and select Options - Check Tape"));
                } 
                break;

            case UnknownMode :
                MBOX(TEXT("Cannot determine camera / Vcr mode"));
                DV_StatusText(TEXT("Unknown Mode"), 2);
                break;

            default :
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Bad return value from DV_GetDVMode"));
                break;
        } 
        
        // initialize the video window
        if (!DV_InitWindow())
        {
            MBOX(TEXT("Could not initialize video window"));
            SendMessage(g_hwndApp, WM_DESTROY, 0,0);
        } 
        else
        {
            //If we've gotten this far, all of these components should be available
            //DV Splitter, DV Muxer, DV Codec is part of QDV
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_DVSplitter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pDVSplitter)));
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_DVMux, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pDVMux)));
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pDVCodec)));
            // Other filters to help in building the graphs
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_AviSplitter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pAviSplitter)));
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pSmartTee)));
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_InfTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pInfTee)));
            EXECUTE_ASSERT(S_OK == CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pVideoRenderer)));
            // The user may not have a sound card installed , so if this fails, we just won't do sound.
            CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pDSound));

            IMediaEventEx *pMe = NULL;
            if (SUCCEEDED(g_pGraphBuilder->QueryInterface(IID_IMediaEventEx, reinterpret_cast<PVOID *>(&pMe))))
            {
                pMe->SetNotifyWindow((LONG_PTR) g_hwndApp, WM_FGNOTIFY, 0);
                SAFE_RELEASE(pMe);
            } 

            SetWindowText(g_hwndApp, DV_APPTITLE);
            DV_StatusText(g_DeviceName, 0);
        } 
    }

}


/*-------------------------------------------------------------------------
Routine:        DV_InitControls
Purpose:        Initializer for app window controls (toolbars, edit controls, etc.)
Arguments:    window handle and hinstance
Returns:        FALSE if creation of any of the controls fails.
Notes:          
------------------------------------------------------------------------*/
BOOL DV_InitControls(HWND hwnd, HINSTANCE hInst)
{
    HWND hwndEdit1 = NULL;
    HWND hwndEdit2 = NULL;
    HWND hwndEdit3 = NULL;
    HWND hwndEdit4 = NULL;
    HWND hwndTCCheck = NULL;

    RECT rect = {0};
        
    InitCommonControls();
    // create status bar windows
    g_hwndStatus = CreateWindowEx( 0,
                                   STATUSCLASSNAME,
                                   TEXT(""),
                                   WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS,
                                   -100, -100,
                                   10, 10,
                                   hwnd,
                                   HMENU(IDB_STATUS),
                                   hInst,
                                   NULL);
    SendMessage(g_hwndStatus, SB_GETRECT, 0, (LPARAM)(LPRECT)&rect);          
    g_statusHeight = rect.bottom;
    //Set the initial size for the status bar parts.  The WM_SIZE handler will take care of it from here
    DV_StatusParts(g_iAppWidth);

    // create toolbar window
    g_hwndTBar = CreateToolbarEx(hwnd, 
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | TBSTYLE_TOOLTIPS, 
                                   ID_TOOLBAR, 
                                   10, 
                                   hInst, 
                                   IDB_TOOLBAR, 
                                   g_rgTbButtons, 
                                   sizeof(g_rgTbButtons) / sizeof(TBBUTTON), 
                                   16,16,16,16, 
                                   sizeof(TBBUTTON)); 

    SendMessage(g_hwndTBar, TB_GETRECT, IDM_SEEKTIMECODE, (LPARAM)&rect);

    if (!rect.right)
    {
        DV_LogOut(LOG_PRIORITY_WARN, LOG_LEVEL_MEDIUM, TEXT("Could not get rect of Seek Time code button"));
        rect.right = 350;
    } 
    // create timecode text boxes on the toolbar            
    hwndEdit1 = CreateWindow(TEXT("edit"), TEXT("00"), WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 
                    rect.right + 6, 4, 22, 18, g_hwndTBar, (HMENU)IDC_EDIT_HOUR, (HINSTANCE) hInst, NULL);
    
    hwndEdit2 = CreateWindow(TEXT("edit"), TEXT("00"), WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | ES_NUMBER,  
                    rect.right + 30, 4, 22, 18, g_hwndTBar, (HMENU)IDC_EDIT_MINUTE, (HINSTANCE) hInst, NULL);
    
    hwndEdit3 = CreateWindow(TEXT("edit"), TEXT("00"), WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 
                    rect.right + 54, 4, 22, 18, g_hwndTBar, (HMENU)IDC_EDIT_SECOND, (HINSTANCE) hInst, NULL);

    hwndEdit4 = CreateWindow(TEXT("edit"), TEXT("00"), WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 
                    rect.right + 78, 4, 22, 18, g_hwndTBar, (HMENU)IDC_EDIT_FRAME, (HINSTANCE) hInst, NULL);

    hwndTCCheck = CreateWindow(TEXT("button"), TEXT("Display Timecodes"), WS_CHILD | BS_AUTOCHECKBOX | WS_VISIBLE | WS_TABSTOP, 
                    rect.right + 106, 5, 150, 18, g_hwndTBar, (HMENU)IDC_TCCHECKBOX, (HINSTANCE) hInst, NULL); 
    Button_SetCheck (hwndTCCheck, BST_CHECKED) ;                    

    //finally, resize the main window so it maintains the proper height despite the toolbar and status bar
    MoveWindow(g_hwndApp, CW_USEDEFAULT, CW_USEDEFAULT, 
               g_iAppWidth, g_iAppHeight + (rect.bottom - rect.top) + 
               (g_statusHeight + (2 * GetSystemMetrics(SM_CYBORDER))), TRUE);
    
    return (!( !hwndEdit1) || (!hwndEdit2) || (!hwndEdit3) || (!hwndEdit4) || (!hwndTCCheck) || (!g_hwndStatus));

} 


/*-------------------------------------------------------------------------
Routine:        DV_InitDevice
Purpose:        Enumerates all video devices, and determines if it is a DV device.  
                    We'll do this by querying for an interface specific to DV - in this
                    case, IAMExtDevice.
Arguments:    None
Returns:        TRUE if a DV device is found
Notes:          Will this solution work all the time. No, but then all other solutions are equally unreliable 
                    like comparing friendlyname or signal mode or device type on the transport. 
                    May be the mediatype of the Source filter should solve the problem, but then maybe not.
------------------------------------------------------------------------*/
BOOL DV_InitDevice(void)
{
    BOOL bStatus = FALSE;
    HRESULT hr;
    UINT uIndex = 0;
    ICreateDevEnum *pCreateDevEnum = NULL;
    // Before we can QI for IAMExtDevice, we need to build a graph and add our filter.
    
    // Create Device Enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<PVOID *>(&pCreateDevEnum));
    if (SUCCEEDED(hr) )
    {
        // get the enumerator for videoinput devices
        IEnumMoniker *pEm;
        hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
        if (SUCCEEDED(hr) && pEm)
        {
            pCreateDevEnum->Release();
            {
                pEm->Reset();
                ULONG cFetched;
                IMoniker *pM;
                //loop while we can still enumerate, the enumeration succeeds, and while we have'nt yet found a dv camera
                while ( S_OK == (hr = pEm->Next(1, &pM, &cFetched)) && FALSE == bStatus )
                {
                    // getting the propert page to get the device name
                    IPropertyBag *pBag;
                    hr = pM->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<PVOID *>(&pBag));
                    if ( SUCCEEDED(hr) )
                    {
                        VARIANT var;
                        var.vt = VT_BSTR;
                        hr = pBag->Read(L"FriendlyName", &var, NULL);
                        if ( SUCCEEDED(hr) )
                        {
    //here, we'll save the device name
#ifndef UNICODE
                            WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, g_DeviceName, DEVICENAME_BUFSIZE,  NULL, NULL);
#else
                            lstrcpyW(g_DeviceName, var.bstrVal);
#endif //UNICODE                        
                            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Device Found:\t%s"), g_DeviceName);
                            SysFreeString(var.bstrVal);
                            // Get the filter of video input device
                            hr = pM->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<PVOID *>(&g_pDVCamera));
                            if ( g_pDVCamera == NULL )
                            {
                                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Error %x: Cannot create video capture filter"), hr);
                                return FALSE;
                            }
                            else
                            {   // get the graph builder & capture graph builder to QI
                                if (DV_GetGraphBuilder())
                                {
                                    hr = CoCreateInstance((REFCLSID)CLSID_CaptureGraphBuilder2,
                                                NULL, CLSCTX_INPROC, (REFIID)IID_ICaptureGraphBuilder2,
                                                reinterpret_cast<PVOID *>(&g_pBuilder));

                                    if (SUCCEEDED(hr))
                                    {
                                        hr = g_pBuilder->SetFiltergraph(g_pGraphBuilder);
                                        if (SUCCEEDED(hr))
                                        {
                                            //add the DV cam to the graph
                                            hr = g_pGraphBuilder->AddFilter(g_pDVCamera, var.bstrVal);
                                            if (SUCCEEDED(hr))
                                            {   // QI for IAMExtDevice
                                                hr = g_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
                                                                  &MEDIATYPE_Video,  
                                                                  g_pDVCamera,IID_IAMExtDevice, 
                                                                  reinterpret_cast<PVOID*>(&g_pExtDev));
                                                if (SUCCEEDED(hr))
                                                {
                                                    //we have a DV camera
                                                    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("We Have a DV camera"));
                                                    bStatus = TRUE;
                                                } 
                                                else
                                                {
                                                    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("This is not a DV camera"));
                                                    //release our interfaces
                                                    SAFE_RELEASE(g_pGraphBuilder);
                                                    SAFE_RELEASE(g_pBuilder);
                                                    SAFE_RELEASE(g_pDVCamera);
                                                }                                                               
                                            } 
                                            else
                                            {
                                                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("AddFilter failed hr. = 0x%x"), hr);
                                            }
                                        } 
                                        else
                                        {
                                            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("SetFilterGraph failed. hr = 0x%x"), hr);
                                        }
                                    }
                                    else
                                    {
                                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("CoCreateInstance failed (ICaptureGraphbuilder2). hr = 0x%x"), hr);
                                    } 
                                }
                                else
                                {
                                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("DV_GetGraphBuilder failed"));
                                } 
                                
                            } //end g_pDVCamera == NULL
                            
                        } //end SUCCEEDED(hr)
                        pBag->Release();
                    }
                    pM->Release();
                    uIndex++;
                }
                pEm->Release();
            }
        }
        else
        {
            // no interesting devices
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("CreateClassEnumerator failed or Enumerator was not initialized. hr = 0x%c"), hr);
            MBOX(TEXT("There are no video devices installed"));
        } 
    }
    return bStatus;
} 


/*-------------------------------------------------------------------------
Routine:        DV_GetGraphBuilder
Purpose:        wrapper to get IGraphBuilder interface pointer
Arguments:    None
Returns:        TRUE if successful
Notes:          
------------------------------------------------------------------------*/
BOOL DV_GetGraphBuilder(void)
{
    //if we have one, release it, and make a new one
    if (g_pGraphBuilder)
    {
        g_pGraphBuilder->Release();
        g_pGraphBuilder = NULL;
    } 
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, reinterpret_cast<PVOID *>(&g_pGraphBuilder));
    return (SUCCEEDED(hr)) ? TRUE : FALSE;
}

/*-------------------------------------------------------------------------
Routine:        DV_InitWindow
Purpose:       Window initialization routine.  This does the following:
                    - Renders the VideoStream
                    - makes the app window the output window
                    - Plays the graph (preview)
Arguments:    None
Returns:        TRUE if Everything works - FALSE otherwise
Notes:          
------------------------------------------------------------------------*/
BOOL DV_InitWindow(void)
{
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Entering DV_InitWindow"));
    BOOL        bStatus = FALSE;
    HRESULT     hr;
    // QI for IAMStreamConfig to make sure
    hr = g_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, g_pDVCamera, IID_IAMStreamConfig, reinterpret_cast<PVOID *>(&g_pStreamConf));
    if (SUCCEEDED(hr))
    {
        AM_MEDIA_TYPE *pmt;
        hr = g_pStreamConf->GetFormat(&pmt); 
        if (SUCCEEDED(hr))
        {
            DeleteMediaType(pmt);
            // build the default preview graph
            bStatus = DV_MakeSpecialGraph(GRAPH_PREVIEW);            
            if (bStatus)
            {
                // update globals, log, toolbar
                g_iCurrentGraph = GRAPH_PREVIEW;
                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Built Preview Graph"));

                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_RECORD, MAKELONG(TBSTATE_INDETERMINATE, 0L));
                //re-enable everything else
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_REW, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_REV, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_PLAY_FAST_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_FF, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_STEP_FWD, MAKELONG(TBSTATE_ENABLED, 0L));
                SendMessage(g_hwndTBar, TB_SETSTATE, IDM_SEEKTIMECODE, MAKELONG(TBSTATE_ENABLED, 0L));              
            } 
        }
        else
        {
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("IAMStreamConfig::GetFormat Failed. hr = 0x%x"), hr);
        } 
    }
    else    
    {
        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not get pointer to IAMStreamConfig. hr = 0x%x"), hr);
    }

    return bStatus;      
    
} 

/*-------------------------------------------------------------------------
Routine:        DV_MakeSpecialGraph
Purpose:        Makes any one of the special graphs needed for reading and writing to/from the DVCR
Arguments:    None
Returns:        BOOL as appropriate
Notes:          
------------------------------------------------------------------------*/
BOOL DV_MakeSpecialGraph(GRAPH_TYPE iGraphType)
{
    BOOL bStatus = FALSE;
    HRESULT hr;

    //track whether the base or source filter is a file or the camera
    static BOOL bBaseFilterIsFile = FALSE;  

    // dont rebuild if the required graph already exists
    if(iGraphType == g_iCurrentGraph && g_pVideoRenderer != NULL)
    {
        bStatus = TRUE;
        return bStatus;
    }

    //stop the graph - this should never fail before rebuilding
    EXECUTE_ASSERT(SUCCEEDED(DV_StopGraph()));

    //let's look at the current graph
    //Disconnect everything
    if (bBaseFilterIsFile)
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("disonnect downstream from the Filesource"));
        //DisconnectAll removes only the downstream filters - we need to remove the file filter by hand.
        DV_DisconnectAll(g_pInputFileFilter);
        if (g_pInputFileFilter)
        {
            hr = g_pGraphBuilder->RemoveFilter(g_pInputFileFilter);
            SAFE_RELEASE(g_pInputFileFilter);        
        }
    } 
    else
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("disconnect downstream from the camera"));
        //DisconnectAll removes only the downstream filters - we need to remove the file filter by hand.
        DV_DisconnectAll(g_pDVCamera);
    }
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("Dumping Graph.  Pins should be disconnected"));

    switch (iGraphType)
    {
        // preview
        case GRAPH_PREVIEW :
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building Preview Graph"));
            // Build the preview graph by rendering via ICaptureGraphBuilder
            hr = g_pBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, g_pDVCamera,  NULL, NULL);
            if (SUCCEEDED(hr))
            {
                hr = DV_SetPreview();
                if (SUCCEEDED(hr))
                {
                    bStatus = TRUE;
                    bBaseFilterIsFile = FALSE;
                }
            } 
            else
            {
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("RenderStream Failed. hr = 0x%x"), hr);
            }               
            break;
        //
        // Each of these type 1 & type 2 capture, transmit graphs have their graph building functions
        //
        // Type 1 File (capture & preview)
        case GRAPH_DV_TO_FILE :
        {
            if (SUCCEEDED(DV_MakeDvToFileGraph()))
            {
                bStatus = TRUE;
                bBaseFilterIsFile = FALSE;
            } 
            break;
        }

        // Type 1 File (capture)
        case GRAPH_DV_TO_FILE_NOPRE :
        {
            if (SUCCEEDED(DV_MakeDvToFileGraph_NoPre()))
            {
                bStatus = TRUE;
                bBaseFilterIsFile = FALSE;
            } 
            break;
        }

        // Type 1 File (transmit & playback)
        case GRAPH_FILE_TO_DV :
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building Type 1 File to DV with Preview Graph"));
            if (SUCCEEDED(DV_MakeFileToDvGraph()))
            {
                bBaseFilterIsFile = TRUE;  
                bStatus = TRUE;
            } 
            break;
        }
        
        // Type 1 File (transmit)
        case GRAPH_FILE_TO_DV_NOPRE :
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building Type 1 File to DV without Preview Graph"));
            if (SUCCEEDED(DV_MakeFileToDvGraph_NoPre()))
            {
                bBaseFilterIsFile = TRUE;  
                bStatus = TRUE;
            } 
            break;
        }
        
        // Type 2 File (capture & preview)
        case GRAPH_DV_TO_FILE_TYPE2 :
        {
            if (SUCCEEDED(DV_MakeDvToFileGraph_Type2()))
            {
                bStatus = TRUE;
                bBaseFilterIsFile = FALSE;

            } 
            break;
        }

        // Type 2 File (capture)
        case GRAPH_DV_TO_FILE_NOPRE_TYPE2 :
        {
            if (SUCCEEDED(DV_MakeDvToFileGraph_NoPre_Type2()))
            {
                bStatus = TRUE;
                bBaseFilterIsFile = FALSE;
            } 
            break;
        }

        // Type 2 File (transmit & playback)
        case GRAPH_FILE_TO_DV_TYPE2 :
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building Type 2 File to DV with Preview Graph"));
            if (SUCCEEDED(DV_MakeFileToDvGraph_Type2()))
            {
                bBaseFilterIsFile = TRUE;  
                bStatus = TRUE;
            } 
            break;
        }

        // Type 2 File (transmit)
        case GRAPH_FILE_TO_DV_NOPRE_TYPE2 :
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building Type 2 File to DV without Preview Graph"));
            if (SUCCEEDED(DV_MakeFileToDvGraph_NoPre_Type2()))
            {
                bBaseFilterIsFile = TRUE;  
                bStatus = TRUE;
            } 
            break;
        }
        
        default :
            // wrong graph type
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Invalid paramater passed to DV_MakeSpecialGraph"));
            return bStatus;
    } 

    // make sure you select MSDV as the master clock and not any of the renderers
    hr = DV_SelectClock(g_pDVCamera);
    return bStatus;
} 

/*-------------------------------------------------------------------------
Routine:        DV_SelectClock
Purpose:        Selects the Clock of the Filter sets it to be the clock of the Graph
Arguments:    Filter to be selected
Returns:        HResult as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_SelectClock(IBaseFilter *pBaseFilter)
{
    HRESULT hr = S_OK;
    IReferenceClock *pRefClock      = NULL;
    IMediaFilter *pMediaFilter      = NULL;

    if(pBaseFilter == NULL)
        return E_POINTER;
    // QI for IReferenceClock on the specified filter
    hr = pBaseFilter->QueryInterface(IID_IReferenceClock, reinterpret_cast<PVOID *> (&pRefClock));
    // QI for IMediaFilter from the graph builder
    hr = g_pGraphBuilder->QueryInterface(IID_IMediaFilter, reinterpret_cast<PVOID *> (&pMediaFilter));
    if(pMediaFilter != NULL && pRefClock != NULL)
    {
        // set the clock of the specified filter on the filtergraph builder
        hr = pMediaFilter->SetSyncSource(pRefClock);
    }
    else
        hr = E_POINTER;

    SAFE_RELEASE(pMediaFilter);
    SAFE_RELEASE(pRefClock);
    return hr;
}       


/*-------------------------------------------------------------------------
Routine:        DV_StartGraph
Purpose:        Starts the Filter Graph 
Arguments:    None
Returns:        HResult as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_StartGraph(void)
{
    HRESULT hr;
    IMediaControl *pMC = NULL;

    // QI for the media control 
	if(!g_pGraphBuilder)
		return S_FALSE;
    hr = g_pGraphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<PVOID *>(&pMC));
    if (SUCCEEDED(hr))
    {
        // start the graph
        hr = pMC->Run();
        if ( FAILED(hr))
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("IMediaControl::Run failed (0x%x)"), hr);
            // stop parts that ran
            pMC->Stop();
        }
        SAFE_RELEASE(pMC);
    }
    return hr;
}

/*-------------------------------------------------------------------------
Routine:        DV_PauseGraph
Purpose:        Starts the Filter Graph 
Arguments:    None
Returns:        HResult as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_PauseGraph(void)
{
    HRESULT hr;
    IMediaControl *pMC = NULL;
    
	if(!g_pGraphBuilder)
		return S_FALSE;
    // QI for the media control 
    hr = g_pGraphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<PVOID *>(&pMC));
    if (SUCCEEDED(hr))
    {
        // Pause the graph
        hr = pMC->Pause();
        if ( FAILED(hr))
        {
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("IMediaControl::Pause failed (0x%x)"), hr);
            // stop parts that ran
            pMC->Stop();
        }
        SAFE_RELEASE(pMC);
    }
    return hr;
}


/*-------------------------------------------------------------------------
Routine:        DV_StopGraph
Purpose:        Starts the Filter Graph 
Arguments:    None
Returns:        HResult as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_StopGraph(void)
{
    HRESULT hr;
    IMediaControl *pMC = NULL;

	if(!g_pGraphBuilder)
		return S_FALSE;
    //stop the current graph
    hr = g_pGraphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<PVOID *>(&pMC));
    if (SUCCEEDED(hr))
    {
        hr = pMC->Stop();
        SAFE_RELEASE(pMC);
    }
    return hr;
}

/*-------------------------------------------------------------------------
Routine:        DV_DisconnectAll
Purpose:        Disconnects all pins in a graph (downstream from pBF)   
Arguments:    pointer to the filter to disconnect pins from (downstream only)
Returns:        None
Notes:          This is the recursive nuke downstream routine from amcap
------------------------------------------------------------------------*/
void DV_DisconnectAll(IBaseFilter *pBF)
{
    IPin *pP, *pTo;
    ULONG u;
    IEnumPins *pins = NULL;
    PIN_INFO pininfo;

    ASSERT(g_pGraphBuilder);
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("Disconnecting Pins..."));
   
    HRESULT hr = pBF->EnumPins(&pins);
    pins->Reset();
    while ( hr == NOERROR )
    {
        hr = pins->Next(1, &pP, &u);
        if ( hr == S_OK && pP )
        {
            pP->ConnectedTo(&pTo);
            if ( pTo )
            {
                hr = pTo->QueryPinInfo(&pininfo);
                if ( hr == NOERROR )
                {
                    if ( pininfo.dir == PINDIR_INPUT )
                    {
                        DV_DisconnectAll(pininfo.pFilter);
                        hr = g_pGraphBuilder->Disconnect(pTo);
                        hr = g_pGraphBuilder->Disconnect(pP);

                        //always leave the Camera filter in the graph
                        //if the base filter is a file,the camera is downstream
                        if (pininfo.pFilter != g_pDVCamera)
                        {
                            hr = g_pGraphBuilder->RemoveFilter(pininfo.pFilter);
                        }
                    }
                    ASSERT(pininfo.pFilter);
                    SAFE_RELEASE(pininfo.pFilter);
                }
                ASSERT(pTo);
                SAFE_RELEASE(pTo);
            }
            ASSERT(pP);
            SAFE_RELEASE(pP);
        }
    }
    if ( pins )
        SAFE_RELEASE(pins);

} 

/*-------------------------------------------------------------------------
Routine:        DV_CleanUp
Purpose:        writes values to the registry, releases all used interfaces. Uninitializes com and the dshow debug stuff
Arguments:    None
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void DV_CleanUp(void)
{

    DV_WriteRegKeys();

    // release helper dshow interfaces
    SAFE_RELEASE(g_pBuilder);
    SAFE_RELEASE(g_pExtDev);
    SAFE_RELEASE(g_pStreamConf);
    SAFE_RELEASE(g_pExtTrans);
    SAFE_RELEASE(g_pTimeReader);
    SAFE_RELEASE(g_pDroppedFrames);
    SAFE_RELEASE(g_pGraphBuilder);

    // release the filters of the graph    
    SAFE_RELEASE(g_pDVCamera);    
    SAFE_RELEASE(g_pVideoWindow);
    SAFE_RELEASE(g_pInputFileFilter);
    SAFE_RELEASE(g_pInfTee);
    SAFE_RELEASE(g_pVideoRenderer);
    SAFE_RELEASE(g_pAviSplitter);
    SAFE_RELEASE(g_pDVSplitter); 
    SAFE_RELEASE(g_pDVCodec);
    SAFE_RELEASE(g_pDvDec);
    SAFE_RELEASE(g_pDVMux);
    SAFE_RELEASE(g_pSmartTee);     
    SAFE_RELEASE(g_pDSound);

    CoUninitialize();
} 


/*-------------------------------------------------------------------------
Routine:        DV_SetPreview
Purpose:        Hooks up stream  *from camera* to preview window
                    Note that the preview for the playback from the file is handled within DV_MakeFileToDvGraph() stuff
Arguments:    None
Returns:        HRESULT as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_SetPreview(void)
{
    HRESULT hr = S_OK;

    if (g_pVideoWindow)
    {   // dismantle the current video window
        g_pVideoWindow->put_Owner(NULL);
        SAFE_RELEASE(g_pVideoWindow);
    } 

    // get the video window and set it up for the preview
    hr = g_pBuilder->FindInterface(NULL, &MEDIATYPE_Video, g_pDVCamera, IID_IVideoWindow, reinterpret_cast<PVOID *>(&g_pVideoWindow));
    if (SUCCEEDED(hr))
    {        
        RECT rc, rect;
        g_pVideoWindow->put_Owner((LONG_PTR)g_hwndApp);     // We own the window now
        g_pVideoWindow->put_WindowStyle(WS_CHILD);     // you are now a child

        // give the preview window all our space but where the tool bar and status bar are        
        GetClientRect(g_hwndApp, &rc);
        GetWindowRect(g_hwndTBar, &rect);

        //g_pVideoWindow->SetWindowPosition(0, rect.bottom - rect.top, rc.right, 
        //      (rc.bottom - (g_statusHeight + GetSystemMetrics(SM_CYBORDER)) - (rect.bottom - rect.top)));

        g_pVideoWindow->SetWindowPosition(0, rect.bottom - rect.top, g_iVWWidth, g_iVWHeight);
        g_pVideoWindow->put_Visible(OATRUE);
    }
    else
    {
        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("FindInterface (IVideoWindow) Failed. hr = 0x%x"), hr);
    }

    return hr;
} 

/*-------------------------------------------------------------------------
Routine:        DV_GetDVMode
Purpose:        Determines camera mode using IAMExtDevice::GetCapability()
Arguments:    None
Returns:        Current mode of camera device
Notes:          
------------------------------------------------------------------------*/
DV_MODE DV_GetDVMode(void)
{
    HRESULT hr = S_OK;
    LONG    lDeviceType = 0;

    ASSERT(g_pExtDev);
    //  Query the Device Type Capability
    hr = g_pExtDev->GetCapability(ED_DEVCAP_DEVICE_TYPE, &lDeviceType, 0);
    if (SUCCEEDED(hr))
    {
        switch (lDeviceType)
        {
            case 0 :
                //device type is unknown
                g_CurrentMode = UnknownMode;
                break;

            case ED_DEVTYPE_VCR :
                g_CurrentMode = VcrMode;
                break;

            case ED_DEVTYPE_CAMERA :
                g_CurrentMode = CameraMode;
                break;

            default :
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("GetCapability returned an unknown device type - 0x%x"), lDeviceType);
                break;
        } 
    }
    return g_CurrentMode;
} 

/*-------------------------------------------------------------------------
Routine:        DV_TimecodeTimerProc
Purpose:        Callback function for the timer proc
Arguments:    Usual Timer Processing Parameters
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void CALLBACK DV_TimecodeTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    DV_DisplayTimecode();
} 

/*-------------------------------------------------------------------------
Routine:        DV_StopRecProc
Purpose:        Callback to stop recording after a specified time
Arguments:    Usual Timer Processing Parameters
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void CALLBACK DV_StopRecProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    SendMessage(g_hwndApp, WM_COMMAND, IDM_STOP, 0);
} 


/*-------------------------------------------------------------------------
Routine:        DV_GetFinalDroppedFramesStats
Purpose:        Get Final dropped frames stats for status bar display
Arguments:    DWORD current time
Returns:        None
Notes:          For both Capture & Transmit graphs
------------------------------------------------------------------------*/
void DV_GetFinalDroppedFramesStats(DWORD dwTime)
{
    BOOL bIsModeTransmit = TRUE;
    // determine if the current graph is a transmit graph
    if (GRAPH_DV_TO_FILE_NOPRE == g_iCurrentGraph || GRAPH_DV_TO_FILE == g_iCurrentGraph ||
                GRAPH_DV_TO_FILE_TYPE2 == g_iCurrentGraph || GRAPH_DV_TO_FILE_NOPRE_TYPE2 == g_iCurrentGraph)
        bIsModeTransmit = FALSE;

    // make sure the interface exists ans can be queried the stats
    if (g_pDroppedFrames)
    {
        HRESULT hr = S_OK;
        long dropped = 0, notdropped = 0, lAvgFrameSize = 0;
        TCHAR buffer[128];
        DWORD time = dwTime - g_CapStartTime;

        // Get the required stats
        hr = g_pDroppedFrames->GetNumDropped(&dropped);
        hr = g_pDroppedFrames->GetNumNotDropped(&notdropped);
        hr = g_pDroppedFrames->GetAverageFrameSize(&lAvgFrameSize);
        
        if (SUCCEEDED(hr) && time != 0)
        {   
            double  framerate = 0;      // acheived frame rate
            LONG    lData     = 0;      // acheived data rate
            // Calculate other stats
            framerate = (double)(LONGLONG)notdropped * 1000. / (double)(LONGLONG)time;
            lData = (LONG)(LONGLONG)(notdropped / (double)(LONGLONG)time * 1000. * (double)(LONGLONG)lAvgFrameSize);
            if(bIsModeTransmit)
                wsprintf(buffer, TEXT("Transmitted %d frames (%d dropped) %d.%d sec. %d.%d fps %d.%d Meg/sec"), 
                             notdropped, dropped, time / 1000, time / 100 - time / 1000 * 10,
                             (int)framerate, (int)(framerate * 10.) - (int)framerate * 10, 
                             lData / 1000000, lData / 1000 - (lData / 1000000 * 1000));
            else
                wsprintf(buffer, TEXT("Captured %d frames (%d dropped) %d.%d sec. %d.%d fps %d.%d Meg/sec"), 
                             notdropped, dropped, time / 1000, time / 100 - time / 1000 * 10,
                             (int)framerate, (int)(framerate * 10.) - (int)framerate * 10, 
                             lData / 1000000, lData / 1000 - (lData / 1000000 * 1000));
            // update the status bar with the gathered info        
            DV_StatusText(buffer, 1);
        }
        else
   	    {
       	    DV_StatusText(TEXT("Cannot report dropped frame information"), 1);
        } 
    }
}    

/*-------------------------------------------------------------------------
Routine:        DV_DroppedFrameProc
Purpose:        Callback proc to display dropped frame info
Arguments:    DWORD current time
Returns:        None
Notes:          For both Capture & Transmit graphs
------------------------------------------------------------------------*/
void CALLBACK DV_DroppedFrameProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    HRESULT hr = S_OK;
    long dropped = 0, notdropped = 0;
    TCHAR buffer[128];
    DWORD time = dwTime - g_CapStartTime;
    BOOL bIsModeTransmit = TRUE;

    // if the dropped frames interface does not already exist, query now!
    if (NULL == g_pDroppedFrames) {
        if (GRAPH_DV_TO_FILE == g_iCurrentGraph || GRAPH_DV_TO_FILE_NOPRE == g_iCurrentGraph ||
            GRAPH_DV_TO_FILE_TYPE2 == g_iCurrentGraph || GRAPH_DV_TO_FILE_NOPRE_TYPE2 == g_iCurrentGraph)
        {
            hr = g_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, g_pDVCamera, IID_IAMDroppedFrames, reinterpret_cast<PVOID *>(&g_pDroppedFrames));
            bIsModeTransmit = FALSE;
        }
        else if (GRAPH_FILE_TO_DV == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE == g_iCurrentGraph ||
            GRAPH_FILE_TO_DV_TYPE2 == g_iCurrentGraph || GRAPH_FILE_TO_DV_NOPRE_TYPE2 == g_iCurrentGraph)
        {
            IPin *pAVIn = NULL;
             hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_INPUT, NULL, NULL, FALSE, 0, &pAVIn);
            if(SUCCEEDED(hr) && pAVIn != NULL)
                hr = pAVIn->QueryInterface(IID_IAMDroppedFrames, reinterpret_cast<PVOID *>(&g_pDroppedFrames));
            else 
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not get Input Pin for obtaining Dropped Frames Statistics"));
        }
        else
        {
            //we shouldn't get here as we dont call for the dropped frames in preview graph
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Undefined graph mode (maybe GRAPH_PREVIEW) while obtaining Dropped Frames"));
        } 
    }

    if (SUCCEEDED(hr))
    {
    	// Get the stats once the interface is successfully obtained
	    hr = g_pDroppedFrames->GetNumDropped(&dropped);
    	hr = g_pDroppedFrames->GetNumNotDropped(&notdropped);
        if (SUCCEEDED(hr))
	    {          
    		if(bIsModeTransmit)
        		wsprintf(buffer, TEXT("Transmitted %d frames (%d dropped) %d.%d sec."), notdropped, dropped, time / 1000, time / 100 - time / 1000 * 10);
            else
               	wsprintf(buffer, TEXT("Captured %d frames (%d dropped) %d.%d sec."), notdropped, dropped, time / 1000, time / 100 - time / 1000 * 10);
	        // update the status bar
    	    DV_StatusText(buffer, 1);
        }
	    else
    	{
        	DV_StatusText(TEXT("Cannot report dropped frame information"), 1);
		} 
    } 
	else
    {
       	DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not get IAMDroppedFrames interface"));
		KillTimer(hwnd, DV_TIMER_FRAMES);
    } 

} 



/*-------------------------------------------------------------------------
Routine:        DV_DisplayTimecode
Purpose:        Routine to display timecodes
Arguments:    None
Returns:        None
Notes:          This is TimeCode Read not Absolute Track Number Read
------------------------------------------------------------------------*/
void DV_DisplayTimecode(void)
{
    TIMECODE_SAMPLE TimecodeSample;
    TimecodeSample.timecode.dwFrames = 0;
    static DWORD i1 = 0, i2 = 0, i3 = 0;

    TimecodeSample.dwFlags = ED_DEVCAP_TIMECODE_READ;
    // Query the TimeCode sample data
    HRESULT hr = g_pTimeReader->GetTimecode(&TimecodeSample);
    TCHAR szBuf[4];

    if (SUCCEEDED(hr))
    {
        // It's worth it to spend a few extra cycles, and to store the static variables
        // to avoid calling wsprintf and setwindowtext on every timer tick.
        if (i1 != (TimecodeSample.timecode.dwFrames & 0xff000000) >> 24)
        {
            wsprintf(szBuf, TEXT("%.2x"),((TimecodeSample.timecode.dwFrames & 0xff000000) >> 24));
            SetDlgItemText(g_hwndTBar, IDC_EDIT_HOUR, szBuf);
        } 
        i1 = (TimecodeSample.timecode.dwFrames & 0xff000000) >> 24;

        if (i2 != (TimecodeSample.timecode.dwFrames & 0x00ff0000) >> 16)
        {
            wsprintf(szBuf, TEXT("%.2x"),((TimecodeSample.timecode.dwFrames & 0x00ff0000) >> 16));
            SetDlgItemText(g_hwndTBar, IDC_EDIT_MINUTE, szBuf);
        } 
        i2 = (TimecodeSample.timecode.dwFrames & 0x00ff0000) >> 16;
        
        if (i3 != (TimecodeSample.timecode.dwFrames & 0x0000ff00) >>  8)
        {
            wsprintf(szBuf, TEXT("%.2x"),((TimecodeSample.timecode.dwFrames & 0x0000ff00) >>  8));
            SetDlgItemText(g_hwndTBar, IDC_EDIT_SECOND, szBuf);
        } 
        i3 = (TimecodeSample.timecode.dwFrames & 0x0000ff00) >>  8;

        //always update the toolbar display
        wsprintf(szBuf, TEXT("%.2x"),(TimecodeSample.timecode.dwFrames & 0x000000ff));
        SetDlgItemText(g_hwndTBar, IDC_EDIT_FRAME, szBuf);
    }
    else
    {
        DV_LogOut(LOG_PRIORITY_WARN, LOG_LEVEL_MEDIUM, TEXT("Failed to get Timecodesample.  hr = 0x%x"), hr);
    } 
} 


/*-------------------------------------------------------------------------
Routine:        DV_SeekATN
Purpose:        ATN Seek function - uses GetTransportBasicParameters to send RAW AVC command 
Arguments:    None
Returns:        TRUE if successful
Notes:          This is Absolute Track Number Seek not TimeCode Seek but uses the timecode display as input
------------------------------------------------------------------------*/
BOOL DV_SeekATN(void)
{
    BOOL bStatus = FALSE;
    HRESULT hr = S_OK;
    int iHr, iMn, iSc, iFr;
    ULONG ulTrackNumToSearch;
    long iCnt = 8;
    // ATN Seek Raw AVC Command 
    BYTE RawAVCPkt[8] = {0x00, 0x20, 0x52, 0x20, 0xff, 0xff, 0xff, 0xff};

    //get the values from the edit fields
    iHr = GetDlgItemInt(g_hwndTBar, IDC_EDIT_HOUR,   &bStatus, FALSE);
    iMn = GetDlgItemInt(g_hwndTBar, IDC_EDIT_MINUTE, &bStatus, FALSE);
    iSc = GetDlgItemInt(g_hwndTBar, IDC_EDIT_SECOND, &bStatus, FALSE);
    iFr = GetDlgItemInt(g_hwndTBar, IDC_EDIT_FRAME,  &bStatus, FALSE);

    if ((iHr < 60) && (iMn < 60) && (iSc < 60) && (iFr <=30))
    {
        //Calculate the ATN
        if (g_AvgTimePerFrame == 40) 
        {
            ulTrackNumToSearch = ((iMn * 60 + iSc) * 25 + iFr) * 12 * 2;
        } 
        else 
        {
            // Drop two frame every minutes
            ulTrackNumToSearch = ((iMn * 60 + iSc) * 30 + iFr - ((iMn - (iMn / 10)) * 2)) * 10 * 2;
        }
        // Update the Raw AVC Command query
        RawAVCPkt[4] = (BYTE)  (ulTrackNumToSearch & 0x000000ff);
        RawAVCPkt[5] = (BYTE) ((ulTrackNumToSearch & 0x0000ff00) >> 8);
        RawAVCPkt[6] = (BYTE) ((ulTrackNumToSearch & 0x00ff0000) >> 16);
        
        // RAW AVC Call
        hr = g_pExtTrans->GetTransportBasicParameters(ED_RAW_EXT_DEV_CMD, &iCnt, (LPOLESTR *)RawAVCPkt);
        Sleep(_MAX_SLEEP*6);
        if (SUCCEEDED(hr))
        {
            bStatus = TRUE;
        }
        else
        {
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Seek to a TimeCode Location failed"));
        }
    }
    else
    {
        MBOX(TEXT("Invalid Parameter - Time entered should be:\nHour:Minute:Second:Frame"));
    } 
    return bStatus;
} 

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeDvToFileGraph
Purpose:        Builds and runs the DV to File graph
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a capture & preview graph for DV Type 1 AVI files           
                    This graph is a bit more complex.  It looks like this:
                    DV_Cam(AV Out)->SmartTee(capture)->AviMux->FileWriter
                                                SmartTee(preview)->DVSplitter(vid)->DVCodec->VideoWindow
                                                                                DVSplitter(aud)->Default DirectSound device
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeDvToFileGraph(void)
{
    HRESULT hr = S_OK;
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building DV to File Graph"));
    // making sure there is a output file selected
    if (!g_OutputFileName[0])
    {
        MBOX(TEXT("Please set up an output file name for recording"));
        hr = E_FAIL;
    } 
    else
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("This is the graph - everything should be disconnected"));

#ifndef UNICODE        
        // convert the output file name to wchar
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_OutputFileName, -1, wFileName, MAX_PATH);
#endif

        //we need a lot of pins to make this work...
        IPin *pAVOut = NULL, *pTeeIn = NULL, *pTeeOut = NULL, *pMuxIn = NULL, *pTeePreOut = NULL, *pSplitIn = NULL;
        IPin *pSplitVout = NULL, *pSplitAout = NULL, *pAudIn = NULL, *pCodecIn = NULL, *pCodecOut = NULL, *pRenderIn = NULL;

        //get the camera output pin
        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_OUTPUT, NULL, &MEDIATYPE_Interleaved, TRUE, 0, &pAVOut);
        //add the smart tee filter, get the smart tee input pin and connect it to the DV Cam AV out
        if (SUCCEEDED(hr))
        {
            g_pGraphBuilder->AddFilter(g_pSmartTee, L"Smart Tee");           
            hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pTeeIn);
            if (SUCCEEDED(hr))
            {
                hr = g_pGraphBuilder->ConnectDirect(pAVOut, pTeeIn, NULL);
                if (SUCCEEDED(hr))
                {
                    // get the capture output pin of smart tee and connect it to avi muxer & file writer
                    hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pTeeOut);
                    if (SUCCEEDED(hr))
                    {
                        //add the avimux, and file writer to the graph 
                        IBaseFilter *ppf = NULL;
                        IFileSinkFilter *pSink = NULL;
#ifndef UNICODE
                        hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wFileName, &ppf, &pSink);
#else
                        hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, g_OutputFileName, &ppf, &pSink);
#endif //UNICODE
                        if (SUCCEEDED(hr))
                        {
                            // Set the AVI Options like interleaving mode etc...
                            DV_SetAviOptions(ppf, INTERLEAVE_NONE);
                            //Now I need the input pin on the mux to connect to the file writer
                            hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxIn);
                            if (SUCCEEDED(hr))
                            {
                                hr = g_pGraphBuilder->ConnectDirect(pTeeOut, pMuxIn, NULL);
                                //
                                //the capture graph is built,but we still need to build the preview graph
                                //
                                // get the preview output pin on the smart tee
                                hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pTeePreOut);
                                 if (SUCCEEDED(hr))
                                {
                                    // Add the filters DV Splitter, DV Decoder, Video Renderer 
                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVSplitter, L"DV Splitter"));
                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVCodec, L"DV Codec"));
                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pVideoRenderer, L"VideoRenderer"));

                                    // Find input pin of dvsplitter & connect smart tee and dvsplitter
                                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitIn);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = g_pGraphBuilder->ConnectDirect(pTeePreOut, pSplitIn, NULL);
                                        if (SUCCEEDED(hr))
                                        {
                                            // find the vid & aud output pins on the dvsplitter
                                            hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Video, TRUE, 0, &pSplitVout);
                                            if (SUCCEEDED(hr))
                                            {
                                                hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Audio, TRUE, 0, &pSplitAout);
                                                if (SUCCEEDED(hr))
                                                {
                                                    // find the input & output pins on the dv codec
                                                    hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pCodecIn);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_OUTPUT, NULL , NULL, TRUE, 0, &pCodecOut);
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            // connect the dvsplitter(vid) & dvcodec
                                                            hr = g_pGraphBuilder->ConnectDirect(pSplitVout, pCodecIn, NULL);
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                //if we have audio, add the filter to the graph connect it to the dvsplitter(aud)
                                                                if (g_pDSound)
                                                                {
                                                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDSound, L"DSound"));
                                                                    hr = g_pBuilder->FindPin(g_pDSound, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pAudIn);
                                                                    if (SUCCEEDED(hr))
                                                                    {
                                                                        hr = g_pGraphBuilder->ConnectDirect(pSplitAout, pAudIn, NULL);
                                                                        SAFE_RELEASE(pAudIn);
                                                                    }
                                                                }   
                                                                if (SUCCEEDED(hr))  //this verifies success of the audio connection when audio exists
                                                                {
                                                                    // get the input pin on the video renderer and connect it to the dvcodec
                                                                    hr = g_pBuilder->FindPin(g_pVideoRenderer, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pRenderIn);
                                                                    if (SUCCEEDED(hr))
                                                                    {
                                                                        hr = g_pGraphBuilder->ConnectDirect(pCodecOut, pRenderIn, NULL);
                                                                        if (SUCCEEDED(hr))
                                                                        {
                                                                            // Woohoo! succeeded all the way
                                                                            // Set the Filter Graph to Preview State (Run in this case)
                                                                            DV_SetPreview();
                                                                        } 
                                                                        SAFE_RELEASE(pRenderIn);
                                                                    } 
                                                                } 
                                                            } 
                                                            SAFE_RELEASE(pCodecOut);
                                                        } 
                                                        SAFE_RELEASE(pCodecIn);
                                                    } 
                                                    SAFE_RELEASE(pSplitAout);
                                                } 
                                                SAFE_RELEASE(pSplitVout);
                                            } 
                                        } 
                                        SAFE_RELEASE(pSplitIn);
                                    } 
                                    SAFE_RELEASE(pTeePreOut);
                                } 
                                SAFE_RELEASE(pMuxIn);
                            } 
                            SAFE_RELEASE(ppf);
                            SAFE_RELEASE(pSink);
                        } 
                        SAFE_RELEASE(pTeeOut);
                    } 
                } // Release interfaces appropriately
                else
                {
                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from camera (0x%x)"), hr);
                } 
                SAFE_RELEASE(pTeeIn);
            } 
            SAFE_RELEASE(pAVOut);
        } 
    }
    return hr;
}

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeDvToFileGraph_NoPre
Purpose:        Builds and runs the DV to File graph with no preview
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a capture only graph for DV Type 1 AVI files            
                    This graph is not too complex.  It looks like this:
                    DV_Cam(AV Out)->AviMux->FileWriter
---------------------------------------------------------------------------------------------------------*/
HRESULT  DV_MakeDvToFileGraph_NoPre(void)
{
    HRESULT hr = S_OK;
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building DV to File Graph (No Preview)"));
    // making sure there is a output file selected
    if (!g_OutputFileName[0])
    {
        MBOX(TEXT("Please set up an output file name for recording"));
        hr = E_FAIL;
    } 
    else
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("This is the graph - everything should be disconnected"));
#ifndef UNICODE        
        // convert the output file name to wchar
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_OutputFileName, -1, wFileName, MAX_PATH);
#endif

        //we'll need a few pins...
        IPin *pAVOut = NULL, *pMuxIn = NULL;

        // get the capture output pin of dvcamera and connect it to avi muxer & file writer
        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_OUTPUT, NULL, &MEDIATYPE_Interleaved, TRUE, 0, &pAVOut);
        if (SUCCEEDED(hr))
        {
            //add the avimux, and file writer to the graph 
            IBaseFilter *ppf = NULL;
            IFileSinkFilter *pSink = NULL;
#ifndef UNICODE
            hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wFileName, &ppf, &pSink);
#else
            hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, g_OutputFileName, &ppf, &pSink);
#endif //UNICODE
            if (SUCCEEDED(hr))
            {
                // Set the AVI Options like interleaving mode etc...
                DV_SetAviOptions(ppf, INTERLEAVE_NONE);
                //Now I need the input pin on the mux to connect to the file writer
                hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxIn);
                if (SUCCEEDED(hr))
                {
                    hr = g_pGraphBuilder->ConnectDirect(pAVOut, pMuxIn, NULL);
                    //
                    //the capture graph is built
                    //
                    if (FAILED(hr))
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from camera (0x%x)"), hr);
                    } 
                                                            
                    SAFE_RELEASE(pMuxIn);
                } 
                SAFE_RELEASE(ppf);
                SAFE_RELEASE(pSink);
            } 
            
        } 
        SAFE_RELEASE(pAVOut);
    } // Release interfaces appropriately
    return hr;
}

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeFileToDvGraph
Purpose:        Builds and runs the File to DV graph
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a transmit & playback graph for DV Type 1 AVI files         
                    This graph is a bit more complex.  It looks like this:
                     FileSource->AVI_Splitter->InfPinTee->DV_Camera
                                                               InfPinTee->DVSplitter(vid)->DVDecoder->VideoWIndow
                                                                                DVSplitter(aud)->Default DirectSound device
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeFileToDvGraph(void)
{
    HRESULT hr = S_OK;
    // making sure there is a input file selected
    if (NULL == g_InputFileName[0])
    {
        MBOX(TEXT("Please set an input file for rendering"));
        hr = E_FAIL;
    } 
    else
    {
         //if we are compiling ANSI, the file name needs to be converted to UNICODE
        // Add the file as source filter to the graph
 #ifndef UNICODE
       WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_InputFileName, -1, wFileName, MAX_PATH);
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(wFileName, wFileName, &g_pInputFileFilter));
#else
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(g_InputFileName, g_InputFileName, &g_pInputFileFilter));
#endif
        //making sure the file source exists
        if(g_pInputFileFilter == NULL)
        {
            MBOX(TEXT("Please set a correct input file for rendering"));
            hr = E_FAIL;
            return hr;
        } 
        // Also add the AVI Splitter
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pAviSplitter, L"AVI Splitter"));

        // now we need to connect the pins
        IPin *pFileOut = NULL, *pSplitterIn = NULL, *pSplitterOut = NULL, *pDvIn = NULL;
        IPin *pInfTeeIn = NULL, *pInfTeeOut1 = NULL, *pInfTeeOut2 = NULL, *pSplitIn = NULL, *pRenderIn = NULL;
        IPin *pSplitVout = NULL, *pSplitAout = NULL, *pCodecIn = NULL, *pCodecOut = NULL, *pAudIn = NULL;

        // obtain the output pin of the source filter and connect it to avi splitter
        hr = g_pBuilder->FindPin(g_pInputFileFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pFileOut);
        if (SUCCEEDED(hr))
        {
            ASSERT(pFileOut);
            //now get the input pin of the splitter
            hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitterIn);
            if (SUCCEEDED(hr))
            {
                ASSERT(pSplitterIn);
                //connect the file output pin to the AVI splitter in
                //until we do this, the output pin on the splitter won't appear
                hr = g_pGraphBuilder->ConnectDirect(pFileOut, pSplitterIn, NULL);
                if (SUCCEEDED(hr))
                {
                    //now we can connect the AVISplitterOut to the InfTee IN, then connect that to the camera
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pSplitterOut);
                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pSplitterOut);
                        //add the the infinite pin tee filter to the graph
                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pInfTee, L"Infinite Tee")); 
                        hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pInfTeeIn);
                        if (SUCCEEDED(hr))
                        {
                            ASSERT(pInfTeeIn);
                            hr = g_pGraphBuilder->ConnectDirect(pSplitterOut, pInfTeeIn, NULL);
                            if (SUCCEEDED(hr))
                            {
                                //get the inftee output pin, and the DV Camera input pin
                                hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pInfTeeOut1);
                                if (SUCCEEDED(hr))
                                {
                                    ASSERT(pInfTeeOut1);
                                    hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDvIn);
                                    if (SUCCEEDED(hr))
                                    {
                                        ASSERT(pDvIn);
                                        hr = g_pGraphBuilder->ConnectDirect(pInfTeeOut1, pDvIn, NULL);
                                        //
                                        //the capture portion of the graph is built, now lets do the preview
                                        //
                                        //get another output pin on the inftee
                                        hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pInfTeeOut2);
                                        if (SUCCEEDED(hr))
                                        {
                                            // Add the filters DV Splitter, DV Decoder, Video Renderer 
                                            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVSplitter, L"DV Splitter"));
                                            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVCodec, L"DV Codec"));
                                            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pVideoRenderer, L"VideoRenderer"));

                                            // Find input pin of dvsplitter & connect inf tee and dvsplitter
                                            hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitIn);
                                            if (SUCCEEDED(hr))
                                            {
                                                hr = g_pGraphBuilder->ConnectDirect(pInfTeeOut2, pSplitIn, NULL);
                                                if (SUCCEEDED(hr))
                                                {
                                                    // find the vid & aud output pins on the dvsplitter
                                                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Video, TRUE, 0, &pSplitVout);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Audio, TRUE, 0, &pSplitAout);
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            // find the input & output pins on the dv codec
                                                            hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pCodecIn);
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_OUTPUT, NULL , NULL, TRUE, 0, &pCodecOut);
                                                                if (SUCCEEDED(hr))
                                                                {
                                                                    // connect the dvsplitter(vid) & dvcodec
                                                                    hr = g_pGraphBuilder->ConnectDirect(pSplitVout, pCodecIn, NULL);
                                                                    if (SUCCEEDED(hr))
                                                                    {
                                                                        //if we have audio, add the filter to the graph connect it to the dvsplitter(aud)
                                                                        if (g_pDSound)
                                                                        {
                                                                            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDSound, L"DSound"));
                                                                            hr = g_pBuilder->FindPin(g_pDSound, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pAudIn);
                                                                            if (SUCCEEDED(hr))
                                                                            {
                                                                                hr = g_pGraphBuilder->ConnectDirect(pSplitAout, pAudIn, NULL);
                                                                            } 
                                                                        } 
                                                                        if (SUCCEEDED(hr)) //verifies audio connection when audio exists
                                                                        {
                                                                            // get the input pin on the video renderer and connect it to the dvcodec
                                                                            hr = g_pBuilder->FindPin(g_pVideoRenderer, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pRenderIn);
                                                                            if (SUCCEEDED(hr))
                                                                            {
                                                                                hr = g_pGraphBuilder->ConnectDirect(pCodecOut, pRenderIn, NULL);
                                                                                if (SUCCEEDED(hr))
                                                                                {
                                                                                    // Woohoo! succeeded all the way
                                                                                    // Set up the Video Window
                                                                                    if (g_pVideoWindow)
                                                                                    {
                                                                                        g_pVideoWindow->put_Owner(NULL);
                                                                                        SAFE_RELEASE(g_pVideoWindow);
                                                                                    } 
                                                                                    hr = g_pBuilder->FindInterface(NULL, &MEDIATYPE_Video, g_pInputFileFilter, IID_IVideoWindow, reinterpret_cast<PVOID *>(&g_pVideoWindow));
                                                                                    if (SUCCEEDED(hr))
                                                                                    {
                                                                                       
                                                                                        RECT rc, rect;
                                                                                        g_pVideoWindow->put_Owner((LONG_PTR)g_hwndApp);     // We own the window now
                                                                                        g_pVideoWindow->put_WindowStyle(WS_CHILD);     // you are now a child
                                                                                        GetClientRect(g_hwndApp, &rc);
                                                                                        GetWindowRect(g_hwndTBar, &rect);
                                                                                        g_pVideoWindow->SetWindowPosition(0, rect.bottom - rect.top, g_iVWWidth, g_iVWHeight);
                                                                                        g_pVideoWindow->put_Visible(OATRUE);
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("FindInterface (IVideoWindow) Failed. hr = 0x%x"), hr);
                                                                                    }
                                                                                } 
                                                                                SAFE_RELEASE(pRenderIn);
                                                                            } 
                                                                            SAFE_RELEASE(pAudIn);
                                                                        } 
                                                                    } 
                                                                    SAFE_RELEASE(pCodecOut);
                                                                } 
                                                                SAFE_RELEASE(pCodecIn);
                                                            } 
                                                            SAFE_RELEASE(pSplitAout);
                                                        } 
                                                        SAFE_RELEASE(pSplitVout);
                                                    } 
                                                } 
                                                SAFE_RELEASE(pSplitIn);
                                            } 
                                            SAFE_RELEASE(pInfTeeOut2);
                                        }   
                                        SAFE_RELEASE(pDvIn);
                                    }
                                    SAFE_RELEASE(pInfTeeOut1);
                                }
                            }
                            SAFE_RELEASE(pInfTeeIn);
                        } 
                        SAFE_RELEASE(pSplitterOut);
                    } 
                } // Release interfaces appropriately
                else
                {
                    // Handler for invalid source file format
                    if (VFW_E_INVALID_FILE_FORMAT == hr)
                    {
                        MBOX(TEXT("The File format is invalid for the input file"));
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("The File format is invalid for the input file"));
                    } 
                    else
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from file (0x%x)"), hr);
                    } 
                }
                SAFE_RELEASE(pSplitterIn);
            } 
            SAFE_RELEASE(pFileOut);
        } 
    }

    //if something failed, remove all of the filters we could have added here, 
    //because DV_DisconnectAll only removes connected filters
    if (FAILED(hr))
    {
        if (g_pAviSplitter)
            g_pGraphBuilder->RemoveFilter(g_pAviSplitter);
        if (g_pInfTee)
            g_pGraphBuilder->RemoveFilter(g_pInfTee);
        if (g_pDVSplitter)
            g_pGraphBuilder->RemoveFilter(g_pDVSplitter);
        if (g_pDVCodec)
            g_pGraphBuilder->RemoveFilter(g_pDVCodec);
        if (g_pVideoRenderer)
            g_pGraphBuilder->RemoveFilter(g_pVideoRenderer);
        if (g_pDSound)
            g_pGraphBuilder->RemoveFilter(g_pDSound);
        if (g_pInputFileFilter)
            g_pGraphBuilder->RemoveFilter(g_pInputFileFilter);
    } 
    
    return hr;
} 

  
/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeFileToDvGraph_NoPre
Purpose:        Builds and runs the File to DV graph without preview
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a transmit only graph for DV Type 1 AVI files           
                    This graph is a bit simplex.  It looks like this:
                    FileSource->AVI_Splitter->DV_Camera
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeFileToDvGraph_NoPre(void)
{
    HRESULT hr = S_OK;
    // making sure there is a input file selected
    if (NULL == g_InputFileName[0])
    {
        MBOX(TEXT("Please set an input file for rendering"));
        hr = E_FAIL;
    } 
    else
    {
        //if we are compiling ANSI, the file name needs to be converted to UNICODE
        // Add the file as source filter to the graph
#ifndef UNICODE
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_InputFileName, -1, wFileName, MAX_PATH);
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(wFileName, wFileName, &g_pInputFileFilter));
#else
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(g_InputFileName, g_InputFileName, &g_pInputFileFilter));
#endif
        //making sure the file source exists
        if(g_pInputFileFilter == NULL)
        {
            MBOX(TEXT("Please set a correct input file for rendering"));
            hr = E_FAIL;
            return hr;
        } 
        // Also add the AVI Splitter
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pAviSplitter, L"AVI Splitter"));

        // now we need to connect the pins
        IPin *pFileOut = NULL, *pSplitterIn = NULL, *pSplitterOut = NULL, *pDvIn = NULL;

        // obtain the output pin of the source filter and connect it to avi splitter
        hr = g_pBuilder->FindPin(g_pInputFileFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pFileOut);
        if (SUCCEEDED(hr))
        {
            ASSERT(pFileOut);
            //now get the input pin of the splitter
            hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitterIn);
            if (SUCCEEDED(hr))
            {
                ASSERT(pSplitterIn);
                //connect the file output pin to the AVI splitter in
                //until we do this, the output pin on the splitter won't appear
                hr = g_pGraphBuilder->ConnectDirect(pFileOut, pSplitterIn, NULL);
                if (SUCCEEDED(hr))
                {
                    //get the AVI Splitter output pin, and the DV Camera input pin and connect them
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pSplitterOut);
                    if (SUCCEEDED(hr))
                    {
                        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDvIn);
                        if (SUCCEEDED(hr))
                        {
                            ASSERT(pDvIn);
                            hr = g_pGraphBuilder->ConnectDirect(pSplitterOut, pDvIn, NULL);
                            //
                            //the capture portion of the graph is built
                            //
                            if (FAILED(hr))
                            {
                                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_MEDIUM, TEXT("Could not connect the AviSplitter to the DV Camera"));
                            } 
                            SAFE_RELEASE(pDvIn);
                        } 
                    SAFE_RELEASE(pSplitterOut);
                    }
                } // Release interfaces appropriately
                else
                {
                    // Handler for invalid source file format
                    if (VFW_E_INVALID_FILE_FORMAT == hr)
                    {
                        MBOX(TEXT("The File format is invalid for the input file"));
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("The File format is invalid for the input file"));
                    } 
                    else
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from file (0x%x)"), hr);
                    } 
                }
                SAFE_RELEASE(pSplitterIn);
            } 
            SAFE_RELEASE(pFileOut);
        } 
    }
    //if something failed, remove all of the filters we could have added here, 
    //because DisconnectAll only removes connected filters
    if (FAILED(hr))
    {
        if (g_pAviSplitter)
            g_pGraphBuilder->RemoveFilter(g_pAviSplitter);
        if (g_pInputFileFilter)
            g_pGraphBuilder->RemoveFilter(g_pInputFileFilter);
    } 
    return hr;
} 

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeDvToFileGraph_Type2
Purpose:        Builds and runs the DV to File graph
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a capture & preview graph for DV Type 2 AVI files           
                    This graph is a bit more complex.  It looks like this:
                    DV_Cam(AV Out)->DVSplitter(vid)->SmartTee(capture)->AviMux->FileWriter
                                                                          SmartTee(preview)->DVCodec->VideoWindow
                                                DVSplitter(aud)->InfinitePinTee->AviMux->FileWriter
                                                                           InfinitePinTee->Default DirectSound device
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeDvToFileGraph_Type2(void)
{
    HRESULT hr = S_OK;
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building DV to File Graph (Adobe)"));
    // making sure there is a output file selected
    if (!g_OutputFileName[0])
    {
        MBOX(TEXT("Please set up an output file name for recording"));
        hr = E_FAIL;
    } 
    else
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("This is the graph - everything should be disconnected"));
#ifndef UNICODE        
        // convert the output file name to wchar
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_OutputFileName, -1, wFileName, MAX_PATH);
#endif

        //we need a lot of pins to make this work...
        IPin *pAVOut = NULL, *pMuxAIn = NULL, *pMuxVIn = NULL;
        IPin *pDVSplitterIn = NULL, *pDVSplitterVOut = NULL, *pDVSplitterAOut = NULL;
        IPin *pTeeIn = NULL, *pTeeOut = NULL, *pTeePreOut = NULL;
        IPin *pInfTeeIn = NULL, *pInfTeeOut = NULL, *pInfTeePreOut = NULL;
        IPin *pAudIn = NULL, *pCodecIn = NULL, *pCodecOut = NULL, *pRenderIn = NULL;

        //get the camera output pin
        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_OUTPUT, NULL, &MEDIATYPE_Interleaved, TRUE, 0, &pAVOut);
        // add the dv splitter, get the input pin of dvsplitter and connect it to the dvcamera
        if (SUCCEEDED(hr))
        {
            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVSplitter, L"DV Splitter"));
            hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDVSplitterIn);
            if(SUCCEEDED(hr))
            {
                hr = g_pGraphBuilder->ConnectDirect(pAVOut, pDVSplitterIn, NULL);
                if(SUCCEEDED(hr))
                {
                    // get the vid & aud output pins of the dvsplitter
                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pDVSplitterVOut);
                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pDVSplitterAOut);

                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pDVSplitterVOut);
                        ASSERT(pDVSplitterAOut);
                        //add the smart tee, infinite tee filters to the graph
                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pSmartTee, L"Smart Tee"));
                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pInfTee, L"Infinite Tee"));

                        // get the input pin of infinite pin tee and connect it to dvsplitter (aud) output pin
                        hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pInfTeeIn);
                        if (SUCCEEDED(hr))
                        {
                            hr = g_pGraphBuilder->ConnectDirect(pDVSplitterAOut, pInfTeeIn, NULL);
                            if (SUCCEEDED(hr))
                            {
                                // get the outpin of infinite pin tee
                                hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pInfTeeOut);
                                if(SUCCEEDED(hr))
                                {          
                                    // get the input pin of smart tee and connect it to dvsplitter (vid) output pin
                                    hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pTeeIn);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = g_pGraphBuilder->ConnectDirect(pDVSplitterVOut, pTeeIn, NULL);
                                        if (SUCCEEDED(hr))
                                        {
                                            // get the capture outpin of smart tee filter and connect it to avi muxer & file writer
                                            hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pTeeOut);
                                            if(SUCCEEDED(hr))
                                            {
                                                //add the avimux, and file writer to the graph 
                                                IBaseFilter *ppf = NULL;
                                                IFileSinkFilter *pSink = NULL;
#ifndef UNICODE
                                                hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wFileName, &ppf, &pSink);
#else
                                                hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, g_OutputFileName, &ppf, &pSink);
#endif //UNICODE
                                                if (SUCCEEDED(hr))
                                                {
                                                    // Set the AVI Options like interleaving mode etc...
                                                    DV_SetAviOptions(ppf, INTERLEAVE_NONE);
                                                    //Now I need the input video pin on the mux to connect to the file writer
                                                    hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxVIn);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        hr = g_pGraphBuilder->ConnectDirect(pTeeOut, pMuxVIn, NULL);
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            //Now I need the input audio pin on the mux
                                                            hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxAIn);
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                hr = g_pGraphBuilder->ConnectDirect(pInfTeeOut, pMuxAIn, NULL);
                                                                //  
                                                                //the capture graph is built,but we still need to build the preview graph
                                                                //
                                                                // get the preview output pin on the smart tee
                                                                hr = g_pBuilder->FindPin(g_pSmartTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pTeePreOut);
                                                                if (SUCCEEDED(hr))
                                                                {
                                                                    // Add the filters DV Decoder, Video Renderer 
                                                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVCodec, L"DV Codec"));
                                                                    EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pVideoRenderer, L"VideoRenderer"));

                                                                    // get the output pin of Infinite pin tee filter
                                                                    hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL , NULL, TRUE, 0, &pInfTeePreOut);
                                                                    if (SUCCEEDED(hr))
                                                                    {
                                                                        // find the input & output pins on the dv codec
                                                                        hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pCodecIn);
                                                                        if (SUCCEEDED(hr))
                                                                        {
                                                                            hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_OUTPUT, NULL , NULL, TRUE, 0, &pCodecOut);
                                                                            if (SUCCEEDED(hr))
                                                                            {
                                                                                // connect the infinite pin tee to dvcodec 
                                                                                hr = g_pGraphBuilder->ConnectDirect(pTeePreOut, pCodecIn, NULL);
                                                                                if (SUCCEEDED(hr))
                                                                                {
                                                                                    //if we have audio, add the filter to the graph connect it to the infinite pin tee
                                                                                    if (g_pDSound)
                                                                                    {
                                                                                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDSound, L"DSound"));
                                                                                        hr = g_pBuilder->FindPin(g_pDSound, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pAudIn);
                                                                                        if (SUCCEEDED(hr))
                                                                                        {
                                                                                            hr = g_pGraphBuilder->ConnectDirect(pInfTeePreOut, pAudIn, NULL);
                                                                                            SAFE_RELEASE(pAudIn);
                                                                                        }
                                                                                    }   
                                                                                    if (SUCCEEDED(hr))  //this verifies success of the audio connection when audio exists
                                                                                    {
                                                                                        // get the input pin ont he video renderer and connect it to the dvcodec
                                                                                        hr = g_pBuilder->FindPin(g_pVideoRenderer, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pRenderIn);
                                                                                        if (SUCCEEDED(hr))
                                                                                        {
                                                                                            hr = g_pGraphBuilder->ConnectDirect(pCodecOut, pRenderIn, NULL);
                                                                                            if (SUCCEEDED(hr))
                                                                                            {
                                                                                                // Woohoo! succeeded all the way
                                                                                                // Set the Filter Graph to Preview State (Run in this case)
                                                                                                DV_SetPreview();
                                                                                            } 
                                                                                        } 
                                                                                        SAFE_RELEASE(pRenderIn);
                                                                                    } 
                                                                                } 
                                                                            } 
                                                                            SAFE_RELEASE(pCodecOut);
                                                                        } 
                                                                        SAFE_RELEASE(pCodecIn);
                                                                    } 
                                                                    SAFE_RELEASE(pInfTeePreOut);
                                                                } 
                                                                SAFE_RELEASE(pTeePreOut);
                                                            } 
                                                            SAFE_RELEASE(pMuxAIn);
                                                        } 
                                                    } 
                                                    SAFE_RELEASE(pMuxVIn);
                                                }
                                                SAFE_RELEASE(ppf);
                                                SAFE_RELEASE(pSink);
                                            }
                                            SAFE_RELEASE(pTeeOut);
                                        } 
                                    }
                                    SAFE_RELEASE(pTeeIn);
                                }
                                SAFE_RELEASE(pInfTeeOut);
                            }
                        }
                        SAFE_RELEASE(pInfTeeIn);
                    }
                    SAFE_RELEASE(pDVSplitterVOut);
                    SAFE_RELEASE(pDVSplitterAOut);
                } // Release interfaces appropriately
                else
                {
                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect output pin from camera (0x%x)"), hr);
                } 
            } 
            SAFE_RELEASE(pDVSplitterIn);
        } 
        SAFE_RELEASE(pAVOut);
    }
    return hr;
}

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeDvToFileGraph_NoPre_Type2
Purpose:        Builds and runs the DV to File graph
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a capture only graph for DV Type 2 AVI files            
                    This graph is a bit simplex .  It looks like this:
                    DV_Cam(AV Out)->DVSplitter(vid)->AviMux->FileWriter
                                                DVSplitter(aud)->AviMux->FileWriter
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeDvToFileGraph_NoPre_Type2(void)
{
    HRESULT hr = S_OK;
    DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Building DV to File Graph (No Preview)"));
    // making sure there is a output file selected
    if (!g_OutputFileName[0])
    {
        MBOX(TEXT("Please set up an output file name for recording"));
        hr = E_FAIL;
    } 
    else
    {
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_VERBOSE, TEXT("This is the graph - everything should be disconnected"));
#ifndef UNICODE        
        // convert the output file name to wchar
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_OutputFileName, -1, wFileName, MAX_PATH);
#endif

        //we'll need a few pins...
        IPin *pAVOut = NULL, *pDVSplitterIn=NULL, *pDVSplitterVOut=NULL, *pDVSplitterAOut=NULL, *pMuxVIn = NULL, *pMuxAIn = NULL;

        //get the camera output pin
        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_OUTPUT, NULL, &MEDIATYPE_Interleaved, TRUE, 0, &pAVOut);
        // add the dv splitter, get the input pin of dvsplitter and connect it to the dvcamera
        if(SUCCEEDED(hr))
        {
            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVSplitter, L"DV Splitter"));
            hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDVSplitterIn);
            if(SUCCEEDED(hr))
            {
                hr = g_pGraphBuilder->ConnectDirect(pAVOut, pDVSplitterIn, NULL);
                if(SUCCEEDED(hr))
                {
                    // get the vid & aud output pins of the dvsplitter
                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pDVSplitterVOut);
                    hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pDVSplitterAOut);
                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pDVSplitterVOut);
                        ASSERT(pDVSplitterAOut);
                        //add the avimux, and file writer to the graph 
                        IBaseFilter *ppf = NULL;
                        IFileSinkFilter *pSink = NULL;
#ifndef UNICODE
                        hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wFileName, &ppf, &pSink);
#else
                        hr = g_pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, g_OutputFileName, &ppf, &pSink);
#endif //UNICODE
                        if (SUCCEEDED(hr))
                        {
                            // Set the AVI Options like interleaving mode etc...
                            DV_SetAviOptions(ppf, INTERLEAVE_NONE);
                            //Now I need the input pin on the mux to connect to the file writer
                            hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxVIn);
                            // connect the video & audio paths between dvsplitter & avi muxer
                            if (SUCCEEDED(hr))
                            {
                                hr = g_pGraphBuilder->ConnectDirect(pDVSplitterVOut, pMuxVIn, NULL);
                                if(SUCCEEDED(hr))
                                {
                                    hr = g_pBuilder->FindPin(ppf, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxAIn);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = g_pGraphBuilder->ConnectDirect(pDVSplitterAOut, pMuxAIn, NULL);
                                        //  
                                        //the capture graph is built
                                        //
                                        if (FAILED(hr))
                                        {
                                            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from camera (0x%x)"), hr);
                                        }                                       
                                                            
                                        SAFE_RELEASE(pMuxAIn);
                                    }
                                }
                                SAFE_RELEASE(pMuxVIn);
                            }
                            SAFE_RELEASE(ppf);
                            SAFE_RELEASE(pSink);
                        } 
            
                    }
                    SAFE_RELEASE(pDVSplitterVOut);
                    SAFE_RELEASE(pDVSplitterAOut);
                }
            }
            SAFE_RELEASE(pDVSplitterIn);
        }
        SAFE_RELEASE(pAVOut);
    } // Release interfaces appropriately
    return hr;
} 

/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeFileToDvGraph_Type2
Purpose:        Builds and runs the File to DV graph 
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a transmit & playback graph for DV Type 2 AVI files         
                    This graph is a bit complex.  It looks like this:
                     FileSource->AVI_Splitter(vid) ->DVMuxer(vid)---------->InfPinTee->DV_Camera
                                        AVI_Splitter(aud)->DVMuxer(aud)                 InfPinTee->DVSplitter(vid)->DVDecoder->VideoWIndow
                                                                                                                                            DVSplitter(aud)->DSoundDevice
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeFileToDvGraph_Type2(void)
{
    HRESULT hr = S_OK;
    // making sure there is a input file selected
    if (NULL == g_InputFileName[0])
    {
        MBOX(TEXT("Please set an input file for rendering"));
        hr = E_FAIL;
    } 
    else
    {
        //if we are compiling ANSI, the file name needs to be converted to UNICODE
        // Add the file as source filter to the graph
#ifndef UNICODE
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_InputFileName, -1, wFileName, MAX_PATH);
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(wFileName, wFileName, &g_pInputFileFilter));
#else
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(g_InputFileName, g_InputFileName, &g_pInputFileFilter));
#endif
        //making sure the file source exists
        if(g_pInputFileFilter == NULL)
        {
            MBOX(TEXT("Please set a correct input file for rendering"));
            hr = E_FAIL;
            return hr;
        } 
        // Also add the AVI Splitter
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pAviSplitter, L"AVI Splitter"));

        // now we need to connect the pins
        IPin *pFileOut = NULL, *pSplitterIn = NULL, *pSplitterVOut = NULL, *pSplitterAOut = NULL, *pDvIn = NULL;
        IPin *pInfTeeIn = NULL, *pInfTeeOut1 = NULL, *pInfTeeOut2 = NULL, *pSplitIn = NULL, *pRenderIn = NULL;
        IPin *pSplitVout = NULL, *pSplitAout = NULL, *pCodecIn = NULL, *pCodecOut = NULL, *pAudIn = NULL;
        IPin *pMuxerVIn = NULL, *pMuxerAIn = NULL, *pMuxerOut = NULL;

        // obtain the output pin of the source filter and connect it to avi splitter
        hr = g_pBuilder->FindPin(g_pInputFileFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pFileOut);
        if (SUCCEEDED(hr))
        {
            ASSERT(pFileOut);
            //now get the input pin of the splitter
            hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitterIn);
            if (SUCCEEDED(hr))
            {
                ASSERT(pSplitterIn);
                //connect the file output pin to the AVI splitter in
                //until we do this, the output pin on the splitter won't appear
                hr = g_pGraphBuilder->ConnectDirect(pFileOut, pSplitterIn, NULL);
                if (SUCCEEDED(hr))
                {
                    // get the video & audio output pins of the avi splitter
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pSplitterVOut);
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pSplitterAOut);
                    if(SUCCEEDED(hr))
                    {
                        ASSERT(pSplitterVOut);
                        ASSERT(pSplitterAOut);
                        // Add the DVMuxer to the graph
                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVMux, L"DV Muxer"));
                        // get the video & audio input pins on the dvmuxer and connect it to the corressponding output pins of avi splitter
                        hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxerVIn);
                        if(SUCCEEDED(hr))
                        {
                            hr = g_pGraphBuilder->ConnectDirect(pSplitterVOut, pMuxerVIn, NULL);
                            if(SUCCEEDED(hr))
                            {
                                hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxerAIn);
                                if(SUCCEEDED(hr))
                                {
                                    hr = g_pGraphBuilder->ConnectDirect(pSplitterAOut, pMuxerAIn, NULL);
                                    // get the ouput pin of the dv muxer
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pMuxerOut);
                                        if (SUCCEEDED(hr))
                                        {
                                            //Add the infinite pin tee filter to the graph and connect it downstream to the dv muxer
                                            EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pInfTee, L"Infinite Tee")); 
                                            hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pInfTeeIn);
                                            if (SUCCEEDED(hr))
                                            {
                                                ASSERT(pInfTeeIn);
                                                hr = g_pGraphBuilder->ConnectDirect(pMuxerOut, pInfTeeIn, NULL);
                                                if (SUCCEEDED(hr))
                                                {
                                                    //get the inftee output pin, and the DV Camera input pin and connect them
                                                    hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pInfTeeOut1);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        ASSERT(pInfTeeOut1);
                                                        hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDvIn);
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            ASSERT(pDvIn);
                                                            hr = g_pGraphBuilder->ConnectDirect(pInfTeeOut1, pDvIn, NULL);
                                                            //
                                                            //the capture portion of the graph should be done now, lets do the preview segment
                                                            //
                                                            //get another output pin on the inftee
                                                            hr = g_pBuilder->FindPin(g_pInfTee, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pInfTeeOut2);
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                // Add the filters DV Splitter, DV Decoder, Video Renderer 
                                                                EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVSplitter, L"DV Splitter"));
                                                                EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVCodec, L"DV Codec"));
                                                                EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pVideoRenderer, L"VideoRenderer"));
                                           
                                                                // Find input pin of dvsplitter & connect inf tee and dvsplitter
                                                                hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitIn);
                                                                if (SUCCEEDED(hr))
                                                                {
                                                                    hr = g_pGraphBuilder->ConnectDirect(pInfTeeOut2, pSplitIn, NULL);
                                                                    if (SUCCEEDED(hr))
                                                                    {
                                                                        // find the vid & aud output pins on the dvsplitter
                                                                        hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Video, TRUE, 0, &pSplitVout);
                                                                        if (SUCCEEDED(hr))
                                                                        {
                                                                            hr = g_pBuilder->FindPin(g_pDVSplitter, PINDIR_OUTPUT, NULL , &MEDIATYPE_Audio, TRUE, 0, &pSplitAout);
                                                                            if (SUCCEEDED(hr))
                                                                            {
                                                                                // find the input & output pins on the dv codec
                                                                                hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pCodecIn);
                                                                                if (SUCCEEDED(hr))
                                                                                {
                                                                                    hr = g_pBuilder->FindPin(g_pDVCodec, PINDIR_OUTPUT, NULL , NULL, TRUE, 0, &pCodecOut);
                                                                                    if (SUCCEEDED(hr))
                                                                                    {
                                                                                        // connect the dvsplitter(vid) & dvcodec
                                                                                        hr = g_pGraphBuilder->ConnectDirect(pSplitVout, pCodecIn, NULL);
                                                                                        if (SUCCEEDED(hr))
                                                                                        {
                                                                                            //if we have audio, add the filter to the graph connect it to the dvsplitter(aud)
                                                                                            if (g_pDSound)
                                                                                            {
                                                                                                EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDSound, L"DSound"));
                                                                                                hr = g_pBuilder->FindPin(g_pDSound, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pAudIn);
                                                                                                if (SUCCEEDED(hr))
                                                                                                {
                                                                                                    hr = g_pGraphBuilder->ConnectDirect(pSplitAout, pAudIn, NULL);
                                                                                                } 
                                                                                            } 
                                                                                            if (SUCCEEDED(hr)) //verifies audio connection when audio exists
                                                                                            {
                                                                                                // get the input pin on the video renderer and connect it to the dvcodec
                                                                                                hr = g_pBuilder->FindPin(g_pVideoRenderer, PINDIR_INPUT, NULL , NULL, TRUE, 0, &pRenderIn);
                                                                                                if (SUCCEEDED(hr))
                                                                                                {
                                                                                                    hr = g_pGraphBuilder->ConnectDirect(pCodecOut, pRenderIn, NULL);
                                                                                                    if (SUCCEEDED(hr))
                                                                                                    {
                                                                                                        // Woohoo! succeeded all the way
                                                                                                        // Set up the Video Window
                                                                                                        if (g_pVideoWindow)
                                                                                                        {
                                                                                                            g_pVideoWindow->put_Owner(NULL);
                                                                                                            SAFE_RELEASE(g_pVideoWindow);
                                                                                                        } 
                                                                                                        hr = g_pBuilder->FindInterface(NULL, &MEDIATYPE_Video, g_pInputFileFilter, IID_IVideoWindow, reinterpret_cast<PVOID *>(&g_pVideoWindow));
                                                                                                        if (SUCCEEDED(hr))
                                                                                                        {
                                                                                       
                                                                                                            RECT rc, rect;
                                                                                                            g_pVideoWindow->put_Owner((LONG_PTR)g_hwndApp);     // We own the window now
                                                                                                            g_pVideoWindow->put_WindowStyle(WS_CHILD);     // you are now a child
                                                                                                            GetClientRect(g_hwndApp, &rc);
                                                                                                            GetWindowRect(g_hwndTBar, &rect);
                                                                                                            g_pVideoWindow->SetWindowPosition(0, rect.bottom - rect.top, g_iVWWidth, g_iVWHeight);
                                                                                                            g_pVideoWindow->put_Visible(OATRUE);
                                                                                                        }
                                                                                                        else
                                                                                                        {
                                                                                                            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("FindInterface (IVideoWindow) Failed. hr = 0x%x"), hr);
                                                                                                        }
                                                                                                    } 
                                                                                                    SAFE_RELEASE(pRenderIn);
                                                                                                } 
                                                                                                SAFE_RELEASE(pAudIn);
                                                                                            } 
                                                                                        } 
                                                                                        SAFE_RELEASE(pCodecOut);
                                                                                    } 
                                                                                    SAFE_RELEASE(pCodecIn);
                                                                                } 
                                                                                SAFE_RELEASE(pSplitAout);
                                                                            } 
                                                                            SAFE_RELEASE(pSplitVout);
                                                                        } 
                                                                    } 
                                                                    SAFE_RELEASE(pSplitIn);
                                                                } 
                                                                SAFE_RELEASE(pInfTeeOut2);
                                                            }   
                                                            SAFE_RELEASE(pDvIn);
                                                        }
                                                        SAFE_RELEASE(pInfTeeOut1);
                                                    }
                                                }
                                                SAFE_RELEASE(pInfTeeIn);
                                            }
                                            SAFE_RELEASE(pMuxerOut);
                                        }
                                    }
                                    SAFE_RELEASE(pMuxerAIn);
                                }
                            }
                            SAFE_RELEASE(pMuxerVIn);
                        } 
                        SAFE_RELEASE(pSplitterVOut);
                        SAFE_RELEASE(pSplitterAOut);
                    } 
                } // Release interfaces appropriately
                else
                {
                    // Handler for invalid source file format
                    if (VFW_E_INVALID_FILE_FORMAT == hr)
                    {
                        MBOX(TEXT("The File format is invalid for the input file"));
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("The File format is invalid for the input file"));
                    } 
                    else
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from file (0x%x)"), hr);
                    } 
                }
                SAFE_RELEASE(pSplitterIn);
            } 
            SAFE_RELEASE(pFileOut);
        } 
    }
    //if something failed, remove all of the filters we could have added here, 
    //because DV_DisconnectAll only removes connected filters
    if (FAILED(hr))
    {
        if (g_pAviSplitter)
            g_pGraphBuilder->RemoveFilter(g_pAviSplitter);
        if (g_pInfTee)
            g_pGraphBuilder->RemoveFilter(g_pInfTee);
        if (g_pDVSplitter)
            g_pGraphBuilder->RemoveFilter(g_pDVSplitter);
        if (g_pDVCodec)
            g_pGraphBuilder->RemoveFilter(g_pDVCodec);
        if (g_pVideoRenderer)
            g_pGraphBuilder->RemoveFilter(g_pVideoRenderer);
        if (g_pDSound)
            g_pGraphBuilder->RemoveFilter(g_pDSound);
        if (g_pInputFileFilter)
            g_pGraphBuilder->RemoveFilter(g_pInputFileFilter);
    } 
    
    return hr;
} 

  
/*---------------------------------------------------------------------------------------------------------
Routine:        DV_MakeFileToDvGraph_NoPre_Type2
Purpose:        Builds and runs the File to DV graph 
Arguments:    None
Returns:        HRESULT as apropriate
Notes:          This is a transmit only graph for DV Type 2 AVI files           
                    This graph looks like this:
                     FileSource->AVI_Splitter(vid) ->DVMuxer(vid)----->DV_Camera
                                        AVI_Splitter(aud)->DVMuxer(aud)                 
---------------------------------------------------------------------------------------------------------*/
HRESULT DV_MakeFileToDvGraph_NoPre_Type2(void)
{
    HRESULT hr = S_OK;
    // making sure there is a input file selected
    if (NULL == g_InputFileName[0])
    {
        MBOX(TEXT("Please set an input file for rendering"));
        hr = E_FAIL;
    } 
    else
    {
        //if we are compiling ANSI, the file name needs to be converted to UNICODE
        // Add the file as source filter to the graph
#ifndef UNICODE
        WCHAR wFileName[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, g_InputFileName, -1, wFileName, MAX_PATH);
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(wFileName, wFileName, &g_pInputFileFilter));
#else
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddSourceFilter(g_InputFileName, g_InputFileName, &g_pInputFileFilter));
#endif
        //making sure the file source exists
        if(g_pInputFileFilter == NULL)
        {
            MBOX(TEXT("Please set a correct input file for rendering"));
            hr = E_FAIL;
            return hr;
        } 
        // Also add the AVI Splitter
        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pAviSplitter, L"AVI Splitter"));

        // now we need to connect the pins
        IPin *pFileOut = NULL, *pSplitterIn = NULL, *pSplitterVOut = NULL, *pSplitterAOut = NULL, *pMuxerVIn=NULL, *pMuxerAIn=NULL, *pMuxerOut=NULL, *pDvIn = NULL;

        // obtain the output pin of the source filter and connect it to avi splitter
        hr = g_pBuilder->FindPin(g_pInputFileFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pFileOut);
        if (SUCCEEDED(hr))
        {
            ASSERT(pFileOut);
            //now get the input pin of the splitter
            hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pSplitterIn);
            if (SUCCEEDED(hr))
            {
                ASSERT(pSplitterIn);
                //connect the file output pin to the AVI splitter in
                //until we do this, the output pin on the splitter won't appear
                hr = g_pGraphBuilder->ConnectDirect(pFileOut, pSplitterIn, NULL);
                if (SUCCEEDED(hr))
                {
                    // get the video & audio output pins of the avi splitter
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pSplitterVOut);
                    hr = g_pBuilder->FindPin(g_pAviSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pSplitterAOut);
                    if(SUCCEEDED(hr))
                    {
                        ASSERT(pSplitterVOut);
                        ASSERT(pSplitterAOut);
                        // Add the DVMuxer to the graph
                        EXECUTE_ASSERT(S_OK == g_pGraphBuilder->AddFilter(g_pDVMux, L"DV Muxer"));
                        // get the video & audio input pins on the dvmuxer and connect it to the corressponding output pins of avi splitter
                        hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxerVIn);
                        if(SUCCEEDED(hr))
                        {
                            hr = g_pGraphBuilder->ConnectDirect(pSplitterVOut, pMuxerVIn, NULL);
                            if(SUCCEEDED(hr))
                            {
                                hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pMuxerAIn);
                                if(SUCCEEDED(hr))
                                {
                                    hr = g_pGraphBuilder->ConnectDirect(pSplitterAOut, pMuxerAIn, NULL);
                                    // get the ouput pin of the dv muxer and the dv camera input pin and connect them
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = g_pBuilder->FindPin(g_pDVMux, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pMuxerOut);
                                        if(SUCCEEDED(hr))
                                        {
                                            hr = g_pBuilder->FindPin(g_pDVCamera, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pDvIn);
                                            if (SUCCEEDED(hr))
                                            {
                                                ASSERT(pDvIn);
                                                hr = g_pGraphBuilder->ConnectDirect(pMuxerOut, pDvIn, NULL);
                                                //
                                                //the capture portion of the graph should be done now, lets do the preview segment
                                                //
                                                if (FAILED(hr))
                                                {
                                                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_MEDIUM, TEXT("Could not connect the DV Muxer to the DV Camera"));
                                                } 
                                                SAFE_RELEASE(pDvIn);
                                            } 
                                        }
                                        SAFE_RELEASE(pMuxerOut);
                                    }
                                }
                                SAFE_RELEASE(pMuxerAIn);
                            }
                        }
                        SAFE_RELEASE(pMuxerVIn);
                    }                   
                    SAFE_RELEASE(pSplitterVOut);
                    SAFE_RELEASE(pSplitterAOut);
                } // Release interfaces appropriately
                else
                {
                    // Handler for invalid source file format
                    if (VFW_E_INVALID_FILE_FORMAT == hr)
                    {
                        MBOX(TEXT("The File format is invalid for the input file"));
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("The File format is invalid for the input file"));
                    } 
                    else
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not connect ouput pin from file (0x%x)"), hr);
                    } 
                }
                SAFE_RELEASE(pSplitterIn);
            } 
            SAFE_RELEASE(pFileOut);
        } 
    }
    //if something failed, remove all of the filters we could have added here, 
    //because DisconnectAll only removes connected filters
    if (FAILED(hr))
    {
        if (g_pAviSplitter)
            g_pGraphBuilder->RemoveFilter(g_pAviSplitter);
        if (g_pInputFileFilter)
            g_pGraphBuilder->RemoveFilter(g_pInputFileFilter);

    } 
    return hr;
} 

/*-------------------------------------------------------------------------
Routine:        DV_SetAviOptions
Purpose:        Routine for changing AVI Mux properties.  In this sample, we just set a few options.  
                    These options could be set through the Avi Mux property sheet, or through a separate dialog
Arguments:    pointer to the AVI renderer (from SetOutputFileName())
Returns:        HRESULT as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_SetAviOptions(IBaseFilter *ppf, InterleavingMode INTERLEAVE_MODE)
{
    HRESULT hr;
    ASSERT(ppf);
    IConfigAviMux       *pMux           = NULL;
    IConfigInterleaving *pInterleaving  = NULL;

    // QI for interface AVI Muxer
    hr = ppf->QueryInterface(IID_IConfigAviMux, reinterpret_cast<PVOID *>(&pMux));
    if (SUCCEEDED(hr))
    {
        pMux->SetOutputCompatibilityIndex(TRUE);
        // QI for interface Interleaving
        hr = ppf->QueryInterface(IID_IConfigInterleaving, reinterpret_cast<PVOID *>(&pInterleaving));
        if (SUCCEEDED(hr))
        {
            // put the interleaving mode (full, none, half)
            pInterleaving->put_Mode(INTERLEAVE_MODE);
            SAFE_RELEASE(pInterleaving);
        } 
        SAFE_RELEASE(pMux);
    } 
    return hr;
} 


/*-------------------------------------------------------------------------
Routine:        DV_AboutDlgProc
Purpose:        simple standard about dialog box proc 
Arguments:    Usual Dialog Processing parameters
Returns:        BOOL
Notes:          
------------------------------------------------------------------------*/
BOOL CALLBACK DV_AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch ( msg )
    {
    //the only command we process are those that close the dialog   
        case WM_COMMAND:
            EndDialog(hwnd, TRUE);
            return TRUE;
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}

/*-------------------------------------------------------------------------
Routine:        DV_ChooseModeDlgProc
Purpose:        dialog proc to select camera/vcr mode on devices that do not respond to the queries
Arguments:    Usual Dialog Processing parameters
Returns:        BOOL
Notes:          
------------------------------------------------------------------------*/
BOOL CALLBACK DV_ChooseModeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch ( msg )
    {
        case WM_COMMAND:
            // Choose whether the camera or vcr unit is selected manually
            switch LOWORD(wParam)
            {
                case IDC_BUTTON_CAMERA :
                    g_CurrentMode = CameraMode;
                    EndDialog(hwnd, TRUE);
                    break;                   
                case IDC_BUTTON_VCR :
                    g_CurrentMode = VcrMode;
                    EndDialog(hwnd, TRUE);
                    break;
                default :
                    break;
            } 
            break;            
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}
  
/*-------------------------------------------------------------------------
Routine:        DV_ChangeDecodeSizeProc
Purpose:        UI wrapper to change DV Decode size
Arguments:    Usual Dialog Processing parameters
Returns:        BOOL
Notes:          
------------------------------------------------------------------------*/
BOOL CALLBACK DV_ChangeDecodeSizeProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int iDisplayPix = 0;
    HRESULT hr;

    switch ( msg )
    {
        case WM_COMMAND:
            // Select which DV format resolution you would like to navigate to
            switch LOWORD(wParam)
            {
                case IDOK :
                    //get selected size, and set size accordingly
                    if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_88x60)))
                    {
                        g_pDvDec->put_IPDisplay(DVRESOLUTION_DC);
                        g_iVWWidth = 88;    g_iVWHeight = 60;
                        g_iAppWidth = g_iVWWidth+5; g_iAppHeight = g_iVWHeight+135;
                        SendMessage(g_hwndApp, WM_SIZE, SIZE_RESTORED, MAKELONG(g_iAppWidth, g_iAppHeight));
                    } 
                    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_180x120)))
                    {
                        g_pDvDec->put_IPDisplay(DVRESOLUTION_QUARTER);
                        g_iVWWidth = 160;   g_iVWHeight = 120;
                        g_iAppWidth = g_iVWWidth+5; g_iAppHeight = g_iVWHeight+115;
                        SendMessage(g_hwndApp, WM_SIZE, SIZE_RESTORED, MAKELONG(g_iAppWidth, g_iAppHeight));
                    } 
                    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_360x240)))
                    {
                        g_pDvDec->put_IPDisplay(DVRESOLUTION_HALF);
                        g_iVWWidth = 360;   g_iVWHeight = 240;
                        g_iAppWidth = g_iVWWidth+5; g_iAppHeight = g_iVWHeight+95;
                        SendMessage(g_hwndApp, WM_SIZE, SIZE_RESTORED, MAKELONG(g_iAppWidth, g_iAppHeight));
                    } 
                    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_720x480)))
                    {
                        g_pDvDec->put_IPDisplay(DVRESOLUTION_FULL);
                        g_iVWWidth = 720;   g_iVWHeight = 480;
                        g_iAppWidth = g_iVWWidth+5; g_iAppHeight = g_iVWHeight+95;
                        SendMessage(g_hwndApp, WM_SIZE, SIZE_RESTORED, MAKELONG(g_iAppWidth, g_iAppHeight));
                    } 
                    else
                    {
                        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Invalid selection in DecodeSize Dialog"));
                    }                    
                //safely fall through                    

                case IDCANCEL :
                    //if it's the preview graph - restart it - otherwise leave it stopped
                    if (GRAPH_PREVIEW == g_iCurrentGraph)
                    {
                        DV_StartGraph();
                    } 
                    SAFE_RELEASE(g_pDvDec);
                    EndDialog(hwnd, TRUE);
                    break;

                default :
                    break;
            } 
            break;
            
        case WM_INITDIALOG:
            /*
                can't set the decode size if the graph is running - so we'll stop it.
                there are other options, such as graying out the menu item whil the graph is 
                playing that could be implemented.  For this simple application, this methos
                will suffice.
            */
            if (SUCCEEDED(DV_StopGraph()))
            {
                hr = g_pDVCodec->QueryInterface(IID_IIPDVDec, reinterpret_cast<PVOID *>(&g_pDvDec));
                if (SUCCEEDED(hr))
                {
                    hr = g_pDvDec->get_IPDisplay(&iDisplayPix);
                    //set button check appropriately
                    switch (iDisplayPix)
                    {
                        case DVRESOLUTION_DC :
                            Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_88x60), TRUE);
                            break;

                        case DVRESOLUTION_QUARTER :
                            Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_180x120), TRUE);
                            break;

                        case DVRESOLUTION_HALF :
                            Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_360x240), TRUE);
                            break;

                        case DVRESOLUTION_FULL :
                            Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_720x480), TRUE);
                            break;

                        default :
                            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Invalid return from get_IPDisplay"));
                            break;
                    } 
                } 
                else
                {
                    MBOX(TEXT("Cannot determine current Decode size"));
                    EndDialog(hwnd, FALSE);
                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not get IIPDVDec interface.  hr = 0x%x"), hr);
                } 
            } 
            else
            {
                MBOX(TEXT("Could not stop graph"));
                EndDialog(hwnd, FALSE);
            } 
            return TRUE;
    }
    return FALSE;
}

/*-------------------------------------------------------------------------
Routine:        DV_CapSizeDlgProc
Purpose:        Dialog proc for cap size dialog
Arguments:    Usual Dialog Processing parameters
Returns:        BOOL
Notes:          
------------------------------------------------------------------------*/
BOOL CALLBACK DV_CapSizeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  
    switch ( msg )
    {
        case WM_INITDIALOG:
        {
            TCHAR           capDisk[8];
            ULARGE_INTEGER  ulFreeBytes;
            ULARGE_INTEGER  ulTotalBytes;
            ULARGE_INTEGER  ulAvailBytes;

            // make sure output file name has been input for capture
            if (!g_OutputFileName[0])
            {
                MBOX(TEXT("Please set up an output file name for recording"));
                EndDialog(hwnd, FALSE);
                return FALSE;
            } 

            //need to determine disk space and init the dialog appropriately
            lstrcpyn(capDisk, g_OutputFileName, 3);
            capDisk[4] = '\0';
            GetDiskFreeSpaceEx(capDisk, &ulFreeBytes, &ulTotalBytes, &ulAvailBytes);
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("Available MB on drive %s is %d MB"), capDisk, ulAvailBytes.QuadPart / (1024 * 1024));

            //let's see what our top limits are, and set our limits appropriately
            if ((ulAvailBytes.QuadPart / DV_BYTES_IN_MEG) < 120)
            {
                //less than 120 MB available - subtract 10 MB at a time until we get a usable amount
                UINT i = 110;
                while ((ulAvailBytes.QuadPart / DV_BYTES_IN_MEG) < i)
                {
                    i -= 10;
                } 
                SendMessage(GetDlgItem(hwnd, IDC_SPIN_TIME), UDM_SETRANGE, 0, MAKELONG(i / 4, 1));
                SendMessage(GetDlgItem(hwnd, IDC_SPIN_SIZE), UDM_SETRANGE, 0, MAKELONG(i, 1));
            }
            else
            {
                SendMessage(GetDlgItem(hwnd, IDC_SPIN_TIME), UDM_SETRANGE, 0, MAKELONG( ((ulAvailBytes.QuadPart / (1024 * 1024) ) - 10) / 4, 1));
                SendMessage(GetDlgItem(hwnd, IDC_SPIN_SIZE), UDM_SETRANGE, 0, MAKELONG( (ulAvailBytes.QuadPart / (1024 * 1024) ) - 10, 1));
            }

            //enable / disable the controls as appropriate
            switch (g_dwCaptureLimit)
            {
                case DV_CAPLIMIT_NONE :
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_NOLIMIT), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TIME), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_TIME), FALSE);
                    break;
                
                case DV_CAPLIMIT_TIME :
                {
                    /*check the radio button, disable the size based controls */
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_TIME), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_SIZE), FALSE);

                    break;
                }
                case DV_CAPLIMIT_SIZE :
                {
                    /*check the radio button, disable the time based controls */
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_SIZE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TIME), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_TIME), FALSE);
                    
                    break;                    
                }
            }
            SetDlgItemInt(hwnd, IDC_EDIT_TIME, g_dwTimeLimit, FALSE);
            SetDlgItemInt(hwnd, IDC_EDIT_SIZE, g_dwDiskSpace, FALSE);
            break;
        } 
        case WM_COMMAND :
            switch LOWORD(wParam)
            {
                // Update the controls ui according to the choices made
                case IDC_RADIO_NOLIMIT :
                {
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_NOLIMIT), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TIME), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_TIME), FALSE);
                    break;
                }
                    
                case IDC_RADIO_TIME :
                {
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_TIME), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_SIZE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TIME), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_TIME), TRUE);
                    break;
                } 
                
                case IDC_RADIO_SIZE :
                {
                    Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_SIZE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TIME), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_TIME), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SIZE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_SPIN_SIZE), TRUE);
                    break;
                } 

                case IDOK :
                {
                    BOOL bTranslated = FALSE;
                    // The selections are made
                    // update the new global capture flag
                    if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_NOLIMIT)))
                    {
                        g_dwCaptureLimit = DV_CAPLIMIT_NONE;
                        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("No capture size limit"));
                    } 
                    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_TIME)))   
                    {
                        g_dwTimeLimit = GetDlgItemInt(hwnd, IDC_EDIT_TIME, &bTranslated, FALSE);
                        g_dwCaptureLimit = DV_CAPLIMIT_TIME;
                        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Time based limit - %d"), g_dwTimeLimit);
                    } 
                    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_SIZE)))   
                    {
                        g_dwDiskSpace = GetDlgItemInt(hwnd, IDC_EDIT_SIZE, &bTranslated, FALSE);
                        g_dwCaptureLimit = DV_CAPLIMIT_SIZE;
                        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Disk based limit - %d MB"), g_dwDiskSpace);
                    } 
                    EndDialog(hwnd, TRUE);
                    return TRUE;
                }
                
                case IDCANCEL :
                    // update nothing much
                    EndDialog(hwnd, FALSE);
                    return FALSE;
                default :
                    return FALSE;
            }            
            return TRUE;
    }
    return FALSE;
}

/*-------------------------------------------------------------------------
Routine:        DV_GetTapeInfo
Purpose:        Get Frame rate and availability of dvcr tape
Arguments:    None
Returns:        HRESULT as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_GetTapeInfo(void)
{
    HRESULT hr;
    LONG    lMediaType = 0;
    LONG    lInSignalMode = 0;

    // Query Media Type of the transport
    hr = g_pExtTrans->GetStatus(ED_MEDIA_TYPE, &lMediaType);
    if (SUCCEEDED(hr))
    {
        if (ED_MEDIA_NOT_PRESENT == lMediaType)
        {
            // Update the log & Status Window
            DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_SUCCINCT, TEXT("No tape is inserted"));
            DV_StatusText(TEXT("VCR Mode - No tape, or unknown format"), 2);
            //we want to return failure if there is no tape installed
            hr = S_FALSE;  
        } 
        else
        {
            //tape type should always be DVC
            ASSERT(ED_MEDIA_DVC == lMediaType);

            // Now lets query for the signal mode of the tape.
            g_pExtTrans->GetTransportBasicParameters(ED_TRANSBASIC_INPUT_SIGNAL, &lInSignalMode, NULL);
            Sleep(_MAX_SLEEP);
            if (SUCCEEDED(hr))
            {   // determine whether the camcorder supports ntsc or pal
                switch (lInSignalMode)
                {
                    case ED_TRANSBASIC_SIGNAL_525_60_SD :
                    g_AvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS)
                    DV_StatusText(TEXT("VCR Mode - NTSC"), 2);
                    break;

                case ED_TRANSBASIC_SIGNAL_525_60_SDL :
                    g_AvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS)
                    DV_StatusText(TEXT("VCR Mode - NTSC"), 2);
                    break;

                case ED_TRANSBASIC_SIGNAL_625_50_SD :
                    g_AvgTimePerFrame = 40;  // 40 milli-sec (25FPS)
                    DV_StatusText(TEXT("VCR Mode - PAL"), 2);
                    break;

                case ED_TRANSBASIC_SIGNAL_625_50_SDL :
                    g_AvgTimePerFrame = 40;  // 40 milli-sec (25FPS)
                    DV_StatusText(TEXT("VCR Mode - PAL"), 2);
                    break;

                default : 
                    DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Unsupported or unrecognized tape format type"));
                    g_AvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS); default
                    break;
                }

                DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Avg time per frame is %d FPS"), g_AvgTimePerFrame);
            } 
            else
            {
                DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("GetTransportBasicParameters Failed (0x%x)"), hr);
            } 
        }
    }
    else
    {
        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("GetStatus Failed (0x%x)"), hr);
    } 
    return hr;
}

/*-------------------------------------------------------------------------
Routine:        DV_RefreshMode
Purpose:        Use this to rebuild the necessary stuff to switch between VCR and camera mode
Arguments:    None
Returns:        TRUE if successful
Notes:          
------------------------------------------------------------------------*/
BOOL DV_RefreshMode(void)
{
    BOOL bStatus    = FALSE;
    // Query the current device type    
    switch(DV_GetDVMode())
    {
        case CameraMode :
            // update the Graph Mode menu items & status window
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV, MF_GRAYED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_NOPRE, MF_GRAYED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_TYPE2, MF_GRAYED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_NOPRE_TYPE2, MF_GRAYED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_CHECKTAPE, MF_GRAYED);
            DV_StatusText(TEXT("Camera Mode"), 2);
            bStatus = TRUE;
            break;

        case VcrMode :
            // Query the tape info & update the status bar
            DV_GetTapeInfo();  
            // update the Graph Mode menu items & status window
            EnableMenuItem(GetMenu(g_hwndApp), IDM_CHECKTAPE, MF_ENABLED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV, MF_ENABLED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_NOPRE, MF_ENABLED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_TYPE2, MF_ENABLED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_FILETODV_NOPRE_TYPE2, MF_ENABLED);
            EnableMenuItem(GetMenu(g_hwndApp), IDM_CHECKTAPE, MF_ENABLED);
            bStatus = TRUE;
            break;

        case UnknownMode :
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Should not have reached unknown mode handler"));
            break;
            
        default :
            DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Could not determine Camera/Vcr Mode"));
            break;
    }
    return bStatus;
} 

/*-------------------------------------------------------------------------
Routine:        DV_WriteRegKeys
Purpose:        Save the input and output file names to the registry
Arguments:    None
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void DV_WriteRegKeys(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp = 0;
    // Create DVApp's registry keys
    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, 
                             TEXT("Software\\Microsoft\\DVApp"), 
                             NULL, 
                             NULL, 
                             REG_OPTION_NON_VOLATILE, 
                             KEY_ALL_ACCESS, 
                             NULL,
                             &hKey,
                             &dwDisp);

    // Write the input & output filenames
    if (REG_OPENED_EXISTING_KEY == dwDisp ||  REG_CREATED_NEW_KEY == dwDisp) 
    {
        //note that the sizeof parameter expects BYTES (not characters)
        RegSetValueEx(hKey, TEXT("InputFile"), NULL, REG_SZ, (PBYTE)g_InputFileName, (lstrlen(g_InputFileName) * sizeof(TCHAR)) + sizeof(TCHAR));
        RegSetValueEx(hKey, TEXT("OutputFile"), NULL, REG_SZ, (PBYTE)g_OutputFileName, (lstrlen(g_OutputFileName) * sizeof(TCHAR)) + sizeof(TCHAR));
        RegCloseKey(hKey);
    }
} 


/*-------------------------------------------------------------------------
Routine:        DV_ReadRegKeys
Purpose:        Save the input and output file names to the registry
Arguments:    None
Returns:        None
Notes:          
------------------------------------------------------------------------*/
void DV_ReadRegKeys(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwType;
    DWORD a = MAX_PATH * sizeof(TCHAR), b = MAX_PATH * sizeof(TCHAR);
    // Read the DVApp's registry keys
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\DVApp"), NULL, KEY_ALL_ACCESS, &hKey);

    // Load the input & output filenames
    if (ERROR_SUCCESS == lResult)
    {
        RegQueryValueEx(hKey, TEXT("InputFile"), NULL, &dwType, (PBYTE)g_InputFileName, &a);
        RegQueryValueEx(hKey, TEXT("OutputFile"), NULL, &dwType, (PBYTE)g_OutputFileName, &b);
        RegCloseKey(hKey);
    }
    
}


/*-------------------------------------------------------------------------
Routine:        DV_SaveGraph
Purpose:        Save the filter graph into a *.grf file
Arguments:    FileName
Returns:        HRESULT as appropriate
Notes:          
------------------------------------------------------------------------*/
HRESULT DV_SaveGraph(TCHAR* sGraphFile)
{
    IStorage *          pStorage = NULL;
    IStream *           pStream = NULL;
    IPersistStream *    pPersistStream = NULL;
    HRESULT             hr = S_OK;

    if(g_pGraphBuilder == NULL || sGraphFile == NULL)
        return E_FAIL;

#ifndef UNICODE
    WCHAR swGraphFile[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, sGraphFile, -1, swGraphFile, MAX_PATH);

    // Either Open or Create the *.GRF file
    hr = StgOpenStorage( swGraphFile, NULL, STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_WRITE, NULL, NULL, &pStorage );
    if ( STG_E_FILENOTFOUND == hr )
        hr = StgCreateDocfile( swGraphFile, STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE , NULL , &pStorage);
#else
    // Either Open or Create the *.GRF file
    hr = StgOpenStorage( sGraphFile, NULL, STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_WRITE, NULL, NULL, &pStorage );
    if ( STG_E_FILENOTFOUND == hr )
        hr = StgCreateDocfile( sGraphFile, STGM_CREATE | STGM_TRANSACTED | STGM_2READWRITE | STGM_SHARE_EXCLUSIVE , NULL , &pStorage);

#endif

    if ( SUCCEEDED(hr) )
        hr = pStorage->CreateStream( L"ActiveMovieGraph", STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, NULL, NULL, &pStream );

    // Persist the stream, save & commit to disk
    if ( SUCCEEDED(hr) )
        hr = g_pGraphBuilder->QueryInterface( IID_IPersistStream, (void **) &pPersistStream );
    if ( SUCCEEDED(hr) )
        hr = pPersistStream->Save(pStream, TRUE);
    if ( SUCCEEDED(hr) )
        hr = pStorage->Commit( STGC_DEFAULT );

    if ( SUCCEEDED(hr) )
        DV_LogOut(LOG_PRIORITY_INFO, LOG_LEVEL_MEDIUM, TEXT("Save current Filter Graph to GRF file succeded"));
    else
    {
        MessageBox( NULL, "Save GRF file failed", "DVApp", MB_ICONEXCLAMATION | MB_OK );
        DV_LogOut(LOG_PRIORITY_ERROR, LOG_LEVEL_SUCCINCT, TEXT("Save current Filter Graph to GRF file failed"));
    }
    SAFE_RELEASE(pStorage);
    SAFE_RELEASE(pStream);
    SAFE_RELEASE(pPersistStream);

    return hr;
}

/*-------------------------------------------------------------------------
Routine:        DV_LogOut 
Purpose:        Show the Message Window with the Log string 
Arguments:    logging priority, logging level & the message
Returns:        None
Notes:          Logs according to the current logging priority & level set
------------------------------------------------------------------------*/
void __cdecl DV_LogOut (LOG_PRIORITY iPriority, LOG_LEVEL ilevel, TCHAR *szFormat, ... )
{
    static TCHAR msg[1024];
    va_list va;
    va_start(va, szFormat);
    wvsprintf (msg, szFormat, va);
    va_end(va);

    if ( ilevel > g_iLogLevel )         // we can skip this message
        return;
    if ( iPriority > g_iLogPriority )   // we can skip this message
        return;

    MBOX(msg);
    return;
} 

