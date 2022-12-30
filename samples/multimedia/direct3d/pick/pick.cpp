//-----------------------------------------------------------------------------
// File: Pick.cpp
//
// Desc: Example code showing how to do picking in D3D.
//
// Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <math.h>
#include <stdio.h>
#include <D3DX8.h>
#include "D3DApp.h"
#include "D3DFont.h"
#include "D3DFile.h"
#include "D3DUtil.h"
#include "DXUtil.h"


struct D3DVERTEX
{
    D3DXVECTOR3 p;
    D3DXVECTOR3 n;
    FLOAT       tu, tv;
};

#define D3DFVF_VERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)




//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    CD3DFont*   m_pFont;                      // Font for drawing text
    CD3DMesh*   m_pObject;                    // Object to render
    DWORD       m_dwNumPickedTriangles;       // Picking information
    FLOAT       m_fPickT, m_fPickU, m_fPickV; // Picking results
    LPDIRECT3DVERTEXBUFFER8 m_pVB;            // VB for picked triangles
    LPDIRECT3DSURFACE8      m_pCursorBitmap;  // Cursor image

    // Internal member functions
    HRESULT Pick();
    BOOL    IntersectTriangle( const D3DXVECTOR3& orig, const D3DXVECTOR3& dir,
                               D3DXVECTOR3& v0, D3DXVECTOR3& v1, D3DXVECTOR3& v2,
                               FLOAT* t, FLOAT* u, FLOAT* v );
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
    LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
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
    m_strWindowTitle    = _T("Pick: D3D Picking Sample");
    m_bUseDepthBuffer   = TRUE;
    m_bShowCursorWhenFullscreen = TRUE;

    m_pFont           = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_pObject         = new CD3DMesh();
    m_pCursorBitmap   = NULL;
    m_pVB             = NULL;
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
// Name: Pick()
// Desc: Checks if mouse point hits geometry
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Pick()
{
    D3DXVECTOR3 vPickRayDir;
    D3DXVECTOR3 vPickRayOrig;

    m_dwNumPickedTriangles = 0L;

    // Get the pick ray from the mouse position
    if( GetCapture() )
    {
        D3DXMATRIX matProj;
        m_pd3dDevice->GetTransform( D3DTS_PROJECTION, &matProj );

        POINT ptCursor;
        GetCursorPos( &ptCursor );
        ScreenToClient( m_hWnd, &ptCursor );

        // Compute the vector of the pick ray in screen space
        D3DXVECTOR3 v;
        v.x =  ( ( ( 2.0f * ptCursor.x ) / m_d3dsdBackBuffer.Width  ) - 1 ) / matProj._11;
        v.y = -( ( ( 2.0f * ptCursor.y ) / m_d3dsdBackBuffer.Height ) - 1 ) / matProj._22;
        v.z =  1.0f;

        // Get the inverse view matrix
        D3DXMATRIX matView, m;
        m_pd3dDevice->GetTransform( D3DTS_VIEW, &matView );
        D3DXMatrixInverse( &m, NULL, &matView );

        // Transform the screen space pick ray into 3D space
        vPickRayDir.x  = v.x*m._11 + v.y*m._21 + v.z*m._31;
        vPickRayDir.y  = v.x*m._12 + v.y*m._22 + v.z*m._32;
        vPickRayDir.z  = v.x*m._13 + v.y*m._23 + v.z*m._33;
        vPickRayOrig.x = m._41;
        vPickRayOrig.y = m._42;
        vPickRayOrig.z = m._43;
    }

    // Get the picked triangle
    if( GetCapture() )
    {
        LPDIRECT3DVERTEXBUFFER8 pVB;
        LPDIRECT3DINDEXBUFFER8 pIB;
        m_pObject->GetLocalMesh()->GetVertexBuffer( &pVB );
        m_pObject->GetLocalMesh()->GetIndexBuffer( &pIB );

        struct VERTEX { D3DXVECTOR3 p, n; FLOAT tu; FLOAT tv; };
        DWORD dwFVF = m_pObject->GetLocalMesh()->GetFVF();

        WORD*      pIndices;
        VERTEX*    pVertices;
        DWORD      dwNumFaces = m_pObject->GetLocalMesh()->GetNumFaces();
        pIB->Lock( 0,0,(BYTE**)&pIndices, 0 );
        pVB->Lock( 0,0,(BYTE**)&pVertices, 0 );

        for( DWORD i=0; i<dwNumFaces; i++ )
        {
            D3DXVECTOR3 v0 = pVertices[pIndices[3*i+0]].p;
            D3DXVECTOR3 v1 = pVertices[pIndices[3*i+1]].p;
            D3DXVECTOR3 v2 = pVertices[pIndices[3*i+2]].p;

            // Check if the pick ray passes through this point
            if( IntersectTriangle( vPickRayOrig, vPickRayDir, v0, v1, v2,
                                   &m_fPickT, &m_fPickU, &m_fPickV ) )
            {
                VERTEX* v;
                m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );
                v[3*m_dwNumPickedTriangles+0] = pVertices[pIndices[3*i+0]];
                v[3*m_dwNumPickedTriangles+1] = pVertices[pIndices[3*i+1]];
                v[3*m_dwNumPickedTriangles+2] = pVertices[pIndices[3*i+2]];
                m_pVB->Unlock();

                m_dwNumPickedTriangles++;
            }
        }

        pVB->Unlock();
        pIB->Unlock();

        pVB->Release();
        pIB->Release();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Rotate the camera about the y-axis
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    vFromPt.x = -cosf(m_fTime/3.0f) * 4.0f;
    vFromPt.y = 1.0f;
    vFromPt.z =  sinf(m_fTime/3.0f) * 4.0f;

    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &vFromPt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

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
    // Set up the cursor
    POINT ptCursor;
    GetCursorPos( &ptCursor );
    ScreenToClient( m_hWnd, &ptCursor );
    m_pd3dDevice->SetCursorPosition( ptCursor.x, ptCursor.y, 0L );

    // Check for picked triangles
    Pick();

    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         0x000000ff, 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Set render mode to lit, solid triangles
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

        // If a triangle is picked, draw it
        if( m_dwNumPickedTriangles )
        {
            // Draw the picked triangle
            m_pd3dDevice->SetVertexShader( D3DFVF_VERTEX );
            m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(D3DVERTEX) );
            m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
                                         0, m_dwNumPickedTriangles );

            // Set render mode to unlit, wireframe triangles
            m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
            m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
        }

        m_pObject->Render( m_pd3dDevice );

        // Output statistics
        m_pFont->DrawText( 2,  0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );
        m_pFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );

        // Output text
        CHAR strBuffer[90];
        if( m_dwNumPickedTriangles )
            sprintf( strBuffer, _T("t=%2.2f, u=%2.2f, v=%2.2f"),
                                m_fPickT, m_fPickU, m_fPickV );
        else
            sprintf( strBuffer, _T("Use mouse to pick a polygon") );
        m_pFont->DrawText( 2, 40, D3DCOLOR_ARGB(255,255,255,0), strBuffer );

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
    // Restore the font
    m_pFont->InitDeviceObjects( m_pd3dDevice );

    if( FAILED( m_pObject->Create( m_pd3dDevice, _T("Tiger.x") ) ) )
        return D3DAPPERR_MEDIANOTFOUND;
    m_pObject->SetFVF( m_pd3dDevice, D3DFVF_VERTEX );

    // Create and fill the vertex buffer
    DWORD dwNumVertices = m_pObject->GetSysMemMesh()->GetNumVertices();
    if( FAILED( m_pd3dDevice->CreateVertexBuffer( dwNumVertices*sizeof(D3DVERTEX),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_VERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB ) ) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    m_pFont->RestoreDeviceObjects();

    // Restore device-memory objects for the mesh
    m_pObject->RestoreDeviceObjects( m_pd3dDevice );

    // Set up the textures
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

    // Set miscellaneous render states
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE,   FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,        TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT,        0x00444444 );

    // Set the world matrix
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );
    m_pd3dDevice->SetTransform( D3DTS_WORLD,  &matIdentity );

    // Set the projection matrix
    D3DXMATRIX matProj;
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // Setup a material
    D3DMATERIAL8 mtrl;
    D3DUtil_InitMaterial( mtrl, 1.0f, 1.0f, 1.0f, 1.0f );
    m_pd3dDevice->SetMaterial( &mtrl );

    // Set up lighting states
    D3DLIGHT8 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, 0.1f, -1.0f, 0.1f );
    m_pd3dDevice->SetLight( 0, &light );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    m_pFont->InvalidateDeviceObjects();
    m_pObject->InvalidateDeviceObjects();

    SAFE_RELEASE( m_pCursorBitmap );

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
    m_pObject->Destroy();

    SAFE_RELEASE( m_pVB );
    SAFE_RELEASE( m_pCursorBitmap );

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

    // If this is a TnL device, make sure it supports directional lights
    if( (dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING ) ||
        (dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING ) )
    {
        if( !(pCaps->VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS ) )
            return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Overrrides the main WndProc, so the sample can do custom message
//       handling (e.g. processing mouse, keyboard, or menu commands).
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch( msg )
    {
        case WM_LBUTTONDOWN:
            // User pressed left mouse button
            SetCapture( hWnd );
            break;

        case WM_LBUTTONUP:
            // The user released the left mouse button
            ReleaseCapture();
            break;
    }

    return CD3DApplication::MsgProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: IntersectTriangle()
// Desc: Given a ray origin (orig) and direction (dir), and three vertices of
//       of a triangle, this function returns TRUE and the interpolated texture
//       coordinates if the ray intersects the triangle
//-----------------------------------------------------------------------------
BOOL CMyD3DApplication::IntersectTriangle( const D3DXVECTOR3& orig,
                                       const D3DXVECTOR3& dir, D3DXVECTOR3& v0,
                                       D3DXVECTOR3& v1, D3DXVECTOR3& v2,
                                       FLOAT* t, FLOAT* u, FLOAT* v )
{
    // Find vectors for two edges sharing vert0
    D3DXVECTOR3 edge1 = v1 - v0;
    D3DXVECTOR3 edge2 = v2 - v0;

    // Begin calculating determinant - also used to calculate U parameter
    D3DXVECTOR3 pvec;
    D3DXVec3Cross( &pvec, &dir, &edge2 );

    // If determinant is near zero, ray lies in plane of triangle
    FLOAT det = D3DXVec3Dot( &edge1, &pvec );
    if( det < 0.0001f )
        return FALSE;

    // Calculate distance from vert0 to ray origin
    D3DXVECTOR3 tvec = orig - v0;

    // Calculate U parameter and test bounds
    *u = D3DXVec3Dot( &tvec, &pvec );
    if( *u < 0.0f || *u > det )
        return FALSE;

    // Prepare to test V parameter
    D3DXVECTOR3 qvec;
    D3DXVec3Cross( &qvec, &tvec, &edge1 );

    // Calculate V parameter and test bounds
    *v = D3DXVec3Dot( &dir, &qvec );
    if( *v < 0.0f || *u + *v > det )
        return FALSE;

    // Calculate t, scale parameters, ray intersects triangle
    *t = D3DXVec3Dot( &edge2, &qvec );
    FLOAT fInvDet = 1.0f / det;
    *t *= fInvDet;
    *u *= fInvDet;
    *v *= fInvDet;

    return TRUE;
}



