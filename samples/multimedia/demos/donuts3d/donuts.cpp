//-----------------------------------------------------------------------------
// File: Donuts.cpp
//
// Desc: DirectInput semantic mapper version of Donuts3D game
//
// Copyright (C) 1995-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <basetsd.h>
#include <cguid.h>
#include <tchar.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>
#include <D3DX8.h>
#include "D3DFile.h"
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DIUtil.h"
#include "DMUtil.h"
#include "DXUtil.h"
#include "resource.h"
#include "donuts.h"
#include "gamemenu.h"




//-----------------------------------------------------------------------------
// Custom Direct3D vertex types
//-----------------------------------------------------------------------------
struct SCREENVERTEX
{
    D3DXVECTOR4 p;
    DWORD       color;
};

struct SPRITEVERTEX
{
    D3DXVECTOR3 p;
    DWORD       color;
    FLOAT       tu, tv;
};

struct MODELVERTEX
{
    D3DXVECTOR3 p;
    D3DXVECTOR3 n;
    FLOAT       tu, tv;
};

#define D3DFVF_SCREENVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
#define D3DFVF_SPRITEVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_MODELVERTEX  (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)




//-----------------------------------------------------------------------------
// Application globals
//-----------------------------------------------------------------------------
TCHAR*               g_strAppName     = _T("Donuts3D");
GUID                 g_AppGuid        = { 0x451F8CCC, 0xA7E9, 0x4DF4, 0x9A, 0x6B,
                                          0xF4, 0xA7,0xC7, 0x06, 0xF3, 0x33 };

HWND                 g_hWndMain;               // Main window
DWORD                g_dwScreenWidth  = 800;   // Dimensions for fullscreen modes
DWORD                g_dwScreenHeight = 600;
D3DDISPLAYMODE       g_DesktopMode;
D3DFORMAT            g_d3dfmtFullscreen;       // Pixel format for fullscreen modes
D3DFORMAT            g_d3dfmtTexture;          // Pixel format for textures
BOOL                 g_bFullScreen    = FALSE; // Whether app is fullscreen (or windowed)
BOOL                 g_bIsActive;              // Whether app is active
BOOL                 g_bDisplayReady  = FALSE; // Whether display class is initialized
BOOL                 g_bMouseVisible  = TRUE;  // Whether mouse is visible
HBITMAP              g_hSplashBitmap  = NULL;  // Bitmap for splash screen

DWORD                g_dwAppState;              // Current state the app is in
DWORD                g_dwLevel        = 0;      // Current game level
DWORD                g_dwScore        = 0;      // Current game score

// Player view mode
#define NUMVIEWMODES 3
CD3DCamera           g_Camera;                       // Camera used for 3D scene
DWORD                g_dwViewMode           = 0;     // Which view mode is being used
FLOAT                g_fViewTransition      = 0.0f;  // Amount used to transittion views
BOOL                 g_bAnimatingViewChange = FALSE; // Whether view is transitioning
BOOL                 g_bFirstPersonView     = TRUE;  // Whether view is first-person

// Bullet mode
FLOAT                g_fBulletRechargeTime  = 0.0f;  // Recharge time for firing bullets
DWORD                g_dwBulletType         = 0L;    // Current bullet type

// Display list and player ship
DisplayObject*       g_pDisplayList = NULL;          // Global display list
CShip*               g_pShip        = NULL;          // Player's display object

// DirectDraw/Direct3D objects
LPDIRECT3DDEVICE8       g_pd3dDevice        = NULL;  // Class to handle D3D device
D3DPRESENT_PARAMETERS   g_d3dpp;
LPDIRECT3DSURFACE8      g_pConfigSurface    = NULL;  // Surface for config'ing DInput devices
LPDIRECT3DVERTEXBUFFER8 g_pViewportVB       = NULL;
LPDIRECT3DVERTEXBUFFER8 g_pSpriteVB         = NULL;

// Support for the ship model
CD3DMesh*            g_pShipFileObject   = NULL;      // Geometry model of player's ship
DWORD                g_dwNumShipTypes    = 10L;
DWORD                g_dwCurrentShipType = 0L;
TCHAR*               g_strShipFiles[]    = { _T("Concept Plane 3.x"), _T("Spaceship 2.x"), _T("Shusui.x"),
                                             _T("Space Station 7.x"), _T("Spaceship 8.x"), _T("Orbiter.x"),
                                             _T("Spaceship 13.x"),    _T("Spaceship 5.x"), _T("Star Sail.x"), 
                                             _T("Heli.x"), };
TCHAR*               g_strShipNames[]    = { _T("Concept Plane"), _T("Green Machine"),  _T("Purple Prowler"),
                                             _T("Drone Clone"),   _T("Canyon Fighter"), _T("Roundabout"),
                                             _T("Tie-X7"),        _T("Gunner"),         _T("Star Sail"), 
                                             _T("Helicopter"), };

// DirectMusic objects
CMusicManager*       g_pMusicManager        = NULL;  // Class to manage DMusic objects
CMusicSegment*       g_pBeginLevelSound     = NULL;  // Sounds for the app
CMusicSegment*       g_pEngineIdleSound     = NULL;
CMusicSegment*       g_pEngineRevSound      = NULL;
CMusicSegment*       g_pShieldBuzzSound     = NULL;
CMusicSegment*       g_pShipExplodeSound    = NULL;
CMusicSegment*       g_pFireBulletSound     = NULL;
CMusicSegment*       g_pShipBounceSound     = NULL;
CMusicSegment*       g_pDonutExplodeSound   = NULL;
CMusicSegment*       g_pPyramidExplodeSound = NULL;
CMusicSegment*       g_pCubeExplodeSound    = NULL;
CMusicSegment*       g_pSphereExplodeSound  = NULL;

// Game objects
LPDIRECT3DTEXTURE8   g_pGameTexture1 = NULL; // Texture with game object animations
LPDIRECT3DTEXTURE8   g_pGameTexture2 = NULL; // Texture with game object animations
CD3DMesh*            g_pTerrain        = NULL;    // Geometry model of terrain
CD3DFont*            g_pGameFont       = NULL;    // Font for displaying score, etc.
CD3DFont*            g_pMenuFont       = NULL;    // Font for displaying in-game menus


// Menu objects
CMenuItem*           g_pMainMenu       = NULL;    // Menu class for in-game menus
CMenuItem*           g_pQuitMenu       = NULL;
CMenuItem*           g_pCurrentMenu    = NULL;

// Defines for the in-game menu
#define MENU_MAIN           1
#define MENU_SOUND          2
#define MENU_VIDEO          3
#define MENU_INPUT          4
#define MENU_VIEWDEVICES    5
#define MENU_CONFIGDEVICES  6
#define MENU_WINDOWED       7
#define MENU_640x480        8
#define MENU_800x600        9
#define MENU_1024x768      10
#define MENU_BACK          11
#define MENU_SOUNDON       12
#define MENU_SOUNDOFF      13
#define MENU_QUIT          14


// DirectInput objects
CInputDeviceManager* g_pInputDeviceManager = NULL; // Class for managing DInput devices
DIACTIONFORMAT       g_diafGame;                   // Action format for game play
DIACTIONFORMAT       g_diafBrowser;                // Action format for menu navigation

// Game input variables
FLOAT                g_fBank           = 0.0f;
FLOAT                g_fThrust         = 0.0f;
BOOL                 g_bFiringWeapons  = FALSE;
BOOL                 g_bChangeView     = FALSE;
BOOL                 g_bPaused         = FALSE;

// Menu input variables
BOOL                 g_bMenuLeft       = FALSE;
BOOL                 g_bMenuRight      = FALSE;
BOOL                 g_bMenuUp         = FALSE;
BOOL                 g_bMenuDown       = FALSE;
BOOL                 g_bMenuSelect     = FALSE;
BOOL                 g_bMenuQuit       = FALSE;


//-----------------------------------------------------------------------------
// Game actions (using DInput semantic mapper). The definitions here are kind
// of the whole point of this sample. The game uses these actions to map
// physical input like, "the user pressed the 'W' key", to a more useable
// constant for the game, like "if( dwInput == INPUT_CHANGEWEAPONS )...".
//-----------------------------------------------------------------------------


// Input semantics used by this game
enum INPUT_SEMANTICS
{
    // Gameplay semantics
    INPUT_AXIS_LR=1,     INPUT_AXIS_UD,       INPUT_AXIS_SHIPTYPE,
    INPUT_MOUSE_LR,      INPUT_MOUSE_UD,      INPUT_MOUSE_SHIPTYPE,
    INPUT_TURNLEFT,      INPUT_TURNRIGHT,     INPUT_FORWARDTHRUST,
    INPUT_REVERSETHRUST, INPUT_FIREWEAPONS,   INPUT_CHANGESHIPTYPE,
    INPUT_CHANGEVIEW,    INPUT_CHANGEWEAPONS, INPUT_DISPLAYGAMEMENU,
    INPUT_QUITGAME,      INPUT_START,

    // Menu semantics
    INPUT_MENU_LR,       INPUT_MENU_UD,       INPUT_MENU_WHEEL,
    INPUT_MENU_UP,       INPUT_MENU_DOWN,     INPUT_MENU_LEFT,
    INPUT_MENU_RIGHT,    INPUT_MENU_SELECT,   INPUT_MENU_QUIT,
};

// Game actions used by this game.
DIACTION g_rgGameAction[] =
{
    { INPUT_AXIS_LR,         DIAXIS_SPACESIM_LATERAL,   0, TEXT("Turn"), },
    { INPUT_AXIS_UD,         DIAXIS_SPACESIM_MOVE,      0, TEXT("Move"), },
    { INPUT_FIREWEAPONS,     DIBUTTON_SPACESIM_FIRE,    0, TEXT("Fire weapons"), },
    { INPUT_CHANGEVIEW,      DIBUTTON_SPACESIM_VIEW,    0, TEXT("Change view"), },
    { INPUT_CHANGEWEAPONS,   DIBUTTON_SPACESIM_WEAPONS, 0, TEXT("Change weapons"), },
    { INPUT_CHANGESHIPTYPE,  DIBUTTON_SPACESIM_LOWER,    0, TEXT("Change ship type"), },
    { INPUT_DISPLAYGAMEMENU, DIBUTTON_SPACESIM_DEVICE,    0, TEXT("Display game menu"), },
    { INPUT_START,           DIBUTTON_SPACESIM_MENU,  0, TEXT("Start/pause"), },

    { INPUT_TURNLEFT,        DIKEYBOARD_LEFT,    0, TEXT("Turn left"), },
    { INPUT_TURNRIGHT,       DIKEYBOARD_RIGHT,   0, TEXT("Turn right"), },
    { INPUT_FORWARDTHRUST,   DIKEYBOARD_UP,      0, TEXT("Forward thrust"), },
    { INPUT_REVERSETHRUST,   DIKEYBOARD_DOWN,    0, TEXT("Reverse thrust"), },
    { INPUT_FIREWEAPONS,     DIKEYBOARD_SPACE,   0, TEXT("Fire weapons"), },
    { INPUT_CHANGESHIPTYPE,  DIKEYBOARD_A,       0, TEXT("Change ship type"), },
    { INPUT_CHANGEVIEW,      DIKEYBOARD_V,       0, TEXT("Change view"), },
    { INPUT_CHANGEWEAPONS,   DIKEYBOARD_W,       0, TEXT("Change weapons"), },
    { INPUT_DISPLAYGAMEMENU, DIKEYBOARD_F1,      DIA_APPFIXED, TEXT("Display game menu"), },
    { INPUT_START,           DIKEYBOARD_PAUSE,   0, TEXT("Start/pause"), },
    { INPUT_QUITGAME,        DIKEYBOARD_ESCAPE,  DIA_APPFIXED, TEXT("Quit game"), },

    { INPUT_MOUSE_LR,        DIMOUSE_XAXIS,      0, TEXT("Turn"), },
    { INPUT_MOUSE_UD,        DIMOUSE_YAXIS,      0, TEXT("Move"), },
    { INPUT_MOUSE_SHIPTYPE,  DIMOUSE_WHEEL,      0, TEXT("Change ship type"), },
    { INPUT_FIREWEAPONS,     DIMOUSE_BUTTON0,    0, TEXT("Fire weapons"), },
    { INPUT_CHANGEWEAPONS,   DIMOUSE_BUTTON1,    0, TEXT("Change weapons"), },
};

// Game actions used by this game.
DIACTION g_rgBrowserAction[] =
{
    { INPUT_MENU_LR,         DIAXIS_BROWSER_LATERAL,    0, TEXT("Left/right"), },
    { INPUT_MENU_UD,         DIAXIS_BROWSER_MOVE,       0, TEXT("Up/down"), },
    { INPUT_MENU_SELECT,     DIBUTTON_BROWSER_SELECT,   0, TEXT("Select"), },
    { INPUT_MENU_UP,         DIBUTTON_BROWSER_PREVIOUS, 0, TEXT("Up"), },
    { INPUT_MENU_DOWN,       DIBUTTON_BROWSER_NEXT,     0, TEXT("Down"), },
    { INPUT_MENU_QUIT,       DIBUTTON_BROWSER_DEVICE,   0, TEXT("Quit menu"), },

    { INPUT_MENU_LEFT,       DIKEYBOARD_LEFT,        0, TEXT("Left"), },
    { INPUT_MENU_RIGHT,      DIKEYBOARD_RIGHT,       0, TEXT("Right"), },
    { INPUT_MENU_UP,         DIKEYBOARD_UP,          0, TEXT("Up"), },
    { INPUT_MENU_DOWN,       DIKEYBOARD_DOWN,        0, TEXT("Down"), },
    { INPUT_MENU_SELECT,     DIKEYBOARD_SPACE,       0, TEXT("Select"), },
    { INPUT_MENU_SELECT,     DIKEYBOARD_RETURN,      0, TEXT("Select"), },
    { INPUT_MENU_SELECT,     DIKEYBOARD_NUMPADENTER, 0, TEXT("Select"), },
    { INPUT_MENU_QUIT,       DIKEYBOARD_ESCAPE,      0, TEXT("Quit menu"), },

    { INPUT_MENU_WHEEL,      DIMOUSE_WHEEL,      0, TEXT("Up/down"), },
    { INPUT_MENU_SELECT,     DIMOUSE_BUTTON0,    0, TEXT("Select"), },
};

// Number of actions
#define NUMBER_OF_GAMEACTIONS    (sizeof(g_rgGameAction)/sizeof(DIACTION))
#define NUMBER_OF_BROWSERACTIONS (sizeof(g_rgBrowserAction)/sizeof(DIACTION))




//-----------------------------------------------------------------------------
// Inline helper functions
//-----------------------------------------------------------------------------

// Simple function to define "hilliness" for terrain
inline FLOAT HeightField( FLOAT x, FLOAT z )
{
    return (cosf(x/2.0f+0.2f)*cosf(z/1.5f-0.2f)+1.0f) - 2.0f;
}

// Simple function for generating random numbers
inline FLOAT rnd( FLOAT low, FLOAT high )
{
    return low + ( high - low ) * ( (FLOAT)rand() ) / RAND_MAX;
}

// Convenient macros for playing sounds
inline VOID PlaySound( CMusicSegment* pSound )
{
        if( pSound ) pSound->Play( DMUS_SEGF_SECONDARY );
}

inline VOID StopSound( CMusicSegment* pSound )
{
        if( pSound ) pSound->Stop();
}




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Application entry point
//-----------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow )
{
    // Register the window class
    WNDCLASS wndClass = { CS_DBLCLKS, WndProc, 0, 0, hInstance,
                          LoadIcon( hInstance, MAKEINTRESOURCE(DONUTS_ICON) ),
                          LoadCursor( NULL, IDC_ARROW ),
                          (HBRUSH)GetStockObject( BLACK_BRUSH ),
                          NULL, TEXT("DonutsClass") };
    RegisterClass( &wndClass );

    // Create our main window
    g_hWndMain = CreateWindowEx( 0, TEXT("DonutsClass"), TEXT("Donuts"),
                                 WS_VISIBLE|WS_POPUP|WS_CAPTION|WS_SYSMENU,
                                 0, 0, 640, 480, NULL, NULL,
                                 hInstance, NULL );
    if( NULL == g_hWndMain )
        return FALSE;
    UpdateWindow( g_hWndMain );

        // Create the game objects (display objects, sounds, input devices,
        // menus, etc.)
    if( FAILED( CreateGameObjects( g_hWndMain ) ) )
    {
        DestroyWindow( g_hWndMain );
        return FALSE;
    }

    // Load keyboard accelerators
    HACCEL hAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );

    // Now we're ready to recieve and process Windows messages.
    BOOL bGotMsg;
    MSG  msg;
    PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

    while( WM_QUIT != msg.message  )
    {
        // Use PeekMessage() if the app is active, so we can use idle time to
        // render the scene. Else, use GetMessage() to avoid eating CPU time.
        if( g_bIsActive )
            bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );
        else
            bGotMsg = GetMessage( &msg, NULL, 0U, 0U );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            // Render a frame during idle time (no messages are waiting)
            if( g_bDisplayReady )
            {
                FrameMove();
                RenderFrame();
            }
        }
    }

    return (int)msg.wParam;
}




//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Callback for all Windows messages
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_ACTIVATEAPP:
            g_bIsActive = (BOOL)wParam;

            if( g_bIsActive )
            {
                g_bMouseVisible   = FALSE;
                DXUtil_Timer( TIMER_START );
            }
            else
            {
                g_bMouseVisible = TRUE;
                DXUtil_Timer( TIMER_STOP );
            }
            break;

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 320;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
            break;

        case WM_SETCURSOR:
            if( !g_bMouseVisible && g_dwAppState!=APPSTATE_DISPLAYSPLASH )
                SetCursor( NULL );
            else
                SetCursor( LoadCursor( NULL, IDC_ARROW ) );
            return TRUE;

        case WM_SYSCOMMAND:
            // Prevent moving/sizing and power loss
            switch( wParam )
            {
                case SC_MOVE:
                case SC_SIZE:
                case SC_MAXIMIZE:
                case SC_KEYMENU:
                case SC_MONITORPOWER:
                        return 1;
            }
            break;

                case WM_SYSKEYDOWN:
            // Handle Alt+Enter to do mode-switching
            if( VK_RETURN == wParam )
            {
                SwitchDisplayModes( !g_bFullScreen, g_dwScreenWidth,
                                    g_dwScreenHeight );
            }
            break;

        case WM_KEYDOWN:
            // Move from splash screen when user presses a key
            if( g_dwAppState == APPSTATE_DISPLAYSPLASH )
            {
                if( wParam==VK_ESCAPE )
                {
                    // Escape keys exits the app
                    PostMessage( hWnd, WM_CLOSE, 0, 0 );
                    g_bDisplayReady = FALSE;
                }
                else
                {
                    // Get rid of splash bitmap
                    DeleteObject( g_hSplashBitmap );

                    // Advance to the first level
                    g_dwAppState = APPSTATE_BEGINLEVELSCREEN;
                    DXUtil_Timer( TIMER_START );
                    AdvanceLevel();
                }
            }
            return 0;

        case WM_PAINT:
            if( g_dwAppState == APPSTATE_DISPLAYSPLASH )
            {
                BITMAP bmp;
                RECT rc;
                GetClientRect( g_hWndMain, &rc );

                // Display the splash bitmap in the window
                HDC hDCWindow = GetDC( g_hWndMain );
                HDC hDCImage  = CreateCompatibleDC( NULL );
                SelectObject( hDCImage, g_hSplashBitmap );
                GetObject( g_hSplashBitmap, sizeof(bmp), &bmp );
                StretchBlt( hDCWindow, 0, 0, rc.right, rc.bottom,
                            hDCImage, 0, 0,
                            bmp.bmWidth, bmp.bmHeight, SRCCOPY );
                DeleteDC( hDCImage );
                ReleaseDC( g_hWndMain, hDCWindow );
            }
            else
            {
                if( g_bDisplayReady )
                {
                    DrawDisplayList();
                    ShowFrame();
                }
            }
            break;

        case WM_DESTROY:
            DestroyGameObjects();
            PostQuitMessage( 0 );
            g_bDisplayReady = FALSE;
            break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: CreateGameObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CreateGameObjects( HWND hWnd )
{
    HRESULT hr;

    // Initialize the DirectInput stuff
    if( FAILED( hr = CreateInputObjects( hWnd ) ) )
        return hr;

    // Initialize the DirectSound stuff. Note: if this call fails, we can
        // continue with no sound.
    CreateSoundObjects( hWnd );

    // Create the display objects
    if( FAILED( hr = CreateDisplayObjects( hWnd ) ) )
        return hr;

    // Add a ship to the displaylist
    g_pShip = new CShip( D3DXVECTOR3(0.0f,0.0f,0.0f) );
    g_pDisplayList = g_pShip;

    // Construct the game menus
    ConstructMenus();

    // Initial program state is to display the splash screen
    g_dwAppState = APPSTATE_LOADSPLASH;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DestroyGameObjects()
// Desc:
//-----------------------------------------------------------------------------
VOID DestroyGameObjects()
{
    DestroyDisplayObjects();
    DestroySoundObjects();
    DestroyInputObjects();
    DestroyMenus();
}




//-----------------------------------------------------------------------------
// Name: AdvanceLevel()
// Desc:
//-----------------------------------------------------------------------------
VOID AdvanceLevel()
{
    // Up the level
    g_dwLevel++;

    srand( timeGetTime() );

    // Clear any stray objects (anything but the ship) out of the display list
    while( g_pShip->pNext )
    {
        DeleteFromList( g_pShip->pNext );
    }

    // Create donuts for the new level
    for( WORD i=0; i<(2*g_dwLevel+3); i++ )
    {
        D3DVECTOR vPosition = 3.0f * D3DXVECTOR3( rnd(), rnd(), 0.0f );
        D3DVECTOR vVelocity = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

        AddToList( new CDonut( vPosition, vVelocity ) );
    }

    // Delay for 2 seconds before displaying ship
    g_pShip->vPos       = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    g_pShip->vVel       = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    g_pShip->bVisible   = FALSE;
    g_pShip->bExploded  = FALSE;
    g_pShip->fShowDelay = 2.0f;

    // Clear out iput states
    g_fBank          = 0.0f;
    g_fThrust        = 0.0f;
    g_bFiringWeapons = FALSE;

    // Stop engine sounds
    StopSound( g_pEngineIdleSound );
    StopSound( g_pEngineRevSound );
}




//-----------------------------------------------------------------------------
// Name: DisplayObject()
// Desc:
//-----------------------------------------------------------------------------
DisplayObject::DisplayObject( DWORD type, D3DVECTOR p, D3DVECTOR v )
{
    // Set object attributes
    pNext    = NULL;
    pPrev    = NULL;
    bVisible = TRUE;
    dwType   = type;
    vPos     = p;
    vVel     = v;
}




//-----------------------------------------------------------------------------
// Name: C3DSprite()
// Desc:
//-----------------------------------------------------------------------------
C3DSprite::C3DSprite( DWORD type, D3DVECTOR p, D3DVECTOR v )
          :DisplayObject( type, p, v )
{
    dwColor = 0xffffffff;
}




//-----------------------------------------------------------------------------
// Name: CDonut()
// Desc:
//-----------------------------------------------------------------------------
CDonut::CDonut( D3DVECTOR p, D3DVECTOR v )
       :C3DSprite( OBJ_DONUT, p, v )
{
    // Set object attributes
    dwTextureWidth   = DONUT_WIDTH;
    dwTextureHeight  = DONUT_HEIGHT;
    dwTextureOffsetX = 0;
    dwTextureOffsetY = 0;

    fSize           = dwTextureWidth / 256.0f;
    vVel            += 0.5f * D3DXVECTOR3( rnd(), rnd(), 0.0f );

    delay           = rnd( 3.0f, 12.0f );
    dwFramesPerLine = 8;
    frame           = rnd( 0.0f, 30.0f );
    fMaxFrame       = NUM_DONUT_FRAMES;
}




//-----------------------------------------------------------------------------
// Name: CPyramid()
// Desc:
//-----------------------------------------------------------------------------
CPyramid::CPyramid( D3DVECTOR p, D3DVECTOR v )
         :C3DSprite( OBJ_PYRAMID, p, v )
{
    // Set object attributes
    dwTextureWidth   = PYRAMID_WIDTH;
    dwTextureHeight  = PYRAMID_HEIGHT;
    dwTextureOffsetX = 0;
    dwTextureOffsetY = 0;

    fSize           = dwTextureWidth / 256.0f;
    vVel            += 0.5f * D3DXVECTOR3( rnd(), rnd(), 0.0f );

    delay           = rnd( 12.0f, 40.0f );
    dwFramesPerLine = 8;
    frame           = rnd( 0.0f, 30.0f );
    fMaxFrame       = NUM_PYRAMID_FRAMES;

}




//-----------------------------------------------------------------------------
// Name: CSphere()
// Desc:
//-----------------------------------------------------------------------------
CSphere::CSphere( D3DVECTOR p, D3DVECTOR v )
        :C3DSprite( OBJ_SPHERE, p, v )
{
    // Set object attributes
    dwTextureWidth   = SPHERE_WIDTH;
    dwTextureHeight  = SPHERE_HEIGHT;
    dwTextureOffsetX = 0;
    dwTextureOffsetY = 128;

    fSize           = dwTextureWidth / 256.0f;
    vVel            += 0.5f * D3DXVECTOR3( rnd(), rnd(), 0.0f );

    delay           = rnd( 60.0f, 80.0f );
    dwFramesPerLine = 16;
    frame           = rnd( 0.0f, 30.0f );
    fMaxFrame       = NUM_SPHERE_FRAMES;
}





//-----------------------------------------------------------------------------
// Name: CCube()
// Desc:
//-----------------------------------------------------------------------------
CCube::CCube( D3DVECTOR p, D3DVECTOR v )
      :C3DSprite( OBJ_CUBE, p, v )
{
    // Set object attributes
    dwTextureWidth   = CUBE_WIDTH;
    dwTextureHeight  = CUBE_HEIGHT;
    dwTextureOffsetX = 0;
    dwTextureOffsetY = 176;

    fSize           = dwTextureWidth / 256.0f;
    vVel            += 0.5f * D3DXVECTOR3( rnd(), rnd(), 0.0f );

    delay           = rnd( 32.0f, 80.0f );
    dwFramesPerLine = 16;
    frame           = rnd( 0.0f, 30.0f );
    fMaxFrame       = NUM_CUBE_FRAMES;
}




//-----------------------------------------------------------------------------
// Name: CCloud()
// Desc:
//-----------------------------------------------------------------------------
CCloud::CCloud( D3DVECTOR p, D3DVECTOR v )
       :C3DSprite( OBJ_CLOUD, p, v )
{
    // Set object attributes
    dwTextureWidth   = CLOUD_WIDTH;
    dwTextureHeight  = CLOUD_WIDTH;
    dwTextureOffsetX = 224;
    dwTextureOffsetY = 224;

    fSize           = dwTextureWidth / 256.0f;
    delay           = rnd( 1.0f, 3.0f );
    dwFramesPerLine = 1;
    frame           = 0.0f;
    fMaxFrame       = 1;
}




//-----------------------------------------------------------------------------
// Name: CBullet()
// Desc:
//-----------------------------------------------------------------------------
CBullet::CBullet( D3DVECTOR p, D3DVECTOR v, DWORD dwCType )
        :C3DSprite( OBJ_BULLET, p, v )
{
    // Set object attributes
    dwTextureWidth   = CLOUD_WIDTH;
    dwTextureHeight  = CLOUD_HEIGHT;
    dwTextureOffsetX = 224;
    dwTextureOffsetY = 224;

    if( dwCType == 0 )
        dwColor = 0xff2020ff;
    if( dwCType == 1 )
        dwColor = 0xff208020;
    if( dwCType == 2 )
        dwColor = 0xff208080;
    if( dwCType == 3 )
        dwColor = 0xff802020;

    fSize           = 4 / 256.0f;
    fMaxFrame       = NUM_BULLET_FRAMES;

    delay           = 1000.0f;
    dwFramesPerLine = 1;
    frame           = 0.0f;
}




//-----------------------------------------------------------------------------
// Name: CShip()
// Desc:
//-----------------------------------------------------------------------------
CShip::CShip( D3DVECTOR p )
      :DisplayObject( OBJ_SHIP, p, D3DXVECTOR3(0,0,0) )
{
    fSize           = 10.0f / 256.0f;
    bExploded       = FALSE;
    fShowDelay      = 0.0f;

    fRoll           = 0.0f;
    fAngle          = 0.0f;
}




//-----------------------------------------------------------------------------
// Name: AddToList()
// Desc:
//-----------------------------------------------------------------------------
VOID AddToList( DisplayObject* pObject )
{
    pObject->pNext = g_pDisplayList->pNext;
    pObject->pPrev = g_pDisplayList;

    if( g_pDisplayList->pNext )
        g_pDisplayList->pNext->pPrev = pObject;
    g_pDisplayList->pNext = pObject;
}




//-----------------------------------------------------------------------------
// Name: IsDisplayListEmpty()
// Desc:
//-----------------------------------------------------------------------------
BOOL IsDisplayListEmpty()
{
    DisplayObject* pObject = g_pDisplayList->pNext;

    while( pObject )
    {
        if( pObject->dwType != OBJ_BULLET )
            return FALSE;

        pObject = pObject->pNext;
    }

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: LoadTerrainModel()
// Desc: Loads the 3D geometry for the terrain
//-----------------------------------------------------------------------------
HRESULT LoadTerrainModel()
{
    LPDIRECT3DVERTEXBUFFER8 pVB;
    DWORD        dwNumVertices;
    MODELVERTEX* pVertices;

    // Delete old object
    SAFE_DELETE( g_pTerrain );

    // Create new object
    g_pTerrain = new CD3DMesh();
    if( FAILED( g_pTerrain->Create( g_pd3dDevice, _T("SeaFloor.x") ) ) )
        return E_FAIL;

    // Set the FVF to a reasonable type
    g_pTerrain->SetFVF( g_pd3dDevice, D3DFVF_MODELVERTEX );

    // Gain access to the model's vertices
    g_pTerrain->GetSysMemMesh()->GetVertexBuffer( &pVB );
    dwNumVertices = g_pTerrain->GetSysMemMesh()->GetNumVertices();
    pVB->Lock( 0, 0, (BYTE**)&pVertices, 0 );

    for( DWORD i=0; i<dwNumVertices; i++ )
    {
        pVertices[i].p.x *= 0.1f;
        pVertices[i].p.z *= 0.1f;
        pVertices[i].p.y = HeightField( pVertices[i].p.x, pVertices[i].p.z );
    }

    // Done with the vertex buffer
    pVB->Unlock();
    pVB->Release();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: LoadShipModel()
// Desc: Loads the 3D geometry for the player's ship
//-----------------------------------------------------------------------------
HRESULT LoadShipModel()
{
    LPDIRECT3DVERTEXBUFFER8 pVB;
    DWORD        dwNumVertices;
    MODELVERTEX* pVertices;
    D3DXVECTOR3  vCenter;
    FLOAT        fRadius;

    // Delete old object
    SAFE_DELETE( g_pShipFileObject );

    // Create new object
    g_pShipFileObject = new CD3DMesh();
    if( FAILED( g_pShipFileObject->Create( g_pd3dDevice,
                                           g_strShipFiles[g_dwCurrentShipType] ) ) )
        return E_FAIL;

    // Set the FVF to a reasonable type
    g_pShipFileObject->SetFVF( g_pd3dDevice, D3DFVF_MODELVERTEX );

    // Gain access to the model's vertices
    g_pShipFileObject->GetSysMemMesh()->GetVertexBuffer( &pVB );
    dwNumVertices = g_pShipFileObject->GetSysMemMesh()->GetNumVertices();
    pVB->Lock( 0, 0, (BYTE**)&pVertices, 0 );

    // Scale the new object to a standard size  
    D3DXComputeBoundingSphere( pVertices, dwNumVertices,
                               D3DFVF_MODELVERTEX, &vCenter, &fRadius );
    for( DWORD i=0; i<dwNumVertices; i++ )
    {
        pVertices[i].p /= 12*fRadius;
    }

    // Done with the vertex buffer
    pVB->Unlock();
    pVB->Release();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SwitchModel()
// Desc:
//-----------------------------------------------------------------------------
HRESULT SwitchModel()
{
    // Select next model
    g_dwCurrentShipType++;
    if( g_dwCurrentShipType >= g_dwNumShipTypes )
        g_dwCurrentShipType = 0L;

    // Create new object
    if( SUCCEEDED( LoadShipModel() ) )
    {
        // Initialize the new object's device dependent objects
        if( SUCCEEDED( g_pShipFileObject->RestoreDeviceObjects( g_pd3dDevice ) ) )
            return S_OK;
    }

    // Return with a fatal error
    PostMessage( g_hWndMain, WM_CLOSE, 0, 0 );
    return E_FAIL;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc:
//-----------------------------------------------------------------------------
HRESULT FrameMove()
{
    switch( g_dwAppState )
    {
        case APPSTATE_LOADSPLASH:
            // Set the app state to displaying splash
            g_dwAppState = APPSTATE_DISPLAYSPLASH;

            // Draw the splash bitmap
            g_hSplashBitmap = (HBITMAP)LoadImage( GetModuleHandle( NULL ),
                                                  TEXT("SPLASH"), IMAGE_BITMAP,
                                                  0, 0, LR_CREATEDIBSECTION );
            SendMessage( g_hWndMain, WM_PAINT, 0, 0 );
            break;

        case APPSTATE_ACTIVE:
            UpdateDisplayList();
            CheckForHits();

            if( IsDisplayListEmpty() )
            {
                AdvanceLevel();
                g_dwAppState = APPSTATE_BEGINLEVELSCREEN;
            }
            break;

        case APPSTATE_BEGINLEVELSCREEN:
            PlaySound( g_pBeginLevelSound );
            DXUtil_Timer( TIMER_RESET );
            g_dwAppState = APPSTATE_DISPLAYLEVELSCREEN;
            break;

        case APPSTATE_DISPLAYLEVELSCREEN:
            // Only show the Level intro screen for 3 seconds

            if( DXUtil_Timer( TIMER_GETAPPTIME ) > 3.0f )
            {
                g_dwAppState = APPSTATE_ACTIVE;
            }
            break;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderFrame()
// Desc:
//-----------------------------------------------------------------------------
HRESULT RenderFrame()
{
    // Test cooperative level
    HRESULT hr;

    // Test the cooperative level to see if it's okay to render
    if( FAILED( hr = g_pd3dDevice->TestCooperativeLevel() ) )
    {
        // If the device was lost, do not render until we get it back
        if( D3DERR_DEVICELOST == hr )
            return S_OK;

        // Check if the device needs to be resized.
        if( D3DERR_DEVICENOTRESET == hr )
        {
            g_bDisplayReady = FALSE;

            InvalidateDisplayObjects();

            // Resize the device
            if( SUCCEEDED( g_pd3dDevice->Reset( &g_d3dpp ) ) )
            {
                // Initialize the app's device-dependent objects
                if( SUCCEEDED( RestoreDisplayObjects() ) )
                {
                    g_bDisplayReady = TRUE;
                    return S_OK;
                }
            }

            PostMessage( g_hWndMain, WM_CLOSE, 0, 0 );
        }
        return hr;
    }

    // Render the scene based on current state of the app
    switch( g_dwAppState )
    {
        case APPSTATE_LOADSPLASH:
            // Nothing to render while loading the splash screen
            break;

        case APPSTATE_DISPLAYSPLASH:
            // Rendering of the splash screen is handled by WM_PAINT
            break;

        case APPSTATE_BEGINLEVELSCREEN:
            // Nothing to render while starting sound to advance a level
            break;

        case APPSTATE_DISPLAYLEVELSCREEN:
            DisplayLevelIntroScreen( g_dwLevel );
            ShowFrame();
            break;

        case APPSTATE_ACTIVE:
            DrawDisplayList();
            ShowFrame();
            break;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DarkenScene()
// Desc:
//-----------------------------------------------------------------------------
VOID DarkenScene( FLOAT fAmount )
{
    if( g_pd3dDevice==NULL )
        return;

    // Setup a dark square to cover the scene
    DWORD dwAlpha = (fAmount<1.0f) ? ((DWORD)(255*fAmount))<<24L : 0xff000000;
    SCREENVERTEX* v;
    g_pViewportVB->Lock( 0, 0, (BYTE**)&v, 0 );
    v[0].color = v[1].color = v[2].color = v[3].color = dwAlpha;
    g_pViewportVB->Unlock();

    // Set renderstates
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,   FALSE );
    g_pd3dDevice->SetTexture( 0, NULL );

    // Draw a big, gray square
    g_pd3dDevice->SetVertexShader( D3DFVF_SCREENVERTEX );
    g_pd3dDevice->SetStreamSource( 0, g_pViewportVB, sizeof(SCREENVERTEX) );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,0, 2 );

    // Restore states
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
VOID RenderFieryText( CD3DFont* pFont, TCHAR* strText )
{
    if( NULL==pFont || NULL==strText )
        return;

    // Render the fiery portion of the text
    for( DWORD i=0; i<20; i++ )
    {
        FLOAT x = -0.5f;
        FLOAT y =  1.8f;

        FLOAT v1 = rnd(0.0f, 1.0f);
        FLOAT red1 = v1*v1*v1;
        FLOAT grn1 = v1*v1;
        FLOAT blu1 = v1;


        FLOAT a1 = rnd(0.0f, 2*D3DX_PI);
        FLOAT r1 = v1 * 0.05f;

        x += r1*sinf(a1);
        y += r1*cosf(a1);

        if( cosf(a1) < 0.0f )
            y -= 2*r1*cosf(a1)*cosf(a1);

        DWORD r = (CHAR)((1.0f-red1)*256.0f);
        DWORD g = (CHAR)((1.0f-grn1)*256.0f);
        DWORD b = (CHAR)((1.0f-blu1)*256.0f);
        DWORD a = (CHAR)255;
        DWORD dwColor = (a<<24) + (r<<16) + (g<<8) + b;

        g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
        g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

        pFont->DrawTextScaled( x, y, 0.9f, 0.25f, 0.25f, dwColor, strText, D3DFONT_FILTERED );
    }

    // Render the plain, black portion of the text
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    FLOAT x = -0.5f;
    FLOAT y =  1.8f;
    pFont->DrawTextScaled( x, y, 0.9f, 0.25f, 0.25f, 0xff000000, strText, D3DFONT_FILTERED );
}




//-----------------------------------------------------------------------------
// Name: DisplayLevelIntroScreen()
// Desc:
//-----------------------------------------------------------------------------
VOID DisplayLevelIntroScreen( DWORD dwLevel )
{
    if( g_pd3dDevice==NULL )
        return;

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Erase the screen
        g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0L, 1.0f, 0L );

        TCHAR strLevel[80];
        _stprintf( strLevel, _T("Level %ld"), dwLevel );
        RenderFieryText( g_pGameFont, strLevel );

        DarkenScene( 1.0f - sinf(D3DX_PI*DXUtil_Timer( TIMER_GETAPPTIME )/3.0f) );

        // End the scene
        g_pd3dDevice->EndScene();
    }
}




//-----------------------------------------------------------------------------
// Name: UpdateDisplayList()
// Desc:
//-----------------------------------------------------------------------------
VOID UpdateDisplayList()
{
    DisplayObject* pObject;

        // Get the time lapsed since the last frame
        static FLOAT fLastTime = 0.0f;
    FLOAT fTimeLapsed = DXUtil_Timer( TIMER_GETAPPTIME ) - fLastTime;
        if( fTimeLapsed <= 0.0f )
                fTimeLapsed = 0.01f;
        fLastTime = DXUtil_Timer( TIMER_GETAPPTIME );

    // Read input from the joystick/keyboard/etc
    GetInput();

    // Check for game menu condition
    if( g_pCurrentMenu )
    {
        UpdateMenus();
        return;
    }

    if( g_bPaused )
        return;

    if( g_pShip->fShowDelay > 0.0f )
    {
        g_pShip->fShowDelay -= fTimeLapsed;

        if( g_pShip->fShowDelay <= 0.0f )
        {
            g_pShip->vVel       = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
            g_pShip->fShowDelay = 0.0f;
            g_pShip->bVisible   = TRUE;
            g_pShip->bExploded  = FALSE;
        }
    }

    // Update the ship
    if( g_pShip->bVisible )
    {
        g_pShip->vPos += g_pShip->vVel * fTimeLapsed;
    }

    // Apply banking motion
    g_pShip->fRoll += g_fBank * 1.0f * fTimeLapsed;
    if( g_pShip->fRoll > 0.5f )
        g_pShip->fRoll = 0.5f;
    if( g_pShip->fRoll < -0.5f )
        g_pShip->fRoll = -0.5f;

    g_pShip->fAngle += 5 * g_pShip->fRoll * fTimeLapsed;

    if( g_fBank < 0.2f && g_fBank > -0.2f )
    {
        g_pShip->fRoll *= 0.95f;
    }

    // Slow the ship down
    g_pShip->vVel.x *= 0.97f;
    g_pShip->vVel.y *= 0.97f;

    // Apply thrust
    g_pShip->vVel.x +=  sinf( g_pShip->fAngle ) * g_fThrust * 5.0f * fTimeLapsed;
    g_pShip->vVel.y += -cosf( g_pShip->fAngle ) * g_fThrust * 5.0f * fTimeLapsed;

    // Play thrusting sounds
    {
        static bPlayingEngineRevSound = FALSE;

        if( g_fThrust > 0.5f )
        {
            if( FALSE == bPlayingEngineRevSound )
            {
                bPlayingEngineRevSound = TRUE;
            }
        }
        else
        {
            if( TRUE == bPlayingEngineRevSound )
            {
                StopSound( g_pEngineRevSound );
                bPlayingEngineRevSound = FALSE;
            }
        }
    }

    g_fBulletRechargeTime -= fTimeLapsed;

     // Fire a bullet
    if( g_bFiringWeapons && g_fBulletRechargeTime <= 0.0f )
    {
        // Ship must be visible and have no shields on to fire
        if( g_pShip->bVisible )
        {
            // Bullets cost one score point
            if( g_dwScore )
                g_dwScore--;

            // Play the "fire" effects
            PlaySound( g_pFireBulletSound );

            // Add a bullet to the display list
            if( g_dwBulletType == 0 )
            {
                D3DXVECTOR3 vDir = D3DXVECTOR3( sinf( g_pShip->fAngle ), -cosf( g_pShip->fAngle ), 0.0f );

                AddToList( new CBullet( g_pShip->vPos, g_pShip->vVel + 2*vDir, 0 ) );
                g_fBulletRechargeTime = 0.05f;
            }
            else if( g_dwBulletType == 1 )
            {
                D3DXVECTOR3 vOffset = 0.02f * D3DXVECTOR3( cosf(g_pShip->fAngle), sinf(g_pShip->fAngle), 0.0f );
                D3DXVECTOR3 vDir = D3DXVECTOR3( sinf( g_pShip->fAngle ), -cosf( g_pShip->fAngle ), 0.0f );

                AddToList( new CBullet( g_pShip->vPos + vOffset, g_pShip->vVel + 2*vDir, 1 ) );
                AddToList( new CBullet( g_pShip->vPos - vOffset, g_pShip->vVel + 2*vDir, 1 ) );
                g_fBulletRechargeTime = 0.10f;
            }
            else if( g_dwBulletType == 2 )
            {
                FLOAT fBulletAngle = g_pShip->fAngle + 0.2f*rnd();
                D3DXVECTOR3 vDir = D3DXVECTOR3( sinf(fBulletAngle), -cosf(fBulletAngle), 0.0f );

                AddToList( new CBullet( g_pShip->vPos, g_pShip->vVel + 2*vDir, 2 ) );
                g_fBulletRechargeTime = 0.01f;
            }
            else
            {
                for( DWORD i=0; i<50; i++ )
                {
                    FLOAT fBulletAngle = g_pShip->fAngle + D3DX_PI*rnd();
                    D3DXVECTOR3 vDir = D3DXVECTOR3( sinf(fBulletAngle), -cosf(fBulletAngle), 0.0f );

                    AddToList( new CBullet( g_pShip->vPos, 2*vDir, 3 ) );
                }

                g_fBulletRechargeTime = 1.0f;
            }
        }
    }

    // Keep ship in bounds
    if( g_pShip->vPos.x < -5.0f || g_pShip->vPos.x > +5.0f ||
        g_pShip->vPos.y < -5.0f || g_pShip->vPos.y > +5.0f )
    {
         D3DXVec3Normalize( &g_pShip->vVel, &g_pShip->vPos );
         g_pShip->vVel.x *= -1.0f;
         g_pShip->vVel.y *= -1.0f;
         g_pShip->vVel.z *= -1.0f;
    }

    // Finally, move all objects on the screen
    for( pObject = g_pDisplayList; pObject; pObject = pObject->pNext )
    {
        // The ship is moved by the code above
        if( pObject->dwType == OBJ_SHIP )
            continue;

        C3DSprite* pSprite = (C3DSprite*)pObject;

        // Update the position and animation frame
        pSprite->vPos  += pSprite->vVel * fTimeLapsed;
        pSprite->frame += pSprite->delay * fTimeLapsed;

        // If this is an "expired" cloud, removed it from list
        if( pObject->dwType == OBJ_CLOUD )
        {
            if( pSprite->frame >= pSprite->fMaxFrame )
            {
                DisplayObject* pVictim = pObject;
                pObject = pObject->pPrev;
                DeleteFromList( pVictim );
            }
        }
        else if( pObject->dwType == OBJ_BULLET )
        {
            // Remove bullets when they leave the scene
            if( pObject->vPos.x < -6.0f || pObject->vPos.x > +6.0f ||
                pObject->vPos.y < -6.0f || pObject->vPos.y > +6.0f )
            {
                DisplayObject* pVictim = pObject;
                pObject = pObject->pPrev;
                DeleteFromList( pVictim );
            }
        }
        else if( pObject->dwType != OBJ_CLOUD )
        {
            // Keep object in bounds in X
            if( pObject->vPos.x < -4.0f || pObject->vPos.x > +4.0f )
            {
                if( pObject->vPos.x < -4.0f ) pObject->vPos.x = -4.0f;
                if( pObject->vPos.x > +4.0f ) pObject->vPos.x = +4.0f;
                pObject->vVel.x = -pObject->vVel.x;
            }

            // Keep object in bounds in Y
            if( pObject->vPos.y < -4.0f || pObject->vPos.y > +4.0f )
            {
                if( pObject->vPos.y < -4.0f ) pObject->vPos.y = -4.0f;
                if( pObject->vPos.y > +4.0f ) pObject->vPos.y = +4.0f;
                pObject->vVel.y = -pObject->vVel.y;
            }

            // Keep animation frame in bounds
            if( pSprite->frame < 0.0f )
                pSprite->frame += pSprite->fMaxFrame;
            if( pSprite->frame >= pSprite->fMaxFrame )
                pSprite->frame -= pSprite->fMaxFrame;
        }
    }

    D3DXVECTOR3 vEyePt[NUMVIEWMODES];
    D3DXVECTOR3 vLookatPt[NUMVIEWMODES];
    D3DXVECTOR3 vUpVec[NUMVIEWMODES];

    // Update the view
    if( g_bChangeView )
    {
        g_bAnimatingViewChange = TRUE;
        g_bChangeView = FALSE;
    }

    if( g_bAnimatingViewChange )
    {
        g_fViewTransition += fTimeLapsed;

        if( g_fViewTransition >= 1.0f )
        {
            g_dwViewMode++;
            if( g_dwViewMode >= NUMVIEWMODES )
                g_dwViewMode = 0;

            g_fViewTransition      = 0.0f;
            g_bAnimatingViewChange = FALSE;
        }
    }

    FLOAT fX =  g_pShip->vPos.x;
    FLOAT fZ = -g_pShip->vPos.y;
    FLOAT fY = 0.1f + HeightField( fX, fZ );

    // View mode 0 (third person)
    vEyePt[0]      = D3DXVECTOR3( fX-sinf(g_pShip->fAngle)/2, fY+0.2f, fZ-cosf(g_pShip->fAngle)/2 );
    vLookatPt[0]   = D3DXVECTOR3( fX+sinf(g_pShip->fAngle)/2, fY, fZ+cosf(g_pShip->fAngle)/2 );
    vUpVec[0]      = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    // View mode 1 (first person)
    FLOAT fX2 = fX+sinf(g_pShip->fAngle);
    FLOAT fZ2 = fZ+cosf(g_pShip->fAngle);
    FLOAT fY2 = 0.1f + HeightField( fX2, fZ2 );
    vEyePt[1]    = D3DXVECTOR3( fX, fY+0.1f, fZ );
    vLookatPt[1] = D3DXVECTOR3( fX2, fY2+0.1f, fZ2 );
    vUpVec[1]    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    // View mode 2 (top down view)
    vEyePt[2]    = D3DXVECTOR3( fX+1.5f, fY+1.5f, fZ+1.5f );
    vLookatPt[2] = D3DXVECTOR3( fX, fY, fZ );
    vUpVec[2]    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    DWORD start = g_dwViewMode;
    DWORD end   = ( start < (NUMVIEWMODES-1) ) ? g_dwViewMode+1: 0;

    if( start == 1 && g_fViewTransition<0.2f)
        g_bFirstPersonView = TRUE;
    else
        g_bFirstPersonView = FALSE;

    D3DXVECTOR3 vEyePt0    = (1.0f-g_fViewTransition)*vEyePt[start]    + g_fViewTransition*vEyePt[end];
    D3DXVECTOR3 vLookatPt0 = (1.0f-g_fViewTransition)*vLookatPt[start] + g_fViewTransition*vLookatPt[end];
    D3DXVECTOR3 vUpVec0    = (1.0f-g_fViewTransition)*vUpVec[start]    + g_fViewTransition*vUpVec[end];

    // Shake screen if ship exploded
    if( g_pShip->bExploded == TRUE )
        vEyePt0 += D3DXVECTOR3( rnd(), rnd(), rnd() ) * g_pShip->fShowDelay / 50.0f;

    g_Camera.SetViewParams( vEyePt0, vLookatPt0, vUpVec0 );
}




//-----------------------------------------------------------------------------
// Name: CheckForHits()
// Desc:
//-----------------------------------------------------------------------------
VOID CheckForHits()
{
    DisplayObject* pObject;
    DisplayObject* pBullet;

    for( pBullet = g_pDisplayList; pBullet; pBullet = pBullet->pNext )
    {
        BOOL bBulletHit = FALSE;

        // Only bullet objects and the ship (if shieleds are on) can hit
        // other objects. Skip all others.
        if( (pBullet->dwType != OBJ_BULLET) && (pBullet->dwType != OBJ_SHIP) )
            continue;

        for( pObject = g_pDisplayList->pNext; pObject; pObject = pObject->pNext )
        {
            // Only trying to hit explodable targets
            if( ( pObject->dwType != OBJ_DONUT ) &&
                ( pObject->dwType != OBJ_PYRAMID ) &&
                ( pObject->dwType != OBJ_SPHERE ) &&
                ( pObject->dwType != OBJ_CUBE ) )
                continue;

            // Check if bullet is in radius of object
            FLOAT fDistance = D3DXVec3Length( &(pBullet->vPos - pObject->vPos) );

            if( fDistance < (pObject->fSize+pBullet->fSize) )
            {
                // The object was hit
                switch( pObject->dwType )
                {
                    case OBJ_DONUT:
                        PlaySound( g_pDonutExplodeSound );
                        AddToList( new CPyramid( pObject->vPos, pObject->vVel ) );
                        AddToList( new CPyramid( pObject->vPos, pObject->vVel ) );
                        AddToList( new CPyramid( pObject->vPos, pObject->vVel ) );
                        AddToList( new CPyramid( pObject->vPos, pObject->vVel ) );
                        g_dwScore += 10;
                        break;

                    case OBJ_PYRAMID:
                        PlaySound( g_pPyramidExplodeSound );
                        AddToList( new CCube( pObject->vPos, pObject->vVel ) );
                        AddToList( new CCube( pObject->vPos, pObject->vVel ) );
                        AddToList( new CCube( pObject->vPos, pObject->vVel ) );
                        AddToList( new CCube( pObject->vPos, pObject->vVel ) );
                        g_dwScore += 20;
                        break;

                    case OBJ_CUBE:
                        PlaySound( g_pCubeExplodeSound );
                        AddToList( new CSphere( pObject->vPos, pObject->vVel ) );
                        g_dwScore += 40;
                        break;

                    case OBJ_SPHERE:
                        PlaySound( g_pSphereExplodeSound );
                        g_dwScore += 20;
                        break;
                }

                // Add explosion effects to scene
                for( DWORD c=0; c<4; c++ )
                    AddToList( new CCloud( pObject->vPos, 0.05f*D3DXVECTOR3(rnd(),rnd(),0.0f) ) );

                // Remove the victim from the scene
                DisplayObject* pVictim = pObject;
                pObject = pObject->pPrev;
                DeleteFromList( pVictim );

                bBulletHit = TRUE;
            }

            if( bBulletHit )
            {
                if( pBullet->dwType == OBJ_SHIP )
                {
                    bBulletHit = FALSE;

                    if( g_pShip->bVisible )
                    {
                        // Ship has exploded
                        PlaySound( g_pShipExplodeSound );

                        if( g_dwScore < 150 )
                            g_dwScore = 0;
                        else
                            g_dwScore -= 150;

                        // Add explosion debris to scene
                        for( DWORD sphere=0; sphere<4; sphere++ )
                            AddToList( new CSphere( g_pShip->vPos, pObject->vVel ) );

                        for( DWORD bullet=0; bullet<20; bullet++ )
                        {
                            FLOAT     angle     = D3DX_PI * rnd();
                            D3DVECTOR vDir      = D3DXVECTOR3(cosf(angle),sinf(angle),0.0f);
                            AddToList( new CBullet( g_pShip->vPos, 500.0f*vDir, 0 ) );
                        }

                        for( DWORD cloud=0; cloud<100; cloud++ )
                        {
                            FLOAT     magnitude = 1.0f + 0.1f*rnd();
                            FLOAT     angle     = D3DX_PI * rnd();
                            D3DVECTOR vDir      = D3DXVECTOR3(cosf(angle),sinf(angle),0.0f);

                            AddToList( new CCloud( g_pShip->vPos, magnitude*vDir ) );
                        }

                        // Clear out ship params
                        g_pShip->vVel.x = 0.0f;
                        g_pShip->vVel.y = 0.0f;
                        g_fThrust       = 0.0f;
                        g_fBank         = 0.0f;

                        // Delay for 2 seconds before displaying ship
                        g_pShip->fShowDelay = 2.0f;
                        g_pShip->bVisible   = FALSE;
                        g_pShip->bExploded  = TRUE;
                    }
                }

                break;
            }
        }

        if( bBulletHit )
        {
            DisplayObject* pLastBullet = pBullet;
            pBullet = pBullet->pPrev;
            DeleteFromList( pLastBullet );
        }
    }
}




//-----------------------------------------------------------------------------
// Name: DrawDisplayList()
// Desc:
//-----------------------------------------------------------------------------
VOID DrawDisplayList()
{
    TCHAR strBuffer[80];

    // Set the world matrix
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity( &matWorld );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set the app view matrix for normal viewing
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &g_Camera.GetViewMatrix() );

    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING,     TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x33333333 );

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Clear the display
        g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0L, 1.0f, 0L );

        // Draw the terrain
        g_pTerrain->Render( g_pd3dDevice );

        // Render the ship
        if( g_pShip->bVisible && g_bFirstPersonView == FALSE )
        {
            // Point of ship, on terrain
            D3DXVECTOR3 vShipPt;
            vShipPt.x =  g_pShip->vPos.x;
            vShipPt.z = -g_pShip->vPos.y;
            vShipPt.y = 0.1f + HeightField( vShipPt.x, vShipPt.z );

            // Point ahead of ship, on terrain
            D3DXVECTOR3 vForwardPt;
            vForwardPt.x = vShipPt.x+sinf(g_pShip->fAngle);
            vForwardPt.z = vShipPt.z+cosf(g_pShip->fAngle);
            vForwardPt.y = 0.1f + HeightField( vForwardPt.x, vForwardPt.z );

            // Point to side of ship, on terrain
            D3DXVECTOR3 vSidePt;
            vSidePt.x = vShipPt.x+sinf(g_pShip->fAngle + D3DX_PI/2.0f);
            vSidePt.z = vShipPt.z+cosf(g_pShip->fAngle + D3DX_PI/2.0f);
            vSidePt.y = 0.1f + HeightField( vSidePt.x, vSidePt.z );

            // Compute vectors of the ship's orientation
            D3DXVECTOR3 vForwardDir = vForwardPt - vShipPt;
            D3DXVECTOR3 vSideDir    = vSidePt - vShipPt;
            D3DXVECTOR3 vNormalDir;
            D3DXVec3Cross( &vNormalDir, &vForwardDir, &vSideDir );

            // Construct matrix to orient ship
            D3DXMATRIX matWorld, matLookAt, matRotateZ;
            D3DXMatrixRotationZ( &matRotateZ, g_pShip->fRoll );
            D3DXMatrixLookAtLH( &matLookAt, &vShipPt, &(vShipPt-vForwardDir), &vNormalDir );
            D3DXMatrixInverse( &matLookAt, NULL, &matLookAt );
            D3DXMatrixIdentity( &matWorld );
            D3DXMatrixMultiply( &matWorld, &matWorld, &matRotateZ );
            D3DXMatrixMultiply( &matWorld, &matWorld, &matLookAt );

            // Set renderstates for rendering the ship
            g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
            g_pd3dDevice->SetRenderState( D3DRS_LIGHTING,           TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS,   TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   FALSE );
            g_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE,    FALSE );

            // Render the ship - opaque parts
            g_pShipFileObject->Render( g_pd3dDevice, TRUE, FALSE );

            // Render the ship - transparent parts
            g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
            g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,     D3DBLEND_ONE );
            g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,    D3DBLEND_ONE );
            g_pShipFileObject->Render( g_pd3dDevice, FALSE, TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
        }

        // Remaining objects don't need lighting
        g_pd3dDevice->SetRenderState( D3DRS_LIGHTING,           FALSE );

        // Enable alpha blending and testing
        g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        g_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
        g_pd3dDevice->SetRenderState( D3DRS_ALPHAREF,         0x08 );
        g_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

        // Display all visible objects in the display list
        for( DisplayObject* pObject = g_pDisplayList; pObject; pObject = pObject->pNext )
        {
            if( !pObject->bVisible )
                continue;
            if( pObject->dwType == OBJ_SHIP )
                continue;
            if( pObject->dwType == OBJ_BULLET )
                continue;

            // This is really a 3D sprite
            C3DSprite* pSprite = (C3DSprite*)pObject;

            FLOAT fX =  pObject->vPos.x;
            FLOAT fZ = -pObject->vPos.y;
            FLOAT fY =  HeightField( fX, fZ );

            FLOAT x1 = -pObject->fSize;
            FLOAT x2 =  pObject->fSize;
            FLOAT y1 = -pObject->fSize;
            FLOAT y2 =  pObject->fSize;

            FLOAT u1 = (FLOAT)(pSprite->dwTextureOffsetX + pSprite->dwTextureWidth *(((int)pSprite->frame)%pSprite->dwFramesPerLine));
            FLOAT v1 = (FLOAT)(pSprite->dwTextureOffsetY + pSprite->dwTextureHeight*(((int)pSprite->frame)/pSprite->dwFramesPerLine));

            FLOAT tu1 = u1 / (256.0f-1.0f);
            FLOAT tv1 = v1 / (256.0f-1.0f);
            FLOAT tu2 = (u1 + pSprite->dwTextureWidth -1) / (256.0f-1.0f);
            FLOAT tv2 = (v1 + pSprite->dwTextureHeight-1) / (256.0f-1.0f);

            // Set the game texture
            switch( pObject->dwType )
            {
                case OBJ_DONUT:
                case OBJ_CUBE:
                case OBJ_SPHERE:
                    g_pd3dDevice->SetTexture( 0, g_pGameTexture1 );
                    break;
                case OBJ_PYRAMID:
                case OBJ_CLOUD:
                    g_pd3dDevice->SetTexture( 0, g_pGameTexture2 );
                    break;
            }

            // Translate the billboard into place
            D3DXMATRIX mat = g_Camera.GetBillboardMatrix();
            mat._41 = fX;
            mat._42 = fY;
            mat._43 = fZ;
            g_pd3dDevice->SetTransform( D3DTS_WORLD, &mat );

            DWORD dwColor = pSprite->dwColor;

            g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
            g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
            g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

            if( pObject->dwType == OBJ_CLOUD )
            {
                DWORD red = 255-(int)(pSprite->frame*255.0f);
                DWORD grn = 255-(int)(pSprite->frame*511.0f);
                DWORD blu = 255-(int)(pSprite->frame*1023.0f);
                if( grn > 255 ) grn = 0;
                if( blu > 255 ) blu = 0;
                dwColor = 0xff000000 + (red<<16) + (grn<<8) + blu;

                g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
                g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
                g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
                g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
            }

            FLOAT h = 300.0f*pObject->vPos.z + 0.1f;

            SPRITEVERTEX* v;
            g_pSpriteVB->Lock( 0, 0, (BYTE**)&v, 0 );
            v[0].p = D3DXVECTOR3(x1,y1+h,0); v[0].color=dwColor; v[0].tu=tu1; v[0].tv=tv2;
            v[1].p = D3DXVECTOR3(x1,y2+h,0); v[1].color=dwColor; v[1].tu=tu1; v[1].tv=tv1;
            v[2].p = D3DXVECTOR3(x2,y1+h,0); v[2].color=dwColor; v[2].tu=tu2; v[2].tv=tv2;
            v[3].p = D3DXVECTOR3(x2,y2+h,0); v[3].color=dwColor; v[3].tu=tu2; v[3].tv=tv1;
            g_pSpriteVB->Unlock();

            // Render the billboarded sprite
            g_pd3dDevice->SetVertexShader( D3DFVF_SPRITEVERTEX );
            g_pd3dDevice->SetStreamSource( 0, g_pSpriteVB, sizeof(SPRITEVERTEX) );
            g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
        }

        // Display all bullets
        for( pObject = g_pDisplayList; pObject; pObject = pObject->pNext )
        {
            if( pObject->dwType != OBJ_BULLET )
                continue;

            // This is really a 3D sprite
            C3DSprite* pSprite = (C3DSprite*)pObject;

            FLOAT u1 = (FLOAT)(pSprite->dwTextureOffsetX + pSprite->dwTextureWidth *(((int)pSprite->frame)%pSprite->dwFramesPerLine));
            FLOAT v1 = (FLOAT)(pSprite->dwTextureOffsetY + pSprite->dwTextureHeight*(((int)pSprite->frame)/pSprite->dwFramesPerLine));
            u1 = (FLOAT)(pSprite->dwTextureOffsetX);
            v1 = (FLOAT)(pSprite->dwTextureOffsetY);

            FLOAT tu1 = u1 / (256.0f-1.0f);
            FLOAT tv1 = v1 / (256.0f-1.0f);
            FLOAT tu2 = (u1 + pSprite->dwTextureWidth -1) / (256.0f-1.0f);
            FLOAT tv2 = (v1 + pSprite->dwTextureHeight-1) / (256.0f-1.0f);

            // Set render states
            g_pd3dDevice->SetTexture( 0, g_pGameTexture2 );
            g_pd3dDevice->SetVertexShader( D3DFVF_SPRITEVERTEX );
            g_pd3dDevice->SetStreamSource( 0, g_pSpriteVB, sizeof(SPRITEVERTEX) );
            g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,     D3DBLEND_ONE );
            g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,    D3DBLEND_ONE );
            g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
            g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );

            FLOAT x1 = -0.01f;
            FLOAT x2 =  0.01f;
            FLOAT y1 = -0.01f;
            FLOAT y2 =  0.01f;

            DWORD dwColor = pSprite->dwColor;

            for( DWORD a=0; a<6; a++ )
            {
                FLOAT fX =  pObject->vPos.x - a*a*0.0005f*pObject->vVel.x;
                FLOAT fZ = -pObject->vPos.y + a*a*0.0005f*pObject->vVel.y;
                FLOAT fY =  HeightField( fX, fZ );

                // Translate the billboard into place
                D3DXMATRIX mat = g_Camera.GetBillboardMatrix();
                mat._41 = fX;
                mat._42 = fY;
                mat._43 = fZ;
                g_pd3dDevice->SetTransform( D3DTS_WORLD, &mat );

                FLOAT h = 300.0f*pObject->vPos.z + 0.1f;

                SPRITEVERTEX* v;
                g_pSpriteVB->Lock( 0, 0, (BYTE**)&v, 0 );
                v[0].p = D3DXVECTOR3(x1,y1+h,0); v[0].color=dwColor; v[0].tu=tu1; v[0].tv=tv2;
                v[1].p = D3DXVECTOR3(x1,y2+h,0); v[1].color=dwColor; v[1].tu=tu1; v[1].tv=tv1;
                v[2].p = D3DXVECTOR3(x2,y1+h,0); v[2].color=dwColor; v[2].tu=tu2; v[2].tv=tv2;
                v[3].p = D3DXVECTOR3(x2,y2+h,0); v[3].color=dwColor; v[3].tu=tu2; v[3].tv=tv1;
                g_pSpriteVB->Unlock();

                // Render the billboarded sprite
                g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
            }
        }

        // Restore state
        g_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
        g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

        // Display score
        _stprintf( strBuffer, _T("Score: %08ld"), g_dwScore );
        g_pGameFont->DrawTextScaled( -1.0f, 1.0f, 0.9f, 0.05f, 0.05f,
                                     0xffffff00, strBuffer, D3DFONT_FILTERED );

        // Display ship type
        _stprintf( strBuffer, _T("Ship: %s"), g_strShipNames[g_dwCurrentShipType] );
        g_pGameFont->DrawTextScaled( 0.0f, 1.0f, 0.9f, 0.05f, 0.05f,
                                     0xffffff00, strBuffer, D3DFONT_FILTERED );

        // Display weapon type
        TCHAR* strWeapon;
        if( g_dwBulletType == 0 )      strWeapon = _T("Weapon: Blaster");
        else if( g_dwBulletType == 1 ) strWeapon = _T("Weapon: Double blaster");
        else if( g_dwBulletType == 2 ) strWeapon = _T("Weapon: Spray gun");
        else                           strWeapon = _T("Weapon: Proximity killer");
        g_pGameFont->DrawTextScaled( 0.0f, 1.1f, 0.9f, 0.05f, 0.05f,
                                     0xffffff00, strWeapon, D3DFONT_FILTERED );

        // Render "Paused" text if game is paused
        if( g_bPaused && g_pGameFont )
        {
            DarkenScene( 0.5f );
            RenderFieryText( g_pMenuFont, _T("Paused") );
        }

        if( g_pShip->fShowDelay > 0.0f )
            DarkenScene( g_pShip->fShowDelay/2.0f );

        // Render game menu
        if( g_pCurrentMenu )
        {
            DarkenScene( 0.5f );
            g_pCurrentMenu->Render( g_pd3dDevice, g_pMenuFont );
        }

        g_pd3dDevice->EndScene();
    }
}




//-----------------------------------------------------------------------------
// Name: DeleteFromList()
// Desc:
//-----------------------------------------------------------------------------
VOID DeleteFromList( DisplayObject* pObject )
{
    if( pObject->pNext )
        pObject->pNext->pPrev = pObject->pPrev;
    if( pObject->pPrev )
        pObject->pPrev->pNext = pObject->pNext;
    delete( pObject );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
VOID ConstructMenus()
{
    // Build video sub menu
    CMenuItem* pVideoSubMenu = new CMenuItem( _T("Video Menu"), MENU_VIDEO );
    pVideoSubMenu->Add( new CMenuItem( _T("Windowed"), MENU_WINDOWED ) );
    pVideoSubMenu->Add( new CMenuItem( _T("640x480"),  MENU_640x480 ) );
    pVideoSubMenu->Add( new CMenuItem( _T("800x600"),  MENU_800x600 ) );
    pVideoSubMenu->Add( new CMenuItem( _T("1024x768"), MENU_1024x768 ) );
    pVideoSubMenu->Add( new CMenuItem( _T("Back"),     MENU_BACK ) );

    // Build sound menu
    CMenuItem* pSoundSubMenu = new CMenuItem( _T("Sound Menu"), MENU_SOUND );
    pSoundSubMenu->Add( new CMenuItem( _T("Sound On"),  MENU_SOUNDON ) );
    pSoundSubMenu->Add( new CMenuItem( _T("Sound Off"), MENU_SOUNDOFF ) );
    pSoundSubMenu->Add( new CMenuItem( _T("Back"),      MENU_BACK ) );

    // Build input menu
    CMenuItem* pInputSubMenu = new CMenuItem( _T("Input Menu"), MENU_INPUT );
    pInputSubMenu->Add( new CMenuItem( _T("View Devices"),   MENU_VIEWDEVICES ) );
    pInputSubMenu->Add( new CMenuItem( _T("Config Devices"), MENU_CONFIGDEVICES ) );
    pInputSubMenu->Add( new CMenuItem( _T("Back"),           MENU_BACK ) );

    // Build main menu
    g_pMainMenu = new CMenuItem( _T("Main Menu"),  MENU_MAIN );
    g_pMainMenu->Add( pVideoSubMenu );
    g_pMainMenu->Add( pSoundSubMenu );
    g_pMainMenu->Add( pInputSubMenu );
    g_pMainMenu->Add( new CMenuItem( _T("Back to Game"), MENU_BACK ) );

    // Build "quit game?" menu
    g_pQuitMenu = new CMenuItem( _T("Quit Game?"),  MENU_MAIN );
    g_pQuitMenu->Add( new CMenuItem( _T("Yes"),     MENU_QUIT ) );
    g_pQuitMenu->Add( new CMenuItem( _T("No"),      MENU_BACK ) );

    return;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
VOID DestroyMenus()
{
    SAFE_DELETE( g_pQuitMenu );
    SAFE_DELETE( g_pMainMenu );
}




//-----------------------------------------------------------------------------
// Name: UpdateMenus()
// Desc:
//-----------------------------------------------------------------------------
VOID UpdateMenus()
{
    if( g_pCurrentMenu == NULL )
        return;

    // Keep track of current selected menu, to check later for changes
    DWORD dwCurrentSelectedMenu = g_pCurrentMenu->dwSelectedMenu;

    // Check for menu up/down input
    if( g_bMenuUp )
    {
        if( g_pCurrentMenu->dwSelectedMenu > 0 )
            g_pCurrentMenu->dwSelectedMenu--;
    }
    else if( g_bMenuDown )
    {
        if( (g_pCurrentMenu->dwSelectedMenu+1) < g_pCurrentMenu->dwNumChildren )
            g_pCurrentMenu->dwSelectedMenu++;
    }

    // The the current menu changed, play a sound
    if( dwCurrentSelectedMenu != g_pCurrentMenu->dwSelectedMenu )
        PlaySound( g_pSphereExplodeSound );

    if( g_bMenuSelect )
    {
        PlaySound( g_pSphereExplodeSound );

        DWORD dwID = g_pCurrentMenu->pChild[g_pCurrentMenu->dwSelectedMenu]->dwID;

        switch( dwID )
        {
            case MENU_BACK:
                g_pCurrentMenu = g_pCurrentMenu->pParent;
                break;

            case MENU_VIDEO:
            case MENU_SOUND:
            case MENU_INPUT:
                g_pCurrentMenu = g_pCurrentMenu->pChild[g_pCurrentMenu->dwSelectedMenu];
                break;

            case MENU_WINDOWED:
                SwitchDisplayModes( FALSE, 0L, 0L );
                g_pCurrentMenu = NULL;
                break;

            case MENU_640x480:
                SwitchDisplayModes( TRUE, 640, 480 );
                g_pCurrentMenu = NULL;
                break;

            case MENU_800x600:
                SwitchDisplayModes( TRUE, 800, 600 );
                g_pCurrentMenu = NULL;
                break;

            case MENU_1024x768:
                SwitchDisplayModes( TRUE, 1024, 768 );
                g_pCurrentMenu = NULL;
                break;

            case MENU_SOUNDON:
                if( g_pMusicManager == NULL )
                    CreateSoundObjects( g_hWndMain );
                g_pCurrentMenu = NULL;
                break;

            case MENU_SOUNDOFF:
                if( g_pMusicManager )
                    DestroySoundObjects();
                g_pCurrentMenu = NULL;
                break;

            case MENU_VIEWDEVICES:
                // Put action format to game play actions
                g_pInputDeviceManager->SetActionFormat( g_diafGame, TRUE );

                g_bMouseVisible = TRUE;

                // Configure the devices (with view capability only)
                if( g_bFullScreen )
                    g_pInputDeviceManager->ConfigureDevices( g_hWndMain,
                                                             g_pConfigSurface,
                                                             (VOID*)ConfigureInputDevicesCB,
                                                             DICD_DEFAULT );
                else
                    g_pInputDeviceManager->ConfigureDevices( g_hWndMain, NULL, NULL,
                                                             DICD_DEFAULT );
                g_bMouseVisible = FALSE;

                g_pCurrentMenu = NULL;
                break;

            case MENU_CONFIGDEVICES:
                // Put action format to game play actions
                g_pInputDeviceManager->SetActionFormat( g_diafGame, TRUE );

                g_bMouseVisible = TRUE;

                // Configure the devices (with edit capability)
                if( g_bFullScreen )
                    g_pInputDeviceManager->ConfigureDevices( g_hWndMain,
                                                             g_pConfigSurface,
                                                             (VOID*)ConfigureInputDevicesCB,
                                                             DICD_EDIT );
                else
                    g_pInputDeviceManager->ConfigureDevices( g_hWndMain, NULL, NULL,
                                                             DICD_EDIT );
                g_bMouseVisible = FALSE;

                g_pCurrentMenu = NULL;
                break;

            case MENU_QUIT:
                PostMessage( g_hWndMain, WM_CLOSE, 0, 0 );
                g_pCurrentMenu = NULL;
                break;
        }
    }

    // Check if the menu system is being exitted
    if( g_bMenuQuit )
        g_pCurrentMenu = NULL;

    // If the menu is going away, go back to game play actions
    if( g_pCurrentMenu == NULL )
        g_pInputDeviceManager->SetActionFormat( g_diafGame, TRUE );

    // Clear menu inputs
    g_bMenuUp     = FALSE;
    g_bMenuDown   = FALSE;
    g_bMenuSelect = FALSE;
    g_bMenuQuit   = FALSE;
}




//-----------------------------------------------------------------------------
// Display support code (using Direct3D functionality from D3DUtil.h)
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Name: CreateDisplayObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CreateDisplayObjects( HWND hWnd )
{
    HRESULT hr, hr1, hr2;

    // Construct a new display
    LPDIRECT3D8 pD3D = Direct3DCreate8( D3D_SDK_VERSION );
    if( NULL == pD3D )
    {
        CleanupAndDisplayError( DONUTS3DERR_NODIRECT3D );
        return E_FAIL;
    }

    // Get the current desktop format
    pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &g_DesktopMode );

    const D3DFORMAT fmtFullscreenArray[] = 
    {
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A1R5G5B5,
        D3DFMT_X8R8G8B8,
        D3DFMT_A8R8G8B8,
    };
    const INT numFullscreenFmts = sizeof(fmtFullscreenArray) / sizeof(fmtFullscreenArray[0]);
    INT iFmt;

    // Find a pixel format that will be good for fullscreen back buffers
    for( iFmt = 0; iFmt < numFullscreenFmts; iFmt++ )
    {
        if( SUCCEEDED( pD3D->CheckDeviceType( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
            fmtFullscreenArray[iFmt], fmtFullscreenArray[iFmt], FALSE ) ) )
        {
            g_d3dfmtFullscreen = fmtFullscreenArray[iFmt];
            break;
        }
    }

    const D3DFORMAT fmtTextureArray[] = 
    {
        D3DFMT_A1R5G5B5,
        D3DFMT_A4R4G4B4,
        D3DFMT_A8R8G8B8,
    };
    const INT numTextureFmts = sizeof(fmtTextureArray) / sizeof(fmtTextureArray[0]);

    // Find a format that is supported as a texture map for the current mode
    for( iFmt = 0; iFmt < numTextureFmts; iFmt++ )
    {
        if( SUCCEEDED( pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
            g_DesktopMode.Format, 0, D3DRTYPE_TEXTURE, fmtTextureArray[iFmt] ) ) )
        {
            g_d3dfmtTexture = fmtTextureArray[iFmt];
            break;
        }
    }

    // Set up presentation parameters for the display
    ZeroMemory( &g_d3dpp, sizeof(g_d3dpp) );
    g_d3dpp.Windowed         = !g_bFullScreen;
    g_d3dpp.BackBufferCount  = 1;
    g_d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    if( g_bFullScreen )
    {
        g_d3dpp.hDeviceWindow    = hWnd;
        g_d3dpp.BackBufferWidth  = g_dwScreenWidth;
        g_d3dpp.BackBufferHeight = g_dwScreenHeight;
        g_d3dpp.BackBufferFormat = g_d3dfmtFullscreen;
    }
    else
    {
        g_d3dpp.BackBufferFormat = g_DesktopMode.Format;
    }

    // Create the device
    hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                             D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                             &g_d3dpp, &g_pd3dDevice );
    pD3D->Release();
    if( FAILED(hr) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NOD3DDEVICE );
        return E_FAIL;
    }

    // Create some game fonts
    g_pGameFont = new CD3DFont( _T("Tahoma"), 30, 0L );
    g_pGameFont->InitDeviceObjects( g_pd3dDevice );

    g_pMenuFont = new CD3DFont( _T("Impact"), 48, 0L );
    g_pMenuFont->InitDeviceObjects( g_pd3dDevice );

    // Find the media files (textures and geometry models) for the game
    TCHAR strGameTexture1[512];
    TCHAR strGameTexture2[512];
    hr1 = DXUtil_FindMediaFile( strGameTexture1, _T("Donuts1.bmp") );
    hr2 = DXUtil_FindMediaFile( strGameTexture2, _T("Donuts2.bmp") );

    if( FAILED(hr1) || FAILED(hr2) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NOTEXTURES );
        return E_FAIL;
    }

    // Load the game textures
    if( FAILED( D3DUtil_CreateTexture( g_pd3dDevice, strGameTexture1,
                                       &g_pGameTexture1, g_d3dfmtTexture ) ) ||
        FAILED( D3DUtil_CreateTexture( g_pd3dDevice, strGameTexture2,
                                       &g_pGameTexture2, g_d3dfmtTexture ) ) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NOTEXTURES );
        return E_FAIL;
    }

    D3DUtil_SetColorKey( g_pGameTexture1, 0x00000000 );
    D3DUtil_SetColorKey( g_pGameTexture2, 0x00000000 );

    // Load the geometry models
    hr1 = LoadShipModel();
    hr2 = LoadTerrainModel();

    if( FAILED(hr1) || FAILED(hr2) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NOGEOMETRY );
        return E_FAIL;
    }

    // Create a viewport covering sqaure
    if( FAILED( g_pd3dDevice->CreateVertexBuffer( 4*sizeof(SCREENVERTEX),
                                       D3DUSAGE_WRITEONLY, D3DFVF_SCREENVERTEX,
                                       D3DPOOL_MANAGED, &g_pViewportVB ) ) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NO3DRESOURCES );
        return E_FAIL;
    }

    // Create a sqaure for rendering the sprites
    if( FAILED( g_pd3dDevice->CreateVertexBuffer( 4*sizeof(SPRITEVERTEX),
                                       D3DUSAGE_WRITEONLY, D3DFVF_SPRITEVERTEX,
                                       D3DPOOL_MANAGED, &g_pSpriteVB ) ) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NO3DRESOURCES );
        return E_FAIL;
    }

    // Now that all the display objects are created, "restore" them (create
    // local mem objects and set state)
    if( FAILED( RestoreDisplayObjects() ) )
                return E_FAIL;

        // The display is now ready
        g_bDisplayReady = TRUE;
        return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDisplayObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT RestoreDisplayObjects()
{
    HRESULT hr;
    HWND hWnd = g_hWndMain;

    if( FALSE == g_bFullScreen )
    {
        // If we are still a WS_POPUP window we should convert to a normal app
        // window so we look like a windows app.
        DWORD dwStyle  = GetWindowStyle( hWnd );
        dwStyle &= ~WS_POPUP;
        dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;
        SetWindowLong( hWnd, GWL_STYLE, dwStyle );

        // Aet window size
        RECT rc;
        SetRect( &rc, 0, 0, 640, 480 );

        AdjustWindowRectEx( &rc, GetWindowStyle(hWnd), GetMenu(hWnd) != NULL,
                            GetWindowExStyle(hWnd) );

        SetWindowPos( hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

        SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );

        //  Make sure our window does not hang outside of the work area
        RECT rcWork;
        SystemParametersInfo( SPI_GETWORKAREA, 0, &rcWork, 0 );
        GetWindowRect( hWnd, &rc );
        if( rc.left < rcWork.left ) rc.left = rcWork.left;
        if( rc.top  < rcWork.top )  rc.top  = rcWork.top;
        SetWindowPos( hWnd, NULL, rc.left, rc.top, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
    }

    // Create the device-dependent objects for the file-based mesh objects
    g_pShipFileObject->RestoreDeviceObjects( g_pd3dDevice );
    g_pTerrain->RestoreDeviceObjects( g_pd3dDevice );
    g_pGameFont->RestoreDeviceObjects();
    g_pMenuFont->RestoreDeviceObjects();

    // Get viewport dimensions
    D3DVIEWPORT8 vp;
    g_pd3dDevice->GetViewport(&vp);
    FLOAT sx = (FLOAT)vp.Width;
    FLOAT sy = (FLOAT)vp.Height;

    // Setup dimensions for the viewport covering sqaure
    SCREENVERTEX* v;
    g_pViewportVB->Lock( 0, 0, (BYTE**)&v, 0 );
    v[0].p = D3DXVECTOR4( 0,sy,0.0f,1.0f);
    v[1].p = D3DXVECTOR4( 0, 0,0.0f,1.0f);
    v[2].p = D3DXVECTOR4(sx,sy,0.0f,1.0f);
    v[3].p = D3DXVECTOR4(sx, 0,0.0f,1.0f);
    g_pViewportVB->Unlock();

    // Create a surface for confguring DInput devices
    hr = g_pd3dDevice->CreateImageSurface( 640, 480, g_d3dfmtFullscreen, &g_pConfigSurface );
    if( FAILED(hr) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NO3DRESOURCES );
        return E_FAIL;
    }

    // Set up the camera
    g_Camera.SetViewParams( D3DXVECTOR3(0.0f,0.0f,0.0f), D3DXVECTOR3(0.0f,0.0f,1.0f),
                            D3DXVECTOR3(0.0f,1.0f,0.0f) );
    g_Camera.SetProjParams( D3DX_PI/4, 1.0f, 0.1f, 100.0f );

    // Set up default matrices (using the CD3DCamera class)
    g_pd3dDevice->SetTransform( D3DTS_VIEW,       &g_Camera.GetViewMatrix() );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &g_Camera.GetProjMatrix() );

    // Setup a material
    D3DMATERIAL8 mtrl;
    D3DUtil_InitMaterial( mtrl, 1.0f, 1.0f, 1.0f );
    g_pd3dDevice->SetMaterial( &mtrl );

    // Set up lighting states
    D3DLIGHT8 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, 1.0f, -1.0f, 1.0f );
    g_pd3dDevice->SetLight( 0, &light );
    g_pd3dDevice->LightEnable( 0, TRUE );

    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x33333333 );

    // Set miscellaneous render states
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDisplayObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT InvalidateDisplayObjects()
{
    g_pShipFileObject->InvalidateDeviceObjects();
    g_pTerrain->InvalidateDeviceObjects();
    g_pGameFont->InvalidateDeviceObjects();
    g_pMenuFont->InvalidateDeviceObjects();

    SAFE_RELEASE( g_pConfigSurface );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: DestroyDisplayObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT DestroyDisplayObjects()
{
    DisplayObject* pDO;
    while( g_pDisplayList != NULL )
    {
        pDO = g_pDisplayList;
        g_pDisplayList = g_pDisplayList->pNext;
        delete pDO;
        if( g_pDisplayList != NULL)
            g_pDisplayList->pPrev = NULL;
    }
    
    SAFE_RELEASE( g_pGameTexture1 );
    SAFE_RELEASE( g_pGameTexture2 );

    SAFE_DELETE( g_pGameFont );
    SAFE_DELETE( g_pMenuFont );

    SAFE_RELEASE( g_pConfigSurface );

    if( g_pShipFileObject != NULL )
        g_pShipFileObject->Destroy();
    if( g_pTerrain != NULL )
        g_pTerrain->Destroy();

    SAFE_DELETE( g_pTerrain );
    SAFE_DELETE( g_pShipFileObject );

    SAFE_RELEASE( g_pViewportVB );
    SAFE_RELEASE( g_pSpriteVB );
    SAFE_RELEASE( g_pd3dDevice );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SwitchDisplayModes()
// Desc:
//-----------------------------------------------------------------------------
HRESULT SwitchDisplayModes( BOOL bFullScreen, DWORD dwWidth, DWORD dwHeight )
{
    HRESULT hr;

    if( FALSE==g_bIsActive || FALSE==g_bDisplayReady )
        return S_OK;

    // Check to see if a change was actually requested
    if( bFullScreen )
    {
        if( g_dwScreenWidth==dwWidth && g_dwScreenHeight==dwHeight &&
            g_bFullScreen==bFullScreen )
            return S_OK;
    }
    else
    {
        if( g_bFullScreen == FALSE )
            return S_OK;
    }

    // Invalidate the old display objects
    g_bDisplayReady = FALSE;
    InvalidateDisplayObjects();

    // Set up the new presentation paramters
    if( bFullScreen )
    {
        g_d3dpp.Windowed         = FALSE;
        g_d3dpp.hDeviceWindow    = g_hWndMain;
        g_d3dpp.BackBufferWidth  = g_dwScreenWidth  = dwWidth;
        g_d3dpp.BackBufferHeight = g_dwScreenHeight = dwHeight;
        g_d3dpp.BackBufferFormat = g_d3dfmtFullscreen;
    }
    else
    {
        g_d3dpp.Windowed         = TRUE;
        g_d3dpp.hDeviceWindow    = NULL;
        g_d3dpp.BackBufferWidth  = 0L;
        g_d3dpp.BackBufferHeight = 0L;

        g_d3dpp.BackBufferFormat = g_DesktopMode.Format;
    }

    // Reset the device
    if( SUCCEEDED( hr = g_pd3dDevice->Reset( &g_d3dpp ) ) )
    {
        g_bFullScreen   = bFullScreen;
        if( SUCCEEDED( hr = RestoreDisplayObjects() ) )
        {
            g_bDisplayReady = TRUE;
            return S_OK;
        }
    }

    // If we get here, a fatal error occurred
    PostMessage( g_hWndMain, WM_CLOSE, 0, 0 );
    return E_FAIL;
}




//-----------------------------------------------------------------------------
// Name: ShowFrame()
// Desc:
//-----------------------------------------------------------------------------
VOID ShowFrame()
{
    if( NULL == g_pd3dDevice )
        return;

    // Present the backbuffer contents to the front buffer
    g_pd3dDevice->Present( 0, 0, 0, 0 );
}




//-----------------------------------------------------------------------------
// Sound support code (using DMusic functionality from DMUtil.h)
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Name: CreateSoundObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CreateSoundObjects( HWND hWnd )
{
    // Create the music manager class, used to create the sounds
    g_pMusicManager = new CMusicManager();
    if( FAILED( g_pMusicManager->Initialize( hWnd ) ) )
        return E_FAIL;

    // Instruct the music manager where to find the files
    g_pMusicManager->SetSearchDirectory( DXUtil_GetDXSDKMediaPath() );

    // Create the sounds
    g_pMusicManager->CreateSegmentFromResource( &g_pBeginLevelSound,     _T("BEGINLEVEL"), _T("WAV") );
    g_pMusicManager->CreateSegmentFromResource( &g_pEngineIdleSound,     _T("ENGINEIDLE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pEngineRevSound,      _T("ENGINEREV") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pShieldBuzzSound,     _T("SHIELDBUZZ") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pShipExplodeSound,    _T("SHIPEXPLODE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pFireBulletSound,     _T("GUNFIRE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pShipBounceSound,     _T("SHIPBOUNCE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pDonutExplodeSound,   _T("DONUTEXPLODE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pPyramidExplodeSound, _T("PYRAMIDEXPLODE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pCubeExplodeSound,    _T("CUBEEXPLODE") , _T("WAV"));
    g_pMusicManager->CreateSegmentFromResource( &g_pSphereExplodeSound,  _T("SPHEREEXPLODE") , _T("WAV"));

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DestroySoundObjects()
// Desc:
//-----------------------------------------------------------------------------
VOID DestroySoundObjects()
{
    SAFE_DELETE( g_pBeginLevelSound );
    SAFE_DELETE( g_pEngineIdleSound );
    SAFE_DELETE( g_pEngineRevSound );
    SAFE_DELETE( g_pShieldBuzzSound );
    SAFE_DELETE( g_pShipExplodeSound );
    SAFE_DELETE( g_pFireBulletSound );
    SAFE_DELETE( g_pShipBounceSound );
    SAFE_DELETE( g_pDonutExplodeSound );
    SAFE_DELETE( g_pPyramidExplodeSound );
    SAFE_DELETE( g_pCubeExplodeSound );
    SAFE_DELETE( g_pSphereExplodeSound );

    SAFE_DELETE( g_pMusicManager );
}




//-----------------------------------------------------------------------------
// Input support code (using DInput functionality from DIUtil.h)
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Name: CreateInputObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CreateInputObjects( HWND hWnd )
{
    HRESULT hr;

    // Setup action format for the acutal gameplay
    ZeroMemory( &g_diafGame, sizeof(DIACTIONFORMAT) );
    g_diafGame.dwSize          = sizeof(DIACTIONFORMAT);
    g_diafGame.dwActionSize    = sizeof(DIACTION);
    g_diafGame.dwDataSize      = NUMBER_OF_GAMEACTIONS * sizeof(DWORD);
    g_diafGame.guidActionMap   = g_AppGuid;
    g_diafGame.dwGenre         = DIVIRTUAL_SPACESIM;
    g_diafGame.dwNumActions    = NUMBER_OF_GAMEACTIONS;
    g_diafGame.rgoAction       = g_rgGameAction;
    g_diafGame.lAxisMin        = -10;
    g_diafGame.lAxisMax        = 10;
    g_diafGame.dwBufferSize    = 16;
    _tcscpy( g_diafGame.tszActionMap, _T("Donuts3D New") );

    // Setup action format for the in-game menus
    ZeroMemory( &g_diafBrowser, sizeof(DIACTIONFORMAT) );
    g_diafBrowser.dwSize          = sizeof(DIACTIONFORMAT);
    g_diafBrowser.dwActionSize    = sizeof(DIACTION);
    g_diafBrowser.dwDataSize      = NUMBER_OF_BROWSERACTIONS * sizeof(DWORD);
    g_diafBrowser.guidActionMap   = g_AppGuid;
    g_diafBrowser.dwGenre         = DIVIRTUAL_BROWSER_CONTROL;
    g_diafBrowser.dwNumActions    = NUMBER_OF_BROWSERACTIONS;
    g_diafBrowser.rgoAction       = g_rgBrowserAction;
    g_diafBrowser.lAxisMin        = -10;
    g_diafBrowser.lAxisMax        = 10;
    g_diafBrowser.dwBufferSize    = 16;
    _tcscpy( g_diafBrowser.tszActionMap, _T("Donuts New") );

    // Create a new input device manager
    g_pInputDeviceManager = new CInputDeviceManager();

    if( FAILED( hr = g_pInputDeviceManager->Create( hWnd, NULL, g_diafGame ) ) )
    {
        CleanupAndDisplayError( DONUTS3DERR_NOINPUT );
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DestroyInputObjects()
// Desc:
//-----------------------------------------------------------------------------
VOID DestroyInputObjects()
{
    // Delete input device manager
    SAFE_DELETE( g_pInputDeviceManager );
}




//-----------------------------------------------------------------------------
// Name: ConfigureInputDevicesCB()
// Desc: Callback function for configuring input devices. This function is
//       called in fullscreen modes, so that the input device configuration
//       window can update the screen.
//-----------------------------------------------------------------------------
BOOL CALLBACK ConfigureInputDevicesCB( IUnknown* pUnknown, VOID* pUserData )
{
    if( g_dwAppState != APPSTATE_ACTIVE )
        return TRUE;

    // Get access to the surface
    LPDIRECT3DSURFACE8 pConfigSurface;
    if( FAILED( pUnknown->QueryInterface( IID_IDirect3DSurface8,
                                          (VOID**)&pConfigSurface ) ) )
        return TRUE;

    // Draw the scene, with the config surface blitted on top
    DrawDisplayList();

    RECT  rcSrc;
    SetRect( &rcSrc, 0, 0, 640, 480 );

    POINT ptDst;
    ptDst.x = (g_dwScreenWidth-640)/2;
    ptDst.y = (g_dwScreenHeight-480)/2;

    LPDIRECT3DSURFACE8 pBackBuffer;
    g_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    g_pd3dDevice->CopyRects( pConfigSurface, &rcSrc, 1, pBackBuffer, &ptDst );
    pBackBuffer->Release();

    ShowFrame();

    // Release the surface
    pConfigSurface->Release();

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: GetInput()
// Desc: Processes data from the input device.  Uses GetDeviceState().
//-----------------------------------------------------------------------------
VOID GetInput()
{
    // Buffers for accumulating data from input devices
    static FLOAT fLeftThrust[10]    = {0};
    static FLOAT fRightThrust[10]   = {0};
    static FLOAT fForwardThrust[10] = {0};
    static FLOAT fReverseThrust[10] = {0};

    // Get access to the list of semantically-mapped input devices
    LPDIRECTINPUTDEVICE8* pdidDevices;
    DWORD                 dwNumDevices;
    g_pInputDeviceManager->GetDevices( &pdidDevices, &dwNumDevices );

    // Loop through all devices and check game input
    for( DWORD i=0; i<dwNumDevices; i++ )
    {
        DIDEVICEOBJECTDATA rgdod[10];
        DWORD   dwItems = 10;
        HRESULT hr;

        hr = pdidDevices[i]->Acquire();
        hr = pdidDevices[i]->Poll();
        hr = pdidDevices[i]->GetDeviceData( sizeof(DIDEVICEOBJECTDATA),
                                           rgdod, &dwItems, 0 );
        if( FAILED(hr) )
            continue;

        // Get the sematics codes for the game menu
        for( DWORD j=0; j<dwItems; j++ )
        {
            BOOL  bButtonState = (rgdod[j].dwData==0x80) ? TRUE : FALSE;
            FLOAT fButtonState = (rgdod[j].dwData==0x80) ? 1.0f : 0.0f;
            FLOAT fAxisState   = (FLOAT)((int)rgdod[j].dwData)/10.0f;

            switch( rgdod[j].uAppData )
            {
                // Handle semantics for the game menu
                case INPUT_MENU_LR:     // There are no nieghboring menus so ignore this one
                    break;

                case INPUT_MENU_UD:
                {
                    // The axis goes between -1 and 1.  We need to set g_bMenuDown or g_bMenuUp
                    // only one time.
                    static BOOL s_bOneTimeUD = FALSE;

                    if( fabs(fAxisState) < 0.2 )
                    {
                        s_bOneTimeUD = FALSE;
                    }
                    else
                    {
                        if ( !s_bOneTimeUD )
                        {
                            if( fAxisState > 0.0f )
                                g_bMenuDown = TRUE;
                            else
                                g_bMenuUp = TRUE;

                            s_bOneTimeUD = TRUE;
                        }
                    }

                    break;
                }

                case INPUT_MENU_UP:     g_bMenuUp     |= bButtonState; break;
                case INPUT_MENU_DOWN:   g_bMenuDown   |= bButtonState; break;
                case INPUT_MENU_LEFT:   g_bMenuLeft   |= bButtonState; break;
                case INPUT_MENU_RIGHT:  g_bMenuRight  |= bButtonState; break;
                case INPUT_MENU_SELECT: g_bMenuSelect |= bButtonState; break;
                case INPUT_MENU_QUIT:   g_bMenuQuit   |= bButtonState; break;

                case INPUT_MENU_WHEEL:
                    if( fAxisState > 0.0f )
                        g_bMenuUp = TRUE;
                    else
                        g_bMenuDown = TRUE;
                    break;

                // Handle semantics for normal game play
                case INPUT_AXIS_LR:
                case INPUT_MOUSE_LR:
                    fLeftThrust[i]  = 0.0f;
                    fRightThrust[i] = 0.0f;
                    if( fAxisState > 0.0f )
                        fRightThrust[i] = +fAxisState;
                    else
                        fLeftThrust[i]  = -fAxisState;
                    break;
                case INPUT_AXIS_UD:
                case INPUT_MOUSE_UD:
                    fForwardThrust[i] = 0.0f;
                    fReverseThrust[i] = 0.0f;
                    if( fAxisState > 0.0f )
                        fReverseThrust[i] = +fAxisState;
                    else
                        fForwardThrust[i] = -fAxisState;
                    break;

                // User wants to change ship type
                case INPUT_AXIS_SHIPTYPE:
                case INPUT_MOUSE_SHIPTYPE:
                    SwitchModel();
                    break;

                case INPUT_TURNLEFT:      fLeftThrust[i]    = fButtonState; break;
                case INPUT_TURNRIGHT:     fRightThrust[i]   = fButtonState; break;
                case INPUT_FORWARDTHRUST: fForwardThrust[i] = fButtonState; break;
                case INPUT_REVERSETHRUST: fReverseThrust[i] = fButtonState; break;

                case INPUT_FIREWEAPONS:   g_bFiringWeapons  = bButtonState; break;
                case INPUT_CHANGEVIEW:    g_bChangeView     = bButtonState; break;

                // User wants to change weapons
                case INPUT_CHANGEWEAPONS:
                    if( bButtonState )
                    {
                        if( ++g_dwBulletType > 3 )
                            g_dwBulletType = 0L;
                    }
                    break;

                // User wants to change ship type
                case INPUT_CHANGESHIPTYPE:
                    if( bButtonState )
                        SwitchModel();
                    break;

                case INPUT_START:
                    if( bButtonState )
                        g_bPaused = !g_bPaused;
                    break;

                case INPUT_DISPLAYGAMEMENU:
                    if( bButtonState )
                    {
                        PlaySound( g_pSphereExplodeSound );
                        g_pCurrentMenu = g_pMainMenu;
                        g_pInputDeviceManager->SetActionFormat( g_diafBrowser, FALSE );
                    }
                    break;

                case INPUT_QUITGAME:
                    if( bButtonState )
                    {
                        PlaySound( g_pSphereExplodeSound );
                        g_pCurrentMenu = g_pQuitMenu;
                        g_pInputDeviceManager->SetActionFormat( g_diafBrowser, FALSE );
                    }
                    break;
            }
        }
    }

    if( g_pShip->bVisible  )
    {
        // Accumulate thrust inputs
        g_fBank   = 0.0f;
        g_fThrust = 0.0f;

        for( DWORD i=0; i<dwNumDevices; i++ )
        {
            if( fRightThrust[i] > 0.4f )   g_fBank   += fRightThrust[i];
            if( fLeftThrust[i] > 0.4f )    g_fBank   -= fLeftThrust[i];

            if( fForwardThrust[i] > 0.4f ) g_fThrust += fForwardThrust[i];
            if( fReverseThrust[i] > 0.4f ) g_fThrust -= fReverseThrust[i];
        }
    }
    else
    {
        // If the ship is not visible, zero out appropriate inputs
        g_fBank           = 0.0f;
        g_fThrust         = 0.0f;
        g_bFiringWeapons  = FALSE;
        g_bChangeView     = FALSE;
    }
}




//-----------------------------------------------------------------------------
// Error handling
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Name: CleanupAndDisplayError()
// Desc:
//-----------------------------------------------------------------------------
VOID CleanupAndDisplayError( DWORD dwError )
{
    TCHAR* strDbgOut;
    TCHAR* strMsgBox;

    // Cleanup the app
    DestroyGameObjects();

    // Make the cursor visible
    SetCursor( LoadCursor( NULL, IDC_ARROW ) );
    g_bMouseVisible = TRUE;

    // Get the appropriate error strings
    switch( dwError )
    {
        case DONUTS3DERR_NODIRECT3D:
            strDbgOut = _T("Could not create Direct3D\n");
            strMsgBox = _T("Could not create Direct3D.\n\n")
                        _T("Please make sure you have the latest DirectX\n")
                        _T(".dlls installed on your system.");
            break;
        case DONUTS3DERR_NOD3DDEVICE:
            strDbgOut = _T("Could not create a Direct3D device\n");
            strMsgBox = _T("Could not create a Direct3D device. Your\n")
                        _T("graphics accelerator is not sufficient to\n")
                        _T("run this demo, or your desktop is using\n")
                        _T("a color format that cannot be accelerated by\n")
                        _T("your graphics card (try 16-bit mode).");
            break;
        case DONUTS3DERR_NOTEXTURES:
            strDbgOut = _T("Could not load textures\n");
            strMsgBox = _T("Couldn't load game textures.\n\n")
                        _T("Either your graphics hardware does not have\n")
                        _T("sufficient resources, or the DirectX SDK was\n")
                        _T("not properly installed.");
            break;
        case DONUTS3DERR_NOGEOMETRY:
            strDbgOut = _T("Could not load .x models\n");
            strMsgBox = _T("Couldn't load game geometry.\n\n")
                        _T("Either your graphics hardware does not have\n")
                        _T("sufficient resources, or the DirectX SDK was\n")
                        _T("not properly installed.");
            break;
        case DONUTS3DERR_NO3DRESOURCES:
            strDbgOut = _T("Couldn't load create a d3d object\n");
            strMsgBox = _T("Couldn't create display objects.\n")
                        _T("Yourr graphics hardware does not have\n")
                        _T("sufficient resources to run this app.");
            break;
        case DONUTS3DERR_NOINPUT:
            strDbgOut = _T("Could not create input objects\n");
            strMsgBox = _T("Could not create input objects.");
            break;
    }

    // Output the error strings
    OutputDebugString( strDbgOut );
    MessageBox( g_hWndMain, strMsgBox, _T("Donuts3D"), MB_OK );
}

