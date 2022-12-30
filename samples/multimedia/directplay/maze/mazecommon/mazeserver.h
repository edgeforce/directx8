//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _MAZESERVER_H
#define _MAZESERVER_H





//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include "NetAbstract.h"
#include "SyncObjects.h"
#include "ThingID.h"
#include "Random.h"
#include "Packets.h"
#include "Trig.h"

class   CMaze;
struct  ClientPosPacket;

#define SERVER_MAX_WIDTH    128
#define SERVER_MAX_HEIGHT   128
#define DEFAULT_MAZE_WIDTH  16
#define DEFAULT_MAZE_HEIGHT 16
#define DEFAULT_SEED        314159
#define LOCK_GRID_SIZE      16




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct ServerThing
{
    ThingID         ID;                     // Thing ID
    DWORD           NetID;                  // NetID for owning player (0 if none)
    DWORD           dwVersion;              // Version of the owning player
    BOOL            bAllow;                 // If FALSE, then we should drop this player

    // Links for the various lists
    ServerThing*    pNext;                  // Free/active thing list (double link)
    ServerThing*    pPrevious;
    ServerThing*    pNextInCell;            // Cell list (single link)
    ServerThing*    pNextInIDHashBucket;    // ID hash bucket (single link)

    FLOAT           fLastDisplayTime;
    BOOL            bActive;

    float           fPosX;                  // Floating point position
    float           fPosY;
    WORD            wCellX;                 // Coordinates of the cell this thing is in
    WORD            wCellY;                 // or (0xffff,0xffff) for off-map
    ANGLE           aCameraYaw;

    DWORD           pad[5];
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct ServerCell
{
    ServerThing* pFirstThing;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class CMazeServer : public INetServer
{
public:
    CMazeServer();

    // From INetServer
    void    OnAddConnection( DWORD dwID );
    HRESULT OnPacket( DWORD from, void* pData, DWORD dwSize );
    void    OnRemoveConnection( DWORD dwID );
    void    OnSessionLost( DWORD dwReason );

    // Hook up to the network service
    void    SetOutboundServer( IOutboundServer* poutnet ) { m_pNet = poutnet; };

    // Initialisation - need to hook up a maze object
    HRESULT Init( const CMaze* pmaze );
    void    Shutdown();

    // Chance of server sending packets via reliable transport
    void    SetServerReliableRate( DWORD percent ) { m_dwServerReliableRate = percent; };
    DWORD   GetServerReliableRate() const { return m_dwServerReliableRate; };

    // Timeout of server's packets
    void    SetServerTimeout( DWORD timeout ) { m_dwServerTimeout = timeout; };
    DWORD   GetServerTimeout() const { return m_dwServerTimeout; };

    // Change of client sending packets via reliable transport. Setting this causes the server
    // to propagate this setting to all currently connected clients
    void    SetClientReliableRate( DWORD percent );
    DWORD   GetClientReliableRate() const { return DWORD(m_ClientNetConfig.ubReliableRate); };

    // Change client update rate. Setting this causes the server to propagate this setting to all
    // currently connected clients
    void    SetClientUpdateRate( DWORD rate );
    DWORD   GetClientUpdateRate() const { return DWORD(m_ClientNetConfig.wUpdateRate); };

    // Change client timeout. Setting this causes the server to propagate this setting to all
    // currently connected clients
    void    SetClientTimeout( DWORD timeout );
    DWORD   GetClientTimeout() const { return DWORD(m_ClientNetConfig.wTimeout); };

    // Various commands
    void    DisplayConnectionInfo( DWORD dwID );
    void    DisplayAllInfo();
    void    PrintStats();

    void    SetLogLevel( DWORD dwLogLevel ) { m_dwLogLevel = dwLogLevel; }
    DWORD   GetLogLevel() { return m_dwLogLevel; }

protected:
    IOutboundServer* m_pNet;
    const CMaze*    m_pMaze;
    DWORD           m_dwLogLevel;
    DWORD           m_dwWidth;
    DWORD           m_dwHeight;

    CCriticalSection    m_AddRemoveLock;

    // A fixed sized grid of locks which we lay over the maze to control access to it
    // We demand that the maze dimensions are a power-of-2 times the dimensions of this
    // grid, and pre-store that power to allow fast translation
    CLockArray<LOCK_GRID_SIZE,LOCK_GRID_SIZE>   m_LockGrid;
    DWORD               m_dwMazeXShift;
    DWORD               m_dwMazeYShift;
    void                LockCell( DWORD x , DWORD y );
    void                UnlockCell( DWORD x , DWORD y );
    void                LockRange( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 );
    void                UnlockRange( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 );
    void                LockCellPair( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 );
    void                UnlockCellPair( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 );
    CCriticalSection    m_OffMapLock;

    // The thing lists
    ServerThing         m_Things[MAX_THINGS];
    ServerThing*        m_pFirstActiveThing;
    ServerThing*        m_pFirstFreeThing;
    DWORD               m_dwActiveThingCount;
    DWORD               m_dwThingUniqueValue;
    CCriticalSection    m_ThingListLock;

    // The thing locks
    enum { NUM_THING_LOCKS = 16 };
    CCriticalSection    m_ThingLocks[NUM_THING_LOCKS];
    void                LockThing( ServerThing* pThing ) { m_ThingLocks[((pThing-m_Things) & (NUM_THING_LOCKS-1))].Enter(); };
    void                UnlockThing( ServerThing* pThing ) { m_ThingLocks[((pThing-m_Things) & (NUM_THING_LOCKS-1))].Leave(); };

    ServerThing*        CreateThing();
    void                DestroyThing( ServerThing* pThing );

    // The cell array and the "off-map" cell.
    ServerCell          m_OffMapCell;
    ServerCell          m_Cells[SERVER_MAX_WIDTH][SERVER_MAX_HEIGHT];

    // Remove thing from its cell
    void    RemoveThingFromCell( ServerThing* pThing );

    // Unsafe versions of add/remove. Must have thing and cell locked when you call this
    void    UnsafeRemoveThingFromCell( ServerThing* pThing );
    void    UnsafeAddThingToCell( ServerThing* pThing );

    ServerCell* GetCell( ServerThing* pThing )
    {
        if ( pThing->wCellX == 0xffff )
            return &m_OffMapCell;
        else
            return &m_Cells[pThing->wCellY][pThing->wCellX];
    };

    void    HandleClientPosPacket( DWORD dwFrom, ClientPosPacket* pPacket );
    void    HandleClientVersionPacket( DWORD dwFrom, ClientVersionPacket* pClientVersionPack );
    void    HandleUnknownPacket( DWORD dwFrom, ClientPacket* pClientPack, DWORD size );

    BOOL    IsClientVersionSupported( DWORD dwClientVersion );

    DWORD   m_dwPlayerCount;
    DWORD   m_dwActiveThreadCount;
    DWORD   m_dwPeakPlayerCount;

    // Hashing DPIDs to ServerThing pointers
    void                SetThingForID( DWORD dwID, ServerThing* pThing );
    ServerThing*        GetThingForID( DWORD dwID );
    void                RemoveThingID( ServerThing* pThing );
    DWORD               IDHash( DWORD dwID );
    enum { NUM_ID_HASH_BUCKETS = 1024 };
    enum { NUM_ID_HASH_BUCKET_LOCKS = 16 };
    ServerThing*        m_pstIDHashBucket[NUM_ID_HASH_BUCKETS];
    CCriticalSection    m_IDHashBucketLocks[NUM_ID_HASH_BUCKET_LOCKS];

    // Random number generator
    CRandom     m_Rand;

    // Send packet wrapper
    HRESULT SendPacket( DWORD dwTo, void* pData, DWORD dwSize, BOOL bReliable, DWORD dwTimeout );
    void SendConfigPacketToAll( ServerConfigPacket* pPacket );

    // Network configuration parameters
    DWORD               m_dwServerReliableRate;
    DWORD               m_dwServerTimeout;
    ClientNetConfig     m_ClientNetConfig;
    CCriticalSection    m_ClientNetConfigLock;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: This function is called by the server to output informational text.
//       In the client it should probably just be a dummy function, in the server 
//       it should probably just spew out to the console
//-----------------------------------------------------------------------------
enum EnumBufferType { LINE_PROMPT, LINE_INPUT, LINE_LOG, LINE_CMD };
void ConsolePrintf( EnumBufferType enumBufferType, const TCHAR* fmt , ... );



#endif
