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
#include <d3dx.h>
#include <stdio.h>
#include <math.h>
#include <mmsystem.h>
#include <process.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <dxerr8.h>
#include <tchar.h>
#include <time.h>
#include "SyncObjects.h"
#include "DPlay8Client.h"
#include "MazeClient.h"
#include "DXUtil.h"





//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define MAX_OUTPUT_QUEUE    256
#define MICROSOFT_SERVER    TEXT("DirectPlayMaze.rte.microsoft.com")
const TCHAR*  g_szKeyname = TEXT("Software\\Microsoft\\DirectPlayMaze\\MazeConsoleClient");

struct  Config
{
    BOOL    bConnectToMicrosoftSite;
    BOOL    bConnectToLocalServer;
    BOOL    bConnectToRemoteServer;
    DWORD   dwNetworkRetryDelay;
    TCHAR   szIPAddress[64];
    BOOL    bAutoDisconnnect;
    BOOL    bAutoConnnect;
    BOOL    bFileLogging;
    DWORD   dwLogLevel;
    DWORD   dwAutoPrintStats;
};


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
HANDLE              g_hConsoleThread;
volatile BOOL       g_bQuitThread;
HANDLE              g_hOutputEvent;
HANDLE              g_hClientThread;
TCHAR               g_szOutputBuffer[MAX_OUTPUT_QUEUE][256];
CONSOLE_SCREEN_BUFFER_INFO g_SavedConsoleInfo;
EnumBufferType      g_enumBufferType[MAX_OUTPUT_QUEUE];
DWORD               g_dwNextOutput          = 0;
DWORD               g_dwNextFreeOutput      = 0;
DWORD               g_dwQueueSize           = 0;
CCriticalSection    g_OutputQueueLock;
HANDLE              g_hStdOut               = NULL;
DWORD               g_dwNumCmdLines         = 11;
DWORD               g_dwSeperatorLine;
DWORD               g_dwNumLogLines;
DWORD               g_dwWindowSizeY;
BOOL                g_bLocalLoopback        = TRUE;
BOOL                g_bOutOfDateClient      = FALSE;
BOOL                g_bAllowConnect         = FALSE;
Config              g_Config;
CDPlay8Client       g_DP8Client;
CMazeClient         g_MazeClient;
TCHAR               g_szIPAddress[256];
BOOL                g_bConnectNow;
BOOL                g_bDisconnectNow;
TCHAR               g_strTimeStamp[50];
FLOAT               g_fTimeStampUpdateCountdown;
HANDLE              g_hLogFile              = NULL;
TCHAR               g_strLogFile[MAX_PATH];
TCHAR               g_strLogDir[MAX_PATH];
BOOL                g_bSaveSettings         = TRUE;


//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
BOOL         ParseCommandLine( int argc, TCHAR* argv[] );
BOOL WINAPI  CtrlHandler( DWORD type );
UINT WINAPI  OutputThread( LPVOID pParam );
UINT WINAPI  ClientThread( LPVOID pParam );
void         ParseInput( TCHAR* pBuffer );
VOID         SetupConsole( DWORD dwWindowSizeY );
VOID         RestoreOldConsoleSettings();
VOID         WriteLine( DWORD nCoordY, TCHAR* strBuffer );
void         DoPrompt( TCHAR* strPromptString, TCHAR* strBuffer );
void         ClearScreen();
BOOL         TryToConnect();
void         ReadConfig();
void         WriteConfig();
void         RunSetupWizard();
void         UpdateTimeStamp();
void         CreateTempLogFile();
VOID         SuspendPowerManagement();




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void __cdecl main( int argc, TCHAR* argv[] )
{
    HRESULT hr;

    UpdateTimeStamp();
    DXUtil_Timer( TIMER_START );
    DWORD dwSRand = (DWORD) (DXUtil_Timer( TIMER_GETABSOLUTETIME ) * UINT_MAX * (DWORD)GetCurrentThreadId() );
    srand( dwSRand );

    // Tell OS's that have power management to not 
    // sleep, since this app will be using the 
    // network connection and need very little user input
    SuspendPowerManagement();

    // Extract configuration settings from the registry
    ReadConfig();

	// Initialize COM
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    // Parse any command line options
    if( FALSE == ParseCommandLine( argc, argv ) )
        return;

    // Initalize DPlay client
    if( FAILED( hr = g_DP8Client.Init() ) )
    {
        DXTRACE_ERR( TEXT("Init"), hr );
        g_DP8Client.Shutdown();
        CoUninitialize();
        return;
    }

    // Initialize maze client object - basically just build the maze
    g_MazeClient.Init();

    // Create an event object to flag pending output messages
    g_hOutputEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    // Set a console control handler so we can clean 
    // up gracefully if we're forcibly shut down
    SetConsoleCtrlHandler( CtrlHandler, TRUE );
    g_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );

    g_bAllowConnect = g_Config.bAutoConnnect;

    // Spin up a thread to print the client output
    UINT dwConsoleThreadID;
    g_hConsoleThread = (HANDLE)_beginthreadex( NULL, 0, OutputThread, NULL, 0, &dwConsoleThreadID );

    // Spin up a thread to update the client output
    UINT dwClientThreadID;
    g_hClientThread = (HANDLE)_beginthreadex( NULL, 0, ClientThread, NULL, 0, &dwClientThreadID );

    ConsolePrintf( LINE_CMD, TEXT("MazeConsoleClient started.") );
    if( g_Config.bAutoConnnect )
    {
        ConsolePrintf( LINE_CMD, TEXT("Type 'AUTOCONNECT OFF' to stop automatically connecting upon startup.") );
        ConsolePrintf( LINE_CMD, TEXT("Type 'SETUP' to change current connections settings.") );
    }
    ConsolePrintf( LINE_CMD, TEXT("Type 'HELP' for a list of commands.") );
    
    // Set up the console
    SetConsoleMode( GetStdHandle(STD_INPUT_HANDLE), 
                    ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT );

    if( g_Config.bFileLogging )
        CreateTempLogFile();

    BOOL bHaveConnectSettings = g_Config.bAutoConnnect;

    // Loop around getting and dealing with keyboard input
    TCHAR buffer[512];
    while( !g_bQuitThread )
    {
        if( !bHaveConnectSettings )
        {
            // If Auto-Connnect is OFF, then prompt for where to connect to
            RunSetupWizard();

            if( g_Config.bFileLogging )
            {
                CreateTempLogFile();
            }
            else
            {
                CloseHandle( g_hOutputEvent );
                g_hOutputEvent = NULL;
            }

            g_bAllowConnect      = TRUE;
            bHaveConnectSettings = TRUE;
        }
        else
        {
            DoPrompt( TEXT("Command> "), buffer );
            ParseInput( buffer );
        }
    };

    // Wait for the threads to exit
    WaitForSingleObject( g_hClientThread, INFINITE );
    WaitForSingleObject( g_hConsoleThread, INFINITE );

    _tprintf( TEXT("Stopping...") );

    // Clean up
    CloseHandle( g_hOutputEvent );
    g_hOutputEvent = NULL;

	g_MazeClient.Shutdown();
	g_DP8Client.Shutdown();
    CoUninitialize();

    WriteConfig();

    RestoreOldConsoleSettings();
}





//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
UINT WINAPI ClientThread( LPVOID pParam )
{
    // Go into loopback mode to start
    HRESULT hr;
    BOOL bLocalLoopbackInitDone = TRUE;

    while ( 1 )
    {
        if( g_bQuitThread )
            break;

        FLOAT fTimeLapsed = DXUtil_Timer( TIMER_GETELAPSEDTIME );
        FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
        static FLOAT s_fLastConnect     = INT_MIN;
        static FLOAT s_fStartEnumTime   = INT_MIN;
        static FLOAT s_fStopEnumTime    = INT_MIN;
        static FLOAT s_fEnumStarted     = FALSE;

        // Update the time stamp
        g_fTimeStampUpdateCountdown -= fTimeLapsed;
        if( g_fTimeStampUpdateCountdown < 0.0f )
            UpdateTimeStamp();

        if( g_DP8Client.IsSessionLost() ) 
        {
            if( FALSE == bLocalLoopbackInitDone )
            {
                ConsolePrintf( LINE_LOG, TEXT("Disconnected from server") );

                // Disconnected, so retry in 10 seconds
                s_fStopEnumTime = fCurTime - g_Config.dwNetworkRetryDelay * 60.0f + 10.0f;

                if( g_DP8Client.GetSessionLostReason() == DISCONNNECT_REASON_CLIENT_OUT_OF_DATE )
                {
                    ConsolePrintf( LINE_LOG, TEXT("Disconnected because MazeConsoleClient is out of date.") );
                    ConsolePrintf( LINE_LOG, TEXT("Please get updated version") );
                    ConsolePrintf( LINE_LOG, TEXT("from http://msdn.microsoft.com/directx/") );
                    g_bOutOfDateClient = TRUE;
                }

                // Reset the maze client
                g_MazeClient.SetMazeReady( FALSE );
                g_MazeClient.LockWorld();
                g_MazeClient.Reset();
                g_MazeClient.UnlockWorld();

                // Now that the session is lost we need to 
                // restart DirectPlay by calling Close() 
                // and Init() on m_pDPlay
                g_DP8Client.Shutdown();
                g_DP8Client.Init();

                bLocalLoopbackInitDone = TRUE;
            }

            if( ( !s_fEnumStarted && fCurTime - s_fStopEnumTime > g_Config.dwNetworkRetryDelay * 60.0f || g_bConnectNow )
                && g_bAllowConnect )
            {
                g_bConnectNow = FALSE;

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

                if( SUCCEEDED(hr) )
                {
                    ConsolePrintf( LINE_LOG, TEXT("Starting DirectPlaying host enumeration") );

                    s_fStartEnumTime = fCurTime;
                    s_fEnumStarted = TRUE;
                }
                else
                {
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
                    g_bLocalLoopback    = FALSE;
                    s_fEnumStarted      = FALSE;
                    g_MazeClient.EngageAutopilot( TRUE );
                }

                s_fLastConnect = fCurTime;
            }

            g_bDisconnectNow = FALSE;
        }
        else
        {
            bLocalLoopbackInitDone = FALSE;

            // Update state of client
            g_MazeClient.Update( fTimeLapsed );

            if( g_Config.dwLogLevel > 1 )
            {
                // Display position every so often
                static float fLastPosUpdate = fCurTime;
                if( fCurTime - fLastPosUpdate > 10.0f )
                {
                    D3DXVECTOR3 vPos = g_MazeClient.GetCameraPos(); 
                    ConsolePrintf( LINE_LOG, TEXT("Position: (%5.1f,%5.1f), Players: %d"), 
                                  vPos.x, vPos.z, g_MazeClient.GetNumPlayers() );
                    fLastPosUpdate = fCurTime;
                }
            }

            // Display connection info every so often
            static float fLastLogUpdate = fCurTime;
            if( g_Config.dwAutoPrintStats > 0 && 
                fCurTime - fLastLogUpdate > g_Config.dwAutoPrintStats * 60.0f )
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

            // If we are testing connect/disconnect, break after so many iterations.
            if( g_Config.bAutoDisconnnect )  
		    {
                // Disconnect between 5-25seconds 
                static float fDisconnectCountdown = 10.0f;

                fDisconnectCountdown -= fTimeLapsed;
    			if(	fDisconnectCountdown < 0.0f )  
			    {
                    fDisconnectCountdown = (float)(rand() % 20000 + 5000 ) / 1000.0f;
    				ConsolePrintf( LINE_LOG, TEXT("Intentional disconnect.  Connecting again for %0.0f seconds..."), fDisconnectCountdown );
                    g_DP8Client.Shutdown();
            		g_MazeClient.Shutdown();
			    }
		    }

            if( g_bDisconnectNow )
            {
                g_bDisconnectNow = FALSE;
    			ConsolePrintf( LINE_LOG, TEXT("Intentional disconnect.") );
                g_DP8Client.Shutdown();
            	g_MazeClient.Shutdown();
            }
        }

        // Sleep for a little bit to avoid maxing the CPU
        Sleep( 30 );
    }

    return 0;
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL TryToConnect()
{
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
BOOL ParseCommandLine( int argc, TCHAR* argv[] )
{
    BOOL bParamFound = FALSE;

    for ( int i = 1 ; i < argc ; i++ )
    {
        const TCHAR* arg = argv[i];

        if( arg[0] == '/' || arg[0] == '-' )
        {
            if( _stricmp( arg+1, TEXT("m") ) == 0 )
			{
				g_Config.bConnectToMicrosoftSite = TRUE;
				g_Config.bConnectToLocalServer   = FALSE;
				g_Config.bConnectToRemoteServer  = FALSE;
                bParamFound = TRUE;
			}
            else if( _stricmp( arg+1, TEXT("l") ) == 0 )
			{
				g_Config.bConnectToMicrosoftSite = FALSE;
				g_Config.bConnectToLocalServer   = TRUE;
				g_Config.bConnectToRemoteServer  = FALSE;
                bParamFound = TRUE;
			}
            else if( arg[1] == TEXT('r') || arg[1] == TEXT('R') )
			{
				g_Config.bConnectToMicrosoftSite = FALSE;
				g_Config.bConnectToLocalServer   = FALSE;
				g_Config.bConnectToRemoteServer  = TRUE;
                bParamFound = TRUE;

                if( arg[2] == TEXT(':') )
                {
                    strcpy( g_Config.szIPAddress, arg+3 );
                }
			}	
            else if( _stricmp( arg+1, TEXT("d") ) == 0 || 
                _stricmp( arg+1, TEXT("d+") ) == 0 )
			{
				g_Config.bAutoDisconnnect = TRUE;
                bParamFound = TRUE;
			}	
            else if( _stricmp( arg+1, TEXT("d-") ) == 0 )
			{
				g_Config.bAutoDisconnnect = FALSE;
                bParamFound = TRUE;
			}	
			if( _stricmp( arg+1, TEXT("f") ) == 0 || 
                _stricmp( arg+1, TEXT("f+") ) == 0 )
			{
				g_Config.bFileLogging = TRUE;
                bParamFound = TRUE;
			}	
            else if( _stricmp( arg+1, TEXT("f-") ) == 0 )
			{
				g_Config.bFileLogging = FALSE;
                bParamFound = TRUE;
			}
            else if( _strnicmp( arg+1, TEXT("log:"), 2 ) == 0 )
			{
				g_Config.dwLogLevel = atoi( arg+5 );
                bParamFound = TRUE;
			}
            else if( _stricmp( arg+1, TEXT("?") ) == 0 )
			{
                printf( "MazeConsoleClient\n" );
                printf( "  Format: MazeConsoleClient.exe [/M] [/L] [/R[:ipaddress]] [/D] [/F] [/LOG]\n" );
                printf( "\n" );
                printf( "  Options:\n" );
                printf( "           /M  : connects to Microsoft server\n" );
                printf( "           /L  : connects to local server\n" );
                printf( "           /R[:ipaddress] : connects to remote server at [ipaddress]\n" );
                printf( "           /D+ : turns on auto-disconnecting\n" );
                printf( "           /D- : turns off auto-disconnecting\n" );
                printf( "           /F+ : turns on file logging\n" );
                printf( "           /F- : turns off file logging\n" );
                printf( "           /LOG:n : sets the log level to n\n" );
                printf( "\n" );
                printf( "  Examples: \n" );
                printf( "       MazeConsoleClient /F /LOG:1\n" );
                printf( "       MazeConsoleClient /M /D+\n" );
                printf( "       MazeConsoleClient /R:myserver.myip.com /F-\n" );
                return FALSE;
			}	
		}
    }

    if( bParamFound )
    {
        g_Config.bAutoConnnect = TRUE;
        g_bSaveSettings        = FALSE;
    }

    return TRUE;
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
        g_enumBufferType[g_dwNextFreeOutput] = enumBufferType;
    
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
BOOL WINAPI CtrlHandler( DWORD type )
{
    switch ( type )
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            // Signal thread to quit
            g_bQuitThread = TRUE;
            SetEvent( g_hOutputEvent );
            WaitForSingleObject( g_hConsoleThread, INFINITE );
            return TRUE;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
UINT WINAPI OutputThread( LPVOID pParam )
{
#define MAX_CMD_LINES 100
#define MAX_LOG_LINES 100

    DWORD i;
    TCHAR szLogBuffer[MAX_LOG_LINES][256];
    TCHAR szCmdBuffer[MAX_CMD_LINES][256];
    DWORD dwLogBufferNext = 0;
    DWORD dwCmdBufferNext = 0;
    DWORD dwLastPrompt    = 0;
    DWORD dwCoordY;
    int   dwCaretPos      = 9;

    ZeroMemory( szCmdBuffer, sizeof(TCHAR)*MAX_CMD_LINES*256 );
    ZeroMemory( szLogBuffer, sizeof(TCHAR)*MAX_LOG_LINES*256 );
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    GetConsoleScreenBufferInfo( g_hStdOut, &g_SavedConsoleInfo );

    if( g_SavedConsoleInfo.dwMaximumWindowSize.Y > 30 )
        g_dwWindowSizeY = 30;
    else
        g_dwWindowSizeY = g_SavedConsoleInfo.dwMaximumWindowSize.Y;

    g_dwSeperatorLine = g_dwWindowSizeY - g_dwNumCmdLines - 2;
    g_dwNumLogLines   = g_dwWindowSizeY - g_dwNumCmdLines - 3;
        
    SetupConsole( g_dwWindowSizeY );
    ClearScreen();

    while ( 1 )
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
            switch( g_enumBufferType[g_dwNextOutput] )
            {
                case LINE_LOG:
                {
                    // Add g_szOutputBuffer[g_dwNextOutput] to szLogBuffer array,
                    // and redisplay the array on the top half of the screen
                    _stprintf( szLogBuffer[dwLogBufferNext], "%s %s", 
                               g_strTimeStamp, g_szOutputBuffer[g_dwNextOutput] );

#ifdef _DEBUG
                    OutputDebugString( szLogBuffer[dwLogBufferNext] );
                    OutputDebugString( "\n" );
#endif
                    if( g_hLogFile )
                    {
                        DWORD dwWritten;
                        WriteFile( g_hLogFile, szLogBuffer[dwLogBufferNext], 
                                   lstrlen( szLogBuffer[dwLogBufferNext] ), &dwWritten, NULL );
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

                    dwLogBufferNext++;
                    dwLogBufferNext %= g_dwNumLogLines;
                    dwCoordY = 0;

                    for( i=dwLogBufferNext; i<g_dwNumLogLines; i++ )
                    {
                        dwCoordY++;
                        WriteLine( dwCoordY, szLogBuffer[i] );
                    }

                    for( i=0; i<dwLogBufferNext; i++ )
                    {
                        dwCoordY++;
                        WriteLine( dwCoordY, szLogBuffer[i] );
                    }
                    break;
                }

                case LINE_PROMPT:
                case LINE_CMD:
                {
                    // Add g_szOutputBuffer[g_dwNextOutput] to szCmdBuffer array,
                    // and redisplay the array on the top half of the screen
                    _tcscpy( szCmdBuffer[dwCmdBufferNext], g_szOutputBuffer[g_dwNextOutput] );

#ifdef _DEBUG
                    if( g_enumBufferType[g_dwNextOutput] != LINE_PROMPT )
                    {
                        OutputDebugString( szCmdBuffer[dwCmdBufferNext] );
                        OutputDebugString( "\n" );
                    }
#endif

                    if( g_enumBufferType[g_dwNextOutput] == LINE_PROMPT )
                    {
                        dwLastPrompt = dwCmdBufferNext;
                        dwCaretPos = _tcslen( szCmdBuffer[dwCmdBufferNext] );
                    }

                    dwCmdBufferNext++;
                    dwCmdBufferNext %= g_dwNumCmdLines;
                    dwCoordY = g_dwSeperatorLine;

                    for( i=dwCmdBufferNext; i<g_dwNumCmdLines; i++ )
                    {
                        dwCoordY++;
                        WriteLine( dwCoordY, szCmdBuffer[i] );
                    }

                    for( i=0; i<dwCmdBufferNext; i++ )
                    {
                        dwCoordY++;
                        WriteLine( dwCoordY, szCmdBuffer[i] );
                    }
                    break;
                }

                case LINE_INPUT:
                {
                    // Update the last prompt line in the szCmdBuffer array with this
                    // string of input, so what was typed in is displayed as it scrolls
                    _tcscpy( &szCmdBuffer[dwLastPrompt][dwCaretPos], g_szOutputBuffer[g_dwNextOutput] );

#ifdef _DEBUG
                    OutputDebugString( szCmdBuffer[dwLastPrompt] );
                    OutputDebugString( "\n" );
#endif

                    break;
                }
            }

            if( g_enumBufferType[g_dwNextOutput] == LINE_PROMPT )
            {                 
                // Reset the cursor position if this is a cmd prompt line
                COORD coord = { (WORD)dwCaretPos, (WORD)g_dwWindowSizeY-2 };
                SetConsoleCursorPosition( g_hStdOut, coord );
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
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = NULL;
    }

    return 0;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID SetupConsole( DWORD dwWindowSizeY )
{
    static TCHAR strEmpty[255] = TEXT("                                                                                                                                                                                                                                                              ");
    DWORD dwWritten;
    COORD coord = { 0, 0 };
    SMALL_RECT rcWindow = { 0, 0, 79, (WORD)dwWindowSizeY-1 };

    SetConsoleWindowInfo( g_hStdOut, TRUE, &rcWindow );

    COORD crdBufferSize;
    crdBufferSize.X = 80;
    crdBufferSize.Y = (WORD)dwWindowSizeY;
    SetConsoleScreenBufferSize( g_hStdOut, crdBufferSize );

    // Write a blank string first
    for( int i=rcWindow.Top; i<rcWindow.Bottom; i++ )
    {
        coord.Y = (WORD)i;
        SetConsoleCursorPosition( g_hStdOut, coord );
        WriteConsole( g_hStdOut, strEmpty, rcWindow.Right + 1, &dwWritten, NULL );
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID RestoreOldConsoleSettings()
{
    static TCHAR strEmpty[255] = TEXT("                                                                                                                                                                                                                                                              ");
    DWORD dwWritten;
    COORD coord = { 0, 0 };

    SetConsoleScreenBufferSize( g_hStdOut, g_SavedConsoleInfo.dwSize );
    SetConsoleWindowInfo( g_hStdOut, TRUE, &g_SavedConsoleInfo.srWindow );

    // Write a blank string first
    for( int i=g_SavedConsoleInfo.srWindow.Top; 
         i<g_SavedConsoleInfo.srWindow.Bottom;
         i++ )
    {
        coord.Y = (WORD)i;
        SetConsoleCursorPosition( g_hStdOut, coord );
        WriteConsole( g_hStdOut, strEmpty, g_SavedConsoleInfo.srWindow.Right + 1, &dwWritten, NULL );
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID WriteLine( DWORD nCoordY, TCHAR* strBuffer )
{
    // Write blanks to make all strings 80 TCHARs long so that
    // the old text is erased as this one is displayed
    for( DWORD dwIndex = _tcslen(strBuffer); dwIndex<80; dwIndex++ )
        strBuffer[dwIndex] = ' ';
    strBuffer[dwIndex] = 0;

    // Write strBuffer at (0,nCoordY)
    DWORD dwWritten;
    COORD coord = { 0, (WORD) nCoordY };
    WriteConsoleOutputCharacter( g_hStdOut, strBuffer, 80, coord, &dwWritten ); 
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void PrintHelp()
{
    ConsolePrintf( LINE_CMD, TEXT("Commands:") );
    ConsolePrintf( LINE_CMD, TEXT("    SETUP, EXIT, LOGLEVEL, CONNECTIONINFO") ); 
    ConsolePrintf( LINE_CMD, TEXT("    FILELOG, CONNECT, DISCONNECT") );
    ConsolePrintf( LINE_CMD, TEXT("    AUTOCONNECT, AUTODISCONNECT") );
    ConsolePrintf( LINE_CMD, TEXT("    SETSTATRATE, SETRETRYDELAY, ") );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void ClearScreen()
{
    static TCHAR strEmpty[255] = TEXT("                                                                                                                                                                                                                                                              ");
    DWORD dwWritten;
    COORD coord = { 0, 0 };

    // Write a blank string first
    for( DWORD i=0; i<g_dwWindowSizeY; i++ )
    {
        coord.Y = (WORD)i;
        SetConsoleCursorPosition( g_hStdOut, coord );
        WriteConsole( g_hStdOut, strEmpty, g_SavedConsoleInfo.srWindow.Right + 1, &dwWritten, NULL );
    }

    // Display a seperator between the two areas of the console window
    TCHAR strBuffer[200];
    _tcscpy( strBuffer, TEXT("-------------------------------------------------------------------------------") );
    WriteLine( g_dwSeperatorLine, strBuffer );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void DoPrompt( TCHAR* strPromptString, TCHAR* strBuffer )
{
    ConsolePrintf( LINE_PROMPT, strPromptString );
    DWORD dwRead;
    ReadConsole( GetStdHandle(STD_INPUT_HANDLE), strBuffer, 128, &dwRead, NULL );
    strBuffer[dwRead-2]=0;
    ConsolePrintf( LINE_INPUT, strBuffer );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void RunSetupWizard()
{
    BOOL bWrongToken;
    TCHAR buffer[512];

    g_Config.bConnectToMicrosoftSite = FALSE;
    g_Config.bConnectToLocalServer   = FALSE;
    g_Config.bConnectToRemoteServer  = FALSE;

    bWrongToken = TRUE;
    while( bWrongToken ) 
    {
        ConsolePrintf( LINE_CMD, TEXT("") );
        ConsolePrintf( LINE_CMD, TEXT("Do you want to connect to DirectPlayMaze.rte.microsoft.com? (YES/NO)") );
        DoPrompt( TEXT("(Default:YES) > "), buffer );

        TCHAR* token = _tcstok( buffer, TEXT(" \t") );
        if( token != NULL )
        {
            _tcsupr( token );
            if( !_tcscmp( token, TEXT("Y") ) || !_tcscmp( token, TEXT("YES") ) )
            {
                g_Config.bConnectToMicrosoftSite = TRUE;
                bWrongToken = FALSE;
            }
            else if( !_tcscmp( token, TEXT("N") ) || !_tcscmp( token, TEXT("NO") ) )
            {
                bWrongToken = FALSE;
            }
        }
        else
        {
            g_Config.bConnectToMicrosoftSite = TRUE;
            bWrongToken = FALSE;
        }
    }

    if( g_Config.bConnectToMicrosoftSite == FALSE )
    {
        bWrongToken = TRUE;
        while( bWrongToken ) 
        {
            ConsolePrintf( LINE_CMD, TEXT("") );
            ConsolePrintf( LINE_CMD, TEXT("Do you want to connect to a local server (searches local subnet)? (YES/NO)") );
            DoPrompt( TEXT("(Default:YES) > "), buffer );

            TCHAR* token = _tcstok( buffer, TEXT(" \t") );
            if( token != NULL )
            {
                _tcsupr( token );
                if( !_tcscmp( token, TEXT("Y") ) || !_tcscmp( token, TEXT("YES") ) )
                {
                    g_Config.bConnectToLocalServer   = TRUE;
                    bWrongToken = FALSE;
                }
                else if( !_tcscmp( token, TEXT("N") ) || !_tcscmp( token, TEXT("NO") ) )
                {
                    bWrongToken = FALSE;
                }
            }
            else
            {
                g_Config.bConnectToLocalServer   = TRUE;
                bWrongToken = FALSE;
            }
        }

        if( g_Config.bConnectToLocalServer == FALSE )
        {
            bWrongToken = TRUE;
            while( bWrongToken ) 
            {
                ConsolePrintf( LINE_CMD, TEXT("") );
                ConsolePrintf( LINE_CMD, TEXT("What IP address do you want to connect to? ") );
                DoPrompt( TEXT("> "), buffer );

                TCHAR* token = _tcstok( buffer, TEXT(" \t") );
                if( token != NULL )
                {
                    _tcscpy( g_Config.szIPAddress, token );
                    g_Config.bConnectToRemoteServer = TRUE;
                    bWrongToken = FALSE;
                }
            }
        }
    }

    bWrongToken = TRUE;
    while( bWrongToken ) 
    {
        ConsolePrintf( LINE_CMD, TEXT("") );
        ConsolePrintf( LINE_CMD, TEXT("Turn file logging on? (YES/NO)") );
        DoPrompt( TEXT("(Default:YES) > "), buffer );

        TCHAR* token = _tcstok( buffer, TEXT(" \t") );
        if( token != NULL )
        {
            _tcsupr( token );
            if( !_tcscmp( token, TEXT("Y") ) || !_tcscmp( token, TEXT("YES") ) )
            {
                g_Config.bFileLogging = TRUE;
                bWrongToken = FALSE;
            }
            else if( !_tcscmp( token, TEXT("N") ) || !_tcscmp( token, TEXT("NO") ) )
            {
                g_Config.bFileLogging = FALSE;
                bWrongToken = FALSE;
            }
        }
        else
        {
            g_Config.bFileLogging = TRUE;
            bWrongToken = FALSE;
        }
    }

    bWrongToken = TRUE;
    while( bWrongToken ) 
    {
        ConsolePrintf( LINE_CMD, TEXT("") );
        ConsolePrintf( LINE_CMD, TEXT("Use these settings when disconnected and when the app starts? (YES/NO)") );
        DoPrompt( TEXT("(Default:YES) > "), buffer );

        TCHAR* token = _tcstok( buffer, TEXT(" \t") );
        if( token != NULL )
        {
            _tcsupr( token );
            if( !_tcscmp( token, TEXT("Y") ) || !_tcscmp( token, TEXT("YES") ) )
            {
                g_Config.bAutoConnnect = TRUE;
                bWrongToken = FALSE;
            }
            else if( !_tcscmp( token, TEXT("N") ) || !_tcscmp( token, TEXT("NO") ) )
            {
                g_Config.bAutoConnnect = FALSE;
                bWrongToken = FALSE;
            }
        }
        else
        {
            g_Config.bAutoConnnect = TRUE;
            bWrongToken = FALSE;
        }
    }

    ConsolePrintf( LINE_CMD, TEXT("") );
    ConsolePrintf( LINE_CMD, TEXT("Type 'SETUP' anytime to change these connections settings.") );
    ConsolePrintf( LINE_CMD, TEXT("Type 'HELP' for a list of commands.") );
    ConsolePrintf( LINE_CMD, TEXT("") );

    g_bSaveSettings = TRUE;
    WriteConfig();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void ParseInput( TCHAR* buffer )
{
    // Strip first token from the buffer and pull to upper case
    TCHAR* token = _tcstok( buffer, TEXT(" \t") );
    if( token == NULL )
        return;

    _tcsupr( token );

    // See what it is and act accordingly
    if( !_tcscmp( token, TEXT("STOP") ) || 
        !_tcscmp( token, TEXT("QUIT") ) || 
        !_tcscmp( token, TEXT("EXIT") ) )
    {
        g_bQuitThread = TRUE;
        SetEvent( g_hOutputEvent );
    } 
    else if( !_tcscmp( token, TEXT("CONNECT") ) )
    {
        g_bConnectNow = TRUE;
    } 
    else if( !_tcscmp( token, TEXT("DISCONNECT") ) )
    {
        g_bDisconnectNow = TRUE;
    } 
    else if( !_tcscmp( token, TEXT("AUTOCONNECT") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            _tcsupr( token );
            if( !_tcscmp( token, TEXT("ON") ) )
                g_Config.bAutoConnnect = TRUE;
            else if( !_tcscmp( token, TEXT("OFF") ) )
                g_Config.bAutoConnnect = FALSE;
            WriteConfig();
        }

        ConsolePrintf( LINE_CMD, TEXT("Auto-Connect set to %s"), (g_Config.bAutoConnnect) ? TEXT("ON") : TEXT("OFF") );
        ConsolePrintf( LINE_CMD, TEXT("  If ON, MazeConsoleClient connects automatically reconnects") );
        ConsolePrintf( LINE_CMD, TEXT("         when disconnected, and connects when loading app.") );
    } 
    else if( !_tcscmp( token, TEXT("SETRETRYDELAY") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD dwDelay = _ttol( token );
            if( g_Config.dwNetworkRetryDelay < 1 || g_Config.dwNetworkRetryDelay > 300 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 1 and 300") );
            }
            else
            {           
                g_Config.dwNetworkRetryDelay = dwDelay;
            }
        }

        ConsolePrintf( LINE_CMD, TEXT("Retry Delay set to %d"), g_Config.dwNetworkRetryDelay );
        ConsolePrintf( LINE_CMD, TEXT("  Sets how long the app waits between attempts to connect to the server.") );
    }     
    else if( !_tcscmp( token, TEXT("LOGLEVEL") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD dwLevel = _ttol( token );
            if( dwLevel > 3 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 0 and 3") );
            }
            else
            {
                g_Config.dwLogLevel = dwLevel;
            }
        }

        ConsolePrintf( LINE_CMD, TEXT("Log level set to %d"), g_Config.dwLogLevel );
        ConsolePrintf( LINE_CMD, TEXT("     Level 1: No client position ") );
        ConsolePrintf( LINE_CMD, TEXT("     Level 2: Client position every 10 seconds") );
    }
    else if( !_tcscmp( token, TEXT("SETSTATRATE") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD dwLevel = _ttol( token );
            g_Config.dwAutoPrintStats = dwLevel;
        }

        if( g_Config.dwAutoPrintStats != 0 )
            ConsolePrintf( LINE_CMD, TEXT("Auto-displaying stats every %d mins"), g_Config.dwAutoPrintStats );
        else
            ConsolePrintf( LINE_CMD, TEXT("Not auto-displaying stats") );

        ConsolePrintf( LINE_CMD, TEXT("     Controls how often the connection stats are auto-displayed") );
    }
    else if( !_tcscmp( token, TEXT("SETUP") ) )
    {
        g_bDisconnectNow = TRUE;
        RunSetupWizard();

        if( g_Config.bFileLogging )
        {
            CreateTempLogFile();
        }
        else
        {
            CloseHandle( g_hOutputEvent );
            g_hOutputEvent = NULL;
        }
    }
    else if( !_tcscmp( token, TEXT("FILELOG") ) )
    {
        ConsolePrintf( LINE_LOG, TEXT("Log Directory: '%s'"), g_strLogDir );
        ConsolePrintf( LINE_LOG, TEXT("Logging to temp file: '%s'"), g_strLogFile );
    }
    else if( !_tcscmp( token, TEXT("AUTODISCONNECT") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            _tcsupr( token );
            if( !_tcscmp( token, TEXT("ON") ) )
                g_Config.bAutoDisconnnect = TRUE;
            else if( !_tcscmp( token, TEXT("OFF") ) )
                g_Config.bAutoDisconnnect = FALSE;
            WriteConfig();
        }

        ConsolePrintf( LINE_CMD, TEXT("Auto-Disconnect set to %s"), (g_Config.bAutoDisconnnect) ? TEXT("ON") : TEXT("OFF") );
        ConsolePrintf( LINE_CMD, TEXT("  If ON, app disconnects randomly") );
    } 
    else if( !_tcscmp( token, TEXT("CONNECTIONINFO") ) || !_tcscmp( token, TEXT("CI") ) )
    {
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
    }
    else if( !_tcscmp( token, TEXT("HELP") ) || !_tcscmp( token, TEXT("?") ) )
    {
        PrintHelp();
    }
    else if( !_tcscmp( token, TEXT("CLS") ) )
    {
        ClearScreen();
    }
    else
    {
        ConsolePrintf( LINE_CMD, TEXT("Unknown command. Type HELP for list of commands") );
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void ReadConfig()
{
    HKEY hKey = NULL;
    RegOpenKeyEx( HKEY_CURRENT_USER, g_szKeyname, 0, KEY_READ, &hKey );

    DXUtil_ReadBoolRegKey( hKey, TEXT("ConnectToMicrosoftSite"), &g_Config.bConnectToMicrosoftSite, TRUE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("ConnectToLocalServer"), &g_Config.bConnectToLocalServer, FALSE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("ConnectToRemoteServer"), &g_Config.bConnectToRemoteServer, FALSE );
    DXUtil_ReadIntRegKey(  hKey, TEXT("NetworkRetryDelay"), &g_Config.dwNetworkRetryDelay, 30 );
    DXUtil_ReadStringRegKey( hKey, TEXT("IPAddress"), g_Config.szIPAddress, sizeof(g_Config.szIPAddress), TEXT("\0") );
    DXUtil_ReadBoolRegKey( hKey, TEXT("AutoDisconnnect"), &g_Config.bAutoDisconnnect, FALSE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("AutoConnnect"), &g_Config.bAutoConnnect, FALSE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("FileLogging"), &g_Config.bFileLogging, TRUE );
    DXUtil_ReadIntRegKey( hKey, TEXT("LogLevel"), &g_Config.dwLogLevel, 2 );
    DXUtil_ReadIntRegKey( hKey, TEXT("AutoPrintStats"), &g_Config.dwAutoPrintStats, 10 );

    RegCloseKey( hKey );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void WriteConfig()
{
    HKEY    hKey;
    DWORD   dwDisposition;
    
    if( !g_bSaveSettings )
        return;

    RegCreateKeyEx( HKEY_CURRENT_USER, g_szKeyname, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                    &hKey, &dwDisposition );

    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToMicrosoftSite"), g_Config.bConnectToMicrosoftSite );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToLocalServer"), g_Config.bConnectToLocalServer );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToRemoteServer"), g_Config.bConnectToRemoteServer );
    DXUtil_WriteIntRegKey(  hKey, TEXT("NetworkRetryDelay"), g_Config.dwNetworkRetryDelay );
    DXUtil_WriteStringRegKey( hKey, TEXT("IPAddress"), g_Config.szIPAddress );
    DXUtil_WriteBoolRegKey( hKey, TEXT("AutoDisconnnect"), g_Config.bAutoDisconnnect );
    DXUtil_WriteBoolRegKey( hKey, TEXT("AutoConnnect"), g_Config.bAutoConnnect );
    DXUtil_WriteBoolRegKey( hKey, TEXT("FileLogging"), g_Config.bFileLogging );
    DXUtil_WriteIntRegKey( hKey, TEXT("LogLevel"), g_Config.dwLogLevel );
    DXUtil_WriteIntRegKey( hKey, TEXT("AutoPrintStats"), g_Config.dwAutoPrintStats );
    
    RegCloseKey( hKey );
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
void CreateTempLogFile()
{
    BOOL bSuccess;
    TCHAR strTempFileName[MAX_PATH];
    TCHAR strTime[MAX_PATH];
    DWORD dwCount;
    
    GetTempPath( MAX_PATH, g_strLogDir );
    lstrcat( g_strLogDir, TEXT("DirectPlayMaze\\") );

    // Create the directory if it doesn't exist
    if( GetFileAttributes( g_strLogDir ) == -1 )
    {
        bSuccess = CreateDirectory( g_strLogDir, NULL );
        if( !bSuccess )
        {
            ConsolePrintf( LINE_LOG, TEXT("Could not create create temp directory '%s'"), g_strLogDir );
            goto LFail;
        }
    }

    ConsolePrintf( LINE_LOG, TEXT("Log Directory: '%s'"), g_strLogDir );

    SYSTEMTIME sysTime;
    GetLocalTime( &sysTime );
    _stprintf( strTime, TEXT("client-%04d-%02d-%02d-"),
               sysTime.wYear, sysTime.wMonth, sysTime.wDay );

    dwCount = 0;

    while(TRUE)
    {
        wsprintf( g_strLogFile, "%s%05d.log", strTime, dwCount );
        lstrcpy( strTempFileName, g_strLogDir );
        lstrcat( strTempFileName, g_strLogFile );
        DWORD dwResult = GetFileAttributes( strTempFileName );
        if( dwResult == -1 )
            break;

        dwCount++;
    }

    if( g_hLogFile )
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = NULL;
    }

    g_hLogFile = CreateFile( strTempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                             CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    if( g_hLogFile == INVALID_HANDLE_VALUE )
    {
        ConsolePrintf( LINE_LOG, TEXT("Could not create create temp file '%s'"), strTempFileName );
        goto LFail;
    }

    ConsolePrintf( LINE_LOG, TEXT("Logging to temp file: '%s'"), g_strLogFile );
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
