//-----------------------------------------------------------------------------
// File: ProgressiveMesh.cpp
//
// Desc: Sample of creating progressive meshes in D3D
//
// Copyright (c) 1998-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <D3DX8.h>
#include "D3DApp.h"
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DXUtil.h"
#include "resource.h"




//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//       from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    TCHAR               m_strInitialDir[512];
    TCHAR               m_strMeshFilename[512]; // Filename of mesh
    LPD3DXPMESH         m_pPMeshSysMem;         // Sysmem version of pmesh, lives through resize's
    LPD3DXPMESH         m_pPMesh;          // Local version of pmesh, rebuilt on resize

    D3DMATERIAL8*       m_mtrlMeshMaterials;
    LPDIRECT3DTEXTURE8* m_pMeshTextures;        // Array of textures, entries are NULL if no texture specified
    DWORD               m_dwNumMaterials;       // Number of materials
    LPD3DXBUFFER        m_pAdjacencyBuffer;     // Contains the adjaceny info loaded with the mesh

    CD3DArcBall         m_ArcBall;              // Mouse rotation utility
    D3DXVECTOR3         m_vObjectCenter;        // Center of bounding sphere of object
    FLOAT               m_fObjectRadius;        // Radius of bounding sphere of object

    CD3DFont*           m_pFont;                // Font for displaying help
    BOOL                m_bDisplayHelp;

public:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT FinalCleanup();
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
// Desc: Constructor
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    // Override base class members
    m_strWindowTitle    = _T("ProgressiveMesh: Using Progressive Meshes in D3D");
    m_bUseDepthBuffer   = TRUE;
    m_bShowCursorWhenFullscreen = TRUE;

    // Initialize member variables
    _tcscpy( m_strInitialDir, DXUtil_GetDXSDKMediaPath() );
    _tcscpy( m_strMeshFilename, _T("Tiger.x") );

    m_pPMesh             = NULL;
    m_pAdjacencyBuffer   = NULL;

    m_dwNumMaterials     = 0L;
    m_mtrlMeshMaterials  = NULL;
    m_pMeshTextures      = NULL;

    m_pFont              = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_bDisplayHelp       = FALSE;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // Set cursor to indicate that user can move the object with the mouse
#ifdef _WIN64
    SetClassLongPtr( m_hWnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor( NULL, IDC_SIZEALL ) );
#else
    SetClassLong( m_hWnd, GCL_HCURSOR, (LONG)LoadCursor( NULL, IDC_SIZEALL ) );
#endif
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Setup world matrix
    D3DXMATRIX matWorld;
    D3DXMATRIX matRotationInverse;
    D3DXMatrixTranslation( &matWorld, -m_vObjectCenter.x,
                                      -m_vObjectCenter.y,
                                      -m_vObjectCenter.z );
    D3DXMatrixInverse( &matRotationInverse, NULL, m_ArcBall.GetRotationMatrix() );
    D3DXMatrixMultiply( &matWorld, &matWorld, &matRotationInverse );
    D3DXMatrixMultiply( &matWorld, &matWorld, m_ArcBall.GetTranslationMatrix() );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up view matrix
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &D3DXVECTOR3( 0, 0,-3*m_fObjectRadius ),
                                  &D3DXVECTOR3( 0, 0, 0 ),
                                  &D3DXVECTOR3( 0, 1, 0 ) );
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
    // Clear the backbuffer
    m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         0x000000ff, 1.0f, 0x00000000 );

    // Begin scene rendering
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        if( m_pPMesh )
        {
            // Set and draw each of the materials in the mesh
            for( DWORD i=0; i<m_dwNumMaterials; i++ )
            {
                m_pd3dDevice->SetMaterial( &m_mtrlMeshMaterials[i] );
                m_pd3dDevice->SetTexture( 0, m_pMeshTextures[i] );
                m_pPMesh->DrawSubset( i );
            }
        }


        // Output statistics
        m_pFont->DrawText( 2,  0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );
        m_pFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );
        TCHAR strBuffer[512];
        sprintf( strBuffer, _T("Num Vertices = %ld"),
                 m_pPMesh ? m_pPMesh->GetNumVertices() : 0L );
        m_pFont->DrawText( 2, 40, 0xffffff00, strBuffer );

        // Output text
        if( m_bDisplayHelp )
        {
            m_pFont->DrawText(  2, 60, 0xffffffff, _T("<F1>\n\n")
                                                   _T("<Up>\n")
                                                   _T("<Down>\n\n")
                                                   _T("<PgUp>\n")
                                                   _T("<PgDn>\n\n")
                                                   _T("<Home>\n")
                                                   _T("<End>"), 0L );
            m_pFont->DrawText( 70, 60, 0xffffffff, _T("Toggle help\n\n")
                                                   _T("Add one vertex\n")
                                                   _T("Subtract one vertex\n\n")
                                                   _T("Add 100 vertices\n")
                                                   _T("Subtract 100 vertices\n\n")
                                                   _T("Max vertices\n")
                                                   _T("Min vertices") );
        }
        else
        {
            m_pFont->DrawText(  2, 60, 0xffffffff, _T("<F1>\n") );
            m_pFont->DrawText( 70, 60, 0xffffffff, _T("Toggle help\n") );
        }

        // End the scene rendering
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
    // Initialize the font
    m_pFont->InitDeviceObjects( m_pd3dDevice );

    // Load mesh
    LPDIRECT3DVERTEXBUFFER8 pVertexBuffer = NULL;
    LPD3DXMESH   pMesh;
    LPD3DXMESH   pTempMesh;
    LPD3DXBUFFER pD3DXMtrlBuffer;
    BYTE*        pVertices;
    TCHAR        strMediaPath[512];
    HRESULT      hr;

    // Find the path to the mesh
    if( FAILED( DXUtil_FindMediaFile( strMediaPath, m_strMeshFilename ) ) )
        return D3DAPPERR_MEDIANOTFOUND;

    // Load the mesh from the specified file
    if( FAILED( hr = D3DXLoadMeshFromX( strMediaPath, D3DXMESH_MANAGED, m_pd3dDevice,
                                        &m_pAdjacencyBuffer, &pD3DXMtrlBuffer,
                                        &m_dwNumMaterials, &pMesh ) ) )
        return hr;

    if( FAILED( hr = D3DXValidMesh( pMesh, (DWORD*)m_pAdjacencyBuffer->GetBufferPointer() ) ) )
    {
        SAFE_RELEASE( m_pAdjacencyBuffer );
        SAFE_RELEASE( pD3DXMtrlBuffer );
        SAFE_RELEASE( pMesh );
        m_dwNumMaterials = 0;
        return E_FAIL;
    }

    // Allocate a material/texture arrays
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    m_mtrlMeshMaterials = new D3DMATERIAL8[m_dwNumMaterials];
    m_pMeshTextures     = new LPDIRECT3DTEXTURE8[m_dwNumMaterials];

    // Copy the materials and load the textures
    for( DWORD i=0; i<m_dwNumMaterials; i++ )
    {
        m_mtrlMeshMaterials[i] = d3dxMaterials[i].MatD3D;
        m_mtrlMeshMaterials[i].Ambient = m_mtrlMeshMaterials[i].Diffuse;

        // Find the path to the texture and create that texture
        DXUtil_FindMediaFile( strMediaPath, d3dxMaterials[i].pTextureFilename );
        if( FAILED( D3DXCreateTextureFromFile( m_pd3dDevice, strMediaPath, 
                                               &m_pMeshTextures[i] ) ) )
            m_pMeshTextures[i] = NULL;
    }
    pD3DXMtrlBuffer->Release();

    // Lock the vertex buffer, to generate a simple bounding sphere
    hr = pMesh->GetVertexBuffer( &pVertexBuffer );
    if( FAILED(hr) )
    {
        pMesh->Release();
        return hr;
    }

    hr = pVertexBuffer->Lock( 0, 0, &pVertices, D3DLOCK_NOSYSLOCK );
    if( FAILED(hr) )
    {
        pVertexBuffer->Release();
        pMesh->Release();
        return hr;
    }

    hr = D3DXComputeBoundingSphere( pVertices, pMesh->GetNumVertices(),
                                    pMesh->GetFVF(),
                                    &m_vObjectCenter, &m_fObjectRadius );
    pVertexBuffer->Unlock();
    pVertexBuffer->Release();

    if( FAILED(hr) || m_dwNumMaterials == 0 )
    {
        pMesh->Release();
        return hr;
    }

    if (pMesh->GetFVF() != (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1))
    {
        hr = pMesh->CloneMeshFVF( D3DXMESH_SYSTEMMEM, D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1, 
                                            m_pd3dDevice, &pTempMesh );
        if (FAILED(hr))
            return hr;

        if ( !(pMesh->GetFVF() & D3DFVF_NORMAL))
        {
            D3DXComputeNormals( pTempMesh );
        }

        pMesh->Release();
        pMesh = pTempMesh;
    }

    hr = D3DXGeneratePMesh( pMesh, (DWORD*)m_pAdjacencyBuffer->GetBufferPointer(),
                            NULL, NULL, 1, D3DXMESHSIMP_VERTEX, &m_pPMesh);
    pMesh->Release();
    if( FAILED(hr) )
        return hr;

    m_pPMesh->SetNumVertices( 0xffffffff );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    m_pFont->RestoreDeviceObjects();

    // Setup render state
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING,     TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

    // Setup the light
    D3DLIGHT8 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, 0.0f, -0.5f, 1.0f );
    m_pd3dDevice->SetLight( 0, &light );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00333333 );

    // Set the arcball parameters
    m_ArcBall.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height,
                         0.85f );
    m_ArcBall.SetRadius( m_fObjectRadius );

    // Set the projection matrix
    D3DXMATRIX matProj;
    FLOAT      fAspect = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, m_fObjectRadius/64.0f,
                                m_fObjectRadius*200.0f);
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    m_pFont->InvalidateDeviceObjects();

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

    if( m_pMeshTextures != NULL)
    {
        for( UINT i=0; i<m_dwNumMaterials; i++ )
            SAFE_RELEASE( m_pMeshTextures[i] );
    }
    SAFE_DELETE_ARRAY( m_pMeshTextures );
    SAFE_RELEASE( m_pPMesh );
    SAFE_RELEASE( m_pAdjacencyBuffer );
    SAFE_DELETE_ARRAY( m_mtrlMeshMaterials );
    m_dwNumMaterials = 0L;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    SAFE_DELETE( m_pFont );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    // Pass mouse messages to the ArcBall so it can build internal matrices
    m_ArcBall.HandleMouseMessages( hWnd, uMsg, wParam, lParam );

    // Trap the context menu
    if( uMsg == WM_CONTEXTMENU )
        return 0;

    if( uMsg == WM_COMMAND )
    {
        // Handle the about command (which displays help)
        if( LOWORD(wParam) == IDM_TOGGLEHELP )
        {
            m_bDisplayHelp = !m_bDisplayHelp;
            return 0;
        }

        // Handle the open file command
        if( m_bWindowed && LOWORD(wParam) == IDM_OPENFILE )
        {
            TCHAR g_strFilename[512]   = _T("");

            // Display the OpenFileName dialog. Then, try to load the specified file
            OPENFILENAME ofn = { sizeof(OPENFILENAME), NULL, NULL,
                                _T(".X Files (.x)\0*.x\0\0"), 
                                NULL, 0, 1, m_strMeshFilename, 512, g_strFilename, 512, 
                                m_strInitialDir, _T("Open Mesh File"), 
                                OFN_FILEMUSTEXIST, 0, 1, NULL, 0, NULL, NULL };

            if( TRUE == GetOpenFileName( &ofn ) )
            {
                _tcscpy( m_strInitialDir, m_strMeshFilename );
                TCHAR* pLastSlash =  _tcsrchr( m_strInitialDir, _T('\\') );
                if( pLastSlash )
                    *pLastSlash = 0;
                SetCurrentDirectory( m_strInitialDir );

                // Destroy and recreate everything
                InvalidateDeviceObjects();
                DeleteDeviceObjects();
                
                if( FAILED( InitDeviceObjects() ) )
                {
                    MessageBox( m_hWnd, _T("Error loading mesh: mesh may not\n")
                                        _T("be valid. See debug output for\n")
                                        _T("more information.\n\nPlease select ") 
                                        _T("a different .x file."), 
                                        _T("ProgressiveMesh"), MB_ICONERROR|MB_OK );
                }

                RestoreDeviceObjects();
            }
        }
    }

    if( uMsg == WM_KEYDOWN )
    {
        if( m_pPMesh )
        {
            DWORD dwNumMeshVertices = m_pPMesh->GetNumVertices();

            if( wParam == VK_UP )
            {
                // Sometimes it is necessary to add more than one
                // vertex when increasing the resolution of a 
                // progressive mesh, so keep adding until the 
                // vertex count increases.
                for( int i = 1; i <= 8; i++ )
                {
                    m_pPMesh->SetNumVertices( dwNumMeshVertices+i );
                    if( m_pPMesh->GetNumVertices() == dwNumMeshVertices+i )
                        break;
                }
            }
            else if( wParam == VK_DOWN )
                m_pPMesh->SetNumVertices( dwNumMeshVertices-1 );
            else if( wParam == VK_PRIOR )
                m_pPMesh->SetNumVertices( dwNumMeshVertices+100 );
            else if( wParam == VK_NEXT )
                m_pPMesh->SetNumVertices( dwNumMeshVertices<=100 ? 1 : dwNumMeshVertices-100 );
            else if( wParam == VK_HOME )
                m_pPMesh->SetNumVertices( 0xffffffff );
            else if( wParam == VK_END )
                m_pPMesh->SetNumVertices( 1 );
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



