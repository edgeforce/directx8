//-----------------------------------------------------------------------------
// File: Text3D.cpp
//
// Desc: Example code showing how to do text in a Direct3D scene.
//
// Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <math.h>
#include <tchar.h>
#include <stdio.h>
#include <D3DX8.h>
#include "D3DApp.h"
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DXUtil.h"
#include "resource.h"




//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    CD3DFont*     m_pFont;
    CD3DFont*     m_pStatsFont;
    TCHAR         m_strFont[100];
    DWORD         m_dwFontSize;

    D3DXMATRIX    m_matObj1;
    D3DXMATRIX    m_matObj2;
    D3DXMATRIX    m_matObj3;
    D3DXMATRIX    m_matObj4;
    D3DXMATRIX    m_matObj5;

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT FinalCleanup();

public:
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    CMyD3DApplication();
};




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_strWindowTitle    = _T("Text3D: Text in a 3D scene");
    m_bUseDepthBuffer   = FALSE;

    // Create fonts
    lstrcpy( m_strFont, _T("Arial") );
    m_dwFontSize = 18;
    m_pFont      = new CD3DFont( m_strFont, m_dwFontSize );
    m_pStatsFont = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Setup five rotation matrices (for rotating text strings)
    D3DXMatrixRotationAxis( &m_matObj1, &D3DXVECTOR3(1.0f,2.0f,0.0f), m_fTime/1.1f  );
    D3DXMatrixRotationAxis( &m_matObj2, &D3DXVECTOR3(2.0f,1.0f,0.0f), m_fTime/1.2f  );
    D3DXMatrixRotationAxis( &m_matObj3, &D3DXVECTOR3(1.0f,0.0f,2.0f), m_fTime/1.3f  );
    D3DXMatrixRotationAxis( &m_matObj4, &D3DXVECTOR3(2.0f,0.0f,1.0f), m_fTime/1.4f  );
    D3DXMatrixRotationAxis( &m_matObj5, &D3DXVECTOR3(0.0f,1.0f,2.0f), m_fTime/1.5f  );

    // Add some translational values to the matrices
    m_matObj1._41 = 1.0f;   m_matObj1._42 = 3.0f;   m_matObj1._43 = 0.0f; 
    m_matObj2._41 = 2.0f;   m_matObj2._42 =-2.0f;   m_matObj2._43 = 0.0f; 
    m_matObj3._41 = 3.0f;   m_matObj3._42 = 1.0f;   m_matObj3._43 = 0.0f; 
    m_matObj4._41 =-1.0f;   m_matObj4._42 =-3.0f;   m_matObj4._43 = 0.0f;
    m_matObj5._41 =-2.0f;   m_matObj5._42 = 2.0f;   m_matObj5._43 = 0.0f; 

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
    D3DMATERIAL8 mtrl;

    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L );

    // Begin the scene 
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Draw the 5 spinning, colored 3D text objects
        D3DUtil_InitMaterial( mtrl, 1.0f, 0.0f, 0.0f );
        m_pd3dDevice->SetMaterial( &mtrl );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj1 );
        m_pFont->Render3DText( _T("Red Text"), D3DFONT_CENTERED|D3DFONT_TWOSIDED|D3DFONT_FILTERED );

        D3DUtil_InitMaterial( mtrl, 0.0f, 1.0f, 0.0f );
        m_pd3dDevice->SetMaterial( &mtrl );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj2 );
        m_pFont->Render3DText( _T("Green Text"), D3DFONT_CENTERED|D3DFONT_TWOSIDED|D3DFONT_FILTERED );

        D3DUtil_InitMaterial( mtrl, 0.0f, 0.0f, 1.0f );
        m_pd3dDevice->SetMaterial( &mtrl );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj3 );
        m_pFont->Render3DText( _T("Blue Text"), D3DFONT_CENTERED|D3DFONT_TWOSIDED|D3DFONT_FILTERED );

        D3DUtil_InitMaterial( mtrl, 1.0f, 0.0f, 1.0f );
        m_pd3dDevice->SetMaterial( &mtrl );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj4 );
        m_pFont->Render3DText( _T("Purple Text"), D3DFONT_CENTERED|D3DFONT_TWOSIDED|D3DFONT_FILTERED );

        D3DUtil_InitMaterial( mtrl, 1.0f, 1.0f, 0.0f );
        m_pd3dDevice->SetMaterial( &mtrl );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj5 );
        m_pFont->Render3DText( _T("Yellow Text"), D3DFONT_CENTERED|D3DFONT_TWOSIDED|D3DFONT_FILTERED );

        // Draw some 2D text
        m_pFont->DrawText( 2, 40, 0xffffffff, _T("2D Text") );

        // Show frame rate
        m_pStatsFont->DrawText( 2,  0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );
        m_pStatsFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    // Restore the fonts
    m_pFont->InitDeviceObjects( m_pd3dDevice );
    m_pStatsFont->InitDeviceObjects( m_pd3dDevice );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    // Restore the fonts
    m_pFont->RestoreDeviceObjects();
    m_pStatsFont->RestoreDeviceObjects();

    // Restore the textures
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT,  0x00ffffff );

    // Set the transform matrices
    D3DXVECTOR3 vEyePt    = D3DXVECTOR3( 0.0f,-5.0f,-10.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f,  0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f,  0.0f );
    D3DXMATRIX  matWorld, matView, matProj;

    D3DXMatrixIdentity( &matWorld );
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    FLOAT fAspect = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, 100.0f );

    m_pd3dDevice->SetTransform( D3DTS_WORLD,      &matWorld );
    m_pd3dDevice->SetTransform( D3DTS_VIEW,       &matView );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    m_pFont->InvalidateDeviceObjects();
    m_pStatsFont->InvalidateDeviceObjects();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
    m_pFont->DeleteDeviceObjects();
    m_pStatsFont->DeleteDeviceObjects();
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    SAFE_DELETE( m_pFont );
    SAFE_DELETE( m_pStatsFont );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    if( WM_COMMAND == uMsg )
    {
        switch( LOWORD(wParam) )
        {
            case IDM_FONT:
            {
                HDC hdc;
                LONG lHeight;
                hdc = GetDC( hWnd );
                lHeight = -MulDiv( m_dwFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
                ReleaseDC( hWnd, hdc );
                hdc = NULL;

                LOGFONT lf;
                lstrcpy( lf.lfFaceName, m_strFont );
                lf.lfHeight = lHeight;

                CHOOSEFONT cf;
                ZeroMemory( &cf, sizeof(cf) );
                cf.lStructSize = sizeof(cf);
                cf.hwndOwner   = m_hWnd;
                cf.lpLogFont   = &lf;
                cf.Flags       = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
                
                if( ChooseFont( &cf ) )
                {
                    SAFE_DELETE( m_pFont );
                    lstrcpy( m_strFont, lf.lfFaceName );
                    m_dwFontSize = cf.iPointSize / 10;
                    m_pFont = new CD3DFont( m_strFont, m_dwFontSize );
                    m_pFont->InitDeviceObjects( m_pd3dDevice );
                    m_pFont->RestoreDeviceObjects();
                }
                break;
            }
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



