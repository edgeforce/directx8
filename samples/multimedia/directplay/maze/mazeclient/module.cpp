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
#include <d3dx.h>
#include <stdio.h>
#include <math.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <dxerr8.h>
#include "DXUtil.h"
#include "SyncObjects.h"
#include "Module.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CModule::CModule()
{
    m_pDevice       = NULL;
    m_pDDraw        = NULL;
    m_pXContext     = NULL;
    m_pBackBuffer   = NULL;
    m_pFrameRate    = NULL;
    m_hWnd          = NULL;
    m_pStateBlocks  = NULL;

    m_pFrameRate    = new CFrameRate;
    m_pStateBlocks  = new CommonStateBlocks;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CModule::~CModule()
{
    SAFE_DELETE(m_pFrameRate);
    SAFE_DELETE(m_pStateBlocks);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CModule::OneTimeInit( HWND hWnd )
{
    m_hWnd = hWnd;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::OneTimeShutdown()
{
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CModule::DisplayInit( DWORD flags, IDirectDrawSurface7* target, RECT* subrect )
{
    HRESULT hr;
    m_dwFlags = flags;

    // Can't specify a target or a sub-rectangle for the root module
    if( target != NULL || subrect != NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("Param"), E_FAIL );

    DWORD device_index;
    if( flags & MOD_DISP_REFRAST )
        device_index = D3DX_HWLEVEL_REFERENCE;
    else if( flags & MOD_DISP_SWRASTER )
        device_index = D3DX_HWLEVEL_2D;
    else if( flags & MOD_DISP_SWTL )
        device_index = D3DX_HWLEVEL_RASTER;
    else
        device_index = D3DX_DEFAULT;

    // Windowed or fullscreen?
    if( flags & MOD_DISP_WINDOWED )
    {
        // Windowed
        RECT    rect;
        GetClientRect( m_hWnd, &rect );
        DWORD   width = rect.right - rect.left;
        DWORD   height = rect.bottom - rect.top;
        hr = D3DXCreateContextEx( device_index, 0, m_hWnd, NULL, D3DX_DEFAULT, 0 ,
                                  D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT ,
                                  width, height, D3DX_DEFAULT, &m_pXContext );
        if( FAILED( hr) )
            return DXTRACE_ERR_NOMSGBOX( TEXT("D3DXCreateContextEx"), hr );
    }
    else
    {
        // Fullscreen. Try for 32-bit if the "prefer 32" flag was set. If that fails, or the flag wasn't set
        // then we just fall back to 16-bit
        hr = E_FAIL;
        if( flags & MOD_DISP_PREFER32BIT )
        {
            hr = D3DXCreateContextEx( device_index, D3DX_CONTEXT_FULLSCREEN, m_hWnd, NULL, 32, 0 ,
                                      D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT ,
                                      640, 480, D3DX_DEFAULT, &m_pXContext );
        }
        if( FAILED( hr) )
        {
            hr = D3DXCreateContextEx( device_index, D3DX_CONTEXT_FULLSCREEN, m_hWnd, NULL, 16, 0 ,
                                      D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT ,
                                      640, 480, D3DX_DEFAULT, &m_pXContext );
        }
        if( FAILED( hr) )
            return DXTRACE_ERR_NOMSGBOX( TEXT("D3DXCreateContextEx"), hr );
    }

    // Grab various useful objects
    m_pDDraw = m_pXContext->GetDD();
    m_pD3D = m_pXContext->GetD3D();
    m_pBackBuffer = m_pXContext->GetBackBuffer( 0 );
    m_pDevice = m_pXContext->GetD3DDevice();
    m_pXContext->GetBufferSize( &m_dwWidth, &m_dwHeight );
    m_pXContext->GetNumBits( &m_dwBPP, &m_dwZBits, NULL, &m_dwStencilBits );
    m_bNeedToRestore = FALSE;

    // Grab caps of device
    ZeroMemory( &m_DeviceCaps, sizeof(m_DeviceCaps) );
    m_pDevice->GetCaps( &m_DeviceCaps );

    // Figure out what kind of memory we need
    if( m_DeviceCaps.deviceGUID == IID_IDirect3DTnLHalDevice )
        m_dwVBMemType = 0;
    else
        m_dwVBMemType = D3DVBCAPS_SYSTEMMEMORY;

    // Figure out if we have software rasterisation (may want to cut features for software)
    if( m_DeviceCaps.deviceGUID == IID_IDirect3DRGBDevice )
        m_bSoftwareRasterisation = TRUE;
    else
        m_bSoftwareRasterisation = FALSE;

    // Initialise some state blocks
    if( FAILED( hr = InitStateBlocks() ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("InitStateBlocks"), hr );

    // Set ourselves up for that rendering target
    if( FAILED( hr = OnSetRenderTarget( target, subrect ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("OnSetRenderTarget"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::DisplayShutdown()
{
    SAFE_RELEASE(m_pDevice);
    SAFE_RELEASE(m_pBackBuffer);
    SAFE_RELEASE(m_pD3D);
    SAFE_RELEASE(m_pDDraw);
    SAFE_RELEASE(m_pXContext);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CModule::OnSetRenderTarget( IDirectDrawSurface7* target, RECT* rect )
{
    HRESULT hr;

    // Figure out what our target and target rectangle are 
    DWORD   x,y;
    m_pBackBuffer = target ? target : m_pBackBuffer;
    DDSURFACEDESC2  desc = {sizeof(desc)};
    m_pBackBuffer->GetSurfaceDesc( &desc );
    if( rect == NULL )
    {
        m_dwWidth = desc.dwWidth;
        m_dwHeight = desc.dwHeight;
        x = 0;
        y = 0;
    }
    else
    {
        x = rect->left;
        y = rect->top;
        m_dwWidth = rect->right - x;
        m_dwHeight = rect->bottom - y;
    }
    m_dwEntireTargetWidth = desc.dwWidth;
    m_dwEntireTargetHeight = desc.dwHeight;

    // Set up viewport
    D3DVIEWPORT7    viewport;
    viewport.dwX = x;
    viewport.dwY = y;
    viewport.dwWidth = m_dwWidth;
    viewport.dwHeight = m_dwHeight;
    viewport.dvMinZ = 0.0f;
    viewport.dvMaxZ = 1.0f; 
    if( FAILED( hr = m_pDevice->SetViewport( &viewport ) ) )
        return hr;

    // Set a sensible projection matrix
    D3DXMATRIX  projection;
    D3DXMatrixPerspectiveFovLH( &projection, 1.5f, float(m_dwHeight)/float(m_dwWidth), 0.1f, 100.0f );
    return m_pDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, projection );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CModule::BeginFrame()
{
    m_pXContext->RestoreSurfaces();
    return m_pDevice->BeginScene();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::EndFrame()
{
    HRESULT hr;
    hr = m_pDevice->EndScene();
    if( FAILED(hr) )
        return;

    m_pXContext->UpdateFrame( 0 );

    m_pFrameRate->DoneFrame();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::RenderFrame( FLOAT fElapsed )
{
    BeginFrame();
    Clear();
    DrawStats();
    EndFrame();
}   




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::Clear( DWORD flags )
{
    // Kludge around GeForce driver bug, have to clear entire render target, so save old
    // viewport, set viewport to entire surface, clear, then put old viewport back

    D3DVIEWPORT7    oldviewport;
    D3DVIEWPORT7    kludgeviewport;
    m_pDevice->GetViewport( &oldviewport );
    kludgeviewport.dwX = kludgeviewport.dwY = 0;
    kludgeviewport.dwWidth = m_dwEntireTargetWidth;
    kludgeviewport.dwHeight = m_dwEntireTargetHeight;
    kludgeviewport.dvMinZ = oldviewport.dvMinZ;
    kludgeviewport.dvMaxZ = oldviewport.dvMaxZ;
    m_pDevice->SetViewport( &kludgeviewport );
    m_pXContext->Clear( flags );
    m_pDevice->SetViewport( &oldviewport );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::OnPaint()
{
    if( m_pXContext && (m_dwFlags & MOD_DISP_WINDOWED) )
    {
        m_pXContext->RestoreSurfaces();
        m_pXContext->UpdateFrame( 0 );
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL    CModule::OkayToProceed()
{
    if( m_pDevice == NULL )
        return FALSE;

    if( FAILED(m_pDDraw->TestCooperativeLevel() ) )
    {
        m_bNeedToRestore = TRUE;
    }

    if( m_bNeedToRestore )
    {
        m_pDDraw->RestoreAllSurfaces();
        RestoreSurfaceContent();
        m_bNeedToRestore = FALSE;
    }

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::RestoreSurfaceContent()
{
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::DrawStats()
{
    static TCHAR tstrText[256];
    wsprintf( tstrText, TEXT("[%dx%d %dbpp] %.1ffps"), m_dwWidth, m_dwHeight, m_dwBPP, m_pFrameRate->GetRate() );

    static CHAR strText[256];
    DXUtil_ConvertGenericStringToAnsi( strText, tstrText );
    m_pXContext->DrawDebugText( 0, 0, 0xffff33, strText );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CModule::InitStateBlocks()
{
    HRESULT hr;

    // Build "modulate texture" state block
    m_pDevice->BeginStateBlock();
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    if( FAILED( hr = m_pDevice->EndStateBlock( &m_pStateBlocks->ModulateTexture ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("EndStateBlock"), hr );

    // Build "copy texture" state block
    m_pDevice->BeginStateBlock();
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    if( FAILED( hr = m_pDevice->EndStateBlock( &m_pStateBlocks->CopyTexture ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("EndStateBlock"), hr );

    // Build "copy colour" state block
    m_pDevice->BeginStateBlock();
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
    if( FAILED( hr = m_pDevice->EndStateBlock( &m_pStateBlocks->CopyColour ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("EndStateBlock"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CModule::OnChar( TCHAR ch ) {}
void CModule::OnMouseMove( DWORD keys, DWORD x, DWORD y ) {};
void CModule::OnLButtonDown( DWORD keys, DWORD x, DWORD y ) {};
void CModule::OnLButtonUp( DWORD keys, DWORD x, DWORD y ) {}
void CModule::OnRButtonDown( DWORD keys, DWORD x, DWORD y ) {};
void CModule::OnRButtonUp( DWORD keys, DWORD x, DWORD y ) {}
