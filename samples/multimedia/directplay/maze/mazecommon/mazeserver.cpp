//----------------------------------------------------------------------------
// File: mazeserver.cpp
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
#include <mmsystem.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <dxerr8.h>
#include "DXUtil.h"
#include "MazeServer.h"
#include "Packets.h"
#include "Maze.h"
#include <malloc.h>


extern BOOL g_bLocalLoopback;


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CMazeServer::CMazeServer()
{
    m_dwPlayerCount         = 0;
    m_dwActiveThreadCount   = 0;
    m_dwServerReliableRate  = 15;
    m_dwServerTimeout       = 150;
    m_dwLogLevel            = 2;
    m_pMaze                 = NULL;

    m_ClientNetConfig.ubReliableRate = 15;
    m_ClientNetConfig.wUpdateRate    = 150;
    m_ClientNetConfig.wTimeout       = 150;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeServer::Init( const CMaze* pMaze )
{
    m_pMaze = pMaze;
    if( m_pMaze == NULL )
        return DXTRACE_ERR( TEXT("Param"), E_FAIL );

    // Grab height and width of maze
    m_dwWidth = m_pMaze->GetWidth();
    m_dwHeight = m_pMaze->GetHeight();

    m_ClientNetConfig.dwMazeWidth  = m_dwWidth;
    m_ClientNetConfig.dwMazeHeight = m_dwHeight;

    // Validate size. Must be a power-of-2 times LOCK_GRID_SIZE. Compute the shifts.
    if( m_dwWidth > SERVER_MAX_WIDTH || m_dwHeight > SERVER_MAX_HEIGHT )
        return DXTRACE_ERR( TEXT("Maze height and width need to be less than 128"), E_INVALIDARG );
    if( (m_dwWidth % LOCK_GRID_SIZE) != 0 || (m_dwHeight % LOCK_GRID_SIZE) != 0 )
        return DXTRACE_ERR( TEXT("Maze height and width need to be divisable by 16"), E_INVALIDARG );

    DWORD scale = m_dwWidth / LOCK_GRID_SIZE;
    m_dwMazeXShift = 0;
    while ( (scale >>= 1) )
        m_dwMazeXShift++;

    scale = m_dwHeight / LOCK_GRID_SIZE;
    m_dwMazeYShift = 0;
    while ( (scale >>= 1) )
        m_dwMazeYShift++;

    if( ((DWORD(LOCK_GRID_SIZE) << m_dwMazeXShift) != m_dwWidth) ||
        ((DWORD(LOCK_GRID_SIZE) << m_dwMazeYShift) != m_dwHeight) )
        return DXTRACE_ERR( TEXT("Maze height and width need to be power of 2"), E_INVALIDARG );

    // Initialise the thing list
    ZeroMemory( m_Things, sizeof(m_Things) );
    m_pFirstActiveThing = NULL;
    m_pFirstFreeThing = m_Things;
    for( DWORD i = 1; i < MAX_THINGS-1; i++ )
    {
        m_Things[i].pNext = &m_Things[i+1];
        m_Things[i].pPrevious = &m_Things[i-1];
    }

    m_Things[0].pNext = &m_Things[1];
    m_Things[MAX_THINGS-1].pPrevious = &m_Things[MAX_THINGS-2];
    m_dwActiveThingCount = 0;
    m_dwThingUniqueValue = 0;

    // Initialise the cells
    ZeroMemory( m_Cells, sizeof(m_Cells) );
    ZeroMemory( &m_OffMapCell, sizeof(m_OffMapCell) );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::Shutdown()
{
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::LockRange( DWORD x1, DWORD y1, DWORD x2, DWORD y2 )
{
    m_LockGrid.LockRange( x1>>m_dwMazeXShift, y1>>m_dwMazeYShift ,
                          x2>>m_dwMazeXShift, y2>>m_dwMazeYShift );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::UnlockRange( DWORD x1, DWORD y1, DWORD x2, DWORD y2 )
{
    m_LockGrid.UnlockRange( x1>>m_dwMazeXShift, y1>>m_dwMazeYShift ,
                            x2>>m_dwMazeXShift, y2>>m_dwMazeYShift );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::LockCell( DWORD x, DWORD y )
{
    if( x == 0xffff )
        m_OffMapLock.Enter();
    else
        m_LockGrid.LockCell(x>>m_dwMazeXShift,y>>m_dwMazeYShift);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::UnlockCell( DWORD x, DWORD y )
{
    if( x == 0xffff )
        m_OffMapLock.Leave();
    else
        m_LockGrid.UnlockCell(x>>m_dwMazeXShift,y>>m_dwMazeYShift);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::LockCellPair( DWORD x1, DWORD y1, DWORD x2, DWORD y2 )
{
    if( x1 == x2 && y1 == y2 )
    {
        LockCell( x1, y1 );
        return;
    }

    DWORD x1shift = x1>>m_dwMazeXShift;
    DWORD x2shift = x2>>m_dwMazeXShift;
    DWORD y1shift = y1>>m_dwMazeYShift;
    DWORD y2shift = y2>>m_dwMazeYShift;

    if( x1 == 0xffff )
    {
        m_OffMapLock.Enter();
        m_LockGrid.LockCell(x2shift,y2shift);
    }
    else if( x2 == 0xffff )
    {
        m_OffMapLock.Enter();
        m_LockGrid.LockCell(x1shift,y1shift);
    }
    else if( ((y1shift <= y2shift)&&(x1shift <= x2shift)) || 
             ((y1shift <  y2shift)&&(x1shift >  x2shift)) )
    {
        m_LockGrid.LockCell(x1shift,y1shift);
        m_LockGrid.LockCell(x2shift,y2shift);
    }
    else
    {
        m_LockGrid.LockCell(x2shift,y2shift);
        m_LockGrid.LockCell(x1shift,y1shift);
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::UnlockCellPair( DWORD x1, DWORD y1, DWORD x2, DWORD y2 )
{
    if( x1 == x2 && y1 == y2 )
    {
        UnlockCell( x1, y1 );
        return;
    }

    DWORD x1shift = x1>>m_dwMazeXShift;
    DWORD x2shift = x2>>m_dwMazeXShift;
    DWORD y1shift = y1>>m_dwMazeYShift;
    DWORD y2shift = y2>>m_dwMazeYShift;

    if( x1 == 0xffff )
    {
        m_LockGrid.UnlockCell(x2shift,y2shift);
        m_OffMapLock.Leave();
    }
    else if( x2 == 0xffff )
    {
        m_LockGrid.UnlockCell(x1shift,y1shift);
        m_OffMapLock.Leave();
    }
    else if( ((y1shift <= y2shift)&&(x1shift <= x2shift)) || 
             ((y1shift <  y2shift)&&(x1shift >  x2shift)) )
    {
        m_LockGrid.UnlockCell(x2shift,y2shift);
        m_LockGrid.UnlockCell(x1shift,y1shift);
    }
    else
    {
        m_LockGrid.UnlockCell(x1shift,y1shift);
        m_LockGrid.UnlockCell(x2shift,y2shift);
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::OnAddConnection( DWORD id )
{
    m_AddRemoveLock.Enter();

    // Increment our count of players
    m_dwPlayerCount++;
    if( m_dwLogLevel > 0 )
    {
        ConsolePrintf( LINE_LOG, TEXT("Adding player DPNID %0.8x"), id );
        ConsolePrintf( LINE_LOG, TEXT("Players connected = %d"), m_dwPlayerCount );
    }

    if( m_dwPlayerCount > m_dwPeakPlayerCount )
        m_dwPeakPlayerCount = m_dwPlayerCount;

    // Create a thing for this player
    ServerThing* pThing = CreateThing();
    if( pThing == NULL )
    {
        ConsolePrintf( LINE_LOG, TEXT("ERROR! Unable to create new thing for player!") );
        DXTRACE_ERR( TEXT("CreateThing"), E_FAIL );
        m_AddRemoveLock.Leave();
        return;
    }

    // Store that pointer as local player data
    SetThingForID( id, pThing );

    // Grab net config into to send to client
    m_ClientNetConfigLock.Enter();
    ServerConfigPacket packet( m_ClientNetConfig );
    m_ClientNetConfigLock.Leave();

    // Send it
    SendPacket( id, &packet, sizeof(packet), TRUE, 0 );

    m_AddRemoveLock.Leave();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::OnRemoveConnection( DWORD id )
{
    m_AddRemoveLock.Enter();

    // Decrement count of players
    m_dwPlayerCount--;

    if( m_dwLogLevel > 0 )
    {
        ConsolePrintf( LINE_LOG, TEXT("Removing player DPNID %0.8x"), id );
        ConsolePrintf( LINE_LOG, TEXT("Players connected = %d"), m_dwPlayerCount );
    }

    // Find thing for this player
    ServerThing* pThing = GetThingForID( id );
    if( pThing != NULL )
    {
        // Destroy it
        RemoveThingID( pThing );
        DestroyThing( pThing );
    }

    m_AddRemoveLock.Leave();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeServer::OnPacket( DWORD dwFrom, void* pData, DWORD size )
{
    InterlockedIncrement( (LONG*)&m_dwActiveThreadCount );

    ClientPacket* pClientPack = (ClientPacket*)pData;
    switch( pClientPack->wType )
    {
        case PACKETTYPE_CLIENT_POS:
            if( size == sizeof(ClientPosPacket) )
                HandleClientPosPacket( dwFrom, (ClientPosPacket*)pClientPack );
            else
                m_pNet->RejectClient( dwFrom, DISCONNNECT_REASON_CLIENT_OUT_OF_DATE );

            break;

        case PACKETTYPE_CLIENT_VERSION:
            if( size == sizeof(ClientVersionPacket) )
                HandleClientVersionPacket( dwFrom, (ClientVersionPacket*)pClientPack );
            else
                m_pNet->RejectClient( dwFrom, DISCONNNECT_REASON_CLIENT_OUT_OF_DATE );
            break;

        default:
            HandleUnknownPacket( dwFrom, pClientPack, size );
            break;
    }

    InterlockedDecrement( (LONG*)&m_dwActiveThreadCount );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::OnSessionLost( DWORD dwReason )
{
    ConsolePrintf( LINE_LOG, TEXT("ERROR! Session was lost") );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
ServerThing* CMazeServer::CreateThing()
{
    m_ThingListLock.Enter();

    // Grab first free thing in the list
    ServerThing* pThing = m_pFirstFreeThing;

    if( pThing )
    {
        LockThing( pThing );

        // Got one, so remove it from the free list
        if( pThing->pPrevious )
            pThing->pPrevious->pNext = pThing->pNext;
        if( pThing->pNext )
            pThing->pNext->pPrevious = pThing->pPrevious;
        m_pFirstFreeThing = pThing->pNext;

        // Add it to the active list
        if( m_pFirstActiveThing )
            m_pFirstActiveThing->pPrevious = pThing;
        pThing->pNext = m_pFirstActiveThing;
        pThing->pPrevious = NULL;
        m_pFirstActiveThing = pThing;

        // Update count of things
        m_dwActiveThingCount++;

        // Generate the ID for this thing
        m_dwThingUniqueValue++;
        pThing->ID = (pThing-m_Things)|(m_dwThingUniqueValue<<THING_SLOT_BITS);

        pThing->pNextInIDHashBucket = NULL;
        pThing->NetID = 0;

        // Insert into the "off-map" cell
        pThing->fPosX = pThing->fPosY = -1;
        pThing->wCellX = pThing->wCellY = 0xffff;
        m_OffMapLock.Enter();
        pThing->pNextInCell = m_OffMapCell.pFirstThing;
        m_OffMapCell.pFirstThing = pThing;
        m_OffMapLock.Leave();

        // Mark as active
        pThing->bActive = TRUE;

        UnlockThing( pThing );
    }

    m_ThingListLock.Leave();

    return pThing;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::DestroyThing( ServerThing* pThing )
{
    m_ThingListLock.Enter();
    LockThing( pThing );

    // Remove the thing from its cell
    RemoveThingFromCell( pThing );

    // Mark as inactive
    pThing->bActive = FALSE;

    // Remove thing from active list
    if( pThing->pPrevious )
        pThing->pPrevious->pNext = pThing->pNext;
    if( pThing->pNext )
        pThing->pNext->pPrevious = pThing->pPrevious;

    if( m_pFirstActiveThing == pThing )
        m_pFirstActiveThing = pThing->pNext;

    // Add it to the free list
    if( m_pFirstFreeThing )
        m_pFirstFreeThing->pPrevious = pThing;
    pThing->pNext = m_pFirstFreeThing;
    pThing->pPrevious = NULL;
    m_pFirstFreeThing = pThing;

    // Update count of things
    m_dwActiveThingCount--;

    UnlockThing( pThing );
    m_ThingListLock.Leave();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::RemoveThingFromCell( ServerThing* pThing )
{
    // Lock the thing
    LockThing( pThing );

    // Lock the cell the thing is in
    ServerCell* pCell;
    if( pThing->wCellX == 0xffff )
    {
        m_OffMapLock.Enter();
        pCell = &m_OffMapCell;
    }
    else
    {
        LockCell( pThing->wCellX, pThing->wCellY );
        pCell = &m_Cells[pThing->wCellY][pThing->wCellX];
    }

    // Remove it from the cell
    ServerThing* pPt = pCell->pFirstThing;
    ServerThing* pPrev = NULL;
    while ( pPt )
    {
        if( pPt == pThing )
        {
            if( pPrev )
                pPrev->pNextInCell = pThing->pNextInCell;
            else
                pCell->pFirstThing = pThing->pNextInCell;

            pThing->pNextInCell = NULL;
            break;
        }
        pPrev = pPt;
        pPt = pPt->pNextInCell;
    }

    // Unlock the cell
    if( pThing->wCellX == 0xffff )
        m_OffMapLock.Leave();
    else
        UnlockCell( pThing->wCellX, pThing->wCellY );

    // Unlock the thing
    UnlockThing( pThing );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::UnsafeRemoveThingFromCell( ServerThing* pThing )
{
    ServerCell* pCell = GetCell( pThing );
    ServerThing* pPt  = pCell->pFirstThing;
    ServerThing* pPrev = NULL;
    while ( pPt )
    {
        if( pPt == pThing )
        {
            if( pPrev )
                pPrev->pNextInCell = pThing->pNextInCell;
            else
                pCell->pFirstThing = pThing->pNextInCell;
            pThing->pNextInCell = NULL;
            break;
        }
        pPrev = pPt;
        pPt = pPt->pNextInCell;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::UnsafeAddThingToCell( ServerThing* pThing )
{
    ServerCell* pCell   = GetCell( pThing );
    pThing->pNextInCell = pCell->pFirstThing;
    pCell->pFirstThing = pThing;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::HandleClientPosPacket( DWORD dwFrom, ClientPosPacket* pClientPosPack )
{
    // Grab thing for this client and lock it
    ServerThing* pThing = GetThingForID( dwFrom );
    if( pThing == NULL )
    {
        if( m_dwLogLevel > 1 )
            ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Could not find data structure for this player"), pThing->NetID );
        return;
    }

    LockThing( pThing );

    if( FALSE == pThing->bAllow )
    {
        if( m_dwLogLevel > 0 )
            ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Got position packet from bad client.  Rejecting client"), pThing->NetID );

        m_pNet->RejectClient( dwFrom, DISCONNNECT_REASON_CLIENT_OUT_OF_DATE );
        UnlockThing( pThing );
        return;
    }

    // Compute the cell the thing should be in now
    DWORD newcellx = int(pClientPosPack->fX);
    DWORD newcelly = int(pClientPosPack->fY);
    DWORD oldcellx = pThing->wCellX;
    DWORD oldcelly = pThing->wCellY;

    // Have we moved cell?
    if( newcellx != oldcellx || newcelly != oldcelly )
    {
        // Yes, so lock the pair of cells in question
        LockCellPair( oldcellx, oldcelly, newcellx, newcelly );

        // Remove from old cell and add to new cell
        UnsafeRemoveThingFromCell( pThing );
        pThing->wCellX = WORD(newcellx); pThing->wCellY = WORD(newcelly);
        UnsafeAddThingToCell( pThing );

        // Unlock cells
        UnlockCellPair( oldcellx, oldcelly, newcellx, newcelly );
    }

    // Update thing position
    pThing->fPosX      = pClientPosPack->fX;
    pThing->fPosY      = pClientPosPack->fY;
    pThing->aCameraYaw = pClientPosPack->aCameraYaw;

    // Allocate space to build the reply packet, and fill in header 
    ServerAckPacket* pSvrAckPack = (ServerAckPacket*)_alloca( 256 );
    *pSvrAckPack = ServerAckPacket(pClientPosPack->lSequence,m_dwPlayerCount);
    pSvrAckPack->wThingCount = 0;
    ServerThingDataChunk* pChunk = (ServerThingDataChunk*)(pSvrAckPack+1);

    // Compute range of cells we're going to scan for things to send
    DWORD minx = (newcellx > 7) ? (newcellx - 7) : 0;
    DWORD miny = (newcelly > 7) ? (newcelly - 7) : 0;
    DWORD maxx = (newcellx+7 >= m_dwWidth) ? m_dwWidth-1 : newcellx+7;
    DWORD maxy = (newcelly+7 >= m_dwHeight) ? m_dwHeight-1 : newcelly+7;

    // Lock that range of cells
    LockRange( minx, miny, maxx, maxy );

    // Scan through the cells, tagging thing data onto the end of
    // our pSvrAckPacket until we run out of room
    for( DWORD y = miny; y <= maxy; y++ )
    {
        for( DWORD x = minx; x <= maxx; x++ )
        {
            ServerThing* pCellthing = m_Cells[y][x].pFirstThing;
            while ( pCellthing )
            {
                if( pCellthing != pThing )
                {
                    pChunk->ID         = pCellthing->ID;
                    pChunk->fX         = pCellthing->fPosX;
                    pChunk->fY         = pCellthing->fPosY;
                    pChunk->aCameraYaw = pCellthing->aCameraYaw;
                    pChunk++;
                    pSvrAckPack->wThingCount++;
                    if( pSvrAckPack->wThingCount == 15 )
                        goto Done;
                }
                pCellthing = pCellthing->pNextInCell;
            }
        }
    }
Done:

    // Unlock range of cells
    UnlockRange( minx, miny, maxx, maxy );

    if( m_dwLogLevel > 2 )
    {
        ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Position is (%0.2f,%0.2f)"), pThing->NetID, pThing->fPosX, pThing->fPosY );
    }
    else if( m_dwLogLevel == 2 )
    {
        FLOAT fTime = DXUtil_Timer( TIMER_GETAPPTIME );
        if( fTime - pThing->fLastDisplayTime > 600.0f )
        {
            ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Position is (%0.2f,%0.2f)"), pThing->NetID, pThing->fPosX, pThing->fPosY );
            pThing->fLastDisplayTime = fTime;
        }
    }

    // Unlock the thing
    UnlockThing( pThing );

    // Send acknowledgement back to client, including list of things
    DWORD acksize = sizeof(ServerAckPacket) + (pSvrAckPack->wThingCount * sizeof(ServerThingDataChunk));
    SendPacket( dwFrom, pSvrAckPack, acksize, FALSE, m_dwServerTimeout );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::HandleClientVersionPacket( DWORD dwFrom, ClientVersionPacket* pClientVersionPack )
{
    // Grab thing for this client and lock it
    ServerThing* pThing = GetThingForID( dwFrom );
    if( pThing == NULL )
        return;
    LockThing( pThing );

    // Record the version number 
    pThing->dwVersion = pClientVersionPack->dwVersion;

    if( g_bLocalLoopback )
        pThing->bAllow = TRUE;
    else
        pThing->bAllow = IsClientVersionSupported( pClientVersionPack->dwVersion );

    if( m_dwLogLevel > 0 )
        ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Client version=%d (%s)"), pThing->NetID, pThing->dwVersion, pThing->bAllow ? TEXT("Accepted") : TEXT("Rejected") );

    if( FALSE == pThing->bAllow )
    {
        if( m_dwLogLevel > 0 )
            ConsolePrintf( LINE_LOG, TEXT("DPNID %0.8x: Rejecting client"), pThing->NetID );

        m_pNet->RejectClient( dwFrom, DISCONNNECT_REASON_CLIENT_OUT_OF_DATE );
        UnlockThing( pThing );
        return;
    }

    // Unlock the thing
    UnlockThing( pThing );

    // Send acknowledgement to client that the client was either accepted or rejected
    ServerAckVersionPacket packet( pThing->bAllow, dwFrom );
    SendPacket( dwFrom, &packet, sizeof(packet), TRUE, 0 );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CMazeServer::IsClientVersionSupported( DWORD dwClientVersion )
{
    switch( dwClientVersion )
    {
        case 101: // v101 is supported
            return TRUE;
        default:
            return FALSE;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::HandleUnknownPacket( DWORD dwFrom, ClientPacket* pClientPack, DWORD size )
{
    if( m_dwLogLevel > 1 )
        ConsolePrintf( LINE_LOG, TEXT("ERROR! Unknown %d byte packet from player %0.8x"), size, dwFrom );

    m_pNet->RejectClient( dwFrom, DISCONNNECT_REASON_CLIENT_OUT_OF_DATE );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
DWORD   CMazeServer::IDHash( DWORD id )
{
    DWORD   hash = ((id) + (id>>8) + (id>>16) + (id>>24)) & (NUM_ID_HASH_BUCKETS-1);
    return hash;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::RemoveThingID( ServerThing* pThing )
{
    // Hash the ID to a bucket number
    DWORD   bucket = IDHash( pThing->NetID );

    // Lock that hash bucket
    const   DWORD   buckets_per_lock = NUM_ID_HASH_BUCKETS / NUM_ID_HASH_BUCKET_LOCKS;
    m_IDHashBucketLocks[bucket/buckets_per_lock].Enter();

    // Loop though things in bucket until we find the right one
    ServerThing* pPt = m_pstIDHashBucket[bucket];
    ServerThing* pPrev = NULL;
    while( pPt )
    {
        if( pPt == pThing )
            break;
        pPrev = pPt;
        pPt = pPt->pNextInIDHashBucket;
    }

    if( pPt )
    {
        if( pPrev )
            pPrev->pNextInIDHashBucket = pPt->pNextInIDHashBucket;
        else
            m_pstIDHashBucket[bucket] = pPt->pNextInIDHashBucket;
        pPt->pNextInIDHashBucket = NULL;
    }

    // Unlock the hash bucket
    m_IDHashBucketLocks[bucket/buckets_per_lock].Leave();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::SetThingForID( DWORD id, ServerThing* pThing )
{
    // Make sure this thing isn't added twice to the m_pstIDHashBucket[]
    // otherwise there will be a circular reference
    ServerThing* pSearch = GetThingForID( id );
    if( pSearch != NULL )
        return;

    // Hash the ID to a bucket number
    DWORD   bucket = IDHash( id );

    // Lock that hash bucket
    const   DWORD   buckets_per_lock = NUM_ID_HASH_BUCKETS / NUM_ID_HASH_BUCKET_LOCKS;
    m_IDHashBucketLocks[bucket/buckets_per_lock].Enter();

    // Add thing onto hash bucket chain
    pThing->pNextInIDHashBucket = m_pstIDHashBucket[bucket];
    m_pstIDHashBucket[bucket] = pThing;

    // Store net id in thing
    pThing->NetID = id;

    // Unlock the hash bucket
    m_IDHashBucketLocks[bucket/buckets_per_lock].Leave();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
ServerThing* CMazeServer::GetThingForID( DWORD id )
{
    // Hash the ID to a bucket number
    DWORD   bucket = IDHash( id );

    // Lock that hash bucket
    const   DWORD   buckets_per_lock = NUM_ID_HASH_BUCKETS / NUM_ID_HASH_BUCKET_LOCKS;
    m_IDHashBucketLocks[bucket/buckets_per_lock].Enter();

    // Loop though things in bucket until we find the right one
    ServerThing* pThing = m_pstIDHashBucket[bucket];
    while ( pThing )
    {
        if( pThing->NetID == id )
            break;
        pThing = pThing->pNextInIDHashBucket;
    }

    // Unlock the hash bucket
    m_IDHashBucketLocks[bucket/buckets_per_lock].Leave();

    // Return the thing we found (will be NULL if we couldn't find it)
    return pThing;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: calls DisplayConnectionInfo for each connection
//-----------------------------------------------------------------------------
void CMazeServer::DisplayAllInfo()
{
    ConsolePrintf( LINE_LOG, TEXT("Displaying connection info for all players") );
    PrintStats();
    if( m_pNet )
    {
        m_ThingListLock.Enter();
        ServerThing* pThing = m_pFirstActiveThing;
        while ( pThing )
        {
            DisplayConnectionInfo( pThing->NetID );
            Sleep( 250 );

            pThing = pThing->pNext;
        }
        m_ThingListLock.Leave();
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::PrintStats()
{
    ConsolePrintf( LINE_LOG, TEXT("Players online (not including server player): %d"), m_dwPlayerCount );
    ConsolePrintf( LINE_LOG, TEXT("Peak player count: %d"), m_dwPeakPlayerCount );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::DisplayConnectionInfo( DWORD dwID )
{
    TCHAR strInfo[5000];
    TCHAR* strEndOfLine;
    TCHAR* strStartOfLine;

    // Query the IOutboudNet for info about the connection to this user
    m_pNet->GetConnectionInfo( dwID, strInfo );

    ConsolePrintf( LINE_LOG, TEXT("Displaying connection info for %0.8x"), dwID );
    ConsolePrintf( LINE_LOG, TEXT("(Key: G=Guaranteed NG=Non-Guaranteed B=Bytes P=Packets)") );

    // Display each line seperately
    strStartOfLine = strInfo;
    while( TRUE )
    {
        strEndOfLine = strchr( strStartOfLine, '\n' );
        if( strEndOfLine == NULL )
            break;

        *strEndOfLine = 0;
        ConsolePrintf( LINE_LOG, strStartOfLine );
        strStartOfLine = strEndOfLine + 1;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeServer::SendPacket( DWORD to, void* pData, 
                                 DWORD size, BOOL reliable, DWORD dwTimeout )
{
    // Chance of forcing any packet to be delivered reliably
    if( m_Rand.Get( 100 ) < m_dwServerReliableRate )
        reliable = TRUE;

    return m_pNet->SendPacket( to, pData, size, reliable, dwTimeout );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::SendConfigPacketToAll( ServerConfigPacket* pPacket )
{
    // If we're up and running, then send this new information to all clients
    if( m_pNet )
    {
        m_ThingListLock.Enter();
        ServerThing* pThing = m_pFirstActiveThing;
        while ( pThing )
        {
            SendPacket( pThing->NetID, pPacket, sizeof(ServerConfigPacket), TRUE, 0 );
            pThing = pThing->pNext;
        }
        m_ThingListLock.Leave();
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::SetClientReliableRate( DWORD percent )
{
    // Update client config, and build packet containing that data
    m_ClientNetConfigLock.Enter();
    m_ClientNetConfig.ubReliableRate = BYTE(percent);
    ServerConfigPacket packet( m_ClientNetConfig );
    m_ClientNetConfigLock.Leave();

    SendConfigPacketToAll( &packet );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::SetClientUpdateRate( DWORD rate )
{
    // Update client config, and build packet containing that data
    m_ClientNetConfigLock.Enter();
    m_ClientNetConfig.wUpdateRate = WORD(rate);
    ServerConfigPacket  packet( m_ClientNetConfig );
    m_ClientNetConfigLock.Leave();

    SendConfigPacketToAll( &packet );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeServer::SetClientTimeout( DWORD timeout )
{
    // Update client config, and build packet containing that data
    m_ClientNetConfigLock.Enter();
    m_ClientNetConfig.wTimeout = WORD(timeout);
    ServerConfigPacket  packet( m_ClientNetConfig );
    m_ClientNetConfigLock.Leave();

    SendConfigPacketToAll( &packet );
}



