//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _MAZE_CLIENT_H
#define _MAZE_CLIENT_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include "DXUtil.h"
#include "Maze.h"
#include "IEngineThing.h"
#include "NetAbstract.h"
#include "Packets.h"
#include "Trig.h"
#include "SimpleStack.h"
#include "ThingID.h"
#include "Packets.h"
#include "MazeServer.h"

// The MAZE_CLIENT_VERSION should be rev'ed whenever the client exposes 
// new functionality that the server expects.  This number is sent to
// the server so the server can accept or reject the client based on its version.
#define MAZE_CLIENT_VERSION         101



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct ClientThing : public IEngineThing
{
public:
    ThingID         ID;
    ClientThing*    pNext;
    D3DXVECTOR3     vPos;
    WORD            wCellX;
    WORD            wCellY;
    FLOAT           fLastValidTime;

    // From IEngineThing
    virtual const D3DXVECTOR3&  GetPos() const { return vPos; };
    virtual IEngineThing*       GetNext() const { return (IEngineThing*)pNext; };
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#define MAZE_WIDTH  16
#define MAZE_HEIGHT 16
#define MAZE_SIZE   (MAZE_WIDTH*MAZE_HEIGHT)




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class CMazeClient : public INetClient
{
public:
    CMazeClient();
    ~CMazeClient();

    // INetClient
    virtual HRESULT OnPacket( DWORD from , void* data , DWORD size );
    virtual void    OnSessionLost( DWORD dwReason );

    // Connect an outbound network provider
    void            SetOutboundClient( IOutboundClient* poutnet ) { m_pNet = poutnet; };

    HRESULT         Init();
    HRESULT         Reset();
    void            Shutdown();
    void            Update( FLOAT elapsed );

    // Lock and unlock the world database
    void            LockWorld() { m_WorldLock.Enter(); };
    void            UnlockWorld() { m_WorldLock.Leave(); };

    // Lock and unlock the world database
    void            SetMazeReady( BOOL bReady ) { if( bReady ) SetEvent( m_hReady ); else ResetEvent( m_hReady ); };
    BOOL            IsMazeReady() { if( WaitForSingleObject( m_hReady, 0 ) == WAIT_OBJECT_0 ) return TRUE; else return FALSE; };

    // Get data useful for the engine (current position, etc. etc.)
    D3DXVECTOR3     GetCameraPos() const { return m_vCameraPos; };
    ANGLE           GetCameraYaw() const { return m_aCameraYaw; };
    DWORD           GetNumThings() const { return m_dwNumThings; };

    // Get first engine thing is a cell
    // NOTE: The engine must lock the world DB before traversing the cells
    IEngineThing*   GetFirstThingInCell( DWORD x , DWORD z );

    // The layout of the maze (engine needs this to draw)
    CMaze           m_Maze;

    // Get network stats
    DWORD           GetThroughputBPS();
    DWORD           GetRoundTripLatencyMS();
    DWORD           GetNumPlayers() const { return m_dwNumPlayers; };
    DWORD           GetLocalClientID() const { return m_dwLocalClientID; };

    // Autopilot
    void    EngageAutopilot( BOOL engage );
    BOOL    IsAutopilot() const { return m_bAutopilot; };

    // Set whether or not we have input focus
    void    SetInputFocus( BOOL havefocus ) { m_bHaveInputFocus = havefocus; };

protected:
    D3DXVECTOR3     m_vCameraPos;
    ANGLE           m_aCameraYaw;

    DWORD           m_dwNumThings;

    CRandom         m_Rand;

    IOutboundClient* m_pNet;

    FLOAT           m_fLastOutboundTime;
    LONG            m_lSequence;

    DWORD           m_dwNumPlayers;
    DWORD           m_dwLocalClientID;

    BOOL            m_bHaveInputFocus;

    void    SendPacket( ClientPacket* packet , DWORD size , BOOL guaranteed, DWORD dwTimeout );

    // Arrays of cells and things (this consitutes the world DB), with associated lock
    ClientThing*        m_pctCells[SERVER_MAX_HEIGHT][SERVER_MAX_WIDTH];
    ClientThing         m_Things[MAX_THINGS];
    CCriticalSection    m_WorldLock;
    HANDLE              m_hReady;

    // Autopilot stuff
    struct  AutopilotCell
    {
        AutopilotCell() {};
        AutopilotCell( BYTE X , BYTE Y ) : x(X),y(Y) {};
        BYTE   x,y;
    };
    SimpleStack<AutopilotCell,MAZE_SIZE>    m_AutopilotStack;
    BYTE                                   m_AutopilotVisited[SERVER_MAX_WIDTH][SERVER_MAX_HEIGHT];
    BOOL            m_bAutopilot;
    BOOL            m_bEngageAutopilot;
    D3DXVECTOR3     m_vAutopilotTarget;
    ANGLE           m_aAutopilotTargetAngle;

    void            DoAutopilot( FLOAT elapsed );
    void            DoManualPilot( FLOAT elapsed ); 
    void            PickAutopilotTarget();

    void    HandleThingsInAckPacket( ServerAckPacket* ppacket );

    void    AddThingToCells( ClientThing* pthing );
    void    RemoveThingFromCells( ClientThing* pthing );
    BOOL    IsThingInCell( DWORD wCellX, DWORD wCellY, ClientThing* pThing );

    ClientNetConfig     m_NetConfig;
    CRandom             m_NetRandom;
    BOOL                m_bDoneInit;

private:
    CMazeClient( const CMazeClient& );
    void operator=( const CMazeClient& );
};




#endif
