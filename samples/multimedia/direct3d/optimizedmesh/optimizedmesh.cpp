//-----------------------------------------------------------------------------
// File: OptimizedMesh.cpp
//
// Desc: Sample of optimizing meshes in D3D
//
// Copyright (c) 1998-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <d3dx8.h>
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
    TCHAR               m_strMeshFilename[512];
    TCHAR               m_strInitialDir[512];
    
    LPD3DXMESH          m_pMesh;              // Local version of mesh, rebuilt on resize
    LPD3DXMESH          m_pMeshOptimized;     // Optimized vid mem mesh, rebuilt on resize

    DWORD               m_dwNumMaterials;     // Number of materials
    LPDIRECT3DTEXTURE8* m_pMeshTextures;
    D3DMATERIAL8*       m_pMeshMaterials;

    BOOL                m_bShowOptimizedMesh; // Which mesh is currently being drawn
    LPD3DXBUFFER        m_pAdjacencyBuffer;   // Contains the adjaceny info loaded with the mesh

    CD3DFont*           m_pFont;              // Font for outputting frame stats

    CD3DArcBall         m_ArcBall;            // Mouse rotation utility
    D3DXVECTOR3         m_vObjectCenter;      // Center of bounding sphere of object
    FLOAT               m_fObjectRadius;      // Radius of bounding sphere of object

    D3DXMATRIX          m_matWorld;
    DWORD               m_cObjectsPerSide;    // sqrt of the number of objects to draw

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
    m_strWindowTitle     = _T("OptimizedMesh: Optimizing Meshes in D3D");
    m_bUseDepthBuffer    = TRUE;
    m_bShowCursorWhenFullscreen = TRUE;

    // Initialize member variables
    m_pMesh         = NULL;
    m_pMeshOptimized     = NULL;
    m_bShowOptimizedMesh = TRUE;
    m_pAdjacencyBuffer   = NULL;

    m_dwNumMaterials     = 0L;
    m_pMeshMaterials     = NULL;

    m_pFont              = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    
    _tcscpy( m_strInitialDir, DXUtil_GetDXSDKMediaPath() );
    _tcscpy( m_strMeshFilename, _T("knot.x") );

    m_cObjectsPerSide = 1;
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
    // Setup viewing postion from ArcBall
    D3DXMATRIX matRotationInverse;
    D3DXMATRIX matTemp;
    D3DXMatrixTranslation( &m_matWorld, -m_vObjectCenter.x,
                                      -m_vObjectCenter.y,
                                      -m_vObjectCenter.z );
    D3DXMatrixInverse( &matRotationInverse, NULL, m_ArcBall.GetRotationMatrix() );
    D3DXMatrixMultiply( &m_matWorld, &m_matWorld, &matRotationInverse );
    D3DXMatrixMultiply( &m_matWorld, &m_matWorld, m_ArcBall.GetTranslationMatrix() );

    D3DXMatrixTranslation( &matTemp, -m_fObjectRadius  * (m_cObjectsPerSide-1),//* 0.5f,
                                      -m_fObjectRadius * (m_cObjectsPerSide-1),//* 0.5f,
                                      0 );
    D3DXMatrixMultiply( &m_matWorld, &m_matWorld, &matTemp );

    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &D3DXVECTOR3( 0, 0,-3.7f*m_fObjectRadius * m_cObjectsPerSide),
                                  &D3DXVECTOR3( 0, 0, 0 ),
                                  &D3DXVECTOR3( 0, 1, 0 ) );
    m_pd3dDevice->SetTransform( D3DTS_VIEW,  &matView );

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
    DWORD xOffset;
    DWORD yOffset;
    D3DXMATRIX matWorld;
    D3DXMATRIX matTemp;
    DWORD cTriangles;
    FLOAT fTrisPerSec;
    TCHAR strInfo[120];

    // Clear the scene
    m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         0x000000ff, 1.0f, 0x00000000 );

    // Draw scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        for (xOffset = 0; xOffset < m_cObjectsPerSide; xOffset++)
        {
            for (yOffset = 0; yOffset < m_cObjectsPerSide; yOffset++)
            {
                D3DXMatrixTranslation( &matTemp, m_fObjectRadius * xOffset * 2,
                                                  m_fObjectRadius * yOffset * 2,
                                                  0 );
                D3DXMatrixMultiply( &matWorld, &m_matWorld, &matTemp );
                m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );


                // Set and draw each of the materials in the mesh
                for( DWORD i=0; i<m_dwNumMaterials; i++ )
                {
                    m_pd3dDevice->SetMaterial( &m_pMeshMaterials[i] );
                    m_pd3dDevice->SetTexture( 0, m_pMeshTextures[i] );

                    // Draw either the optimized or unoptimized mesh
                    if( m_bShowOptimizedMesh )
                        m_pMeshOptimized->DrawSubset( i );
                    else
                        m_pMesh->DrawSubset( i );
                }
            }
        }

        // Calculate and show triangles per sec, a reasonable throughput number
        cTriangles = m_pMesh->GetNumFaces() * m_cObjectsPerSide * m_cObjectsPerSide;
        fTrisPerSec = m_fFPS * cTriangles;

        // Output statistics
        wsprintf( strInfo, _T("%s, %ld tris per sec, %ld triangles"),
                  m_bShowOptimizedMesh ? _T("Optimized") : _T("Non-optimized"),
                  (DWORD)fTrisPerSec,
                  cTriangles);

        m_pFont->DrawText( 2, 0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );
        m_pFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );
        m_pFont->DrawText( 2, 40, D3DCOLOR_ARGB(255,255,255,0), strInfo);


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
    LPDIRECT3DVERTEXBUFFER8 pMeshVB   = NULL;
    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;
    BYTE*        pVertices;
    TCHAR        strMesh[512];
    HRESULT      hr = S_OK;
    BOOL         bNormalsInFile;
    LPD3DXMESH   pMeshTemp;
    LPD3DXMESH   pMeshSysMem = NULL;
    DWORD        *rgdwAdjacencyTemp = NULL;
    DWORD        i;
    D3DXMATERIAL* d3dxMaterials;
    DWORD        dw32Bit;

    // Get a path to the media file
    DXUtil_FindMediaFile( strMesh, m_strMeshFilename );
    
    // Load the mesh from the specified file
    hr = D3DXLoadMeshFromX( strMesh, D3DXMESH_SYSTEMMEM, m_pd3dDevice, 
                            &m_pAdjacencyBuffer, &pD3DXMtrlBuffer, 
                            &m_dwNumMaterials, &pMeshSysMem );
    if( FAILED(hr) )
        goto End;

    // remember if the mesh is 32 or 16 bit, to be added in on the clones
    dw32Bit = pMeshSysMem->GetOptions() & D3DXMESH_32BIT;

    // Get the array of materials out of the returned buffer, and allocate a texture array
    d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    m_pMeshMaterials = new D3DMATERIAL8[m_dwNumMaterials];
    m_pMeshTextures  = new LPDIRECT3DTEXTURE8[m_dwNumMaterials];

    for( i=0; i<m_dwNumMaterials; i++ )
    {
        m_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;
        m_pMeshMaterials[i].Ambient = m_pMeshMaterials[i].Diffuse;
        m_pMeshTextures[i]  = NULL;

        // Get a path to the texture
        TCHAR strPath[512];
        DXUtil_FindMediaFile( strPath, d3dxMaterials[i].pTextureFilename );

        // Load the texture
        D3DXCreateTextureFromFile( m_pd3dDevice, strPath, &m_pMeshTextures[i] );
    }

    // Done with the material buffer
    SAFE_RELEASE( pD3DXMtrlBuffer );

    // Lock the vertex buffer, to generate a simple bounding sphere
    hr = pMeshSysMem->GetVertexBuffer( &pMeshVB );
    if( SUCCEEDED(hr) )
    {
        hr = pMeshVB->Lock( 0, 0, &pVertices, D3DLOCK_NOSYSLOCK );
        if( SUCCEEDED(hr) )
        {
            hr = D3DXComputeBoundingSphere( pVertices, pMeshSysMem->GetNumVertices(),
                                            pMeshSysMem->GetFVF(),
                                            &m_vObjectCenter, &m_fObjectRadius );
            pMeshVB->Unlock();
        }
        pMeshVB->Release();
    }
    if( FAILED(hr) )
        goto End;

    // Initialize the font 
    m_pFont->InitDeviceObjects( m_pd3dDevice );

    // remember if there were normals in the file, before possible clone operation
    bNormalsInFile = pMeshSysMem->GetFVF() & D3DFVF_NORMAL;

    // force 32 byte vertices
    if (pMeshSysMem->GetFVF() != (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1))
    {
        hr = pMeshSysMem->CloneMeshFVF( pMeshSysMem->GetOptions(), D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1, 
                                          m_pd3dDevice, &pMeshTemp );
        if( FAILED(hr) )
            goto End;

        pMeshSysMem->Release();
        pMeshSysMem = pMeshTemp;
    }


    // Compute normals for the mesh, if not present
    if (!bNormalsInFile)
    {
        D3DXComputeNormals( pMeshSysMem );
    }

    // allocate a second adjacency buffer to store post attribute sorted adjacency
    rgdwAdjacencyTemp = new DWORD[pMeshSysMem->GetNumFaces() * 3];
    if (rgdwAdjacencyTemp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    // attribute sort - the un-optimized mesh option
    //          remember the adjacency for the vertex cache optimization
    hr = pMeshSysMem->OptimizeInplace( D3DXMESHOPT_COMPACT|D3DXMESHOPT_ATTRSORT,
                                 (DWORD*)m_pAdjacencyBuffer->GetBufferPointer(),
                                 rgdwAdjacencyTemp, NULL, NULL);
    if( FAILED(hr) )
        goto End;

    // snapshot the attribute sorted mesh, shown as the un-optimized version
    hr = pMeshSysMem->CloneMeshFVF( dw32Bit|D3DXMESH_MANAGED|D3DXMESH_VB_WRITEONLY, pMeshSysMem->GetFVF(), 
                                      m_pd3dDevice, &m_pMesh );
    if( FAILED(hr) )
        goto End;

    // actually do the vertex cache optimization
    hr = pMeshSysMem->OptimizeInplace( D3DXMESHOPT_COMPACT|D3DXMESHOPT_ATTRSORT|D3DXMESHOPT_VERTEXCACHE,
                                 rgdwAdjacencyTemp,
                                 NULL, NULL, NULL);
    if( FAILED(hr) )
        goto End;

    // snapshot as the optimized mesh
    hr = pMeshSysMem->CloneMeshFVF( dw32Bit|D3DXMESH_MANAGED|D3DXMESH_VB_WRITEONLY, pMeshSysMem->GetFVF(), 
                                      m_pd3dDevice, &m_pMeshOptimized );
    if( FAILED(hr) )
        goto End;

    // check current display setting
    CheckMenuItem( GetMenu(m_hWnd), ID_OPTIONS_DISPLAY1 + (m_cObjectsPerSide-1), MF_CHECKED );
    CheckMenuItem( GetMenu(m_hWnd), IDM_SHOWOPTIMIZEDMESH, m_bShowOptimizedMesh ? MF_CHECKED : MF_UNCHECKED);

End:
    SAFE_DELETE_ARRAY( rgdwAdjacencyTemp );
    SAFE_RELEASE( pMeshSysMem );
   
    return hr;
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
    light.Type         = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r    = light.Diffuse.g  = light.Diffuse.b  = 1.0f;
    light.Specular.r   = light.Specular.g = light.Specular.b = 0.0f;
    light.Ambient.r    = light.Ambient.g  = light.Ambient.b  = 0.3f;
    light.Position     = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &D3DXVECTOR3( 0.3f, -1.0f, 1.0f ) );
    light.Attenuation0 = light.Attenuation1 = light.Attenuation2 = 0.0f;
    light.Range        = sqrtf(FLT_MAX);
    m_pd3dDevice->SetLight(0, &light );
    m_pd3dDevice->LightEnable(0, TRUE );

    m_ArcBall.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 0.85f );
    m_ArcBall.SetRadius( m_fObjectRadius );

    FLOAT fAspect = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;

    D3DXMATRIX matProj;
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

    for( UINT i=0; i<m_dwNumMaterials; i++ )
        SAFE_RELEASE( m_pMeshTextures[i] );
    SAFE_DELETE_ARRAY( m_pMeshTextures );
    SAFE_DELETE_ARRAY( m_pMeshMaterials );
    SAFE_RELEASE( m_pAdjacencyBuffer );
    SAFE_RELEASE( m_pMesh );
    SAFE_RELEASE( m_pMeshOptimized );

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
    if( WM_CONTEXTMENU==uMsg )
        return 0;

    if( uMsg == WM_COMMAND )
    {
        // Toggle mesh optimization
        if( LOWORD(wParam) == IDM_SHOWOPTIMIZEDMESH )
        {
            m_bShowOptimizedMesh = !m_bShowOptimizedMesh;

            if( m_bShowOptimizedMesh )
                CheckMenuItem( GetMenu(hWnd), IDM_SHOWOPTIMIZEDMESH, MF_CHECKED );
            else
                CheckMenuItem( GetMenu(hWnd), IDM_SHOWOPTIMIZEDMESH, MF_UNCHECKED );
        }
        

        // Handle the open file command
        else if( LOWORD(wParam) == IDM_OPENFILE )
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
                InitDeviceObjects();
                RestoreDeviceObjects();
            }
        }

        else if ((LOWORD(wParam) >= ID_OPTIONS_DISPLAY1) && (LOWORD(wParam) <= ID_OPTIONS_DISPLAY36))
        {
            // uncheck old item
            CheckMenuItem( GetMenu(hWnd), ID_OPTIONS_DISPLAY1 + (m_cObjectsPerSide-1), MF_UNCHECKED );

            // calc new item
            m_cObjectsPerSide = LOWORD(wParam) - ID_OPTIONS_DISPLAY1 + 1;

            // check new item
            CheckMenuItem( GetMenu(hWnd), ID_OPTIONS_DISPLAY1 + (m_cObjectsPerSide-1), MF_CHECKED );
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



