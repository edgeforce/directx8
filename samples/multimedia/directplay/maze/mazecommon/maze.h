//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _MAZE_H
#define _MAZE_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include <assert.h>
#include "Random.h"
#include "Vector2.h"

const   BYTE   MAZE_WALL_NORTH = (1<<0);
const   BYTE   MAZE_WALL_EAST = (1<<1);
const   BYTE   MAZE_WALL_SOUTH = (1<<2);
const   BYTE   MAZE_WALL_WEST = (1<<3);
const   BYTE   MAZE_WALL_ALL = MAZE_WALL_NORTH|MAZE_WALL_EAST|
                                MAZE_WALL_SOUTH|MAZE_WALL_WEST;




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  MazeCellRef
{
    DWORD   x,y;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class   CMaze
{
public:
    CMaze();
    ~CMaze();

    HRESULT Init( DWORD width , DWORD height , DWORD seed );
    void    Empty();

    DWORD   GetWidth()  const { return m_dwWidth; };
    DWORD   GetHeight() const { return m_dwHeight; };
    DWORD   GetSize()   const { return m_dwWidth*m_dwHeight; };
    BYTE   GetCell( DWORD x , DWORD y )    const { assert(m_pMaze!=NULL); return *(m_pMaze+x+(y*m_dwWidth)); };
    BOOL    CanGoNorth( DWORD x , DWORD y ) const { return !(GetCell(x,y)&MAZE_WALL_NORTH); };
    BOOL    CanGoEast( DWORD x , DWORD y )  const { return !(GetCell(x,y)&MAZE_WALL_EAST); };
    BOOL    CanGoSouth( DWORD x , DWORD y ) const { return !(GetCell(x,y)&MAZE_WALL_SOUTH); };
    BOOL    CanGoWest( DWORD x , DWORD y )  const { return !(GetCell(x,y)&MAZE_WALL_WEST); };

    // Get list of visible cells from the given position. Fills out the list pointed to
    // be plist, and stops if it blows maxlist. Returns number of visible cells
    DWORD   GetVisibleCells( const Vector2& pos , const Vector2& dir ,
                             float fov , MazeCellRef* plist , DWORD maxlist );

    // Get/set max view distance (used by GetVisibleCells)
    DWORD   GetMaxView() const { return m_dwMaxView; };
    void    SetMaxView() { m_dwMaxView = m_dwMaxView; };

    BYTE*  m_pMaze;

protected:
    DWORD   m_dwWidth;
    DWORD   m_dwHeight;
    DWORD   m_dwSize;
    DWORD   m_dwSeed;
    DWORD   m_dwMaxView;
    CRandom m_Random;

    // Local types for the maze generation algorithm
    struct  CellNode
    {
        CellNode*   pPartition;
        CellNode*   pNext;
    };
    struct  WallNode
    {
        DWORD   dwX,dwY;
        DWORD   dwType;
    };

    // Local type for visibilty alg state
    struct  VisState
    {
        DWORD           dwPosX,dwPosY;      // Cell containing view position
        Vector2         vPos;               // View position
        Vector2         vDir;               // View direction
        BYTE*          pArray;             // Array in which cell visits are marked
        DWORD           dwMinX,dwMaxX;      // Extents to consider (also array bounds)
        DWORD           dwMinY,dwMaxY;
        DWORD           dwArrayPitch;       // 'Pitch' of array (width)
        MazeCellRef**   ppVisList;          // Pointer to vis list pointer
        DWORD           dwMaxList;          // Maximum length of vis list
        DWORD           dwListLen;          // Current length of vis list
    };

    void    RecurseCheckCellVis( VisState& state , DWORD x , DWORD y , Vector2 left , Vector2 right );
    BYTE   ComputeVisFlags( const Vector2& dir , const Vector2& left , const Vector2& right , const Vector2& offset );

private:
    CMaze( const CMaze& );
    void operator=( const CMaze& );
};




#endif
