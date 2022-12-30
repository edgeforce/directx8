//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#define D3D_OVERLOADS
#include <windows.h>
#include <basetsd.h>
#include <d3dx.h>
#include <stdio.h>
#include <math.h>
#include <dplay8.h>
#include <dpaddr.h>
#include "DXUtil.h"
#include "SyncObjects.h"
#include "Resource.h"
#include "Config.h"

extern DWORD g_dwStartMode;


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
extern  Config      g_Config;
extern  HINSTANCE   g_hInstance;
const TCHAR*        g_szKeyname = TEXT("Software\\Microsoft\\DirectPlayMaze\\MazeClient");
BOOL                g_bIsScreenSaverSettings;

INT_PTR CALLBACK   SaverConfigDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
void            ExtractSaverConfigDlgSettings( HWND hDlg );
void            PopulateSaverConfigDlg( HWND hDlg );




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
    DXUtil_ReadBoolRegKey( hKey, TEXT("ShowFramerate"), &g_Config.bShowFramerate, TRUE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("ShowIndicators"), &g_Config.bShowIndicators, TRUE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("DrawMiniMap"), &g_Config.bDrawMiniMap, TRUE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("FullScreen"), &g_Config.bFullScreen, TRUE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("Reflections"), &g_Config.bReflections, FALSE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("Prefer32Bit"), &g_Config.bPrefer32Bit, FALSE );
    DXUtil_ReadBoolRegKey( hKey, TEXT("FileLogging"), &g_Config.bFileLogging, TRUE );
    DXUtil_ReadStringRegKey( hKey, TEXT("IPAddress"), g_Config.szIPAddress, sizeof(g_Config.szIPAddress), TEXT("\0") );

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
    
    RegCreateKeyEx( HKEY_CURRENT_USER, g_szKeyname, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                    &hKey, &dwDisposition );

    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToMicrosoftSite"), g_Config.bConnectToMicrosoftSite );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToLocalServer"), g_Config.bConnectToLocalServer );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ConnectToRemoteServer"), g_Config.bConnectToRemoteServer );
    DXUtil_WriteIntRegKey(  hKey, TEXT("NetworkRetryDelay"), g_Config.dwNetworkRetryDelay );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ShowFramerate"), g_Config.bShowFramerate );
    DXUtil_WriteBoolRegKey( hKey, TEXT("ShowIndicators"), g_Config.bShowIndicators );
    DXUtil_WriteBoolRegKey( hKey, TEXT("DrawMiniMap"), g_Config.bDrawMiniMap );
    DXUtil_WriteBoolRegKey( hKey, TEXT("FullScreen"), g_Config.bFullScreen );
    DXUtil_WriteBoolRegKey( hKey, TEXT("Reflections"), g_Config.bReflections );
    DXUtil_WriteBoolRegKey( hKey, TEXT("Prefer32Bit"), g_Config.bPrefer32Bit );
    DXUtil_WriteBoolRegKey( hKey, TEXT("FileLogging"), g_Config.bFileLogging );
    DXUtil_WriteStringRegKey( hKey, TEXT("IPAddress"), g_Config.szIPAddress );

    RegCloseKey( hKey );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
int ScreenSaverDoConfig( BOOL bIsScreenSaverSettings )
{
    g_bIsScreenSaverSettings = bIsScreenSaverSettings;

    ReadConfig();

    if( DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_SAVERCONFIG), 
                   NULL, SaverConfigDlgProc ) == IDOK )
    {
        WriteConfig();
    }

    return 0;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
INT_PTR CALLBACK   SaverConfigDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_INITDIALOG:
            PopulateSaverConfigDlg( hDlg );
            return TRUE;

        case WM_CLOSE:
            EndDialog( hDlg, IDCANCEL );
            break;

        case WM_COMMAND:
            switch( HIWORD(wParam) )
            {
                case BN_CLICKED:
                    switch( LOWORD(wParam) )
                    {
                        case IDOK:
                            ExtractSaverConfigDlgSettings( hDlg );
                            g_dwStartMode = 0;
                            EndDialog( hDlg, IDOK );
                            break;

                        case IDC_LAUNCH:
                            ExtractSaverConfigDlgSettings( hDlg );
                            g_dwStartMode = 1;
                            EndDialog( hDlg, IDOK );
                            break;

                        case IDCANCEL:
                            EndDialog( hDlg, IDCANCEL );
                            return TRUE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void PopulateSaverConfigDlg( HWND hDlg )
{
    if( g_bIsScreenSaverSettings )
        ShowWindow( GetDlgItem( hDlg, IDOK ), SW_SHOW );
    else
        ShowWindow( GetDlgItem( hDlg, IDOK ), SW_HIDE );

    CheckDlgButton( hDlg, IDC_MICROSOFT, g_Config.bConnectToMicrosoftSite );
    CheckDlgButton( hDlg, IDC_LOCAL_SERVER, g_Config.bConnectToLocalServer );
    CheckDlgButton( hDlg, IDC_REMOTE_SERVER, g_Config.bConnectToRemoteServer );
    SetDlgItemInt( hDlg, IDC_RETRY_DELAY, g_Config.dwNetworkRetryDelay, FALSE );
    CheckDlgButton( hDlg, IDC_FRAMERATE, g_Config.bShowFramerate );
    CheckDlgButton( hDlg, IDC_INDICATORS, g_Config.bShowIndicators );
    CheckDlgButton( hDlg, IDC_FULLSCREEN, g_Config.bFullScreen );
    CheckDlgButton( hDlg, IDC_MINIMAP, g_Config.bDrawMiniMap );
    CheckDlgButton( hDlg, IDC_REFLECTIONS, g_Config.bReflections );
    CheckDlgButton( hDlg, IDC_PREFER32BIT, g_Config.bPrefer32Bit );
    CheckDlgButton( hDlg, IDC_FILELOGGING, g_Config.bFileLogging );

    SetDlgItemText( hDlg, IDC_IPADDRESS, g_Config.szIPAddress );
    SendDlgItemMessage( hDlg, IDC_IPADDRESS, EM_SETLIMITTEXT, sizeof(g_Config.szIPAddress)-1, 0 );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void ExtractSaverConfigDlgSettings( HWND hDlg )
{
    g_Config.bConnectToMicrosoftSite    = IsDlgButtonChecked( hDlg, IDC_MICROSOFT );
    g_Config.bConnectToLocalServer      = IsDlgButtonChecked( hDlg, IDC_LOCAL_SERVER );
    g_Config.bConnectToRemoteServer     = IsDlgButtonChecked( hDlg, IDC_REMOTE_SERVER );
    g_Config.dwNetworkRetryDelay        = GetDlgItemInt( hDlg, IDC_RETRY_DELAY, NULL, FALSE );
    g_Config.bShowFramerate             = IsDlgButtonChecked( hDlg, IDC_FRAMERATE );
    g_Config.bShowIndicators            = IsDlgButtonChecked( hDlg, IDC_INDICATORS );
    g_Config.bFullScreen                = IsDlgButtonChecked( hDlg, IDC_FULLSCREEN );
    g_Config.bDrawMiniMap               = IsDlgButtonChecked( hDlg, IDC_MINIMAP );
    g_Config.bReflections               = IsDlgButtonChecked( hDlg, IDC_REFLECTIONS );
    g_Config.bPrefer32Bit               = IsDlgButtonChecked( hDlg, IDC_PREFER32BIT );
    g_Config.bFileLogging               = IsDlgButtonChecked( hDlg, IDC_FILELOGGING );

    GetDlgItemText( hDlg, IDC_IPADDRESS, g_Config.szIPAddress, sizeof(g_Config.szIPAddress) );
}


