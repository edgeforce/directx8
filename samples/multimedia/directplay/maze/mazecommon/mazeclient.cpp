//----------------------------------------------------------------------------
// File: mazecient.cpp
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
#include "MazeClient.h"
#include "Packets.h"
#include "DXUtil.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#define NORTH_ANGLE 0x8000
#define EAST_ANGLE  0xc000
#define SOUTH_ANGLE 0x0000
#define WEST_ANGLE  0x4000


extern BOOL g_bDisconnectNow;
extern BOOL g_bOutOfDateClient;



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CMazeClient::CMazeClient()
{
    ZeroMemory( m_pctCells, sizeof(m_pctCells) );
    m_hReady            = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_pNet              = NULL;
    m_bAutopilot        = FALSE;
    m_bEngageAutopilot  = FALSE;

    m_NetConfig.ubReliableRate = 0;
    m_NetConfig.wUpdateRate    = 150;
    m_NetConfig.wTimeout       = 150;
    m_NetConfig.dwMazeWidth    = 0;
    m_NetConfig.dwMazeHeight   = 0;
}





//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CMazeClient::~CMazeClient()
{
    CloseHandle( m_hReady );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeClient::Init()
{
    m_aCameraYaw            = 0;
    m_lSequence             = 0;
    m_fLastOutboundTime     = DXUtil_Timer( TIMER_GETAPPTIME );
    m_bHaveInputFocus       = TRUE;

    Reset();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeClient::Reset()
{
    m_dwNumPlayers = 0;
    m_bAutopilot   = FALSE;
    SetMazeReady( FALSE ); 

    ZeroMemory( m_pctCells, sizeof(m_pctCells) );

    ClientThing* pThing = m_Things;
    for( DWORD i = 0; i < MAX_THINGS; i++, pThing++ )
    {
        pThing->ID = 0;
        pThing->wCellX = pThing->wCellY = 0xffff;
        pThing->pNext = NULL;
    }

    return S_OK;
}





//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::Shutdown()
{
    // Destroy the maze
    SetMazeReady( FALSE );
    Reset();
    m_Maze.Empty();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::Update( FLOAT fElapsed )
{
    // Don't do anything until we get a server config packet
    if( !IsMazeReady() ) 
        return;

    if( m_bAutopilot )
        DoAutopilot( fElapsed );
    else
        DoManualPilot( fElapsed );

    // See if it's time to send a packet to the server with our updated coordinates
    FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
    if( (fCurTime - m_fLastOutboundTime)*1000.0f > m_NetConfig.wUpdateRate )
    {
        ClientPosPacket packet( m_vCameraPos.x, m_vCameraPos.z, m_aCameraYaw );

        // Sanity check position before we send it - constrain to be within the maze
        if( packet.fX < 0 )
            packet.fX = 0;
        else if( packet.fX >= m_Maze.GetWidth() )
            packet.fX = float(m_Maze.GetWidth())-0.1f;
        if( packet.fY < 0 )
            packet.fY = 0;
        else if( packet.fY >= m_Maze.GetHeight() )
            packet.fY = float(m_Maze.GetHeight())-0.1f;

//        printf( "Sending postion: (%0.2f,%0.2f) Angle=%0.2f\n"), packet.fX, packet.fY, float(m_aCameraYaw&0xffff)*TRIG_ANGLE_SCALE );
        SendPacket( &packet, sizeof(packet), FALSE, m_NetConfig.wTimeout );
        m_fLastOutboundTime = fCurTime;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::DoManualPilot( FLOAT fElapsed )
{
    // Check if we have the input focus
    if( !m_bHaveInputFocus )
        return;

    // Do rotations
    if( GetAsyncKeyState( VK_LEFT ) & 0x8000 )
        m_aCameraYaw += (DWORD) ((fElapsed*1000.0f) * 40.0f);
    if( GetAsyncKeyState( VK_RIGHT ) & 0x8000 )
        m_aCameraYaw -= (DWORD) ((fElapsed*1000.0f) * 40.0f);

    float e = fElapsed*1000.0f;

    // Compute new position based key input
    D3DXVECTOR3 pos = m_vCameraPos;
    if( GetAsyncKeyState( VK_UP ) & 0x8000 )
    {
        pos.x -= Sin(m_aCameraYaw) * 0.002f * e;
        pos.z += Cos(m_aCameraYaw) * 0.002f * e;
    }
        
    if( GetAsyncKeyState( VK_DOWN ) & 0x8000 )
    {
        pos.x += Sin(m_aCameraYaw) * 0.002f * e;
        pos.z -= Cos(m_aCameraYaw) * 0.002f * e;
    }

    m_vCameraPos = pos;    
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::DoAutopilot( FLOAT fElapsed )
{
    // While there is still time to use up...
    while( fElapsed )
    {
        // See if we need to turn
        if( m_aAutopilotTargetAngle != m_aCameraYaw )
        {
            SHORT diff = SHORT((m_aAutopilotTargetAngle - m_aCameraYaw)&TRIG_ANGLE_MASK);
            FLOAT fNeeded = abs(diff)/40.0f;
            if( fNeeded/1000.0f <= fElapsed )
            {
                m_aCameraYaw = m_aAutopilotTargetAngle;
                fElapsed -= fNeeded/1000.0f;
            }
            else
            {
                if( diff < 0 )
                    m_aCameraYaw -= (DWORD) ((fElapsed*1000.0f) * 40.0f);
                else
                    m_aCameraYaw += (DWORD) ((fElapsed*1000.0f) * 40.0f);
                fElapsed = 0;
            }
        }
        else
        {
            // Facing right way, so now compute distance to target
            D3DXVECTOR3 diff = m_vAutopilotTarget - m_vCameraPos;
            float fRange = float(sqrt((diff.x*diff.x)+(diff.z*diff.z)));

            // Are we there yet?
            if( fRange > 0 )
            {
                // No, so compute how long we'd need
                FLOAT fNeeded = fRange / 0.002f;

                // Do we have enough time this frame?
                if( fNeeded/1000.0f <= fElapsed )
                {
                    // Yes, so just snap us there
                    m_vCameraPos.x = m_vAutopilotTarget.x;
                    m_vCameraPos.z = m_vAutopilotTarget.z;
                    fElapsed -= fNeeded/1000.0f;
                }
                else
                {
                    // No, so move us as far as we can
                    m_vCameraPos.x -= Sin(m_aCameraYaw) * 0.002f * fElapsed*1000.0f;
                    m_vCameraPos.z += Cos(m_aCameraYaw) * 0.002f * fElapsed*1000.0f;
                    fElapsed = 0;
                }
            }
            else
            {
                // Reached target, so pick another
                PickAutopilotTarget();
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::EngageAutopilot( BOOL bEngage )
{
    m_bEngageAutopilot = bEngage;

    if( !IsMazeReady() ) 
        return;

    BOOL bPrevious = m_bAutopilot;
    m_bAutopilot = bEngage;

    // If we weren't on autopilot before and are are autopilot now then need to init autopilot
    if( m_bAutopilot && !bPrevious )
    {
        // First of all, snap us to the centre of the current cell
        int cellx = int(m_vCameraPos.x);
        int cellz = int(m_vCameraPos.z);
        m_vCameraPos.x = cellx + 0.5f;
        m_vCameraPos.z = cellz + 0.5f;

        // Ensure we're within the maze boundaries
        if( cellx < 0 ) m_vCameraPos.x = 0.5f;
        if( cellx >= int(m_Maze.GetWidth()) ) m_vCameraPos.x = m_Maze.GetWidth() - 0.5f;
        if( cellz < 0 ) m_vCameraPos.z = 0.5f;
        if( cellz >= int(m_Maze.GetHeight()) ) m_vCameraPos.z = m_Maze.GetHeight() - 0.5f;

        // Clear the visited array and stack
        ZeroMemory( m_AutopilotVisited, sizeof(m_AutopilotVisited) );
        m_AutopilotStack.Empty();

        // Pick the next target cell
        PickAutopilotTarget();
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::PickAutopilotTarget()
{
    // Get current cell and mark as visited
    DWORD currentx = DWORD(m_vCameraPos.x);
    DWORD currentz = DWORD(m_vCameraPos.z);
    m_AutopilotVisited[currentz][currentx] = 1;

    // Figure out which directions are allowed. We're allowed to go in any direction
    // where there isn't a wall in the way and that takes us to a cell we've visited before.
    BYTE cell = m_Maze.GetCell(currentx,currentz);
    ANGLE alloweddirs[5];
    DWORD dwAllowed = 0;

    if( !(cell & MAZE_WALL_NORTH) && !m_AutopilotVisited[currentz-1][currentx] )
        alloweddirs[dwAllowed++] = NORTH_ANGLE;
    if( !(cell & MAZE_WALL_WEST) && !m_AutopilotVisited[currentz][currentx-1] )
        alloweddirs[dwAllowed++] = WEST_ANGLE;
    if( !(cell & MAZE_WALL_EAST) && !m_AutopilotVisited[currentz][currentx+1] )
        alloweddirs[dwAllowed++] = EAST_ANGLE;
    if( !(cell & MAZE_WALL_SOUTH) && !m_AutopilotVisited[currentz+1][currentx] )
        alloweddirs[dwAllowed++] = SOUTH_ANGLE;
/*
    printf( "Walls: ") );
    if( (cell & MAZE_WALL_NORTH) )
        printf( "N ") );
    if( (cell & MAZE_WALL_WEST) )
        printf( "W ") );
    if( (cell & MAZE_WALL_EAST) )
        printf( "E ") );
    if( (cell & MAZE_WALL_SOUTH) )
        printf( "S ") );
    printf( "\n") );
*/

    // Is there anywhere to go?
    if( dwAllowed == 0 )
    {
        // Nope. Can we backtrack?
        if( m_AutopilotStack.GetCount() > 0 )
        {
            // Yes, so pop cell off the stack
            AutopilotCell   cell(m_AutopilotStack.Pop());
            m_vAutopilotTarget.x = float(cell.x) + 0.5f;
            m_vAutopilotTarget.z = float(cell.y) + 0.5f;

            if( cell.x < currentx )
                m_aAutopilotTargetAngle = WEST_ANGLE;
            else if( cell.x > currentx )
                m_aAutopilotTargetAngle = EAST_ANGLE;
            else if( cell.y > currentz )
                m_aAutopilotTargetAngle = SOUTH_ANGLE;
            else
                m_aAutopilotTargetAngle = NORTH_ANGLE;
        }
        else
        {
            // No, so we have explored entire maze and must start again
            ZeroMemory( m_AutopilotVisited, sizeof(m_AutopilotVisited) );
            m_AutopilotStack.Empty();
            PickAutopilotTarget();
        }
    }
    else
    {
        // See if we can continue in current direction
        BOOL bPossible = FALSE;
        for( DWORD i = 0; i < dwAllowed; i++ )
        {
            if( alloweddirs[i] == m_aCameraYaw )
            {
                bPossible = TRUE;
                break;
            }
        }

        // If it's allowed to go forward, then have 1 in 2 chance of doing that anyway, otherwise pick randomly from
        // available alternatives
        if( bPossible && (rand() & 0x1000) )
            m_aAutopilotTargetAngle = m_aCameraYaw;
        else
            m_aAutopilotTargetAngle = alloweddirs[ (rand() % (dwAllowed<<3) ) >>3 ];

        m_vAutopilotTarget.z = float(currentz) + 0.5f;
        m_vAutopilotTarget.x = float(currentx) + 0.5f;

        switch( m_aAutopilotTargetAngle )
        {
            case SOUTH_ANGLE:
                m_vAutopilotTarget.z += 1.0f;
                break;

            case WEST_ANGLE:
                m_vAutopilotTarget.x -= 1.0f;
                break;

            case EAST_ANGLE:
                m_vAutopilotTarget.x += 1.0f;
                break;

            case NORTH_ANGLE:
                m_vAutopilotTarget.z -= 1.0f;
                break;
        }

        // Push current cell onto stack
        m_AutopilotStack.Push( AutopilotCell(BYTE(currentx),BYTE(currentz)) );
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMazeClient::OnPacket( DWORD dwFrom, void* dwData, DWORD dwSize )
{
    HRESULT hr;

    ServerPacket* pPacket = (ServerPacket*)dwData;
    switch( pPacket->wType )
    {
        case PACKETTYPE_SERVER_CONFIG:
        {
            if( dwSize != sizeof(ServerConfigPacket) )
            {
                g_bDisconnectNow = TRUE;
                g_bOutOfDateClient = TRUE;
                ConsolePrintf( LINE_LOG, TEXT("Disconnected because MazeClient is out of date.") );
                ConsolePrintf( LINE_LOG, TEXT("Please get updated version") );
                ConsolePrintf( LINE_LOG, TEXT("from http://msdn.microsoft.com/directx/") );
                break;
            }

            m_NetConfig = ((ServerConfigPacket*)pPacket)->Config;

            ConsolePrintf( LINE_LOG, "Got MazeServer config settings" );
            ConsolePrintf( LINE_LOG, "Maze Size=(%d,%d) ReliableRate=%d%%", 
                     m_NetConfig.dwMazeWidth, m_NetConfig.dwMazeHeight, 
                     DWORD(m_NetConfig.ubReliableRate) );
            ConsolePrintf( LINE_LOG, "UpdateRate=%dms Timeout=%d", 
                     m_NetConfig.wUpdateRate, m_NetConfig.wTimeout );

            // The client expects the server to send a ServerConfigPacket packet first, 
            // then the client sends a ClientVersionPacket, and then the server sends a 
            // ServerAckVersionPacket packet and the game begins
            ClientVersionPacket packet( MAZE_CLIENT_VERSION );
            SendPacket( &packet, sizeof(packet), TRUE, 0 );
            break;
        }

        case PACKETTYPE_SERVER_ACKVERSION:
        {
            if( dwSize != sizeof(ServerAckVersionPacket) )
            {
                g_bDisconnectNow = TRUE;
                g_bOutOfDateClient = TRUE;
                ConsolePrintf( LINE_LOG, TEXT("Disconnected because MazeClient is out of date.") );
                ConsolePrintf( LINE_LOG, TEXT("Please get updated version") );
                ConsolePrintf( LINE_LOG, TEXT("from http://msdn.microsoft.com/directx/") );
                break;
            }

            ServerAckVersionPacket* pAckVersionPacket = (ServerAckVersionPacket*)pPacket;

            // Record the dpnid that the server uses for to talk to us. 
            // This is just done so that we can record this number in the 
            // logs to help match server side logs with client side logs.
            m_dwLocalClientID = pAckVersionPacket->dwClientID;

            ConsolePrintf( LINE_LOG, "Server assigned ID: 0x%0.8x", m_dwLocalClientID );
            ConsolePrintf( LINE_LOG, "Server accepted client version" );

            hr = m_Maze.Init( m_NetConfig.dwMazeWidth, 
                              m_NetConfig.dwMazeHeight, DEFAULT_SEED );
            if( FAILED(hr) )
                DXTRACE_ERR( TEXT("Init"), hr );

            m_Rand.Reset( DEFAULT_SEED );

            // Set random start location
            m_vCameraPos = D3DXVECTOR3( rand() % m_Maze.GetWidth() + 0.5f, 0.5, 
                                        rand() % m_Maze.GetHeight() + 0.5f );

            SetMazeReady( TRUE );

            EngageAutopilot( m_bEngageAutopilot );
            break;
        }

        case PACKETTYPE_SERVER_ACKPOS:
            if( dwSize < sizeof(ServerAckPacket) ||
                dwSize != sizeof(ServerAckPacket) + sizeof(ServerThingDataChunk)*((ServerAckPacket*)pPacket)->wThingCount )
            {
                g_bDisconnectNow = TRUE;
                g_bOutOfDateClient = TRUE;
                ConsolePrintf( LINE_LOG, TEXT("Disconnected because MazeClient is out of date.") );
                ConsolePrintf( LINE_LOG, TEXT("Please get updated version") );
                ConsolePrintf( LINE_LOG, TEXT("from http://msdn.microsoft.com/directx/") );
                break;
            }

            m_dwNumPlayers = ((ServerAckPacket*)pPacket)->wPlayerCount;

            if( ((ServerAckPacket*)pPacket)->wThingCount )
                HandleThingsInAckPacket( (ServerAckPacket*)pPacket );
            break;

        default:
            ConsolePrintf( LINE_LOG, TEXT("Received unknown %d byte packet from server"), dwSize );
            break;
    };

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::OnSessionLost( DWORD dwReason )
{
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::SendPacket( ClientPacket* pPacket, DWORD dwSize, 
                              BOOL bGuaranteed, DWORD dwTimeout )
{
    pPacket->lSequence = m_lSequence;
    pPacket->dwTimestamp = (DWORD) (DXUtil_Timer( TIMER_GETAPPTIME ) * 1000.0f);

    InterlockedIncrement(&m_lSequence);

    if( m_NetConfig.ubReliableRate > m_NetRandom.Get( 100 ) )
        bGuaranteed = TRUE;

    m_pNet->SendPacket( pPacket, dwSize, bGuaranteed, dwTimeout );
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
DWORD CMazeClient::GetRoundTripLatencyMS()
{
    return m_pNet->GetRoundTripLatencyMS();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
DWORD CMazeClient::GetThroughputBPS()
{
    return m_pNet->GetThroughputBPS();
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::HandleThingsInAckPacket( ServerAckPacket* pPacket )
{
    ServerThingDataChunk* pChunk = (ServerThingDataChunk*)(pPacket+1);

    if( !IsMazeReady() ) 
        return;

    // Lock the world database
    LockWorld();

    // Loop though the thing chunks
    for( DWORD count = pPacket->wThingCount; count; count--, pChunk++ )
    {
        // Get the thing we think this is
        ClientThing* pThing = &m_Things[pChunk->ID & THING_SLOT_MASK];

        // Does the ID match the one we have?
        if( pThing->ID != pChunk->ID )
        {
            // No, so the thing we have needs to be deleted (server reused the same slot
            // number, so old thing must be toast)
            RemoveThingFromCells( pThing );

            // Set the ID to the new ID
            pThing->ID = pChunk->ID;
            pThing->wCellX = WORD(pChunk->fX);
            pThing->wCellY = WORD(pChunk->fY);

            // Insert into the appropriate cell list
            AddThingToCells( pThing );
        }
        else
        {
            // Yes, compute the new cell coordinates
            DWORD newcellx = DWORD(pChunk->fX);
            DWORD newcelly = DWORD(pChunk->fY);

            // Are they the same as the ones we already have?
            if( newcellx != pThing->wCellX || newcelly != pThing->wCellY )
            {
                // No, so need to remove from old cell and add to new one
                RemoveThingFromCells( pThing );
                pThing->wCellX = WORD(newcellx);
                pThing->wCellY = WORD(newcelly);
                AddThingToCells( pThing );
            }
        }

        // Update timestamp and position
        pThing->vPos.x = pChunk->fX;
        pThing->vPos.y = 0.5f;
        pThing->vPos.z = pChunk->fY;
        pThing->fLastValidTime = DXUtil_Timer( TIMER_GETAPPTIME );
    }

    // Unlock world database
    UnlockWorld();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::AddThingToCells( ClientThing* pThing )
{
    if( pThing->wCellX == 0xffff )
        return;

    if( FALSE == IsThingInCell( pThing->wCellX, pThing->wCellY, pThing ) )
    {
        ClientThing** ppCell = &m_pctCells[pThing->wCellY][pThing->wCellX];
        pThing->pNext = *ppCell;
        *ppCell = pThing;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CMazeClient::RemoveThingFromCells( ClientThing* pThing )
{
    if( pThing->wCellX == 0xffff )
        return;

    ClientThing** ppCell   = &m_pctCells[pThing->wCellY][pThing->wCellX];
    ClientThing* pCur      = *ppCell;
    ClientThing* pPrev     = NULL;
    while( pCur )
    {
        if( pCur == pThing )
        {
            pCur = pThing->pNext;

            // Found pThing, so remove pThing from the m_pctCells linked list
            if( pPrev )
                pPrev->pNext = pThing->pNext;
            else
                *ppCell = pThing->pNext;
            pThing->pNext = NULL;

            // Update pThing so that it is marked as removed
            pThing->wCellX = pThing->wCellY = 0xffff;

            // Continue searching, and remove any other instances of 
            // pThing from list (there shouldn't be, however)
        }
        else
        {
            pPrev = pCur;
            pCur  = pCur->pNext;
        }
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IEngineThing* CMazeClient::GetFirstThingInCell( DWORD x, DWORD z )
{
    if( !IsMazeReady() ) 
        return NULL;

    FLOAT fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );

    // Remove any things which are out of date (since they're probably not really in this
    // cell any more, but the server has just stopped telling us about them)
    ClientThing** ppCell    = &m_pctCells[z][x];
    ClientThing* pCur       = m_pctCells[z][x];
    ClientThing* pPrev      = NULL;
    ClientThing* pThing     = NULL;
    while( pCur )
    {
        // Too old?
        if( (fCurTime - pCur->fLastValidTime) > 5.0f )
        {
            pThing = pCur;
            pCur = pCur->pNext;

            // pThing is too old, so remove pThing from the m_pctCells linked list
            if( pPrev )
                pPrev->pNext = pThing->pNext;
            else
                *ppCell = pThing->pNext;
            pThing->pNext = NULL;

            // Update pThing so that it is marked as removed
            pThing->wCellX = pThing->wCellY = 0xffff;

            // Continue searching, and remove any other old instances from list 
        }
        else
        {
            pPrev = pCur;
            pCur  = pCur->pNext;
        }
    }

    // Now return first remaining thing in the cell (if any)
    return m_pctCells[z][x];
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CMazeClient::IsThingInCell( DWORD wCellX, DWORD wCellY, ClientThing* pThing )
{
    ClientThing* pThingTmp = m_pctCells[wCellY][wCellX];
    while( pThingTmp )
    {
        if( pThingTmp == pThing )
            return TRUE;

        pThingTmp = pThingTmp->pNext;
    }

    return FALSE;
}

