//----------------------------------------------------------------------------
// File: main.cpp
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#define D3D_OVERLOADS
#include <windows.h>
#include <basetsd.h>
#include <process.h>
#include <d3dx.h>
#include <stdio.h>
#include <math.h>
#include <mmsystem.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <dxerr8.h>
#include <tchar.h>
#include "DXUtil.h"
#include "SyncObjects.h"
#include "Resource.h"
#include "MazeApp.h"
#include "MazeClient.h"
#include "MazeServer.h"
#include "Config.h"
#include "DPlay8Client.h"
#include "DummyConnector.h"


//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define MAX_OUTPUT_QUEUE    256


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HINSTANCE               g_hInstance;
LPCSTR                  g_szCmdLine;
HWND                    g_hWnd;
CModule*                g_pApp;
BOOL                    g_bWindowed;
RECT                    g_ClientRect;
RECT                    g_WindowRect;
BOOL                    g_bSizing                       = FALSE;
BOOL                    g_bSuspended                    = FALSE;
DWORD                   g_dwAppFlags                    = 0;
CMazeClient             g_MazeClient;
CMazeServer             g_MazeServer;
BOOL                    g_bIsTest;
BOOL                    g_bIsPreview;
HWND                    g_hRefWindow;
DWORD                   g_dwMouseMoveCount;
FLOAT                   g_fLastMouseMove                = 0.0f;
Config                  g_Config;
CDPlay8Client           g_DP8Client;
DWORD                   g_dwStartMode;
UINT                    g_dwRenderThreadID;
CDummyConnectorServer   g_DummyServerConnection;
CDummyConnectorClient   g_DummyClientConnection;
BOOL                    g_bLocalLoopback                = TRUE;
BOOL                    g_bOutOfDateClient              = FALSE;
TCHAR                   g_strTimeStamp[50];
FLOAT                   g_fTimeStampUpdateCountdown;
TCHAR                   g_szOutputBuffer[MAX_OUTPUT_QUEUE][256];
HANDLE                  g_hOutputEvent;
HANDLE                  g_hConsoleThread;
DWORD                   g_dwNextOutput                  = 0;
DWORD                   g_dwNextFreeOutput              = 0;
DWORD                   g_dwQueueSize                   = 0;
DWORD                   g_dwNumLogLines;
BOOL                    g_bQuitThread                   = FALSE;
BOOL                    g_bDisconnectNow;
CCriticalSection        g_OutputQueueLock;
HANDLE                  g_hLogFile                      = NULL;


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
LRESULT CALLBACK    WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
UINT WINAPI         OutputThread( LPVOID pParam );
INT_PTR             DoStartupDialog();
int                 ScreenSaverDoConfig( BOOL bIsScreenSaverSettings );
HRESULT             StartSessionEnum();
HRESULT             InitServerForLoopback();
BOOL                TryToConnect();
void                ReadConfig();
void                UpdateTimeStamp();
void                CreateTempLogFile();
VOID                SuspendPowerManagement();


#define FULL_SCREEN_STYLE   (WS_MAXIMIZE|WS_VISIBLE|WS_POPUP)
#define WINDOW_STYLE        (WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_POPUP|WS_SIZEBOX)

#define WM_APP_TOGGLE_FS    (WM_APP + 1)



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int )
{
    HRESULT hr;

    // Store these in globals for convenience
    g_hInstance = hInstance;
    g_szCmdLine = lpCmdLine;

    // Create an event object to flag pending output messages
    g_hOutputEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    // Tell OS's that have power management to not 
    // sleep, since this app will be using the 
    // network connection and need very little user input
    SuspendPowerManagement();

    // Extract configuration settings from the registry
    ReadConfig();

    // Spin up a thread to record the client output
    UINT dwConsoleThreadID;
    g_hConsoleThread = (HANDLE)_beginthreadex( NULL, 0, OutputThread, NULL, 0, &dwConsoleThreadID );

    // Set the flag for 32-bit screen modes if appropriate
    if( g_Config.bPrefer32Bit )
        g_dwAppFlags |= MOD_DISP_PREFER32BIT;

    // Parse the command line 
    DXTRACE( TEXT("\nMaze cmd line=\"%s\"\n"), lpCmdLine );
    CHAR* pSwitch = strtok( lpCmdLine, "/" );    
    if( pSwitch == NULL || *pSwitch == 'c' || *pSwitch == 'C' )
    {
        // No options, so run screen saver config dialog
        if( pSwitch == NULL )
            ScreenSaverDoConfig( FALSE );
        else
            ScreenSaverDoConfig( TRUE );

        if( g_dwStartMode == 0 )
            return 0;
    }
    else
    {
        switch( *pSwitch )
        {
            // Preview mode - need to extract the handle of the 'reference window'
            case 'p':
            case 'P':
                while( *pSwitch && !isdigit(*pSwitch) )
                    pSwitch++;
                if( isdigit(*pSwitch) )
                {
#ifdef IA64
                    g_hRefWindow = (HWND) _atoi64(pSwitch);
#else
                    g_hRefWindow = (HWND) atol(pSwitch);
#endif                    
                    if( g_hRefWindow != NULL )
                        g_bIsPreview = TRUE;
                }
                break;
        }
    }

    // Initalize COM
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    ConsolePrintf( LINE_CMD, TEXT("MazeClient started.") );

    UpdateTimeStamp();
    DXUtil_Timer( TIMER_START );
    DWORD dwSRand = (DWORD) (DXUtil_Timer( TIMER_GETABSOLUTETIME ) * UINT_MAX * (DWORD)GetCurrentThreadId() );
    srand( dwSRand );

    // Initalize DPlay client
    if( FAILED( hr = g_DP8Client.Init() ) )
    {
        DXTRACE_ERR( TEXT("Init"), hr );
        g_DP8Client.Shutdown();
        CoUninitialize();
        return -1;
    }

    // Initialize maze client object - basically just build the maze
    g_MazeClient.Init();

    // Init the server for loopback mode 
    InitServerForLoopback();

    // Initialise the D3DX library
    if( FAILED( hr = D3DXInitialize() ) )
    {
        DXTRACE_ERR( TEXT("D3DXInitialize"), hr );
        g_DP8Client.Shutdown();
        CoUninitialize();
        return -1;
    }

    // Create app object (this is the "graphics engine")
    g_pApp = new CMazeApp;

    // Register window class
    WNDCLASSEX  wc;
    wc.cbSize = sizeof(wc);
    wc.style = CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON1) );
    wc.hCursor = LoadCursor( NULL, MAKEINTRESOURCE(IDC_ARROW) );
    wc.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("DirectPlayMazeWindow");
    wc.hIconSm = NULL;
    RegisterClassEx( &wc );

    // Create appropriate window
    if( !g_bIsPreview && !g_Config.bFullScreen )
    {
        // Test mode, so create initial window with 640x480 client area
        RECT rect;
        rect.left = rect.top = 40;
        rect.right = rect.left+640;
        rect.bottom = rect.top+480;
        if( g_dwAppFlags & MOD_DISP_REFRAST )
        {
            rect.right -= 320;
            rect.bottom -= 240;
        }
        g_bWindowed = TRUE;
        AdjustWindowRect( &rect, WINDOW_STYLE, FALSE );
        g_hWnd = CreateWindow( TEXT("DirectPlayMazeWindow"), TEXT("DirectPlayMaze"), 
                               WINDOW_STYLE, rect.left, rect.top ,
                               rect.right-rect.left, rect.bottom-rect.top, NULL ,
                               NULL, g_hInstance, NULL );
    }
    else if( g_bIsPreview )
    {
        // Preview mode, so make our window a child of the supplied window handle
        RECT rect;
        GetClientRect( g_hRefWindow, &rect );
        AdjustWindowRect( &rect, WS_VISIBLE|WS_CHILD, FALSE );
        g_bWindowed = TRUE;
        g_hWnd = CreateWindow(  TEXT("DirectPlayMazeWindow"), TEXT("Preview") ,
                                WS_VISIBLE|WS_CHILD, rect.left, rect.top ,
                                rect.right-rect.left, rect.bottom-rect.top, g_hRefWindow ,
                                NULL, g_hInstance, NULL );    
        
        // On Win2K the display properties dialog calls WaitForInputIdle. We never go 
        // idle, so it just times out after a long pause. To circumvent this we force ourselves to 
        // go idle and have this timer wake us up. 
        SetTimer( g_hWnd, 1, 50, NULL );
    }
    else
    {
        // Screensaver mode - so create fullscreen window
        g_bWindowed = FALSE;
        g_hWnd = CreateWindow( TEXT("DirectPlayMazeWindow"), TEXT("DirectPlayMaze"), FULL_SCREEN_STYLE, 0, 0 ,
                               640, 480, NULL, NULL, g_hInstance, NULL );
    }

    if( g_hWnd == NULL )
    {
        return 0;
    }

    if( g_Config.bFileLogging )
        CreateTempLogFile();

    // Go into loopback mode to start
    BOOL bLocalLoopbackInitDone = FALSE;

    // If the window was created okay, then run the message pump
    MSG msg;
    msg.message = 0;

    BOOL bFirstTime = g_bIsPreview;
    BOOL bInitDone = FALSE;

    while( msg.message != WM_QUIT )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            FLOAT fTimeLapsed = DXUtil_Timer( TIMER_GETELAPSEDTIME );
            FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
            static FLOAT s_fLastConnect     = INT_MIN;
            static FLOAT s_fStartEnumTime   = INT_MIN;
            static FLOAT s_fStopEnumTime    = INT_MIN;
            static FLOAT s_fEnumStarted     = FALSE;

            if( g_bWindowed )
                Sleep(10);

            // Update the time stamp
            g_fTimeStampUpdateCountdown -= fTimeLapsed;
            if( g_fTimeStampUpdateCountdown < 0.0f )
                UpdateTimeStamp();

            if( g_DP8Client.IsSessionLost() ) 
            {
                if( FALSE == bLocalLoopbackInitDone )
                {
                    if( bInitDone )
                    {
                        ConsolePrintf( LINE_LOG, TEXT("Disconnected from server") );

                        if( g_DP8Client.GetSessionLostReason() == DISCONNNECT_REASON_CLIENT_OUT_OF_DATE )
                        {
                            ConsolePrintf( LINE_LOG, TEXT("Disconnected because MazeClient is out of date.") );
                            ConsolePrintf( LINE_LOG, TEXT("Please get updated version") );
                            ConsolePrintf( LINE_LOG, TEXT("from http://msdn.microsoft.com/directx/") );
                            g_bOutOfDateClient = TRUE;
                        }

                        // Disconnected, so retry in 10 seconds
                        s_fStopEnumTime = fCurTime - g_Config.dwNetworkRetryDelay * 60.0f + 10.0f;     
                    }
                    else
                    {
                        // If just starting up, then retry immediately
                        bInitDone = TRUE;
                    }

                    g_MazeClient.SetMazeReady( FALSE );
                    g_MazeClient.LockWorld();
                    g_MazeClient.Reset();

                    // Now that the session is lost we need to 
                    // restart DirectPlay by calling Close() 
                    // and Init() on m_pDPlay
                    g_DP8Client.Shutdown();
                    g_DP8Client.Init();
                    
                    g_bLocalLoopback = TRUE;
                    InitServerForLoopback();
                    g_DummyClientConnection.SetTarget( &g_MazeServer );
                    g_MazeClient.SetOutboundClient( &g_DummyClientConnection );
                    g_DummyServerConnection.SetTarget( &g_MazeClient );
                    g_MazeServer.SetOutboundServer( &g_DummyServerConnection );

                    g_MazeClient.UnlockWorld();
                    g_DummyClientConnection.Connect( 2 );
                    g_MazeClient.EngageAutopilot( TRUE );

                    bLocalLoopbackInitDone = TRUE;
                }

                if( !s_fEnumStarted && fCurTime - s_fStopEnumTime > g_Config.dwNetworkRetryDelay * 60.0f )
                {
                    if( SUCCEEDED( hr = StartSessionEnum() ) )
                    {
                        // DirectPlaying host enumeration started
                        ConsolePrintf( LINE_LOG, TEXT("Starting DirectPlaying host enumeration") );
                        s_fStartEnumTime = fCurTime;
                        s_fEnumStarted = TRUE;
                    }
                    else
                    {
                        // DirectPlaying host enumeration failed to start
    			        // Will try again in g_Config.dwNetworkRetryDelay minutes
                        ConsolePrintf( LINE_LOG, TEXT("DirectPlaying host enumeration failed to start.") );
    			        ConsolePrintf( LINE_LOG, TEXT("Will try again in %d minutes."), g_Config.dwNetworkRetryDelay );
                        s_fStopEnumTime = fCurTime;
                        s_fEnumStarted = FALSE;
                    }
                }

                if( s_fEnumStarted && fCurTime - s_fStartEnumTime > 5.0f * 60.0f )
                {
                    ConsolePrintf( LINE_LOG, TEXT("No host found. Stopping DirectPlaying host enumeration") );
    			    ConsolePrintf( LINE_LOG, TEXT("Will try again in %d minutes."), g_Config.dwNetworkRetryDelay );

                    // Stop enumeration
                    g_DP8Client.StopSessionEnum();
                    s_fStopEnumTime = fCurTime;
                    s_fEnumStarted = FALSE;
                }

                if( s_fEnumStarted && fCurTime - s_fLastConnect > 0.5f )
                {
                    if( TRUE == TryToConnect() )
                    {
                        // Connect successful 
                        ConsolePrintf( LINE_LOG, TEXT("Connected to server.  Host enumeration stopped.") );
                        g_bLocalLoopback = FALSE;
                        s_fEnumStarted   = FALSE;
                        g_MazeClient.EngageAutopilot( TRUE );
                    }

                    s_fLastConnect = fCurTime;
                }

                g_bDisconnectNow = FALSE;
            }
            else
            {
                bLocalLoopbackInitDone = FALSE;

                if( g_bDisconnectNow )
                {
                    g_bDisconnectNow = FALSE;
    			    ConsolePrintf( LINE_LOG, TEXT("Intentional disconnect.") );

            	    g_MazeClient.Shutdown();

                    g_MazeClient.SetMazeReady( FALSE );
                    g_MazeClient.LockWorld();
                    g_MazeClient.Reset();

                    // Init for loopback
                    // Now that the session is lost we need to 
                    // restart DirectPlay by calling Close() 
                    // and Init() on m_pDPlay
                    g_DP8Client.Shutdown();
                    g_DP8Client.Init();
                    
                    g_bLocalLoopback = TRUE;
                    InitServerForLoopback();
                    g_DummyClientConnection.SetTarget( &g_MazeServer );
                    g_MazeClient.SetOutboundClient( &g_DummyClientConnection );
                    g_DummyServerConnection.SetTarget( &g_MazeClient );
                    g_MazeServer.SetOutboundServer( &g_DummyServerConnection );

                    g_MazeClient.UnlockWorld();
                    g_DummyClientConnection.Connect( 2 );
                    g_MazeClient.EngageAutopilot( TRUE );
                }
            }

            // Update state of client
            g_MazeClient.Update( fTimeLapsed );

            // Display stats every so often
            static float fLastLogUpdate = fCurTime;
            if( fCurTime - fLastLogUpdate > 600.0f )
            {
                D3DXVECTOR3 vPos = g_MazeClient.GetCameraPos(); 
                ConsolePrintf( LINE_LOG, TEXT("Position: (%5.1f,%5.1f), Players: %d"), 
                              vPos.x, vPos.z, g_MazeClient.GetNumPlayers() );

                TCHAR strInfo[5000];
                TCHAR* strEndOfLine;
                TCHAR* strStartOfLine;

                // Query the IOutboudNet for info about the connection to this user
                g_DP8Client.GetConnectionInfo( strInfo );

                ConsolePrintf( LINE_LOG, TEXT("Displaying connection info for 0x%0.8x"), g_MazeClient.GetLocalClientID() );
                ConsolePrintf( LINE_LOG, TEXT("(Key: G=Guaranteed NG=Non-Guaranteed B=Bytes P=Packets)") );

                // Display each line seperately
                strStartOfLine = strInfo;
                while( TRUE )
                {
                    strEndOfLine = strchr( strStartOfLine, '\n' );
                    if( strEndOfLine == NULL )
                        break;

                    *strEndOfLine = 0;
                    ConsolePrintf( LINE_LOG, strStartOfLine );
                    strStartOfLine = strEndOfLine + 1;
                }

                fLastLogUpdate = fCurTime;
            }

            if( g_pApp->OkayToProceed() )
            {
                if( !g_bSizing )
                {
                    // Draw the frame
                    g_pApp->RenderFrame( fTimeLapsed );
                    g_bSuspended = FALSE;
                }
            }
            else
            {
                DXTRACE( TEXT("Suspending app (wrong coop mode)\n") );
                g_bSuspended = TRUE;
            }
        }
    }

    g_bQuitThread = TRUE;
    SetEvent( g_hOutputEvent );

    g_pApp->DisplayShutdown();
    g_pApp->OneTimeShutdown();

    WaitForSingleObject( g_hConsoleThread, INFINITE );

    // Shut everything down
    g_MazeClient.Shutdown();
    delete g_pApp;
    D3DXUninitialize();
    g_DP8Client.Shutdown();
    CoUninitialize();

    // Done
    return int(msg.wParam);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HRESULT hr;
    RECT rect;

    switch( msg )
    {
        case WM_CREATE:
            if( FAILED( hr = g_pApp->OneTimeInit( hWnd ) ) )
            {
                DXTRACE_ERR_NOMSGBOX( TEXT("OneTimeInit"), hr );
                return -1;
            }

            if( FAILED( hr = g_pApp->DisplayInit( g_bWindowed ? MOD_DISP_WINDOWED|g_dwAppFlags : g_dwAppFlags, NULL ) ) )
            {
                DXTRACE_ERR_NOMSGBOX( TEXT("DisplayInit"), hr );

                // Don't warn about error when in preview mode
                if( g_bIsPreview )
                    return -1;

                D3DXUninitialize();
                if( hr == D3DXERR_COLORDEPTHTOOLOW )
                {
                    // Hit when in 256color mode
                    MessageBox( NULL, "This app only supports color depths of 16bits or greater.  Try setting desktop to 16bit color or running mazeconsoleclient.exe instead.", "DirectPlayMazeClient", MB_OK );
                }
                else if( hr == D3DXERR_CAPSNOTSUPPORTED ) 
                {
                    // Hit when in 32bit mode and running a voodoo3 trying to go windowed
                    MessageBox( NULL, "Can not run app.  Try setting desktop to 16bit color or running mazeconsoleclient.exe instead.", "DirectPlayMazeClient", MB_OK );
                }
                else
                {
                    MessageBox( NULL, "Can not initialize graphics.  Try mazeconsoleclient.exe instead.", "DirectPlayMazeClient", MB_OK );
                }
                return -1;
            }

            GetClientRect( hWnd, &g_ClientRect );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        case WM_PAINT:
            PAINTSTRUCT ps;
            BeginPaint( hWnd, &ps );
            if( g_Config.bFullScreen && g_bWindowed && g_bSizing )
                g_pApp->OnPaint();
            EndPaint( hWnd, &ps );
            return 0;

        case WM_ERASEBKGND:
            return TRUE;

        case WM_ENTERSIZEMOVE:
            g_bSizing = TRUE;
            break;

        case WM_EXITMENULOOP:
            if( g_bWindowed )
                DXUtil_Timer( TIMER_GETELAPSEDTIME );
        case WM_EXITSIZEMOVE:
            if( g_bWindowed )
            {
                // Finished doing a move or size. So see if the size has changed
                if( g_bWindowed )
                {
                    GetClientRect( hWnd, &rect );
                    if( (rect.right != g_ClientRect.right) || (rect.bottom != g_ClientRect.bottom) )
                    {
                        // Changed, so update the size
                        g_pApp->DisplayShutdown();
                        if( FAILED(g_pApp->DisplayInit( g_bWindowed ? MOD_DISP_WINDOWED|g_dwAppFlags : g_dwAppFlags, NULL ) ) )
                        {
                            // Couldn't restart with the new size, so snap back to the old size
                            rect = g_ClientRect;
                            AdjustWindowRect( &rect, WINDOW_STYLE, FALSE );
                            SetWindowPos( hWnd, HWND_TOP, 0, 0, rect.right-rect.left, rect.bottom-rect.top, 
                                          SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION );
                            GetClientRect( hWnd, &g_ClientRect );
                            if( FAILED( hr = g_pApp->DisplayInit( g_bWindowed ? MOD_DISP_WINDOWED|g_dwAppFlags : g_dwAppFlags, NULL ) ) )
                            {
                                // Couldn't restore old size, so fail and exit
                                DXTRACE_ERR( TEXT("DisplayInit"), hr );
                                MessageBox( hWnd, TEXT("Unable to restore old window size"), TEXT("D3D Demo"), MB_OK );
                                DestroyWindow( hWnd );
                            }
                        }
                        else
                            g_ClientRect = rect;
                    }
                }
                g_bSizing = FALSE;
            }
            break;

        case WM_SYSKEYUP:
            if( g_bWindowed )
            {
                // System (ALT) key. Have Alt+Enter toggle fullscreen/windowed
                if( wParam == VK_RETURN )
                    PostMessage( hWnd, WM_APP_TOGGLE_FS, 0, 0 );
            }
            break;

        case WM_CHAR:
            if( g_bWindowed )
                g_pApp->OnChar( CHAR(wParam) );
            else if( !g_bIsPreview )
            {
                DXTRACE( TEXT("WM_CHAR: closing\n") );
                PostMessage( hWnd, WM_CLOSE, 0, 0 );
            }
            return 0;

        case WM_MOUSEMOVE:
            if( g_bWindowed )
                //  6/26/2000(RichGr) - IA64: Cast wParam to DWORD.
                g_pApp->OnMouseMove( (DWORD)wParam, LOWORD(lParam), HIWORD(lParam) );
            else if( !g_bIsPreview )
            {
                FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
                if( fCurTime - g_fLastMouseMove > 0.1f )
                    g_dwMouseMoveCount = 0;
                g_fLastMouseMove = fCurTime;

                g_dwMouseMoveCount++;
                if( g_dwMouseMoveCount >= 30 )
                {
                    DXTRACE( TEXT("WM_MOUSEMOVE x 30: closing\n") );
                    PostMessage( hWnd, WM_CLOSE, 0, 0 );
                }
            }
            return 0;

        case WM_LBUTTONDOWN:
            if( g_bWindowed )
                //  6/26/2000(RichGr) - IA64: Cast wParam to DWORD.
                g_pApp->OnLButtonDown( (DWORD)wParam, LOWORD(lParam), HIWORD(lParam) );
            else if( !g_bIsPreview )
            {
                DXTRACE( TEXT("WM_LBUTTONDOWN: closing\n") );
                PostMessage( hWnd, WM_CLOSE, 0, 0 );
            }
            return 0;
    
        case WM_LBUTTONUP:
            if( !g_bIsPreview )
                //  6/26/2000(RichGr) - IA64: Cast wParam to DWORD.
                g_pApp->OnLButtonUp( (DWORD)wParam, LOWORD(lParam), HIWORD(lParam) );
            return 0;

        case WM_RBUTTONDOWN:
            if( g_bWindowed )
                //  6/26/2000(RichGr) - IA64: Cast wParam to DWORD.
                g_pApp->OnRButtonDown( (DWORD)wParam, LOWORD(lParam), HIWORD(lParam) );
            else if( !g_bIsPreview )
            {
                DXTRACE( TEXT("WM_RBUTTONDOWN: closing\n") );
                PostMessage( hWnd, WM_CLOSE, 0, 0 );
            }
            return 0;

        case WM_RBUTTONUP:
            if( !g_bIsPreview )
                //  6/26/2000(RichGr) - IA64: Cast wParam to DWORD.
                g_pApp->OnRButtonUp( (DWORD)wParam, LOWORD(lParam), HIWORD(lParam) );
            return 0;

        case WM_SETCURSOR:
            if( !g_bWindowed )
            {
                SetCursor( NULL );
                return TRUE;
            }
            break;

        case WM_ACTIVATE:
            if( LOWORD(wParam) == WA_INACTIVE )
                g_MazeClient.SetInputFocus( FALSE );
            else
                g_MazeClient.SetInputFocus( TRUE );
            break;

        case WM_SYSCOMMAND: 
            if( !g_bIsPreview && !g_bWindowed )
            {
                switch( wParam )
                {
                    case SC_NEXTWINDOW:
                    case SC_PREVWINDOW:
                    case SC_SCREENSAVE:
                        return FALSE;
                };
            }
            break;

        case WM_TIMER:
            if( wParam == 1 )
                KillTimer( hWnd, 1 );
            break;

        case WM_APP_TOGGLE_FS:
        {
            g_pApp->DisplayShutdown();
            if( g_bWindowed )
            {
                GetWindowRect( g_hWnd, &g_WindowRect );
                SetWindowLong( g_hWnd, GWL_STYLE, FULL_SCREEN_STYLE );
                if( FAILED( hr = g_pApp->DisplayInit( g_dwAppFlags, NULL ) ) )
                {
                    DXTRACE_ERR( TEXT("DisplayInit"), hr );
                    MessageBox( g_hWnd, TEXT("Unable to switch to fullscreen"), TEXT("D3D Demo"), MB_OK );
                    DestroyWindow( g_hWnd );
                }
                g_bWindowed = FALSE;
            }
            else
            {
                SetWindowLong( g_hWnd, GWL_STYLE, WINDOW_STYLE );
                SetWindowPos( g_hWnd, HWND_NOTOPMOST, g_WindowRect.left, g_WindowRect.top ,
                              g_WindowRect.right-g_WindowRect.left ,
                              g_WindowRect.bottom-g_WindowRect.top, SWP_SHOWWINDOW );
                if( FAILED( hr = g_pApp->DisplayInit( MOD_DISP_WINDOWED|g_dwAppFlags, NULL ) ) )
                {
                    DXTRACE_ERR( TEXT("DisplayInit"), hr );
                    MessageBox( g_hWnd, TEXT("Unable to switch to windowed mode"), TEXT("D3D Demo"), MB_OK );
                    DestroyWindow( g_hWnd );
                }
                g_bWindowed = TRUE;
            }
            break;
        }

        case WM_POWERBROADCAST:
            switch( wParam )
            {
                #ifndef PBT_APMQUERYSUSPEND  // Defines are here for old compilers
                    #define PBT_APMQUERYSUSPEND 0x0000
                #endif
                #ifndef BROADCAST_QUERY_DENY
                    #define BROADCAST_QUERY_DENY         0x424D5144 
                #endif
                case PBT_APMQUERYSUSPEND:
                    // Tell the OS not to suspend, otherwise we
                    // will loose the network connection.
                    return BROADCAST_QUERY_DENY;

                #ifndef PBT_APMRESUMESUSPEND
                    #define PBT_APMRESUMESUSPEND 0x0007
                #endif
                case PBT_APMRESUMESUSPEND:
                    return TRUE;
            }
            break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT StartSessionEnum()
{
    HRESULT hr;

    // If we're not the preview, then enum sessions
    if( g_bIsPreview )
        return S_OK;

    // Start enumerating available sessions at specified IP address
    if( g_Config.bConnectToMicrosoftSite )
    {
        ConsolePrintf( LINE_LOG, TEXT("Connecting to DirectPlayMaze.rte.microsoft.com") );
        hr = g_DP8Client.StartSessionEnum( MICROSOFT_SERVER );
    }
    else if( g_Config.bConnectToLocalServer )
    {
        ConsolePrintf( LINE_LOG, TEXT("Connecting to local server (searches the local subnet)") );
        hr = g_DP8Client.StartSessionEnum( TEXT("") );
    }
    else if( g_Config.bConnectToRemoteServer )
    {
        ConsolePrintf( LINE_LOG, TEXT("Connecting to remote server at '%s'"), g_Config.szIPAddress );
        hr = g_DP8Client.StartSessionEnum( g_Config.szIPAddress );
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL TryToConnect()
{
    // If it's the preview window or config says don't 
    // connect, then just stay in local loopback
    if( g_bIsPreview )
        return FALSE;

    if( g_DP8Client.GetNumSessions() > 0 )
    {
        // Connect up client before joining session
        g_MazeClient.SetMazeReady( FALSE );
        g_MazeClient.Reset();
        g_MazeClient.SetOutboundClient( g_DP8Client.GetOutboundClient() );
        g_DP8Client.SetClient( &g_MazeClient );

        // Loop through the available sessions and attempt to connect
        for( DWORD i = 0; i < g_DP8Client.GetNumSessions(); i++ )
        {
            if( SUCCEEDED(g_DP8Client.JoinSession( i ) ) )
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT InitServerForLoopback()
{
    HRESULT hr;

    #define LOOPBACK_MAZE_WIDTH  16
    #define LOOPBACK_MAZE_HEIGHT 16

    // Initalize maze and server objects for loopback mode
    if( FAILED( hr = g_MazeClient.m_Maze.Init( LOOPBACK_MAZE_WIDTH, 
                                               LOOPBACK_MAZE_HEIGHT, 
                                               DEFAULT_SEED ) ) )
    {
        return DXTRACE_ERR( TEXT("m_Maze.Init"), hr );
    }

    // Initialize maze server object - hook up to the maze object in the client
    g_MazeServer.Init( &g_MazeClient.m_Maze );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void UpdateTimeStamp()
{
    SYSTEMTIME sysTime;
    GetLocalTime( &sysTime );
    _stprintf( g_strTimeStamp, TEXT("[%02d-%02d-%02d %02d:%02d:%02d]"),
               sysTime.wMonth, sysTime.wDay, sysTime.wYear % 100, 
               sysTime.wHour, sysTime.wMinute, sysTime.wSecond );

    // Compute how many milliseconds until the next second change
    g_fTimeStampUpdateCountdown = (1000 - sysTime.wMilliseconds) / 1000.0f;
}
  




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void ConsolePrintf( EnumBufferType enumBufferType, const TCHAR* fmt, ... )
{
    // Format the message into a buffer
    TCHAR buffer[512];
    _vstprintf( buffer, fmt, (CHAR*) ((&fmt)+1) );

    // Lock the output queue
    g_OutputQueueLock.Enter();

    // Find free spot
    if( g_dwQueueSize != MAX_OUTPUT_QUEUE )
    {
        // Format message into the buffer
        _vstprintf( g_szOutputBuffer[g_dwNextFreeOutput], fmt, (CHAR*)((&fmt)+1) );
    
        // Increment output pointer and wrap around
        g_dwNextFreeOutput++;
        if( g_dwNextFreeOutput == MAX_OUTPUT_QUEUE )
            g_dwNextFreeOutput = 0;

        // Increment message count
        g_dwQueueSize++;
    }

    // Unlock output queue
    g_OutputQueueLock.Leave();

    // Signal event so the output thread empties the queue
    SetEvent( g_hOutputEvent );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
UINT WINAPI OutputThread( LPVOID pParam )
{
#define MAX_LOG_LINES 100

    TCHAR szLogBuffer[256];

    while( 1 )
    {
        // Wait for output to be added to the queue or the quit flag to be set
        WaitForSingleObject( g_hOutputEvent, INFINITE );
        if( g_bQuitThread )
            break;

        // Lock output queue
        g_OutputQueueLock.Enter();

        // While there are messages to print
        while ( g_dwQueueSize )
        {
            // Add g_szOutputBuffer[g_dwNextOutput] to szLogBuffer array,
            // and redisplay the array on the top half of the screen
            _stprintf( szLogBuffer, "%s %s", 
                       g_strTimeStamp, g_szOutputBuffer[g_dwNextOutput] );

#ifdef _DEBUG
            OutputDebugString( szLogBuffer );
            OutputDebugString( "\n" );
#endif
            if( g_hLogFile )
            {
                DWORD dwWritten;
                WriteFile( g_hLogFile, szLogBuffer, 
                           lstrlen( szLogBuffer ), &dwWritten, NULL );
                TCHAR strEOL = TEXT('\r');
                WriteFile( g_hLogFile, &strEOL, 
                           sizeof(TCHAR), &dwWritten, NULL );
                strEOL = TEXT('\n');
                WriteFile( g_hLogFile, &strEOL, 
                           sizeof(TCHAR), &dwWritten, NULL );

                static float s_fLastFlushTime = DXUtil_Timer( TIMER_GETAPPTIME );
                float fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
                if( fCurTime - s_fLastFlushTime > 0.2f )
                {
                    FlushFileBuffers( g_hLogFile );
                    s_fLastFlushTime = fCurTime;
                }
            }

            g_dwNextOutput++;
            if( g_dwNextOutput == MAX_OUTPUT_QUEUE )
                g_dwNextOutput = 0;

            g_dwQueueSize--;
        }

        // Unlock output queue
        g_OutputQueueLock.Leave();

        if( g_hLogFile )
            FlushFileBuffers( g_hLogFile );
    }

    if( g_hLogFile )
        CloseHandle( g_hLogFile );

    return 0;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CreateTempLogFile()
{
    BOOL bSuccess;
    TCHAR strTempPath[MAX_PATH];
    TCHAR strTempFileName[MAX_PATH];
    TCHAR strBaseName[MAX_PATH];
    TCHAR strTime[MAX_PATH];
    DWORD dwCount;
    
    GetTempPath( MAX_PATH, strTempPath );
    lstrcat( strTempPath, TEXT("DirectPlayMaze\\") );

    // Create the directory if it doesn't exist
    if( GetFileAttributes( strTempPath ) == -1 )
    {
        bSuccess = CreateDirectory( strTempPath, NULL );
        if( !bSuccess )
        {
            ConsolePrintf( LINE_LOG, TEXT("Could not create create temp directory '%s'"), strTempPath );
            goto LFail;
        }
    }

    ConsolePrintf( LINE_LOG, TEXT("Log Directory: '%s'"), strTempPath );

    SYSTEMTIME sysTime;
    GetLocalTime( &sysTime );
    _stprintf( strTime, TEXT("client-%04d-%02d-%02d-"),
               sysTime.wYear, sysTime.wMonth, sysTime.wDay );

    dwCount = 0;

    while(TRUE)
    {
        wsprintf( strBaseName, "%s%05d.log", strTime, dwCount );
        lstrcpy( strTempFileName, strTempPath );
        lstrcat( strTempFileName, strBaseName );
        DWORD dwResult = GetFileAttributes( strTempFileName );
        if( dwResult == -1 )
            break;

        dwCount++;
    }

    g_hLogFile = CreateFile( strTempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                             CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    if( g_hLogFile == INVALID_HANDLE_VALUE )
    {
        ConsolePrintf( LINE_LOG, TEXT("Could not create create temp file '%s'"), strTempFileName );
        goto LFail;
    }

    ConsolePrintf( LINE_LOG, TEXT("Logging to temp file: '%s'"), strBaseName );
    return;

LFail:
    ConsolePrintf( LINE_LOG, TEXT("File logging disabled") );
    g_Config.bFileLogging = FALSE;
}
    



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID SuspendPowerManagement()
{
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstKernel32 = NULL;
    typedef EXECUTION_STATE (WINAPI* LPSETTHREADEXECUTIONSTATE)( EXECUTION_STATE esFlags );
    LPSETTHREADEXECUTIONSTATE pSetThreadExecutionState = NULL;

    GetSystemDirectory(szPath, MAX_PATH);

    // SetThreadExecutionState() isn't availible on some old OS's, 
    // so do a LoadLibrary to get to it.
    lstrcat(szPath, TEXT("\\kernel32.dll"));
    hInstKernel32 = LoadLibrary(szPath);

    if (hInstKernel32 != NULL)
    {
        pSetThreadExecutionState = (LPSETTHREADEXECUTIONSTATE)GetProcAddress(hInstKernel32, "SetThreadExecutionState");
        if( pSetThreadExecutionState != NULL )
        {
            // Tell OS's that have power management to not 
            // sleep, since this app will be using the 
            // network connection and need very little user input
            pSetThreadExecutionState( ES_SYSTEM_REQUIRED | ES_CONTINUOUS );
        }

        FreeLibrary(hInstKernel32);
    }
}




