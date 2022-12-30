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
#include <objbase.h>
#include <process.h>
#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <tchar.h>
#include <DXErr8.h>
#include "DXUtil.h"

#include "idpserver.h"
#include "server.h"
#include "Maze.h"
#include "MazeServer.h"
#include "MazeClient.h"




//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define MAX_OUTPUT_QUEUE    512




//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
CDPlay8Server*      g_pDPlay8Server = NULL;
CMazeServer         g_MazeServer;
CMaze               g_Maze;
HANDLE              g_hOutputThread;
HANDLE              g_hUpdateThread;
volatile BOOL       g_bQuitThread;
HANDLE              g_hOutputEvent;
TCHAR               g_szOutputBuffer[MAX_OUTPUT_QUEUE][256];
CONSOLE_SCREEN_BUFFER_INFO g_SavedConsoleInfo;
EnumBufferType      g_enumBufferType[MAX_OUTPUT_QUEUE];
DWORD               g_dwNextOutput      = 0;
DWORD               g_dwNextFreeOutput  = 0;
DWORD               g_dwQueueSize       = 0;
CCriticalSection    g_OutputQueueLock;
HANDLE              g_hStdOut           = NULL;
DWORD               g_dwNumCmdLines     = 11;
DWORD               g_dwSeperatorLine;
DWORD               g_dwNumLogLines;
DWORD               g_dwWindowSizeY;
BOOL                g_bLocalLoopback    = FALSE;
DWORD               g_dwWidth           = DEFAULT_MAZE_WIDTH;
DWORD               g_dwHeight          = DEFAULT_MAZE_HEIGHT;
TCHAR               g_strTimeStamp[50];
FLOAT               g_fAllInfoRate      = 15.0f;
HANDLE              g_hLogFile          = NULL;
BOOL                g_bFileLogging      = TRUE;  // Change this to turn file logging off




//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
HRESULT      ParseCommandLine( int argc, TCHAR* argv[] );
BOOL WINAPI  CtrlHandler( DWORD type );
UINT WINAPI  OutputThread( LPVOID pParam );
UINT WINAPI  UpdateThread( LPVOID pParam );
void         ParseInput( TCHAR* pBuffer );
VOID         SetupConsole( DWORD dwWindowSizeY );
VOID         RestoreOldConsoleSettings();
VOID         WriteLine( DWORD nCoordY, TCHAR* strBuffer );
void         ClearScreen();
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

    DXUtil_Timer( TIMER_START );
    srand( (DWORD) (DXUtil_Timer( TIMER_GETABSOLUTETIME ) * UINT_MAX) );

    // Tell OS's that have power management to not 
    // sleep, since this app will be using the 
    // network connection and need very little user input
    SuspendPowerManagement();

    // Parse any command line options
    if( FAILED( ParseCommandLine( argc, argv ) ) )
        return;

    // Initialize COM
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    // Create an event object to flag pending output messages
    g_hOutputEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( g_bFileLogging )
        CreateTempLogFile();

    // Initalize maze and server objects
    if( FAILED( hr = g_Maze.Init( g_dwWidth, g_dwHeight, DEFAULT_SEED ) ) )
    {
        DXTRACE_ERR( TEXT("g_Maze.Init"), hr );
        goto LCleanup;
    }

    if( FAILED( hr = g_MazeServer.Init( &g_Maze ) ) )
    {
        DXTRACE_ERR( TEXT("g_MazeServer.Init"), hr );
        goto LCleanup;
    }

    g_pDPlay8Server = new CDPlay8Server();
    if( g_pDPlay8Server == NULL )
    {
        DXTRACE_ERR( TEXT("new"), E_OUTOFMEMORY );
        goto LCleanup;
    }
    
    // Connect maze server module to DP server
    g_pDPlay8Server->SetServer( &g_MazeServer );
    g_MazeServer.SetOutboundServer( g_pDPlay8Server );

    // Set a console control handler so we can clean 
    // up gracefully if we're forcibly shut down
    SetConsoleCtrlHandler( CtrlHandler, TRUE );
    g_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );

    // Start up the DP server
    if( FAILED( hr = g_pDPlay8Server->Start() ) )
    {
        DXTRACE_ERR( TEXT("Start"), hr );
        g_pDPlay8Server->Shutdown();
        goto LCleanup;
    }

    // Spin up a thread to print server output
    UINT dwOutputThreadID;
    g_hOutputThread = (HANDLE)_beginthreadex( NULL, 0, OutputThread, NULL, 0, &dwOutputThreadID );

    // Spin up a thread to update output
    UINT dwUpdateThreadID;
    g_hUpdateThread = (HANDLE)_beginthreadex( NULL, 0, UpdateThread, NULL, 0, &dwUpdateThreadID );

    ConsolePrintf( LINE_CMD, TEXT("DirectPlay8 Server started") );
    ConsolePrintf( LINE_CMD, TEXT("Type 'HELP' for a list of commands.") );

    // Set up the console
    SetConsoleMode( GetStdHandle(STD_INPUT_HANDLE), 
                    ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT );

    // Loop around getting and dealing with keyboard input
    TCHAR buffer[512];
    while( !g_bQuitThread )
    {
        ConsolePrintf( LINE_PROMPT, TEXT("Command> ") );
        DWORD dwRead;
        ReadConsole( GetStdHandle(STD_INPUT_HANDLE), buffer, 128, &dwRead, NULL );
        buffer[dwRead-2]=0;
        ConsolePrintf( LINE_INPUT, buffer );
        ParseInput( buffer );
    };

    // Wait for the output thread to exit
    WaitForSingleObject( g_hOutputThread, INFINITE );
    WaitForSingleObject( g_hUpdateThread, INFINITE );
    
    // Shutdown maze and server objects
    g_MazeServer.Shutdown();
    g_Maze.Empty();

    // Free event object
    CloseHandle( g_hOutputEvent );

LCleanup:
    SAFE_DELETE( g_pDPlay8Server );

    _tprintf( TEXT("Done!\n") );

    // Uninitialize COM
    CoUninitialize();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT ParseCommandLine( int argc, TCHAR* argv[] )
{
    for ( int i = 1 ; i < argc ; i++ )
    {
        const TCHAR* strArg  = argv[i];
        if( strArg[0] == '/' || strArg[0] == '-' )
        {
            if( _tcsicmp( strArg+1, TEXT("size") ) == 0 )
            {
                if( i + 2 < argc )
                {
                    const TCHAR* strWidth  = argv[i+1];
                    const TCHAR* strHeight = argv[i+2];

                    g_dwWidth  = _ttoi( strWidth );
                    g_dwHeight = _ttoi( strHeight );
                }
            }
        }
    }

    return S_OK;
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
            WaitForSingleObject( g_hOutputThread, INFINITE );
            return TRUE;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
UINT WINAPI UpdateThread( LPVOID pParam )
{
    while( 1 )
    {
        if( g_bQuitThread )
            break;

        FLOAT fTimeLapsed = DXUtil_Timer( TIMER_GETELAPSEDTIME );
        FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
        static FLOAT s_fLastAllInfoDisplay = fCurTime;

        if( g_fAllInfoRate != 0.0f && fCurTime - s_fLastAllInfoDisplay > g_fAllInfoRate * 60.0f )
        {
            g_MazeServer.DisplayAllInfo();           
            s_fLastAllInfoDisplay = fCurTime;
        }

        // Sleep for a little bit to avoid maxing the CPU
        Sleep( 100 );
    }

    return 0;
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
    DWORD dwLastPrompt = 0;
    DWORD dwCoordY;
    int   dwCaretPos      = 9;

    ZeroMemory( szCmdBuffer, sizeof(TCHAR)*MAX_CMD_LINES*256 );
    ZeroMemory( szLogBuffer, sizeof(TCHAR)*MAX_LOG_LINES*256 );
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    GetConsoleScreenBufferInfo( g_hStdOut, &g_SavedConsoleInfo );

    if( g_SavedConsoleInfo.dwMaximumWindowSize.Y > 50 )
        g_dwWindowSizeY = 50;
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

        UpdateTimeStamp();

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

    _tprintf( TEXT("Stopping...") );

    // Kill server (if we had one)
    if( g_pDPlay8Server )
        g_pDPlay8Server->Shutdown();

    RestoreOldConsoleSettings();

    if( g_hLogFile )
        CloseHandle( g_hLogFile );

    CoUninitialize();
    
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
    ConsolePrintf( LINE_CMD, TEXT("    STOP, LOGLEVEL, CLIENTUPDATE") );
    ConsolePrintf( LINE_CMD, TEXT("    SERVERRELIABLE, CLIENTRELIABLE") );
    ConsolePrintf( LINE_CMD, TEXT("    SERVERTIMEOUT, CLIENTTIMEOUT") );
    ConsolePrintf( LINE_CMD, TEXT("    CONNECTIONINFO, STATS, ALLINFO, ALLINFORATE") );
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
void ParseInput( TCHAR* buffer )
{
    // Strip first token from the buffer and pull to upper case
    TCHAR* token = _tcstok( buffer, TEXT(" \t") );
    if( token == NULL )
        return;

    _tcsupr( token );

    // See what it is and act accordingly
    if( !_tcscmp( token, TEXT("STATS") ) )
    {
        g_MazeServer.PrintStats();
    }
    else if( !_tcscmp( token, TEXT("STOP") ) || 
             !_tcscmp( token, TEXT("QUIT") ) || 
             !_tcscmp( token, TEXT("EXIT") ) )
    {
        g_bQuitThread = TRUE;
        SetEvent( g_hOutputEvent );
    }
    else if( !_tcscmp( token, TEXT("SERVERRELIABLE") ) || 
             !_tcscmp( token, TEXT("SR") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD chance = _ttol( token );
            if( chance > 100 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 0 and 100") );
            }
            else
            {
                g_MazeServer.SetServerReliableRate( chance );
                ConsolePrintf( LINE_CMD, TEXT("Server reliable packet rate set to %d%%"), chance );
            }
        }
        else
        {
            ConsolePrintf( LINE_CMD, TEXT("Server reliable packet rate is %d%%"), 
                          g_MazeServer.GetServerReliableRate() );
        }
    }
    else if( !_tcscmp( token, TEXT("CLIENTRELIABLE") ) || 
              !_tcscmp( token, TEXT("CR") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD   chance = _ttol( token );
            if( chance > 100 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 0 and 100") );
            }
            else
            {
                g_MazeServer.SetClientReliableRate( chance );
                ConsolePrintf( LINE_CMD, TEXT("Client reliable packet rate set to %d%%"), chance );
            }
        }
        else
        {
            ConsolePrintf( LINE_CMD, TEXT("Client reliable packet rate is %d%%"), 
                          g_MazeServer.GetClientReliableRate() );
        }
    }
    else if( !_tcscmp( token, TEXT("CLIENTUPDATE") ) || !_tcscmp( token, TEXT("CU") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            DWORD   rate = _ttol( token );
            if( rate > 1500 || rate < 50 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 50 and 1500 (milliseconds)") );
            }
            else
            {
                g_MazeServer.SetClientUpdateRate( rate );
                ConsolePrintf( LINE_CMD, TEXT("Client update frequency set to %dms"), rate );
            }
        }
        else
        {
            ConsolePrintf( LINE_CMD, TEXT("Client update frequency is %dms"), g_MazeServer.GetClientUpdateRate() );
        }
    }
    else if( !_tcscmp( token, TEXT("SERVERTIMEOUT") ) || !_tcscmp( token, TEXT("ST") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            long timeout = _ttol( token );
            if( timeout > 3000 || timeout < 0 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 0 and 3000 (milliseconds) (0 for none)") );
            }
            else
            {
                g_MazeServer.SetServerTimeout( timeout );
                if( timeout == 0 )
                    ConsolePrintf( LINE_CMD, TEXT("Server timeout set to 'no timeout'") );
                else
                    ConsolePrintf( LINE_CMD, TEXT("Server timeout set to %dms"), timeout );
            }
        }
        else
        {
            if( g_MazeServer.GetServerTimeout() == 0 )
                ConsolePrintf( LINE_CMD, TEXT("Server timeout set to 'no timeout'") );
            else
                ConsolePrintf( LINE_CMD, TEXT("Server timeout is %dms"), g_MazeServer.GetServerTimeout() );
        }
    }
    else if( !_tcscmp( token, TEXT("CLIENTTIMEOUT") ) || !_tcscmp( token, TEXT("CT") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            long timeout = _ttol( token );
            if( timeout > 3000 || timeout < 0 )
            {
                ConsolePrintf( LINE_CMD, TEXT("Must enter a value between 0 and 3000 (milliseconds) (0 for none)") );
            }
            else
            {
                g_MazeServer.SetClientTimeout( timeout );
                if( timeout == 0 )
                    ConsolePrintf( LINE_CMD, TEXT("Client timeout set to 'no timeout'") );
                else
                    ConsolePrintf( LINE_CMD, TEXT("Client timeout set to %dms"), timeout );
            }
        }
        else
        {
            if( g_MazeServer.GetClientTimeout() == 0 )
                ConsolePrintf( LINE_CMD, TEXT("Client timeout set to 'no timeout'") );
            else
                ConsolePrintf( LINE_CMD, TEXT("Client timeout is %dms"), g_MazeServer.GetClientTimeout() );
        }
    }
    else if( !_tcscmp( token, TEXT("ALLINFORATE") ) || !_tcscmp( token, TEXT("AIRATE") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        if( token )
        {
            g_fAllInfoRate = (float) _ttol( token );
        }

        if( g_fAllInfoRate == 0.0f )
            ConsolePrintf( LINE_CMD, TEXT("ALLINFO will not be called automatically") );
        else
            ConsolePrintf( LINE_CMD, TEXT("ALLINFO will be called every %0.0f mins"), g_fAllInfoRate );
        ConsolePrintf( LINE_CMD, TEXT("     Sets how often ALLINFO is automatically called (0 for never)") );
    }
    else if( !_tcscmp( token, TEXT("ALLINFO") ) || !_tcscmp( token, TEXT("AI") ) )
    {
        g_MazeServer.DisplayAllInfo();
    }
    else if( !_tcscmp( token, TEXT("CONNECTIONINFO") ) || !_tcscmp( token, TEXT("CI") ) )
    {
        token = _tcstok( NULL, TEXT(" \t") );
        BOOL bTokenOK = FALSE;
        if( token )
        {
            if( strlen(token) == 8 )
            {
                DWORD dwID;
                sscanf( token, "%x", &dwID );
                g_MazeServer.DisplayConnectionInfo( dwID );
                bTokenOK = TRUE;
            }
        }

        if( !bTokenOK )
        {
            ConsolePrintf( LINE_CMD, TEXT("Must enter the 8 digit client DPNID") );
        }
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
                g_MazeServer.SetLogLevel( dwLevel );
            }
        }

        ConsolePrintf( LINE_CMD, TEXT("Log level set to %d"), g_MazeServer.GetLogLevel() );
        ConsolePrintf( LINE_CMD, TEXT("     Level 1: Connects and disconnects only") );
        ConsolePrintf( LINE_CMD, TEXT("     Level 2: Client position every 5 seconds") );
        ConsolePrintf( LINE_CMD, TEXT("     Level 3: Show all client position ") );
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
void UpdateTimeStamp()
{
    SYSTEMTIME sysTime;
    GetLocalTime( &sysTime );
    _stprintf( g_strTimeStamp, TEXT("[%02d-%02d-%02d %02d:%02d:%02d]"),
               sysTime.wMonth, sysTime.wDay, sysTime.wYear % 100, 
               sysTime.wHour, sysTime.wMinute, sysTime.wSecond );
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

    ConsolePrintf( LINE_LOG, TEXT("Logging events to temporary file") );

    ConsolePrintf( LINE_LOG, TEXT("Log directory:") );
    ConsolePrintf( LINE_LOG, strTempPath );

    SYSTEMTIME sysTime;
    GetLocalTime( &sysTime );
    _stprintf( strTime, TEXT("server-%04d-%02d-%02d-"),
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

    ConsolePrintf( LINE_LOG, TEXT("Log file name:") );
    ConsolePrintf( LINE_LOG, strBaseName );
    return;

LFail:
    ConsolePrintf( LINE_LOG, TEXT("File logging disabled") );
    g_bFileLogging = FALSE;
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




