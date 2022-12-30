//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _MAZE_APP_H
#define _MAZE_APP_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include "Module.h"
#include "SmartVB.h"
#include "MazeClient.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  TexVertex
{
    D3DVECTOR   vPos;
    D3DVECTOR   vNormal;
    float       fU;
    float       fV;
};

#define FVF_TexVertex   (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  ColourVertex
{
    float   fX;
    float   fY;
    float   fZ;
    DWORD   dwDiffuse;
};

#define FVF_ColourVertex    (D3DFVF_XYZ|D3DFVF_DIFFUSE)




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  ColourTLVertex
{
    float   fX;
    float   fY;
    float   fZ;
    float   fRHW;
    DWORD   dwDiffuse;
};

#define FVF_ColourTLVertex  (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class   CMazeApp : public CModule
{
public:
    CMazeApp();

    // From CModule
    virtual HRESULT OneTimeInit( HWND hWnd );
    virtual void    OneTimeShutdown();
    virtual void    RenderFrame( FLOAT fElapsed );
    virtual HRESULT DisplayInit( DWORD flags , IDirectDrawSurface7* target , RECT* rect = NULL );
    virtual void    DisplayShutdown();
    virtual void    OnChar( TCHAR ch );

protected:
    DWORD                                   m_dwTesselation;
    SmartVB<TexVertex,FVF_TexVertex,1000>   m_WallsSVB;
    IDirect3DVertexBuffer7*                 m_pMiniMapVB;
    D3DXVECTOR3                             m_vCameraPos;
    float                                   m_fCameraYaw;
    D3DXMATRIX                              m_Camera;
    D3DXMATRIX                              m_Projection;
    IDirectDrawSurface7*                    m_pFloorTexture;    
    IDirectDrawSurface7*                    m_pCeilingTexture;  
    IDirectDrawSurface7*                    m_pWallTexture;
    IDirectDrawSurface7*                    m_pNetIconTexture;
    IDirectDrawSurface7*                    m_pLocalIconTexture;
    IDirectDrawSurface7*                    m_pFontTexture;
    IDirectDrawSurface7*                    m_pPlayerIconTexture;
    ID3DXSimpleShape*                       m_pSphere;

    enum { MAX_VISLIST = 300 };
    MazeCellRef                             m_mcrVisList[MAX_VISLIST];
    DWORD                                   m_dwNumVisibleCells;

    void    ComputeCameraMatrix();
    void    DrawFloor();
    void    DrawWalls();
    void    DrawCeiling();
    void    DrawMiniMap();
    void    DrawThings();
    void    DrawIndicators();
    void    DrawText( DWORD x , DWORD y , const TCHAR* string );

    void    LoadQuad( TexVertex* pverts , WORD* pindex , WORD offset ,
                      const D3DXVECTOR3& origin , const D3DXVECTOR3& basis1 ,
                      const D3DXVECTOR3& basis2 , const D3DXVECTOR3& normal );
};




#endif
