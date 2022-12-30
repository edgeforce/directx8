//-----------------------------------------------------------------------------
// File: CubeMap.cpp
//
// Desc: Example code showing how to do environment cube-mapping.
//
// Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <D3DX8.h>
#include "D3DApp.h"
#include "D3DFile.h"
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DXUtil.h"




//-----------------------------------------------------------------------------
// Name: struct ENVMAPPEDVERTEX
// Desc: D3D vertex type for environment-mapped objects
//-----------------------------------------------------------------------------
struct ENVMAPPEDVERTEX
{
    D3DXVECTOR3 p; // Position
    D3DXVECTOR3 n; // Normal
};

#define D3DFVF_ENVMAPVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL)

// CUBEMAP_RESOLUTION indicates how big to make the cubemap texture.  Larger
// textures will generate a better-looking reflection.
#define CUBEMAP_RESOLUTION 256



//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    CD3DFont*     m_pFont;

    LPDIRECT3DCUBETEXTURE8 m_pCubeMap;

    CD3DMesh*     m_pShinyTeapot;
    CD3DMesh*     m_pSkyBox;
    CD3DMesh*     m_pAirplane;
    D3DXMATRIX    m_matAirplane;
    IDirect3DSurface8* m_pCubeFaceZBuffer;

    HRESULT RenderScene( BOOL bRenderTeapot );
    HRESULT RenderSceneIntoCubeMap();

    HRESULT ConfirmDevice( D3DCAPS8*, DWORD, D3DFORMAT );

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
    m_strWindowTitle    = _T("CubeMap: Environment cube-mapping");
    m_bUseDepthBuffer   = TRUE;

    m_pFont             = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_pShinyTeapot      = new CD3DMesh();
    m_pSkyBox           = new CD3DMesh();
    m_pAirplane         = new CD3DMesh();
    m_pCubeMap          = NULL;
    m_pCubeFaceZBuffer  = NULL;
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
    // Animate file object
    D3DXMATRIX  mat1;
    D3DXMatrixScaling( &m_matAirplane, 0.2f, 0.2f, 0.2f );
    D3DXMatrixTranslation( &mat1, 0.0f, 2.0f, 0.0f );
    D3DXMatrixMultiply( &m_matAirplane, &m_matAirplane, &mat1 );
    D3DXMatrixRotationX( &mat1, -2.9f*m_fTime );
    D3DXMatrixMultiply( &m_matAirplane, &m_matAirplane, &mat1 );
    D3DXMatrixRotationY( &mat1, 1.055f*m_fTime );
    D3DXMatrixMultiply( &m_matAirplane, &m_matAirplane, &mat1 );

    // When the window has focus, let the mouse adjust the camera view
    if( GetFocus() )
    {
        D3DXMATRIX matTrackBall, matTrans, matView;
        D3DXQUATERNION quat = D3DUtil_GetRotationFromCursor( m_hWnd );
        D3DXMatrixRotationQuaternion( &matTrackBall, &quat );
        D3DXMatrixTranslation( &matTrans, 0.0f, 0.0f, 5.0f );
        D3DXMatrixMultiply( &matView, &matTrackBall, &matTrans );
        m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
    }

    // Render the scene into the surfaces of the cubemap
    if( FAILED( RenderSceneIntoCubeMap() ) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderSceneIntoCubeMap()
// Desc: Renders the scene to each of the 6 faces of the cube map
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderSceneIntoCubeMap()
{
    // Save transformation matrices of the device
    D3DXMATRIX   matProjSave, matViewSave;
    m_pd3dDevice->GetTransform( D3DTS_VIEW,       &matViewSave );
    m_pd3dDevice->GetTransform( D3DTS_PROJECTION, &matProjSave );

    // Set the projection matrix for a field of view of 90 degrees
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/2, 1.0f, 0.5f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // Get the current view matrix, to concat it with the cubemap view vectors
    D3DXMATRIX matViewDir;
    m_pd3dDevice->GetTransform( D3DTS_VIEW, &matViewDir );
    matViewDir._41 = 0.0f; matViewDir._42 = 0.0f; matViewDir._43 = 0.0f;

    // Store the current backbuffer and zbuffer
    LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
    m_pd3dDevice->GetRenderTarget( &pBackBuffer );
    m_pd3dDevice->GetDepthStencilSurface( &pZBuffer );

    // Render to the six faces of the cube map
    for( DWORD i=0; i<6; i++ )
    {
        // Set the view transform for this cubemap surface
        D3DXMATRIX matView;
        matView = D3DUtil_GetCubeMapViewMatrix( (D3DCUBEMAP_FACES)i );
        D3DXMatrixMultiply( &matView, &matViewDir, &matView );
        m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

        // Set the rendertarget to the i'th cubemap surface
        LPDIRECT3DSURFACE8 pCubeMapFace;
        m_pCubeMap->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pCubeMapFace );
        m_pd3dDevice->SetRenderTarget( pCubeMapFace, m_pCubeFaceZBuffer);
        pCubeMapFace->Release();

        // Render the scene (except for the teapot)
        m_pd3dDevice->BeginScene();
        RenderScene( FALSE );
        m_pd3dDevice->EndScene();
    }

    // Change the rendertarget back to the main backbuffer
    m_pd3dDevice->SetRenderTarget( pBackBuffer, pZBuffer );
    pBackBuffer->Release();
    pZBuffer->Release();

    // Restore the original transformation matrices
    m_pd3dDevice->SetTransform( D3DTS_VIEW,       &matViewSave );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProjSave );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Renders all visual elements in the scene. This is called by the main
//       Render() function, and also by the RenderIntoCubeMap() function.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderScene( BOOL bRenderTeapot )
{
    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER, 0x000000ff, 1.0f, 0L );

    // Set the texture stage states
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_MIRROR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_MIRROR );

    // Render the Skybox
    {
        // Save the current matrix set
        D3DXMATRIX matViewSave, matProjSave;
        m_pd3dDevice->GetTransform( D3DTS_VIEW,       &matViewSave );
        m_pd3dDevice->GetTransform( D3DTS_PROJECTION, &matProjSave );

        // Disable zbuffer, center view matrix, and set FOV to 90 degrees
        D3DXMATRIX matView = matViewSave;
        D3DXMATRIX matProj = matViewSave;
        matView._41 = matView._42 = matView._43 = 0.0f;
        D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/2, 1.0f, 0.5f, 10000.0f );
        m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
        m_pd3dDevice->SetTransform( D3DTS_VIEW,       &matView );
        m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,  FALSE );

        // Render the skybox
        D3DXMATRIX matWorld;
        D3DXMatrixIdentity( &matWorld );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
        m_pSkyBox->Render( m_pd3dDevice );

        // Restore the render states
        m_pd3dDevice->SetTransform( D3DTS_VIEW,       &matViewSave );
        m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProjSave );
        m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,  TRUE );
    }

    // Render the main file-based object
    {
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matAirplane );
        m_pAirplane->Render( m_pd3dDevice );
    }

    // Render the object with the environment-mapped body
    if( bRenderTeapot )
    {
        D3DXMATRIX matWorld;
        D3DXMatrixIdentity( &matWorld );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

        // Turn on texture-coord generation for cubemapping
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 );

        // Render the object with the environment-mapped body
        m_pd3dDevice->SetTexture( 0, m_pCubeMap );
        m_pShinyTeapot->Render( m_pd3dDevice );

        // Restore the render states
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
    }

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
    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Render the scene, including the teapot
        RenderScene( TRUE );

        // Output statistics
        m_pFont->DrawText( 2,  0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );
        m_pFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );

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
    // Load the file objects
    if( FAILED( m_pShinyTeapot->Create( m_pd3dDevice, _T("teapot.x") ) ) )
        return D3DAPPERR_MEDIANOTFOUND;
    if( FAILED( m_pSkyBox->Create( m_pd3dDevice, _T("lobby_skybox.x") ) ) )
        return D3DAPPERR_MEDIANOTFOUND;
    if( FAILED( m_pAirplane->Create( m_pd3dDevice, _T("airplane 2.x") ) ) )
        return D3DAPPERR_MEDIANOTFOUND;

    // Set mesh properties
    m_pAirplane->SetFVF( m_pd3dDevice, D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1 );
    m_pShinyTeapot->SetFVF( m_pd3dDevice, D3DFVF_ENVMAPVERTEX );
    m_pShinyTeapot->UseMeshMaterials( FALSE );

    // Restore the device-dependent objects
    m_pFont->InitDeviceObjects( m_pd3dDevice );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Restore device-memory objects and state after a device is created or
//       resized.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    HRESULT hr;

    // InitDeviceObjects for file objects (build textures and vertex buffers)
    m_pShinyTeapot->RestoreDeviceObjects( m_pd3dDevice );
    m_pSkyBox->RestoreDeviceObjects( m_pd3dDevice );
    m_pAirplane->RestoreDeviceObjects( m_pd3dDevice );
    m_pFont->RestoreDeviceObjects();

    // Create the cubemap, with a format that matches the backbuffer, since
    // we'll be rendering into it
    if( FAILED( hr = m_pd3dDevice->CreateCubeTexture( CUBEMAP_RESOLUTION, 1, D3DUSAGE_RENDERTARGET,
                                                      m_d3dsdBackBuffer.Format,
                                                      D3DPOOL_DEFAULT, &m_pCubeMap ) ) )
        return E_FAIL;

    // We create a separate Z buffer for the cube faces, because user could 
    // resize rendering window so that it is smaller than a cube face. In 
    // this case we cannot use the rendering window Z buffer for cube faces.
    if( FAILED( hr = m_pd3dDevice->CreateDepthStencilSurface(
                CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION, 
                D3DFMT_D16, D3DMULTISAMPLE_NONE, &m_pCubeFaceZBuffer) ) )
        return E_FAIL;

    // Set default render states
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,  TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT,  0x00aaaaaa );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

    // Set the transform matrices
    D3DXMATRIX matProj;
    FLOAT fAspect = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 0.5f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // Setup a material
    D3DMATERIAL8 mtrl;
    D3DUtil_InitMaterial( mtrl, 1.0f, 1.0f, 1.0f, 1.0f );
    m_pd3dDevice->SetMaterial( &mtrl );

    // Set up a light
    if( m_d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS )
    {
        D3DLIGHT8 light;
        D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, 0.0f, 0.0f, 1.0f );
        light.Ambient.r = 0.3f;
        light.Ambient.g = 0.3f;
        light.Ambient.b = 0.3f;
        m_pd3dDevice->SetLight( 0, &light );
        m_pd3dDevice->LightEnable( 0, TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the device-dependent objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    m_pShinyTeapot->InvalidateDeviceObjects();
    m_pSkyBox->InvalidateDeviceObjects();
    m_pAirplane->InvalidateDeviceObjects();
    m_pFont->InvalidateDeviceObjects();

    SAFE_RELEASE( m_pCubeMap );
    SAFE_RELEASE( m_pCubeFaceZBuffer );

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

    m_pShinyTeapot->Destroy();
    m_pSkyBox->Destroy();
    m_pAirplane->Destroy();

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
    SAFE_DELETE( m_pShinyTeapot );
    SAFE_DELETE( m_pSkyBox );
    SAFE_DELETE( m_pAirplane );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device intialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS8* pCaps, DWORD dwBehavior,
                                          D3DFORMAT Format )
{
    if( dwBehavior & D3DCREATE_PUREDEVICE )
        return E_FAIL; // GetTransform doesn't work on PUREDEVICE

    // Check for cubemapping devices
    if( 0 == ( pCaps->TextureCaps & D3DPTEXTURECAPS_CUBEMAP ) )
        return E_FAIL;

    // Check that we can create a cube texture that we can render into
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, 
        pCaps->DeviceType, Format, D3DUSAGE_RENDERTARGET,
        D3DRTYPE_CUBETEXTURE, Format ) ) )
    {
       return E_FAIL;
    }

    return S_OK;
}



