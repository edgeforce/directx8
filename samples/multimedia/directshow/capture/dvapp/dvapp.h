//------------------------------------------------------------------------------
// File: DVApp.h
//
// Desc: DirectShow sample code - DV control/capture example header file.
//
// Copyright (c) 1993 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef _DVAPP_H_
#define _DVAPP_H_

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tchar.h>
#include <dbt.h>

#define __EDEVDEFS__    //don't include edevdefs.h

#include <mmreg.h>
#include <streams.h>
#include <initguid.h>
#include <xprtdefs.h>   //include this instead of edevdefs

#include "resource.h"

typedef TCHAR *PTCHAR;

//track device mode or active 
enum DV_MODE
{
    CameraMode  = 0L,
    VcrMode     = 1L,
    UnknownMode = 2L
}; 

enum GRAPH_TYPE
{
    GRAPH_PREVIEW, GRAPH_DV_TO_FILE, GRAPH_DV_TO_FILE_NOPRE, GRAPH_FILE_TO_DV, GRAPH_FILE_TO_DV_NOPRE, GRAPH_DV_TO_FILE_TYPE2, GRAPH_DV_TO_FILE_NOPRE_TYPE2, GRAPH_FILE_TO_DV_TYPE2, GRAPH_FILE_TO_DV_NOPRE_TYPE2
};

// Logging Support
enum LOG_LEVEL
{
    LOG_LEVEL_SUCCINCT, LOG_LEVEL_MEDIUM, LOG_LEVEL_VERBOSE
};
enum LOG_PRIORITY
{
    LOG_PRIORITY_ERROR, LOG_PRIORITY_WARN, LOG_PRIORITY_INFO
};

//a few macros...
#define MBOX(s) MessageBox(g_hwndApp, s, APPNAME, MB_OK);

#ifdef DEBUG
#define SAFE_RELEASE(pUnk) if (pUnk) \
{ \
    pUnk->Release(); \
    pUnk = NULL; \
} \
else \
{ \
}
#else
#define SAFE_RELEASE(pUnk) if (pUnk)\
{\
    pUnk->Release();\
    pUnk = NULL;\
}
#endif //DEBUG


//our constants
#define DEVICENAME_BUFSIZE 80
// capture constants
#define DV_CAPLIMIT_NONE         10L
#define DV_CAPLIMIT_TIME         11L
#define DV_CAPLIMIT_SIZE         12L

#define DV_BYTESPERMSEC          352  //DV captures at 3600K per second (3600 / 1024 == ~3.52)
#define DV_TIMERFREQ             55  //milliseconds between timer ticks
#define DV_BYTES_IN_MEG          1048576L  //(1024 * 1024)

// App Names
#define APPNAME                  TEXT("DV App")
#define DV_APPTITLE              TEXT("Digital Video Sample Application")

//Timer ID's
#define DV_TIMER_ATN             1L
#define DV_TIMER_CAPLIMIT        2L
#define DV_TIMER_FRAMES          3L

// File Names
#define         DEFAULT_CAP_FILE_NAME       TEXT(".\\DVApp.avi")
#define         DEFAULT_FG_FILE_NAME        TEXT(".\\DVApp.grf")

///////////////////////////////////////
// device notification definitions
#if (WINVER < 0x0500)

#define DBT_DEVTYP_DEVICEINTERFACE      0x00000005  // device interface class
#define DEVICE_NOTIFY_WINDOW_HANDLE     0x00000000
typedef  PVOID           HDEVNOTIFY;

#endif

//extern "C"
//{
    typedef BOOL (/* WINUSERAPI */ WINAPI *PUnregisterDeviceNotification)(
        IN HDEVNOTIFY Handle
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationA)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationW)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );
//}
#ifdef UNICODE
#define PRegisterDeviceNotification  PRegisterDeviceNotificationW
#else
#define PRegisterDeviceNotification  PRegisterDeviceNotificationA
#endif // !UNICODE

#if (WINVER < 0x0500)

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_A {
    DWORD       dbcc_size;
    DWORD       dbcc_devicetype;
    DWORD       dbcc_reserved;
    GUID        dbcc_classguid;
    char        dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE_A, *PDEV_BROADCAST_DEVICEINTERFACE_A;

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_W {
    DWORD       dbcc_size;
    DWORD       dbcc_devicetype;
    DWORD       dbcc_reserved;
    GUID        dbcc_classguid;
    wchar_t     dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE_W, *PDEV_BROADCAST_DEVICEINTERFACE_W;

#ifdef UNICODE
typedef DEV_BROADCAST_DEVICEINTERFACE_W   DEV_BROADCAST_DEVICEINTERFACE;
typedef PDEV_BROADCAST_DEVICEINTERFACE_W  PDEV_BROADCAST_DEVICEINTERFACE;
#else
typedef DEV_BROADCAST_DEVICEINTERFACE_A   DEV_BROADCAST_DEVICEINTERFACE;
typedef PDEV_BROADCAST_DEVICEINTERFACE_A  PDEV_BROADCAST_DEVICEINTERFACE;
#endif // UNICODE
#endif // WINVER


///////////////////////////////////////
#define     _MAX_ARGC   6
#define     _MAX_SLEEP  500

// function prototypes
// Initialize functions
void    DV_AppSetup(void);
BOOL    DV_InitDevice(void);
BOOL    DV_InitWindow(void);
BOOL    DV_InitControls(HWND hwnd, HINSTANCE hInst);

// clean up functions
void    DV_CleanUp(void);
void    DV_DisconnectAll(IBaseFilter *pBF);

// Device Type functions
DV_MODE DV_GetDVMode(void);
HRESULT DV_GetTapeInfo(void);

// FilterGraph State Manipulation
BOOL    DV_GetGraphBuilder(void);
HRESULT DV_StopGraph(void);
HRESULT DV_PauseGraph(void);
HRESULT DV_StartGraph(void);
HRESULT DV_SaveGraph(TCHAR* swGraphFile);

// Timecode or ATN functions
void    DV_DisplayTimecode(void);
BOOL    DV_SeekATN(void);

// Misc helper functions
HRESULT DV_SetPreview(void);
HRESULT DV_SelectClock(IBaseFilter *pBaseFilter);
HRESULT DV_SetAviOptions(IBaseFilter *ppf, InterleavingMode INTERLEAVE_MODE);
BOOL    DV_RefreshMode(void);
void    DV_GetFinalDroppedFramesStats(DWORD dwTime);

// Graph Building functions
BOOL    DV_MakeSpecialGraph(GRAPH_TYPE iGraphType);

// Type 1 File (capture\playback\transmit)
HRESULT DV_MakeDvToFileGraph(void);
HRESULT DV_MakeDvToFileGraph_NoPre(void);
HRESULT DV_MakeFileToDvGraph(void);
HRESULT DV_MakeFileToDvGraph_NoPre(void);

// Type 2 File (capture\playback\transmit)
HRESULT DV_MakeDvToFileGraph_Type2(void);
HRESULT DV_MakeDvToFileGraph_NoPre_Type2(void);
HRESULT DV_MakeFileToDvGraph_Type2(void);
HRESULT DV_MakeFileToDvGraph_NoPre_Type2(void);

// Read & Write RegKeys so help remember the last known state
void    DV_WriteRegKeys(void);
void    DV_ReadRegKeys(void);

// Message Processing Functions
LRESULT CALLBACK DV_WndProc (HWND, UINT, WPARAM, LPARAM) ;
void CALLBACK DV_TansportCommand_WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void CALLBACK DV_GraphModeCommand_WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

// Timer & Dialog Processing Functions
void CALLBACK DV_DroppedFrameProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
BOOL CALLBACK DV_CapSizeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DV_ChooseModeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CALLBACK DV_StopRecProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
BOOL CALLBACK DV_AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CALLBACK DV_TimecodeTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
BOOL CALLBACK DV_ChangeDecodeSizeProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Logging Support
void    __cdecl DV_LogOut(LOG_PRIORITY iPriority, LOG_LEVEL ilevel, TCHAR *szFormat, ... );

// globals

// Handles to the windows
HWND                  g_hwndApp               = NULL;
HWND                  g_hwndTBar              = NULL; 
HWND                  g_hwndStatus            = NULL;
DWORD                g_statusHeight          = 0;

// metrics of the windows
int                         g_iAppHeight            = 480+95;
int                         g_iAppWidth             = 720+5;        
int                     g_iVWHeight             = 480;
int                     g_iVWWidth              = 720;        

// Names of files & devices
TCHAR                   g_InputFileName[_MAX_PATH]  = {DEFAULT_CAP_FILE_NAME};
TCHAR                   g_OutputFileName[_MAX_PATH] = {DEFAULT_CAP_FILE_NAME};
TCHAR                   g_FilterGraphFileName[_MAX_PATH]    = {DEFAULT_FG_FILE_NAME};

// capture variables
DWORD                   g_dwCaptureLimit        = DV_CAPLIMIT_NONE; //track whether we are using time, disk, or no based capture limits
DWORD                   g_dwDiskSpace           = 120;              //roughly the same

// time variables
BOOL                    g_bUseAtnTimer          = FALSE;            //track if we want to constantly update the timer
LONG                    g_AvgTimePerFrame       = 33L;              //default to 33
DWORD                   g_dwTimeLimit           = 30;               //default to 30 seconds of capture
DWORD                   g_CapStartTime          = 0;
DWORD                   g_curTime               = 0;

// device notification globals
TCHAR                   g_DeviceName[DEVICENAME_BUFSIZE];
BOOL                    g_bDeviceFound = FALSE;
HDEVNOTIFY              g_hDevNotify            = NULL;
PUnregisterDeviceNotification g_pUnregisterDeviceNotification;
PRegisterDeviceNotification g_pRegisterDeviceNotification;

// filtergraph variables
DV_MODE                 g_CurrentMode           = UnknownMode;
GRAPH_TYPE              g_iCurrentGraph         = GRAPH_PREVIEW;    //need to track the current graph

// DirectShow interfaces to help build the filter graph & control the capture device
ICaptureGraphBuilder2   *g_pBuilder             = NULL;
IGraphBuilder           *g_pGraphBuilder        = NULL;
IBaseFilter             *g_pDVCamera            = NULL;
IAMExtDevice            *g_pExtDev              = NULL;
IAMExtTransport  *g_pExtTrans            = NULL;
IAMTimecodeReader       *g_pTimeReader          = NULL;
IAMStreamConfig         *g_pStreamConf          = NULL;
IVideoWindow            *g_pVideoWindow         = NULL;
IIPDVDec                *g_pDvDec               = NULL;
IAMDroppedFrames        *g_pDroppedFrames       = NULL;

// Filters used in the DirectShow filtergraph
IBaseFilter             *g_pAviSplitter         = NULL;
IBaseFilter             *g_pInputFileFilter     = NULL;  
IBaseFilter             *g_pDVMux               = NULL;
IBaseFilter             *g_pDVSplitter          = NULL;
IBaseFilter             *g_pDVCodec             = NULL;
IBaseFilter             *g_pSmartTee            = NULL;
IBaseFilter             *g_pDSound              = NULL;
IBaseFilter             *g_pInfTee              = NULL;
IBaseFilter             *g_pVideoRenderer       = NULL;

// Logging Support
LOG_PRIORITY        g_iLogPriority                              = LOG_PRIORITY_ERROR;
LOG_LEVEL               g_iLogLevel                                 = LOG_LEVEL_SUCCINCT;

//toolbar buttons 
TBBUTTON g_rgTbButtons[] = 
{ 
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},     
    {8, IDM_STEP_REV,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {5, IDM_REW,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {9, IDM_PLAY_FAST_REV,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {0, IDM_PLAY,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0}, 
    {6, IDM_PLAY_FAST_FF,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {4, IDM_FF,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {7, IDM_STEP_FWD,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {1, IDM_PAUSE,          TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0}, 
    {2, IDM_STOP,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0},
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {3, IDM_RECORD,         TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0,0},
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,    0,0},   
    {10, IDM_SEEKTIMECODE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0,0}
}; 


// inline functions
// put the VCR Mode
inline HRESULT DV_PutVcrMode(long Mode)
{
    if(!g_pExtTrans)
    	return S_FALSE;
    return g_pExtTrans->put_Mode(Mode);
} 

// update the status windows with appropriate text
inline LRESULT DV_StatusText(LPCTSTR statusText, UINT nPart)
{
    return SendMessage(g_hwndStatus, SB_SETTEXT, (WPARAM) 0 | nPart, (LPARAM)statusText);
} 

//divide the status bar into thirds, and give the middle frame an extra 100 pixels.
inline LRESULT DV_StatusParts(UINT width)
{
    int rg[3];
    rg[0] = (width / 3) - 50;
    rg[1] = ((rg[0]+50) * 2) + 50;
    rg[2] = -1;
    return SendMessage(g_hwndStatus, SB_SETPARTS, 3, (LPARAM)(LPINT) rg);
}    

#endif //_DVAPP_H_
