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
#include "MazeApp.h"
#include "DPlay8Client.h"
#include "MiscUtils.h"
#include "Resource.h"
#include "MazeClient.h"
#include "Config.h"


extern  Config      g_Config;
extern  CMazeClient g_MazeClient;
extern  CDPlay8Client   g_DP8Client;
extern  BOOL        g_bOutOfDateClient;



//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
#define DDLOCK_NORMAL   (DDLOCK_NOSYSLOCK|DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR)



//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CMazeApp::CMazeApp()
{
    m_pFloorTexture     = NULL;
    m_pCeilingTexture   = NULL;
    m_pWallTexture      = NULL;
    m_pNetIconTexture   = NULL;
    m_pLocalIconTexture = NULL;
    m_pFontTexture      = NULL;
    m_pPlayerIconTexture = NULL;
    m_pMiniMapVB        = NULL;
    m_pSphere           = NULL;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CMazeApp::OneTimeInit( HWND hWnd )
{
    HRESULT hr;

    // Do base class init
    if( FAILED( hr = CModule::OneTimeInit( hWnd ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("OneTimeInit"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::OneTimeShutdown()
{
    // Do base class shutdown
    CModule::OneTimeShutdown();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CMazeApp::DisplayInit( DWORD flags, IDirectDrawSurface7* target, RECT* rect )
{
    HRESULT hr;

    // Do base class init
    if( FAILED( hr = CModule::DisplayInit( flags, target, rect ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("DisplayInit"), hr );

    // Load textures
    if( (m_pFloorTexture = LoadTextureFromResource( m_pDevice, IDB_FLOORTEXTURE ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadTextureFromResource"), E_FAIL );
    if( (m_pWallTexture = LoadTextureFromResource( m_pDevice, IDB_WALLTEXTURE ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadTextureFromResource"), E_FAIL );
    if( (m_pCeilingTexture = LoadTextureFromResource( m_pDevice, IDB_CEILINGTEXTURE ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadTextureFromResource"), E_FAIL );
    if( (m_pFontTexture = LoadTextureFromResource( m_pDevice, IDB_FONTTEXTURE ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadTextureFromResource"), E_FAIL );

    // Load textures, creating an alpha channel in the texture (based on black==transparent)
    if( (m_pNetIconTexture = LoadAlphaTextureFromResource( m_pDevice, IDB_NETICON ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadAlphaTextureFromResource"), E_FAIL );
    if( (m_pLocalIconTexture = LoadAlphaTextureFromResource( m_pDevice, IDB_LOCALICON ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadAlphaTextureFromResource"), E_FAIL );
    if( (m_pPlayerIconTexture = LoadAlphaTextureFromResource( m_pDevice, IDB_PLAYERICON ) ) == NULL )
        return DXTRACE_ERR_NOMSGBOX( TEXT("LoadAlphaTextureFromResource"), E_FAIL );

    // Create sphere for rendering Things
    if( FAILED( hr = D3DXCreateSphere( m_pDevice, 0.2f, 8, 8, 1, &m_pSphere ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("D3DXCreateSphere"), hr );

    // Initialise the SmartVB for the walls
    if( FAILED( hr = m_WallsSVB.Init( m_pD3D, m_pDevice, 2048, m_dwVBMemType ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("Init"), hr );

    // Set projection matrix
    const float fov = 1.8f;
    const float znear = 0.1f;
    const float zfar = 30.0f;
    const float viewwidth = float(tan(fov*0.5f) ) * znear;
    const float viewheight = viewwidth * float(m_dwHeight)/float(m_dwWidth);
    D3DXMatrixPerspectiveLH( &m_Projection, viewwidth, viewheight, znear, zfar );
    if( FAILED( hr = m_pDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, m_Projection ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("SetTransform"), hr );

    // Create vertex buffer for mini-map vertices
    D3DVERTEXBUFFERDESC vbdesc;
    vbdesc.dwSize = sizeof(vbdesc);
    vbdesc.dwFVF = FVF_ColourVertex;
    vbdesc.dwCaps = D3DVBCAPS_WRITEONLY|m_dwVBMemType;
    vbdesc.dwNumVertices = 1024;
    if( FAILED( hr = m_pD3D->CreateVertexBuffer( &vbdesc, &m_pMiniMapVB, 0 ) ) )
        return DXTRACE_ERR_NOMSGBOX( TEXT("CreateVertexBuffer"), hr );

    // Set renderstates and lighting
    D3DXMATRIX  identity; D3DXMatrixIdentity( &identity );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, identity );
    m_pDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_AMBIENT, 0x333333 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
    m_pDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );
    m_pDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL );
    m_pDevice->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, FALSE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, FALSE );

    D3DMATERIAL7 mat;
    mat.diffuse  = D3DXCOLOR(1,1,1,0);
    mat.ambient  = D3DXCOLOR(1,1,1,0);
    mat.specular = D3DXCOLOR(0,0,0,0);
    mat.emissive = D3DXCOLOR(0,0,0,0);
    mat.power    = 0;
    m_pDevice->SetMaterial( &mat );

    // Kill some features if we have software rasterisation
    if( m_bSoftwareRasterisation )
    {
        m_dwTesselation = 1;
        g_Config.bReflections = FALSE;
        m_pDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTFP_POINT );
    }
    else
    {
        m_dwTesselation = 3;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DisplayShutdown()
{
    SAFE_RELEASE(m_pSphere);
    SAFE_RELEASE(m_pPlayerIconTexture);
    SAFE_RELEASE(m_pFontTexture);
    SAFE_RELEASE(m_pLocalIconTexture);
    SAFE_RELEASE(m_pNetIconTexture);
    SAFE_RELEASE(m_pFloorTexture);
    SAFE_RELEASE(m_pCeilingTexture);
    SAFE_RELEASE(m_pWallTexture);
    SAFE_RELEASE(m_pMiniMapVB);
    m_WallsSVB.Uninit();

    // Do base class shutdown
    CModule::DisplayShutdown();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::RenderFrame( FLOAT fElapsed )
{
    if( FAILED( BeginFrame() ) )
        return;

    Clear( D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET );

    // Compute and set camera matrix
    m_vCameraPos = g_MazeClient.GetCameraPos();
    m_fCameraYaw = AngleToFloat(g_MazeClient.GetCameraYaw() );
    ComputeCameraMatrix();
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, m_Camera );

    // Set light attached to camera
    D3DLIGHT7 light;
    light.dltType = D3DLIGHT_POINT;
    light.dcvDiffuse = D3DXCOLOR(1,1,1,0);
    light.dcvSpecular = D3DXCOLOR(0,0,0,0);
    light.dcvAmbient = D3DXCOLOR(0,0,0,0);
    light.dvPosition = m_vCameraPos;
    light.dvDirection = D3DVECTOR(0,0,0);
    light.dvRange = 80.0f;
    light.dvFalloff = light.dvTheta = light.dvPhi = 0;
    light.dvAttenuation0 = 1.0f;
    light.dvAttenuation1 = 0.0f;
    light.dvAttenuation2 = 1.0f;
    m_pDevice->SetLight( 0, &light );
    m_pDevice->LightEnable( 0, TRUE );

    // Get visibility set of maze
    Vector2 dir;
    dir.y = float(cos(m_fCameraYaw) );
    dir.x = float(-sin(m_fCameraYaw) );
    m_dwNumVisibleCells = g_MazeClient.m_Maze.GetVisibleCells( Vector2(m_vCameraPos.x, m_vCameraPos.z) ,
                                                               dir, 1.8f, m_mcrVisList, MAX_VISLIST );

    // Draw reflection of walls and ceiling in floor
    if( g_Config.bReflections )
    {
        D3DXMATRIX  reflectcamera = m_Camera;
        reflectcamera.m10 = -reflectcamera.m10;
        reflectcamera.m11 = -reflectcamera.m11;
        reflectcamera.m12 = -reflectcamera.m12;
        reflectcamera.m13 = -reflectcamera.m13;
        m_pDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, reflectcamera );

        m_pDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, D3DCULL_CW );

        light.dcvDiffuse = D3DXCOLOR(0.7f,0.7f,0.7f,0);
        m_pDevice->SetRenderState( D3DRENDERSTATE_AMBIENT, 0x222222 );
        m_pDevice->SetLight( 0, &light );

        DrawWalls();
        DrawCeiling();
        DrawThings();

        light.dcvDiffuse = D3DXCOLOR(1,1,1,0);
        m_pDevice->SetRenderState( D3DRENDERSTATE_AMBIENT, 0x333333 );
        m_pDevice->SetLight( 0, &light );

        m_pDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, D3DCULL_CCW );
        m_pDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, m_Camera );
    }

    // Draw floor
    if( g_Config.bReflections )
    {
        m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
        m_pDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE );
        m_pDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE );
    }
    DrawFloor();
    if( g_Config.bReflections )
        m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

    // Draw walls and ceiling
    DrawWalls();
    DrawCeiling();

    // Draw Things
    DrawThings();

    // Draw mini-map
    if( g_Config.bDrawMiniMap )
        DrawMiniMap();

    // Draw indicators
    if( g_Config.bShowIndicators )
        DrawIndicators();

/*
    TCHAR text[16];
    wsprintf( text, TEXT("Vis %d"), m_dwNumVisibleCells );
    m_pXContext->DrawDebugText( 0, 0, 0xffff22, text );
*/

    EndFrame();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::ComputeCameraMatrix()
{
    D3DXMATRIX  trans,rotate;
    D3DXMatrixTranslation( &trans, -m_vCameraPos.x, -m_vCameraPos.y, -m_vCameraPos.z );
    D3DXMatrixRotationY( &rotate, m_fCameraYaw );

    m_Camera = trans * rotate;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawThings()
{
    MazeCellRef* pCell = m_mcrVisList;

    m_pDevice->ApplyStateBlock( m_pStateBlocks->ModulateTexture );
    m_pDevice->SetTexture( 0, m_pCeilingTexture );

    g_MazeClient.LockWorld();
    D3DXMATRIX  world;
    for( DWORD i = 0; i < m_dwNumVisibleCells; i++, pCell++ )
    {
        IEngineThing* pThing = g_MazeClient.GetFirstThingInCell( pCell->x, pCell->y );
        while( pThing )
        {
            const D3DXVECTOR3&  pos = pThing->GetPos();
            D3DXMatrixTranslation( &world, pos.x, pos.y, pos.z );
            m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, world );
            m_pSphere->Draw();
            pThing = pThing->GetNext();
        }
    }
    g_MazeClient.UnlockWorld();
    D3DXMatrixIdentity( &world );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, world );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawFloor()
{
    MazeCellRef*    pCell = m_mcrVisList;
    TexVertex*      pVert;
    WORD*           pIndex;
    WORD            offset;
    D3DXVECTOR3     normal(0,1,0);
    DWORD         verts_per_side = (m_dwTesselation+1)*(m_dwTesselation+1);
    DWORD         indicies_per_side = m_dwTesselation*m_dwTesselation*6;

    m_pDevice->ApplyStateBlock( m_pStateBlocks->ModulateTexture );
    m_pDevice->SetTexture( 0, m_pFloorTexture );

    m_WallsSVB.Begin();
    for( DWORD i = 0; i < m_dwNumVisibleCells; i++, pCell++ )
    {
        D3DXVECTOR3 pos( float(pCell->x), 0, float(pCell->y) );
        m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
        LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(1,0,0), D3DXVECTOR3(0,0,1) ,
                  D3DXVECTOR3(0,1,0) );
    }
    m_WallsSVB.End();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawCeiling()
{
    MazeCellRef*    pCell = m_mcrVisList;
    TexVertex*      pVert;
    WORD*           pIndex;
    WORD            offset;
    D3DXVECTOR3     normal(0,1,0);
    DWORD         verts_per_side = (m_dwTesselation+1)*(m_dwTesselation+1);
    DWORD         indicies_per_side = m_dwTesselation*m_dwTesselation*6;

    m_pDevice->ApplyStateBlock( m_pStateBlocks->ModulateTexture );
    m_pDevice->SetTexture( 0, m_pCeilingTexture );

    m_WallsSVB.Begin();
    for( DWORD i = 0; i < m_dwNumVisibleCells; i++, pCell++ )
    {
        D3DXVECTOR3 pos( float(pCell->x), 1, float(1+pCell->y) );
        m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
        LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(1,0,0), D3DXVECTOR3(0,0,-1) ,
                  D3DXVECTOR3(0,-1,0) );
    }
    m_WallsSVB.End();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawWalls()
{
    MazeCellRef*  pCell = m_mcrVisList;
    TexVertex*    pVert;
    WORD*         pIndex;
    WORD          offset;
    DWORD         verts_per_side = (m_dwTesselation+1)*(m_dwTesselation+1);
    DWORD         indicies_per_side = m_dwTesselation*m_dwTesselation*6;

    m_pDevice->ApplyStateBlock( m_pStateBlocks->ModulateTexture );
    m_pDevice->SetTexture( 0, m_pWallTexture );

    m_WallsSVB.Begin();
    for( DWORD i = 0; i < m_dwNumVisibleCells; i++, pCell++ )
    {
        BYTE cell = g_MazeClient.m_Maze.GetCell(pCell->x,pCell->y);

        if( cell & MAZE_WALL_NORTH )
        {
            D3DXVECTOR3 pos( float(pCell->x), 1, float(pCell->y) );
            m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
            LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(1,0,0), D3DXVECTOR3(0,-1,0) ,
                      D3DXVECTOR3(0,0,1) );
        }
        if( cell & MAZE_WALL_SOUTH )
        {
            D3DXVECTOR3 pos( float(1+pCell->x), 1, float(1+pCell->y) );
            m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
            LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(-1,0,0), D3DXVECTOR3(0,-1,0) ,
                      D3DXVECTOR3(0,0,-1) );
        }
        if( cell & MAZE_WALL_WEST )
        {
            D3DXVECTOR3 pos( float(pCell->x), 1, float(1+pCell->y) );
            m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
            LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(0,0,-1), D3DXVECTOR3(0,-1,0) ,
                      D3DXVECTOR3(1,0,0) );
        }
        if( cell & MAZE_WALL_EAST )
        {
            D3DXVECTOR3 pos( float(1+pCell->x), 1, float(pCell->y) );
            m_WallsSVB.MakeRoom( verts_per_side, indicies_per_side, &pVert, &pIndex, &offset );
            LoadQuad( pVert, pIndex, offset, pos, D3DXVECTOR3(0,0,1), D3DXVECTOR3(0,-1,0) ,
                      D3DXVECTOR3(-1,0,0) );
        }
    }
    m_WallsSVB.End();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::LoadQuad( TexVertex* pVerts, WORD* pIndex, WORD offset ,
                         const D3DXVECTOR3& origin, const D3DXVECTOR3& basis1 ,
                         const D3DXVECTOR3& basis2, const D3DXVECTOR3& normal )
{
    // Compute edge steps
    float       step = 1.0f / m_dwTesselation;
    D3DXVECTOR3 step1 = basis1 * step;
    D3DXVECTOR3 step2 = basis2 * step;

    // Fill out grid of vertices
    D3DXVECTOR3 rowstart = origin;
    float u = 0;
    for( DWORD i = 0; i <= m_dwTesselation; i++, rowstart += step1, u += step )
    {
        D3DXVECTOR3 pos = rowstart;
        float     v = 0;
        for( DWORD j = 0; j <= m_dwTesselation; j++, pos += step2, v += step )
        {
            pVerts->vPos = pos;
            pVerts->vNormal = normal;
            pVerts->fU = u;
            pVerts->fV = v;
            pVerts++;
        }
    }

    // Fill out grid of indicies
    for( i = 0; i < m_dwTesselation; i++ )
    {
        for( DWORD j = 0; j < m_dwTesselation; j++ )
        {
            *pIndex++ = offset;
            *pIndex++ = offset+1;
            *pIndex++ = offset+1+(WORD(m_dwTesselation)+1);
            *pIndex++ = offset;
            *pIndex++ = offset+1+(WORD(m_dwTesselation)+1);
            *pIndex++ = offset+(WORD(m_dwTesselation)+1);
            offset++;
        }
        offset++;
    }
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawMiniMap()
{
    HRESULT         hr;
    ColourVertex*   verts;
    D3DXVECTOR3     pos;
    D3DXVECTOR3     origin;
    D3DXMATRIX      world;
    DWORD           y;
    MazeCellRef*    pCell = NULL;
    DWORD           x;

    // Compute size of mini-map based on display size
    DWORD mapsize = m_dwWidth / 4;
    int mapx = m_dwWidth - mapsize - 16;
    int mapy = m_dwHeight - mapsize - 16;

    if( mapx < 0 || mapy < 0 )
        return;

    // Disable z-buffering
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, FALSE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE, FALSE );

    // Draw alpha blended background
    ColourTLVertex  v[4];
    v[0].fX = float(mapx)-0.5f; v[0].fY = float(mapy)-0.5f;
    v[1].fX = float(mapx+mapsize)-0.5f; v[1].fY = float(mapy)-0.5f;
    v[2].fX = float(mapx+mapsize)-0.5f; v[2].fY = float(mapy+mapsize)-0.5f;
    v[3].fX = float(mapx)-0.5f; v[3].fY = float(mapy+mapsize)-0.5f;
    v[0].fZ  = v[1].fZ = v[2].fZ = v[3].fZ = 0.1f;
    v[0].fRHW = v[1].fRHW = v[2].fRHW = v[3].fRHW = 1.0f;
    v[0].dwDiffuse = v[1].dwDiffuse = v[2].dwDiffuse = v[3].dwDiffuse = 0x80008000;

    //m_pDevice->ApplyStateBlock( m_pStateBlocks->CopyColour );
    // Set texture stages explicitly rather than apply the state block. Some drivers
    // appear not to do the state block properly all the time for some reason.
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
    m_pDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
    m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, FVF_ColourTLVertex, v, 4, 0 );
    m_pDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

    DWORD linecount = 0;

    // Now set matrices for orthogonal top-down line drawing into mini-map area
    D3DXMATRIX  orthproj;
    D3DXMatrixOrthoLH( &orthproj, 8, 8, 1, 2 );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, orthproj );

    D3DXMATRIX  orthcam; D3DXMatrixIdentity( &orthcam );
    orthcam.m30 = -m_vCameraPos.x;
    orthcam.m31 = -m_vCameraPos.z;
    orthcam.m00 = orthcam.m21 = orthcam.m12 = 1;
    orthcam.m11 = orthcam.m22 = 0;
    D3DXMATRIX  rot; D3DXMatrixRotationZ( &rot, -m_fCameraYaw );
    orthcam = orthcam * rot;
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, orthcam );

    // Compute range of cells we're interested in
    DWORD centrecellx = DWORD(m_vCameraPos.x);
    DWORD centrecelly = DWORD(m_vCameraPos.z);
    DWORD mazewidth = g_MazeClient.m_Maze.GetWidth();
    DWORD mazeheight = g_MazeClient.m_Maze.GetHeight();
    DWORD minx = (centrecellx < 6) ? 0 : centrecellx - 6;
    DWORD miny = (centrecelly < 6) ? 0 : centrecelly - 6;
    DWORD maxx = centrecellx+6 > mazewidth - 1  ? mazewidth - 1 : centrecellx+5;
    DWORD maxy = centrecelly+6 > mazeheight - 1 ? mazeheight - 1 : centrecelly+5;

    // Change viewport to clip to the map region
    D3DVIEWPORT7    oldvp;
    m_pDevice->GetViewport( &oldvp );
    D3DVIEWPORT7    mapvp;
    mapvp.dwX = mapx; mapvp.dwY = mapy;
    mapvp.dwWidth = mapvp.dwHeight = mapsize;
    mapvp.dvMinZ = 0; mapvp.dvMaxZ = 1;
    m_pDevice->SetViewport( &mapvp );

    // Lock mini-map VB
    hr = m_pMiniMapVB->Lock( DDLOCK_NOSYSLOCK|DDLOCK_WRITEONLY|DDLOCK_SURFACEMEMORYPTR|
                             DDLOCK_DISCARDCONTENTS, (void**)&verts, NULL );
    if( FAILED(hr) )
    {
        hr = m_pMiniMapVB->Lock( DDLOCK_SURFACEMEMORYPTR, (void**)&verts, NULL );
        if( FAILED(hr) )
        {
            DXTRACE_ERR_NOMSGBOX( "m_pMiniMapVB->Lock", hr );
            goto LCleanup;
        }
    }

    // Loop through cells
    origin = D3DXVECTOR3(float(minx),1,float(miny) );
    for( x = minx; x <= maxx; x++ )
    {
        pos = origin;
        for( DWORD y = miny; y <= maxy; y++ )
        {
            // Grab wall bitmask
            BYTE cell = g_MazeClient.m_Maze.GetCell(x,y);

            // Check for north wall
            if( cell & MAZE_WALL_NORTH )
            {
                verts->fX = pos.x; verts->fY = pos.y; verts->fZ = pos.z;
                verts->dwDiffuse = 0xffffff; verts++;
                verts->fX = pos.x+1; verts->fY = pos.y; verts->fZ = pos.z;
                verts->dwDiffuse = 0xffffff; verts++;
                linecount++;
            }

            // Check for west wall
            if( cell & MAZE_WALL_WEST )
            {
                verts->fX = pos.x; verts->fY = pos.y; verts->fZ = pos.z;
                verts->dwDiffuse = 0xffffff; verts++;
                verts->fX = pos.x; verts->fY = pos.y; verts->fZ = pos.z+1;
                verts->dwDiffuse = 0xffffff; verts++;
                linecount++;
            }

            if( y==maxy )
            {
                // Check for north wall
                if( cell & MAZE_WALL_SOUTH )
                {
                    verts->fX = pos.x; verts->fY = pos.y; verts->fZ = pos.z+1;
                    verts->dwDiffuse = 0xffffff; verts++;
                    verts->fX = pos.x+1; verts->fY = pos.y; verts->fZ = pos.z+1;
                    verts->dwDiffuse = 0xffffff; verts++;
                    linecount++;
                }
            }

            if( x==maxx )
            {
                // Check for west wall
                if( cell & MAZE_WALL_EAST )
                {
                    verts->fX = pos.x+1; verts->fY = pos.y; verts->fZ = pos.z;
                    verts->dwDiffuse = 0xffffff; verts++;
                    verts->fX = pos.x+1; verts->fY = pos.y; verts->fZ = pos.z+1;
                    verts->dwDiffuse = 0xffffff; verts++;
                    linecount++;
                }
            }

            pos.z += 1;
        }

        origin.x += 1;
    }

    // Unlock VB and submit for rendering
    m_pMiniMapVB->Unlock();
    m_pDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
    m_pDevice->DrawPrimitiveVB( D3DPT_LINELIST, m_pMiniMapVB, 0, linecount*2, 0 );

    // Draw marker for camera
    D3DXMatrixRotationY( &rot, -m_fCameraYaw );
    rot.m30 = m_vCameraPos.x; rot.m32 = m_vCameraPos.z;
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, rot );
    ColourVertex    cv[4];
    cv[0].fY = cv[1].fY = cv[2].fY = cv[3].fY = 1;
    cv[0].dwDiffuse = cv[1].dwDiffuse = cv[2].dwDiffuse = cv[3].dwDiffuse = 0xb01010;
    cv[0].fX = 0;       cv[0].fZ = 0.4f;
    cv[1].fX = 0.2f;    cv[1].fZ = 0;
    cv[2].fX = 0;       cv[2].fZ = -0.2f;
    cv[3].fX = -0.2f;   cv[3].fZ = 0;
    m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, FVF_ColourVertex, cv, 4, 0 );
    D3DXMatrixIdentity( &rot );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, rot );

    // Draw things in mini-map
    g_MazeClient.LockWorld();
    cv[0].fX = 0;  cv[0].fZ = 0.2f;
    cv[0].dwDiffuse = cv[1].dwDiffuse = cv[2].dwDiffuse = cv[3].dwDiffuse = 0x00FFFF00;
    for( x = minx; x <= maxx; x++ )
    {
        for( y = miny; y <= maxy; y++ )
        {
            IEngineThing* pThing = g_MazeClient.GetFirstThingInCell( x, y );
            while( pThing )
            {
                const D3DXVECTOR3&  pos = pThing->GetPos();
                D3DXMatrixTranslation( &world, pos.x, pos.y, pos.z );
                m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, world );
                m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, FVF_ColourVertex, cv, 4, 0 );
                pThing = pThing->GetNext();
            }

            pos.z += 1;
        }
    }
    g_MazeClient.UnlockWorld();

LCleanup:
    D3DXMatrixIdentity( &rot );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, rot );

    // Re-enable z-buffering and reset viewport and matrices
    m_pDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE, TRUE );
    m_pDevice->SetViewport( &oldvp );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, m_Projection );
    m_pDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, m_Camera );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawIndicators()
{
    D3DTLVERTEX v[4];
    v[0].specular = v[1].specular = v[2].specular = v[3].specular = 0;
    v[0].sz = v[1].sz = v[2].sz = v[3].sz = 0;
    v[0].rhw = v[1].rhw = v[2].rhw = v[3].rhw = 1;
    v[0].tu = 0; v[0].tv = 0;
    v[1].tu = 1; v[1].tv = 0;
    v[2].tu = 1; v[2].tv = 1;
    v[3].tu = 0; v[3].tv = 1;

    float xscale = float(m_dwWidth)/640.0f;
    float yscale = float(m_dwHeight)/480.0f;

    // Draw network/local icon and player icon (if connected)
    m_pDevice->ApplyStateBlock( m_pStateBlocks->CopyTexture );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
    m_pDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    if( g_DP8Client.IsSessionLost() )
        m_pDevice->SetTexture( 0, m_pLocalIconTexture );
    else
        m_pDevice->SetTexture( 0, m_pNetIconTexture );
    v[0].sx = (xscale*16)-0.5f; v[0].sy = ((480-48)*yscale)-0.5f;
    v[1].sx = (xscale*48)-0.5f; v[1].sy = ((480-48)*yscale)-0.5f;
    v[2].sx = (xscale*48)-0.5f; v[2].sy = ((480-16)*yscale)-0.5f;
    v[3].sx = (xscale*16)-0.5f; v[3].sy = ((480-16)*yscale)-0.5f;
    m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, v, 4, 0 );
    if( FALSE == g_DP8Client.IsSessionLost() )
    {
        m_pDevice->SetTexture( 0, m_pPlayerIconTexture );

        DWORD dwWidth  = 110;
        DWORD dwHeight = 60;
        POINT ptStart  = { 60, 480-70 };

        v[0].sx = (xscale*(ptStart.x) )-0.5f;         v[0].sy = ((ptStart.y)*yscale)-0.5f;
        v[1].sx = (xscale*(ptStart.x+dwWidth) )-0.5f; v[1].sy = ((ptStart.y)*yscale)-0.5f;
        v[2].sx = (xscale*(ptStart.x+dwWidth) )-0.5f; v[2].sy = ((ptStart.y+dwHeight)*yscale)-0.5f;
        v[3].sx = (xscale*(ptStart.x) )-0.5f;         v[3].sy = ((ptStart.y+dwHeight)*yscale)-0.5f;

        float fMaxTU = (float) dwWidth / 128.0f;
        float fMaxTV = (float) dwHeight / 128.0f;

        v[0].tu = 0;      v[0].tv = 0;
        v[1].tu = fMaxTU; v[1].tv = 0;
        v[2].tu = fMaxTU; v[2].tv = fMaxTV;
        v[3].tu = 0;      v[3].tv = fMaxTV;
        m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, v, 4, 0 );
    }
    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
    m_pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

    // If we're connected, then need to draw some network stats
    if( FALSE == g_DP8Client.IsSessionLost() )
    {
        DWORD rate = g_MazeClient.GetThroughputBPS();
        DWORD ping = g_MazeClient.GetRoundTripLatencyMS();

        TCHAR    text[16];
        wsprintf( text, TEXT("%4d"), g_MazeClient.GetNumPlayers() );
        DrawText( 170, 480-68, text );
        wsprintf( text, TEXT("%4dm"), ping );
        DrawText( 170, 480-48, text );
        wsprintf( text, TEXT("%4d%%"), rate );
        DrawText( 170, 480-28, text );
    }
    else
    {
        if( g_bOutOfDateClient )
        {
            TCHAR text[16];
            wsprintf( text, TEXT("!") );
            DrawText( 10, 480-105, text );
        }
    }

    // Draw framerate
    if( g_Config.bShowFramerate )
    {
        TCHAR    text[16];
        wsprintf( text, TEXT("%3dH"), DWORD(m_pFrameRate->GetRate() ) );
        DrawText( 640 - 60, 4, text );
    }
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::DrawText( DWORD x, DWORD y, const TCHAR* string )
{
    D3DTLVERTEX v[4];
    v[0].specular = v[1].specular = v[2].specular = v[3].specular = 0;
    v[0].sz = v[1].sz = v[2].sz = v[3].sz = 0;
    v[0].rhw = v[1].rhw = v[2].rhw = v[3].rhw = 1;

    m_pDevice->ApplyStateBlock( m_pStateBlocks->CopyTexture );
    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE );
    m_pDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR );
    m_pDevice->SetTexture( 0, m_pFontTexture );

    float xscale = float(m_dwWidth)/640.0f;
    float yscale = float(m_dwHeight)/480.0f;
    float xpos = (float(x) * xscale)-0.5f;
    float ypos = (float(y) * yscale)-0.5f;
    float xstep = 14 * xscale;
    float xwidth = 16 * xscale;
    float ystep = 16 * yscale;

    v[0].sy = v[1].sy = ypos;
    v[2].sy = v[3].sy = ypos + ystep;

    for(; *string; string++, xpos += xstep )
    {
        TCHAR    ch = *string;

        float tu,tv;
        float fDeltaX = 0.125f;
        float fDeltaY = 0.125f;

        if( ch >= '0' && ch <= '9' )
        {
            ch -= '0';
            tu = (ch % 4) * 0.125f;
            tv = (ch / 4) * 0.125f;
        }
        else if( ch == '%' )
        {
            tu = 0.125f * 4.0f;
            tv = 0.125f * 0.0f;
            fDeltaX = 0.125f * 2.0f;
            fDeltaY = 0.125f * 1.0f;
            xwidth = 16*2 * xscale;
        }
        else if( ch == 'm' || ch == 'M' )
        {
            tu = 0.125f*3.0f;
            tv = 0.125f*2.0f;
        }
        else if( ch == 'H' || ch == 'h' )
        {
            tu = 0.125f*0.0f;
            tv = 0.125f*3.0f;
        }
        else if( ch == '!' )
        {
            tu = 0.125f*0.0f;
            tv = 0.125f*4.0f;
            fDeltaX = 1.0f;
            fDeltaY = 0.125f * 3.0f;
            xwidth = 16*8 * xscale;
            v[2].sy = v[3].sy = ypos + ystep * 3;
        }
        else
            continue;

        v[0].tu = tu; v[0].tv = tv;
        v[1].tu = tu + fDeltaX; v[1].tv = tv;
        v[2].tu = tu + fDeltaX; v[2].tv = tv + fDeltaY;
        v[3].tu = tu; v[3].tv = tv + fDeltaY;

        v[0].sx = v[3].sx = xpos;
        v[1].sx = v[2].sx = xpos + xwidth;

        m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, v, 4, 0 );
    }
    m_pDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
void CMazeApp::OnChar( TCHAR ch )
{
    switch( ch )
    {
        case 'r':
        case 'R':
            g_Config.bReflections = !g_Config.bReflections;
            break;

        case 'a':
        case 'A':
            g_MazeClient.EngageAutopilot( !g_MazeClient.IsAutopilot() );
            break;
    }
}
