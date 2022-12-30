//-----------------------------------------------------------------------------
// File: MultiMapper.cpp
//
// Desc: This is a simple sample to demonstrate how to code using the DirectInput
//       mapper feature.
//
// Copyright (C) 1995-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#define INITGUID
#include <windows.h>
#include <basetsd.h>
#include <tchar.h>
#include <dinput.h>
#include <d3d8types.h>  // included to get the D3DCOLOR_RGBA macro.
#include <stdio.h>
#include "resource.h"

// The following constants control the number of users the app 
// supports, and the maximum number of devices it will tolerate.
// Generally, these are used to allocate arrays, but they're also
// used for bounding iterations on those arrays.
#define MIN_USERS 1
#define MAX_USERS 4
#define MAX_DEVICES 25  // more than reasonable max

// The following constants control spacing and alignment 
// for text display in the status area.
#define LINE_SPACING 20 
#define COL_SPACING  70
#define FIRST_LINE   20 

// Controls the number of actions the app asks DirectInput to buffer.
#define BUFFER_SIZE  16 

//
// Internal return values
//

// E_APPERR_DEVICESTAKEN is returned by the manager class when one player
// on the machine has enough RECENT devices to prevent other players from 
// playing. This return code is needed because this sample attempts to give 
// all RECENT devices to that player.
#define E_APPERR_DEVICESTAKEN MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,998) 

// E_APPERR_TOOMANYUSERS is returned by the manager class when the number of 
// players exceeds the number of devices present on the system. For example, 
// if you ask for 4 players on a machine that only has a keyboard and mouse,
// you're 2 short of what you need. 
#define E_APPERR_TOOMANYUSERS MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,999)

// Flags to control the creation of the manager class.
#define APP_DICREATE_DEFAULT    FALSE 
#define APP_DICREATE_FORCEINIT  TRUE

// Flags to control the destruction of the internal device lists.
// There are cases (i.e. reassigning devices) where it makes sense
// to reset the devices held by the manager class, while preserving 
// ownership information.
#define APP_CLEANUP_DEFAULT            FALSE
#define APP_CLEANUP_PRESERVEASSIGNMENT TRUE


//-----------------------------------------------------------------------------
// Name: CInputDeviceManager
// Desc: This is a class to wrap up the DirectInput functionality for the app. 
//       All DirectInput initialization, enumeration, and management of 
//       DirectInput devices in contained within this class.
//-----------------------------------------------------------------------------
class CInputDeviceManager
{
    LPDIRECTINPUT8       m_pDI;
    LPDIRECTINPUTDEVICE8 m_ppdidDevices[MAX_USERS][MAX_DEVICES];
	DWORD				 m_dwNumUsers;
    TCHAR*               m_szUserNames;
    HWND                 m_hWnd;    
    DIACTIONFORMAT       m_diaf;
    DWORD                m_dwCreateFlags;
    DWORD                m_dwTotalDevices;
    
    // Callback for enumerating suitable devices
    static BOOL CALLBACK EnumSuitableDevicesCB( LPCDIDEVICEINSTANCE, LPDIRECTINPUTDEVICE8, 
                                         DWORD, DWORD, LPVOID );

    static BOOL CALLBACK BuildFlatListCB( LPCDIDEVICEINSTANCE, LPDIRECTINPUTDEVICE8, 
                                          DWORD, DWORD, LPVOID );


public:
	// Creation methods
    CInputDeviceManager();
    HRESULT Create( HWND, DWORD*, DIACTIONFORMAT*, DWORD);

    // Cleanup
    VOID    Cleanup();
    VOID    CleanupDevices( DWORD );

    // Add a device to the list
    VOID    AddDeviceForPlayer( const DWORD dwPlayerNum, 
                                const DIDEVICEINSTANCE* pdidi, 
					            const LPDIRECTINPUTDEVICE8 pdidDevice );
    
    // Assign devices to other users.
    VOID    ReassignDevices();

    // Access the device list for a user.
    HRESULT GetDevicesForPlayer( DWORD, LPDIRECTINPUTDEVICE8**);
	
    // Invoke the Default configuration UI.
	HRESULT ConfigureDevices(IUnknown*, VOID*,DWORD);

    // Simple data retrieval.
    DWORD GetNumUsers();
    DWORD GetNumDevices();
};

// The PLAYERDATA structure that contains all the relevant data
// for a single player in the game. An array of these structures is used
// to track the information for all players. This array is initialized 
// in the window procedure when the WM_CREATE message is received. 
typedef struct _PLAYERDATA {
    BOOL  bTurningRight;
    BOOL  bReverseThrust;
    BOOL  bTurningLeft;
    BOOL  bForwardThrust;
    BOOL  bFiringWeapons;
    BOOL  bEnableShields;
    BOOL  bDisplayingMenu;
    DWORD dwLRAxisData;
    DWORD dwUDAxisData;
} PLAYERDATA, *LPPLAYERDATA;

// The ENUMDATA structure is used to carry information needed by the 
// EnumDevicesBySemantics callback to add devices to the manager class's
// internal lists, build/set action maps, and return the status (device 
// found or not) of the enumeration.
typedef struct _ENUMDATA {
    CInputDeviceManager* pDevMan;
    DWORD                dwCreateFlags;
    HWND                 hWnd;
    DWORD                dwPlayerNum;
    TCHAR*               pszPlayerName;
    LPDIACTIONFORMAT     lpdiaf;
    BOOL                 bRet;
} ENUMDATA, *LPENUMDATA;


// Global function prototypes.
HRESULT CreateInputStuff(HWND, DWORD);
BOOL ParsePlayerInput(HWND, LPDIRECTINPUTDEVICE8*, LPPLAYERDATA);
void InvokeDefaultUI(HWND);

//-----------------------------------------------------------------------------
// App-defined game actions for the DirecInput Mapper.
//-----------------------------------------------------------------------------


// The following constants are custom for each app, depending on what input the
// app needs. This simple sample is pretending to be a space simulator game, so
// relevant inputs are for turning left, thrusting, firing weapons, etc.. Also
// note that some constants are defines for the axes, which are for recieving
// axis data from joysticks and similiar analog devices.
#define INPUT_LEFTRIGHT_AXIS   1L
#define INPUT_UPDOWN_AXIS      2L
#define INPUT_TURNLEFT         3L
#define INPUT_TURNRIGHT        4L
#define INPUT_FORWARDTHRUST    5L
#define INPUT_REVERSETHRUST    6L
#define INPUT_FIREWEAPONS      7L
#define INPUT_ENABLESHIELD     8L
#define INPUT_DISPLAYGAMEMENU  9L
#define INPUT_QUITGAME        10L


// The following array is the global, app-defined game actions, which map real
// device input into a semantic. The first column is the app-defined semantics
// as defined above. These are the constants the game actually sees in its
// input loop. The second column is the physical action recieved by the device
// which is to be mapped to the app-defined semantic. For instance, in the
// array below, if the user hits the "Left" key on the keyboard, the app will
// recieve an input code equal to INPUT_TURNLEFT. The last column is a text
// string that DirectInput uses for displaying a configuration dialog box.
#define NUMBER_OF_SEMANTICS 17

DIACTION g_rgGameAction[NUMBER_OF_SEMANTICS] =
{
    // Device input (joystick, etc.) that is pre-defined by DirectInput, 
    // according to genre type. The genre for this app is space simulators.
    { INPUT_LEFTRIGHT_AXIS,  DIAXIS_SPACESIM_LATERAL,         0, _T("Turn"), },
    { INPUT_UPDOWN_AXIS,     DIAXIS_SPACESIM_MOVE,            0, _T("Move"), },
    { INPUT_FIREWEAPONS,     DIBUTTON_SPACESIM_FIRE,          0, _T("Shoot"), },
    { INPUT_ENABLESHIELD,    DIBUTTON_SPACESIM_GEAR,          0, _T("Enable shields"), },
    { INPUT_DISPLAYGAMEMENU, DIBUTTON_SPACESIM_DISPLAY,       0, _T("Display"), },

    // Keyboard input mappings
    { INPUT_TURNLEFT,        DIKEYBOARD_LEFT,    0, _T("Turn left"), },
    { INPUT_TURNRIGHT,       DIKEYBOARD_RIGHT,   0, _T("Turn right"), },
    { INPUT_FORWARDTHRUST,   DIKEYBOARD_UP,      0, _T("Forward thrust"), },
    { INPUT_REVERSETHRUST,   DIKEYBOARD_DOWN,    0, _T("Reverse thrust"), },
    { INPUT_FIREWEAPONS,     DIKEYBOARD_F,       0, _T("Shoot"), },
    { INPUT_ENABLESHIELD,    DIKEYBOARD_S,       0, _T("Enable shields"), },
    { INPUT_DISPLAYGAMEMENU, DIKEYBOARD_D, DIA_APPFIXED, _T("Display"), },
    { INPUT_QUITGAME,        DIKEYBOARD_ESCAPE,  DIA_APPFIXED, _T("Quit Game"), },

    // Mouse input mappings
    { INPUT_LEFTRIGHT_AXIS,  DIMOUSE_XAXIS,      0, _T("Turn"), },
    { INPUT_UPDOWN_AXIS,     DIMOUSE_YAXIS,      0, _T("Move"), },
    { INPUT_FIREWEAPONS,     DIMOUSE_BUTTON0,    0, _T("Shoot"), },
    { INPUT_ENABLESHIELD,    DIMOUSE_BUTTON1,    0, _T("Enable shields"), },
};

//-----------------------------------------------------------------------------
// Globals for the app
//-----------------------------------------------------------------------------
GUID                 g_AppGuid    = { 0x451F8DEF, 0xA7E9, 0x4DF4, 0x9A, 0x6B,
                                      0xF4, 0xb6,0xC7, 0x06, 0xF3, 0x98 };
LPTSTR               g_strAppName = _T("DirectInput8 MultiMapper Sample");
CInputDeviceManager* g_pInputDeviceManager = NULL;


//-----------------------------------------------------------------------------
// Name: CInputDeviceManager()
// Desc: Constructor for the class
//-----------------------------------------------------------------------------
CInputDeviceManager::CInputDeviceManager()
{
    m_pDI                  = NULL;
    m_dwCreateFlags        = 0;
    m_dwTotalDevices       = 0;

    // Uses canned names for players.
    m_szUserNames  = _T("Player 1\0Player 2\0Player 3\0Player 4\0\0");
    ZeroMemory( m_ppdidDevices, sizeof(m_ppdidDevices) );
}


//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creation method for the class. Creates DirectInput, and enumerates 
//       "suitable" devices for each player. By "suitable", we mean devices
//       that work with the genre specified in the DIACTIONFORMAT structure.
//       This method initializes the class in two ways, as controlled by 
//       the dwFlags field. In the default mode, the method can assign any 
//       devices recently used by each user upon startup. This invites a case
//       where a user has "ownership" of enough devices to limit the number of
//       players who can interact with the app. If this happens, the class is 
//       reinitialized with the APP_DICREATE_FORCEINIT, which causes the class
//       to allocate a single device to each user (with default configs).
//-----------------------------------------------------------------------------
HRESULT CInputDeviceManager::Create( HWND hWnd, DWORD* lpdwNumUsers, 
                                     DIACTIONFORMAT* pdiaf, DWORD dwFlags )
{
    HRESULT hr;

    // Copy passed in arguments for internal use.
    m_hWnd        = hWnd;
    memcpy( &m_diaf, pdiaf, sizeof(DIACTIONFORMAT) );
	m_dwNumUsers = *lpdwNumUsers;
    m_dwCreateFlags = dwFlags;

    // Create the main DirectInput object.
    hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                             IID_IDirectInput8, (VOID**)&m_pDI, NULL );
    if( FAILED(hr) )
    {
        return E_FAIL;
    }

    TCHAR* pszNameScan = m_szUserNames;
    for(DWORD dwPlay = 0; dwPlay < m_dwNumUsers; dwPlay++)
    {        
        ENUMDATA ed;
        ZeroMemory( &ed, sizeof(ed) );
        ed.pDevMan = this;
        ed.dwPlayerNum = dwPlay;
        ed.pszPlayerName = pszNameScan;
        ed.bRet = FALSE; // Will be TRUE if a device was found.
        ed.lpdiaf = &m_diaf;
        ed.hWnd = m_hWnd;
        ed.dwCreateFlags = m_dwCreateFlags;

        if( m_dwCreateFlags == APP_DICREATE_DEFAULT)
        {
            // Enumerate "suitable" devices for this user.
            hr = m_pDI->EnumDevicesBySemantics( pszNameScan, &m_diaf, 
                                                EnumSuitableDevicesCB, 
                                                (LPVOID)&ed, DIEDBSFL_THISUSER | DIEDBSFL_AVAILABLEDEVICES);
        }
        else // m_dwCreateFlags == APP_DICREATE_FORCEINIT
        {
            // Enumerate available devices for any user.
            hr = m_pDI->EnumDevicesBySemantics( NULL, &m_diaf, 
                                                EnumSuitableDevicesCB, 
                                                (LPVOID)&ed, DIEDBSFL_AVAILABLEDEVICES);
        }

        if( FAILED(hr))
        {
            return hr;
        }
        
        if(!ed.bRet)
        {
            if(m_dwTotalDevices < m_dwNumUsers)
                return E_APPERR_TOOMANYUSERS;
            else
                return E_APPERR_DEVICESTAKEN;
        }
    
        // Scan to the next username in the multi-sz list.
        pszNameScan += strlen(pszNameScan) + 1;
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetNumUsers()
// Desc: Returns the number of users currently using the "game".
//-----------------------------------------------------------------------------
DWORD CInputDeviceManager::GetNumUsers()
{
    return m_dwNumUsers;
}

//-----------------------------------------------------------------------------
// Name: GetNumDevices()
// Desc: Returns the number of devices present on the machine.
//-----------------------------------------------------------------------------
DWORD CInputDeviceManager::GetNumDevices()
{
    return m_dwTotalDevices;
}


//-----------------------------------------------------------------------------
// Name: EnumSuitableDevicesCB()
// Desc: Callback function for enumerating suitable devices. The manager class
//       enumerates devices for each player, so this method is called a lot 
//       (#_of_players * installed_devices).
//       
//       This method attempts to assign devices recently used by a player in a 
//       previous instance to that player for this instance. As such, any device 
//       that is enumerated with the DIEDBS_RECENTDEVICE flag is assigned to the 
//       player for which the devices are being enumerated. Devices marked by 
//       DIEDBS_NEWDEVICE and DIEDBS_MAPPEDPRI1 are added to a flat list.
//       If no recent devices are found for the user, we take the topmost device
//       in the flat list (the most suitable) and throw the others away.
//-----------------------------------------------------------------------------
BOOL CALLBACK CInputDeviceManager::EnumSuitableDevicesCB( LPCDIDEVICEINSTANCE  pdidi,
                                                          LPDIRECTINPUTDEVICE8 pdidDevice, 
														  DWORD  dwFlags,
														  DWORD  dwRemainingDevices,
														  LPVOID pContext )
{
    HRESULT hr;
    ENUMDATA ed = *(LPENUMDATA)pContext;
    DWORD dwBuildFlags;    

    // Temp array to hold non-recent/new devices for later allocation.
    // A linked list would be a better choice, but for simplicity, we'll 
    // use a simple array.
    static LPDIRECTINPUTDEVICE8 lprgDevTemp[MAX_DEVICES]; 
    static DIDEVICEINSTANCE     didTemp[MAX_DEVICES];
    static int iIndex = 0;
        

    // Devices of type DI8DEVTYPE_DEVICECTRL are specialized devices not generally
    // considered appropriate to control game actions. We just ignore these.
    if( DI8DEVTYPE_DEVICECTRL == GET_DIDEVICE_TYPE(pdidi->dwDevType) )
    {
        return DIENUM_CONTINUE;     
    }

    if( dwFlags & DIEDBS_RECENTDEVICE )
    {
        // Set the cooperative level.
        hr = pdidDevice->SetCooperativeLevel( ed.hWnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND );
        if( FAILED(hr) )
        {
            return DIENUM_CONTINUE;     
        }

        // Set the action map for this player to remove it from DirecInput's internal
        // list of available devices.
        dwBuildFlags = (ed.dwCreateFlags == APP_DICREATE_DEFAULT) ? DIDBAM_DEFAULT : DIDBAM_HWDEFAULTS;
        pdidDevice->BuildActionMap( ed.lpdiaf, ed.pszPlayerName, dwBuildFlags );
        hr = pdidDevice->SetActionMap( ed.lpdiaf, ed.pszPlayerName, DIDSAM_DEFAULT ); 

        // Add the device to the device manager's internal list
        ed.pDevMan->AddDeviceForPlayer( ed.dwPlayerNum, pdidi, pdidDevice );

        // If BuildActionMap fails, so will SetActionMap. Using a single error check here for simplicity.
        // These may fail if no controls on this device are appropriate. However, other devices may be 
        // just fine, so continue the enumeration.
        if( FAILED(hr) )
        {
            return DIENUM_CONTINUE;
        }
        
#ifdef _DEBUG
        TCHAR str[MAX_PATH];
        OutputDebugString(_T("\n--> !!!RECENT DEVICE ASSIGNED!!! <--\n"));
        sprintf(str, _T("  Player == %s\n"), ed.pszPlayerName );
        OutputDebugString(str);
        sprintf(str, _T("  Device == %s\n"), (*pdidi).tszProductName );
        OutputDebugString(str);
                
        SYSTEMTIME st;
        FILETIME   ft;  
        // Convert the time to local time.
        FileTimeToLocalFileTime(&ed.lpdiaf->ftTimeStamp, &ft);
        // Convert the local file time from UTC to system time.
        FileTimeToSystemTime(&ft, &st);

        // Build a string showing the date and time.
        wsprintf( str, _T("  Timestamp == %02d/%02d/%d  %02d:%02d:%02d \n"), 
                  st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
        OutputDebugString(str);

        sprintf(str, _T("  dwFlags == %x\n"), dwFlags);
        OutputDebugString(str);
#endif
        // Set return value in ENUMDATA struct to indicate success and 
        // stop enumeration.
        ((LPENUMDATA)pContext)->bRet = TRUE;
    }
    else if( dwFlags & ( DIEDBS_MAPPEDPRI1 | DIEDBS_NEWDEVICE ) )
    {   // This is neither a RECENT nor a NEW device, add it to an internal list
        // of devices. If we get to the end of the devices without finding a recently
        // used or new device, we'll try one of these. 
        lprgDevTemp[iIndex] = pdidDevice;
        didTemp[iIndex]     = *(LPDIDEVICEINSTANCE)pdidi;
        lprgDevTemp[iIndex++]->AddRef();

#ifdef _DEBUG
        TCHAR str[MAX_PATH];
        if( dwFlags & DIEDBS_NEWDEVICE )
            OutputDebugString(_T("\n--> NEW DEVICE FOUND (not yet assigned) <--\n"));
        else if ( dwFlags & DIEDBS_MAPPEDPRI1 )
            OutputDebugString(_T("\n--> PRI1-CAPABLE DEVICE FOUND (not yet assigned) <--\n"));

        sprintf(str, _T("  Device == %s\n"), (*pdidi).tszProductName );
        OutputDebugString(str);
#endif
    }

    // If no more devices will be enumerated, use one of the other appropriate devices 
    // (according to DIEDBS_MAPPEDPRI1) for this user. For simplicity, we're just taking 
    // the first one in the list. Real applications would probably iterate through action
    // maps for these devices and choose the one with the most game-critical actions assigned. 
    if( 0 == dwRemainingDevices &&  (((LPENUMDATA)pContext)->bRet != TRUE) ) 
    {
        dwBuildFlags = (ed.dwCreateFlags == APP_DICREATE_DEFAULT) ? DIDBAM_DEFAULT : DIDBAM_HWDEFAULTS;

        // Set the action map for this player to remove it from DirectInput's internal
        // list of available devices.
        if( lprgDevTemp[0] )
        {
            // Add the device to the device manager's internal list
            ed.pDevMan->AddDeviceForPlayer( ed.dwPlayerNum, &didTemp[0], lprgDevTemp[0] );

            lprgDevTemp[0]->BuildActionMap( ed.lpdiaf, ed.pszPlayerName, dwBuildFlags );
            hr = lprgDevTemp[0]->SetActionMap( ed.lpdiaf, ed.pszPlayerName, DIDSAM_DEFAULT ); 
            // If BuildActionMap fails, so will SetActionMap. Using a single error check here for simplicity.
            // These may fail if no controls on this device are appropriate. However, other devices may be 
            // just fine, so continue the enumeration.
            if( FAILED(hr) )
                return DIENUM_CONTINUE;

            ((LPENUMDATA)pContext)->bRet = TRUE;

#ifdef _DEBUG
            TCHAR str[MAX_PATH];
            OutputDebugString(_T("\n--> !!! DEVICE ASSIGNED !!! <--\n"));
            sprintf(str, _T("  Player == %s\n"), ed.pszPlayerName );
            OutputDebugString(str);
            sprintf(str, _T("  Device == %s\n"), didTemp[0].tszProductName );
            OutputDebugString(str);
            
            SYSTEMTIME st;
            FILETIME   ft;  
            // Convert the time to local time.
            FileTimeToLocalFileTime(&ed.lpdiaf->ftTimeStamp, &ft);
            // Convert the local file time from UTC to system time.
            FileTimeToSystemTime(&ft, &st);

            // Build a string showing the date and time.
            wsprintf( str, _T("  Timestamp == %02d/%02d/%d  %02d:%02d:%02d \n"), 
                      st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
            OutputDebugString(str);

            sprintf(str, _T("  dwFlags == %x\n"), dwFlags);
            OutputDebugString(str);
#endif
        }

        // Release all of the remaining devices in the list.
        for (int i=1 ; i<iIndex ; i++ )
        {
            if( lprgDevTemp[i] )
            {
                lprgDevTemp[i]->Release();
            }
        }

        iIndex = 0; // reset counter for next pass.
    }

    return DIENUM_CONTINUE;
}


//-----------------------------------------------------------------------------
// Name: BuildFlatListCB()
// Desc: Callback function for the enumeration invoked by ReassignDevices(). 
//       This method just adds devices to a flat array that represents all the 
//       devices present on the machine.
//-----------------------------------------------------------------------------
BOOL CALLBACK CInputDeviceManager::BuildFlatListCB( LPCDIDEVICEINSTANCE  pdidi,
                                                    LPDIRECTINPUTDEVICE8 pdidDevice, 
													DWORD  dwFlags,
													DWORD  dwRemainingDevices,
													LPVOID pContext )
{
    static DWORD dwIndex = 0;
    LPDIRECTINPUTDEVICE8* pdidDevArray;
    pdidDevArray = (LPDIRECTINPUTDEVICE8*)pContext;

    pdidDevArray[dwIndex++] = pdidDevice;
    pdidDevice->AddRef(); // Must AddRef any interfaces we keep.

    if(!dwRemainingDevices)
    {
        dwIndex = 0; 
    }

    return DIENUM_CONTINUE;
}



//-----------------------------------------------------------------------------
// Name: AddDeviceForPlayer()
// Desc: Adds a device to the internal array for a player. For gaming devices, 
//       this method sets an axis deadzone. This could be done elsewhere, but 
//       it makes sense to take care of it here since this is a centralized 
//       location.
//-----------------------------------------------------------------------------
VOID CInputDeviceManager::AddDeviceForPlayer( const DWORD dwPlayerNum, 
                                              const DIDEVICEINSTANCE* pdidi, 
									          const LPDIRECTINPUTDEVICE8 pdidDevice )
{
    DWORD                dwDeviceType = pdidi->dwDevType;

    // If this is a gaming device, set an axis deadzone.
    if(GET_DIDEVICE_TYPE(dwDeviceType) & (~(DI8DEVTYPE_MOUSE & DI8DEVTYPE_KEYBOARD)))
        {
        DIPROPDWORD dipdw;
        dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwObj        = 0;
        dipdw.diph.dwHow        = DIPH_DEVICE;
        dipdw.dwData            = 1000;

        // If we can't set the deadzone, don't worry. Setting it is a good idea to 
        // avoid jitter on axes, however.
        pdidDevice->SetProperty( DIPROP_DEADZONE, &dipdw.diph );
    }

    // Add the device to the end of the list for this user.
    DWORD iDev = 0;
    while( NULL != m_ppdidDevices[dwPlayerNum][iDev] ) iDev++;

    m_ppdidDevices[dwPlayerNum][iDev] = pdidDevice;
    m_ppdidDevices[dwPlayerNum][iDev]->AddRef();  // Must increment the ref cout when duplicating interface pointers.

    m_dwTotalDevices++;
}

//-----------------------------------------------------------------------------
// Name: ReassignDevices()
// Desc: Called after the the DirectInput Mapper Default UI returns from the 
//       ConfigureDevices call. This method starts by flushing the manager 
//       class's internal lists, then it creates a flat list of all devices
//       on the machine (by calling EnumDevicesBySemantics with a NULL username).
//       Once the list is built, it rolls through all devices to retrieve the
//       current owner (via DIPROP_USERNAME). If the device has a username, the 
//       method calls AddDeviceForPlayer method to give that device to the 
//       appropriate user.
//-----------------------------------------------------------------------------
VOID CInputDeviceManager::ReassignDevices()
{
    LPDIRECTINPUTDEVICE8 prgAllDev[MAX_DEVICES]; // Flat array for all devices on the machine.
    TCHAR szNameDev[MAX_PATH];
    DWORD dwDev;

    // Clean-out the class-internal array of devices for each user.
    CleanupDevices( APP_CLEANUP_PRESERVEASSIGNMENT );

    // Build a simple flat list of all devices currently attached to the machine. This
    // array will be used to reassign devices to each user.
    // 
    // Using a NULL username and omitting the DIEDBSFL_THISUSER flag enumerates all devices.
    ZeroMemory( prgAllDev, sizeof(prgAllDev) );
    m_pDI->EnumDevicesBySemantics( NULL, &m_diaf, BuildFlatListCB, 
                                   prgAllDev, DIEDBSFL_ATTACHEDONLY); 
   
    DIPROPSTRING dips;
	dips.diph.dwSize       = sizeof(DIPROPSTRING); 
	dips.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
	dips.diph.dwObj        = 0; // device property 
	dips.diph.dwHow        = DIPH_DEVICE; 

    // Now we've got an array with every device attached to the system. 
    // Loop through them all and assign them to a player in the temp array.
    dwDev = 0;
    while( prgAllDev[dwDev] )
    {
        prgAllDev[dwDev]->GetProperty( DIPROP_USERNAME,
							           &dips.diph );
        
        // Convert the string from Unicode to ANSI.
        WideCharToMultiByte( CP_ACP, 0, dips.wsz, -1,
                             szNameDev, MAX_PATH,NULL, NULL);

        // Determine who this device is now assigned to (as a DWORD value)
        // If the device is unassigned (i.e. no username), we skip it.
        if( strlen(szNameDev) )
        {
            DWORD dwAssignedTo;
            dwAssignedTo = (DWORD) ( (byte)szNameDev[strlen(szNameDev)-1] - ((byte)'1') );
            
            DIDEVICEINSTANCE didi;
            ZeroMemory( &didi, sizeof(didi) );
            didi.dwSize = sizeof(didi);
            prgAllDev[dwDev]->GetDeviceInfo( &didi );

            // Get the device ready to go again for their user.
            prgAllDev[dwDev]->BuildActionMap( &m_diaf, szNameDev, DIDBAM_DEFAULT );
            prgAllDev[dwDev]->SetActionMap( &m_diaf, szNameDev, DIDSAM_DEFAULT );
            prgAllDev[dwDev]->SetCooperativeLevel( m_hWnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND );
            
            // Now add it for the player.
            AddDeviceForPlayer( dwAssignedTo, &didi, prgAllDev[dwDev] );

#ifdef _DEBUG
            TCHAR str[MAX_PATH];
            sprintf(str, _T("--> DEVICE ASSIGNED <--\n") );
            OutputDebugString(str);
            sprintf(str, _T("  Player == %s\n"), szNameDev );
            OutputDebugString(str);
            sprintf(str, _T("  Device == %s\n"), didi.tszProductName );
            OutputDebugString(str);
            SYSTEMTIME st;
            FILETIME   ft;  

            // Convert the time to local time.
            FileTimeToLocalFileTime(&m_diaf.ftTimeStamp, &ft);

            // Convert the local file time from UTC to system time.
            FileTimeToSystemTime(&ft, &st);
    
            // Build a string showing the date and time.
            wsprintf( str, _T("  Timestamp == %02d/%02d/%d  %02d:%02d:%02d \n"), 
                      st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
            OutputDebugString(str);
#endif
        }

        dwDev++;
    }

    // All devices have been assigned a to a user in the new array. 
    // Clean up the local flat array
    dwDev = 0;
    while( prgAllDev[dwDev] )
    {
        prgAllDev[dwDev]->Release();
        prgAllDev[dwDev] = NULL;
        dwDev++;
    }
}


//-----------------------------------------------------------------------------
// Name: GetDevicesForPlayer()
// Desc: Allows access to the private, internal list of devices for a given player.
//-----------------------------------------------------------------------------
HRESULT CInputDeviceManager::GetDevicesForPlayer(DWORD dwPlayer, LPDIRECTINPUTDEVICE8** ppDevices)
{
    *ppDevices = (LPDIRECTINPUTDEVICE8*)&m_ppdidDevices[dwPlayer][0];

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: ConfigureDevices()
// Desc: Wrapper function for the ConfigureDevices call. Before calling 
//       ConfigureDevices, this function sets up a list of DIACTIONFORMATs and 
//       user names, and creates a custom colorset with which the UI should 
//       be displayed.
//-----------------------------------------------------------------------------
HRESULT CInputDeviceManager::ConfigureDevices( IUnknown* pSurface,
											   VOID* ConfigureDevicesCB,
											   DWORD dwFlags )
{
	HRESULT hr;

	// Initialize all the colors here
	DICOLORSET dics;
	ZeroMemory(&dics, sizeof(DICOLORSET));
	dics.dwSize = sizeof(DICOLORSET);

   	// Fill in all the params
	DICONFIGUREDEVICESPARAMS dicdp;
	ZeroMemory(&dicdp, sizeof(dicdp));
	dicdp.dwSize = sizeof(dicdp);
	dicdp.dwcUsers       = m_dwNumUsers;
	dicdp.lptszUserNames = m_szUserNames;

	dicdp.dwcFormats     = 1;
	dicdp.lprgFormats    = &m_diaf;
	dicdp.hwnd           = m_hWnd;
	dicdp.lpUnkDDSTarget = NULL;

    // Set UI color scheme
    dicdp.dics.dwSize = sizeof(dics);
    dicdp.dics.cTextFore        = D3DCOLOR_RGBA(255,255,255,255);
    dicdp.dics.cTextHighlight   = D3DCOLOR_RGBA(60,191,241,255);
    dicdp.dics.cCalloutLine     = D3DCOLOR_RGBA(255,255,255,128);
    dicdp.dics.cCalloutHighlight= D3DCOLOR_RGBA(60,191,241,255);
    dicdp.dics.cBorder          = D3DCOLOR_RGBA(140,152,140,128);
    dicdp.dics.cControlFill     = D3DCOLOR_RGBA(113,0,0,128);
    dicdp.dics.cHighlightFill   = D3DCOLOR_RGBA(0,0,0,128);
    dicdp.dics.cAreaFill        = D3DCOLOR_RGBA(0,0,0,128);


	// Unacquire all devices so that they don't control the game while 
	// in the default Config UI.
    DWORD dwDev = 0;
    for( DWORD i = 0; i < m_dwNumUsers; i++ )
    {
        while( m_ppdidDevices[i][dwDev] )
        {
		    m_ppdidDevices[i][dwDev]->Unacquire();
            dwDev++;
        }
        dwDev = 0;
    }

	hr = m_pDI->ConfigureDevices( NULL, &dicdp, dwFlags, NULL );

	return hr;
}	
	

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases the DirectInput objects used by the class.
//-----------------------------------------------------------------------------
VOID CInputDeviceManager::Cleanup()
{
    CleanupDevices( APP_CLEANUP_DEFAULT );
    
    if( m_pDI )
    {
        m_pDI->Release();
    }

    m_pDI = NULL;
}

//-----------------------------------------------------------------------------
// Name: CleanupDevices()
// Desc: Unacquires and releases the DirectInput device objects used by the class.
//-----------------------------------------------------------------------------
VOID CInputDeviceManager::CleanupDevices(DWORD dwCleanFlags)
{
    DWORD dwD = 0;

    for( DWORD dwP = 0; dwP < m_dwNumUsers; dwP++ )
    {
        while( m_ppdidDevices[dwP][dwD] )
        {     
            m_ppdidDevices[dwP][dwD]->Unacquire();
            if(APP_CLEANUP_DEFAULT == dwCleanFlags)
            {
                m_ppdidDevices[dwP][dwD]->SetActionMap( &m_diaf, NULL, DIDSAM_NOUSER );
            }
            m_ppdidDevices[dwP][dwD]->Release();
            m_ppdidDevices[dwP][dwD] = NULL;
            dwD++;
        }

        dwD = 0;
    }
}

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Global cleanup function for the app. This function simply invokes the 
//       manager class's internal cleanup method (to handle all of the DirectInput
//       objects), then deletes the manager class itself.
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( g_pInputDeviceManager )
    {
        g_pInputDeviceManager->Cleanup();
        delete g_pInputDeviceManager;
        g_pInputDeviceManager = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: PaintPlayerStatus()
// Desc: Paints a rudimentary UI to provide visual feedback based on each 
//       player's actions on the devices they own.
//-----------------------------------------------------------------------------
void PaintPlayerStatus(HWND hWnd, DWORD dwPlayer, PLAYERDATA pd)
{    
    // Outout strings.
	TCHAR* strBlank = _T("         ");
    TCHAR* str;

    HDC hDC;
    DWORD  dwY; // Y value for text display.
    DWORD  dwX; // X value for text display.

    // Init text and DC. 
    hDC = GetDC( hWnd );
    SetBkColor( hDC, RGB(0,0,0) );
    SetBkMode( hDC, OPAQUE );

    // Internally to the class, the Player number is zero-based. It's easier to 
    // work with them here as one-based.
    dwPlayer++;

    // Set text foreground color by player.
    switch( dwPlayer )
    {
        case 1: 
            // purple== Player 1
            SetTextColor( hDC, RGB(225, 50, 255) );
            str = _T("Player 1");
            break;

        case 2: 
            // red   == Player 2
            SetTextColor( hDC, RGB(255, 40, 40) );
            str = _T("Player 2");
            break;

        case 3: 
            // green == Player 3
            SetTextColor( hDC, RGB(60, 255, 60) );
            str = _T("Player 3");
            break;

        case 4: 
            // blue  == Player 4
            SetTextColor( hDC, RGB(60, 60, 255) );
            str = _T("Player 4");
            break;
    }

    // Generate the Y value at which to display text for this player. 
    dwY = (dwPlayer * LINE_SPACING);
    dwX = 0;

    TextOut( hDC, dwX, dwY, str, lstrlen(str) );
    dwX += COL_SPACING;

    // The remainder of this function is simply to output the 
    // results of gathering the input.
    if(pd.bTurningLeft)
        str = _T("Left   ");
    else if (pd.bTurningRight)
        str = _T("Right"); 
    else 
        str = strBlank;

    TextOut( hDC, dwX, dwY, str, lstrlen(str) );
    dwX += COL_SPACING;

    if(pd.bForwardThrust)
        str = _T("Fore");
    else if(pd.bReverseThrust)
        str = _T("Aft   ");
    else
        str = strBlank;
    
    TextOut( hDC, dwX, dwY, str, lstrlen(str) );
    dwX += COL_SPACING;

    str =  pd.bFiringWeapons ? _T("Fire") : strBlank;
    TextOut( hDC, dwX, dwY, str, lstrlen(str) );

    dwX += COL_SPACING;
    str =  pd.bEnableShields ? _T("Up     ") : _T("Down");
    TextOut( hDC, dwX, dwY, str, lstrlen(str) );

    ReleaseDC( hWnd, hDC );
}


//-----------------------------------------------------------------------------
// Name: GameLoop()
// Desc: This is the input loop for the app. Input is gathered from the 
//       DirecInput devices, and output is displayed simply in the app's window.
//-----------------------------------------------------------------------------
BOOL GameLoop( HWND hWnd )
{
    BOOL bRet;
	DIDEVICEOBJECTDATA didObjData;
    ZeroMemory( &didObjData, sizeof(didObjData) );
    static PLAYERDATA pd[MAX_USERS]; // State of each player
    LPDIRECTINPUTDEVICE8* pdidDevices;
    
    DWORD dwNumPlayers = g_pInputDeviceManager->GetNumUsers();
            
    // Loop through all devices for all players and check game input.
    for( DWORD i=0; i < dwNumPlayers; i++ )
    {
        // Get access to the list of input devices.
        g_pInputDeviceManager->GetDevicesForPlayer( i, &pdidDevices );

        bRet = ParsePlayerInput(hWnd, pdidDevices, &pd[i]);
        if( !bRet )
        {
            return bRet;
        }

        // Remove any conflicts (in a game, you couldn't go left and right at the same
        // time. Actual conflicts depend on the game logic, and not on the DirectInput
        // semantic mappings.)
        //
        // !!NOTE!!: This imposes some logic that a game MIGHT want (i.e. can't go
        //           forward and back at the same time). Your app might want to do
        //           this differently. 
        if( pd[i].bTurningLeft && pd[i].bTurningRight )
        {
            pd[i].bTurningLeft = pd[i].bTurningRight = FALSE;
        }
        if( pd[i].bForwardThrust && pd[i].bReverseThrust )
        {
            pd[i].bForwardThrust = pd[i].bReverseThrust = FALSE;
        }

        // Paint the status for this device. This could just as easily be a call 
        // to update the frame of your game. 
        PaintPlayerStatus( hWnd, i, pd[i] );
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ParsePlayerInput()
// Desc: Parses the actions buffered for the passed-in devices, and places the 
//       results into the provided PLAYERDATA structure. This data represents
//       the in game actions for a player.
//-----------------------------------------------------------------------------
BOOL ParsePlayerInput(HWND hWnd, LPDIRECTINPUTDEVICE8* ppdidDevices, LPPLAYERDATA ppd)
{
    HRESULT hr;
    DWORD   dwItems;
    DIDEVICEOBJECTDATA adod[BUFFER_SIZE];

    DWORD   dwDevice = 0;
    while ( ppdidDevices[dwDevice] )
    {
        
        // Poll the device to read the current state
        if( FAILED(ppdidDevices[dwDevice]->Poll() ) )  
        {
            // DirectInput is telling us that the input stream has been
            // interrupted. We aren't tracking any state between polls, so
            // we don't have any special reset that needs to be done. We
            // just re-acquire and try again.
            hr = ppdidDevices[dwDevice]->Acquire();
            if( DIERR_INPUTLOST  == hr) 
            {
                hr = ppdidDevices[dwDevice]->Acquire();
            }

            // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
            // may occur when the app is minimized or in the process of 
            // switching, so just try again later 
            return TRUE; 
        }        
        		
        // Retrieve the buffered actions from the device.
        dwItems = BUFFER_SIZE;
        hr = ppdidDevices[dwDevice]->GetDeviceData( sizeof(DIDEVICEOBJECTDATA),
                                                    adod, &dwItems, 0 );


		if( SUCCEEDED(hr) )
		{           
            // Get the actions. The number of input events is stored in
			// dwItems, and all the events are stored in the "adod" array. Each
			// event has a type stored in uAppData, and actual data stored in
			// dwData.
			for( DWORD j=0; j<dwItems; j++ )
			{
				// Non-axis data is recieved as "button pressed" or "button
				// released". Parse input as such.
				BOOL bState = (adod[j].dwData != 0 ) ? TRUE : FALSE;

            	switch (adod[j].uAppData)
				{
				case INPUT_LEFTRIGHT_AXIS: // Parse the left-right axis data
					(*ppd).dwLRAxisData  = adod[j].dwData;
					(*ppd).bTurningRight = (*ppd).bTurningLeft  = FALSE;					
					if( (int)(*ppd).dwLRAxisData > 0 )
						(*ppd).bTurningRight = TRUE;
					else if( (int)(*ppd).dwLRAxisData < 0 )
						(*ppd).bTurningLeft = TRUE;
                    break;

				case INPUT_UPDOWN_AXIS: // Parse the up-down axis data
					(*ppd).dwUDAxisData   = adod[j].dwData;
					(*ppd).bReverseThrust = (*ppd).bForwardThrust = FALSE;

					if( (int)(*ppd).dwUDAxisData > 0 )
						(*ppd).bReverseThrust = TRUE;
					else if( (int)(*ppd).dwUDAxisData < 0 )
						(*ppd).bForwardThrust = TRUE;
					break;
					
				case INPUT_TURNLEFT:        (*ppd).bTurningLeft    = bState; break;
				case INPUT_TURNRIGHT:       (*ppd).bTurningRight   = bState; break;
				case INPUT_FORWARDTHRUST:   (*ppd).bForwardThrust  = bState; break;
				case INPUT_REVERSETHRUST:   (*ppd).bReverseThrust  = bState; break;
				case INPUT_FIREWEAPONS:     (*ppd).bFiringWeapons  = bState; break;
				case INPUT_ENABLESHIELD:    (*ppd).bEnableShields  = bState; break;
				
                case INPUT_QUITGAME:        return FALSE;				
				
				case INPUT_DISPLAYGAMEMENU: 
					(*ppd).bDisplayingMenu = bState; 
                    InvokeDefaultUI(hWnd);
					return TRUE;
                }
			}
        }

        // Go to the next device in the list.
        dwDevice++;
   	}

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InvokeDefaultUI()
// Desc: Global function to invoke the Default UI for the DirectInput mapper. 
//       This function uses the ConfigureDevices wrapper method for this task.
//       Note that when the UI is invoked in edit mode (DICD_EDIT), you should
//       assume that devices and the configuration of those devices will probably
//       change due to user interaction. After ConfigureDevices returns, this 
//       function calls the ReassignDevices method on the manager class to 
//       sort out which devices belong to whom, and rebuild action maps to reflect
//       any config changes that may have taken place.
//-----------------------------------------------------------------------------
void InvokeDefaultUI(HWND hWnd)
{
    g_pInputDeviceManager->ConfigureDevices(NULL, NULL, DICD_EDIT);
    g_pInputDeviceManager->ReassignDevices();
}


//-----------------------------------------------------------------------------
// Name: CreateInputStuff()
// Desc: Creates the DirectInput helper class. The DIACTIONFORMAT specifies what
//       type of devices we are looking for (via the genre). Note: genres are
//       defined in the docs and the dinput.h header file. The DIACTIONFORMAT
//       structure also is used to specify the game action array, defined
//       at the beginnning of this file.
//-----------------------------------------------------------------------------
HRESULT CreateInputStuff( HWND hWnd, DWORD dwNumUsers )
{
    HRESULT hr;

    // Setup action format for suitable input devices for this app
    DIACTIONFORMAT diaf;
    ZeroMemory( &diaf, sizeof(DIACTIONFORMAT) );
    diaf.dwSize        = sizeof(DIACTIONFORMAT);
    diaf.dwActionSize  = sizeof(DIACTION);
    diaf.dwDataSize    = NUMBER_OF_SEMANTICS * sizeof(DWORD);
    diaf.dwNumActions  = NUMBER_OF_SEMANTICS;
    diaf.guidActionMap = g_AppGuid;
    diaf.dwGenre       = DIVIRTUAL_SPACESIM;
    diaf.rgoAction     = g_rgGameAction;
    diaf.dwBufferSize  = BUFFER_SIZE;
    diaf.lAxisMin      = -100;
    diaf.lAxisMax      = 100;
    _tcscpy( diaf.tszActionMap, _T("MultiMapper Sample Application") );

    // Create a new input device manager
    g_pInputDeviceManager = new CInputDeviceManager();
    if( !g_pInputDeviceManager )
    {
        return E_OUTOFMEMORY;
    }

    hr = g_pInputDeviceManager->Create( hWnd, &dwNumUsers, &diaf, APP_DICREATE_DEFAULT );

    if( FAILED(hr) )
    {
        TCHAR msg[MAX_PATH];
        int iRet;
        
        switch(hr)
        {
        // It's possible that a single user could "own" too many devices for the other
        // players to get into the game. If so, we reinit the manager class to provide 
        // each user with a device that has a default configuration.
        case E_APPERR_DEVICESTAKEN:
            sprintf( msg, 
                 _T("You have entered more users than there are suitable devices, " \
                 "or some users are claiming too many devices.\n\n" \
                 "Click Yes to give each user a default device, or click No " \
                 "to close the application"));

            iRet = MessageBox( hWnd, msg, _T("Devices Are Taken"), 
                               MB_YESNO | MB_ICONEXCLAMATION );

            if(iRet == IDYES)
            {
                g_pInputDeviceManager->Cleanup();
                g_pInputDeviceManager->Create( hWnd, &dwNumUsers, &diaf, APP_DICREATE_FORCEINIT );
            }
            else
                return E_FAIL;
            
            break;

        // Another common error is if more users are attempting to play than there are devices
        // attached to the machine. In this case, the number of players is automatically 
        // lowered to make playing the game possible. 
        case E_APPERR_TOOMANYUSERS:
            DWORD dwNumDevices = g_pInputDeviceManager->GetNumDevices();
            dwNumUsers = dwNumDevices;
            TCHAR* str = _T( "There are not enough devices attached to the system " \
                             "for the number of users you entered.\n\n" \
                             "The number of users has been automatically changed " \
                             "to %i (the number of devices available on the system).");

            sprintf( msg, str, dwNumDevices);
            MessageBox( hWnd, msg, _T("Too Many Users"), 
                        MB_OK | MB_ICONEXCLAMATION );

            g_pInputDeviceManager->Cleanup();
            g_pInputDeviceManager->Create( hWnd, &dwNumUsers, &diaf, APP_DICREATE_DEFAULT );                           
            break;
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: MainWndproc()
// Desc: Callback for all Windows messages
//-----------------------------------------------------------------------------
long FAR PASCAL MainWndproc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            // Output message to user
            CHAR* str;
            HDC hDC = GetDC( hWnd );
            
            SetTextColor( hDC, RGB(255,255,255) );
            SetBkColor( hDC, RGB(0,0,0) );
            SetBkMode( hDC, OPAQUE );
            
            str = _T("Name");
            TextOut( hDC, 0, 0, str, lstrlen(str) );
            str = _T("Turn");
            TextOut( hDC, COL_SPACING*1, 0, str, lstrlen(str) );
            str = _T("Thrust");
            TextOut( hDC, COL_SPACING*2, 0, str, lstrlen(str) );
            str = _T("Weapon");
            TextOut( hDC, COL_SPACING*3, 0, str, lstrlen(str) );
            str = _T("Shield");
            TextOut( hDC, COL_SPACING*4, 0, str, lstrlen(str) );

            str = _T("Looking for game input... press Escape to exit.");
            TextOut( hDC, 0, 140, str, lstrlen(str) );

			str = _T("Press D to display input device settings.");
            TextOut( hDC, 0, 158, str, lstrlen(str) );

            ReleaseDC( hWnd, hDC );
            break;
        }

        // Handling global keystrokes as WM_CHAR messages. In this case, it's 
        // actually easier to use Windows than a specialized action map.
        case WM_CHAR:
            if( VK_ESCAPE == (TCHAR)wParam )
                SendMessage( hWnd, WM_DESTROY, 0, 0 );
            else if ( 0x64 == (TCHAR)wParam ) // 0x64 == 'D'
                InvokeDefaultUI(hWnd);
            break;

        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

INT_PTR CALLBACK NumPlayerFunc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUMPLAYERS));
        SetDlgItemInt( hDlg, IDC_EDIT_NUMPLAYERS, MIN_USERS, FALSE);
        SendDlgItemMessage(hDlg, IDC_EDIT_NUMPLAYERS, EM_SETSEL, 0, -1);
        return FALSE;

    case WM_COMMAND:
        if (wParam == IDOK) 
        { 
            UINT nNumPlayers = 0;
            BOOL bSuccess;
            nNumPlayers= GetDlgItemInt( hDlg, IDC_EDIT_NUMPLAYERS, 
                                        &bSuccess, FALSE);

            if(bSuccess && (nNumPlayers >0 && nNumPlayers <=MAX_USERS))
                EndDialog(hDlg, (INT)nNumPlayers); 
            else
            {
                SetDlgItemInt( hDlg, IDC_EDIT_NUMPLAYERS, MIN_USERS, FALSE);
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUMPLAYERS));
                SendDlgItemMessage(hDlg, IDC_EDIT_NUMPLAYERS, EM_SETSEL, 0, -1);
                MessageBeep(MB_OK);
            }
            return TRUE; 
        } 
    }
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Application entry point
//-----------------------------------------------------------------------------
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow )
{
    // Register the window class
    WNDCLASS wndClass = { CS_DBLCLKS, MainWndproc, 0, 0, hInstance, NULL,
                          LoadCursor( NULL, IDC_ARROW ), 
                          (HBRUSH)GetStockObject( BLACK_BRUSH ),
                          NULL, _T("IMapClass") };
    RegisterClass( &wndClass );

    // Create our main window
    HWND hWnd = CreateWindowEx( 0, _T("IMapClass"), _T("MultiMapper"),
                                WS_VISIBLE|WS_POPUP|WS_CAPTION|WS_SYSMENU,
                                200, 200, 400, 200, NULL, NULL, hInstance, NULL );
    if( NULL == hWnd )
    {
        return FALSE;
    }

    UpdateWindow( hWnd );

    UINT dwNumUsers;
    // Query the user(s) for the number of players.
    dwNumUsers = DialogBox( hInstance, 
                            MAKEINTRESOURCE(IDD_NUMPLAYERS),
                            hWnd,
                            (DLGPROC)NumPlayerFunc);

    // Create the DirectInput helper class
    if( FAILED( CreateInputStuff( hWnd, dwNumUsers ) ) )
    {
        SendMessage( hWnd, WM_DESTROY, 0, 0 );
        return 0;
    }

    // Traditional message loop to "run" the app
    while( TRUE )
    {
        MSG msg;

        if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
        {
            if( FALSE == GetMessage( &msg, NULL, 0, 0 ) )
                return msg.wParam;

            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            // Most apps use idle time for processing the game loop
            if( FALSE == GameLoop( hWnd ) )
                SendMessage( hWnd, WM_DESTROY, 0, 0 );
        }
    }
}

