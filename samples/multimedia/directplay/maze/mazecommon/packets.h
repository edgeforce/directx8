//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _PACKETS_H
#define _PACKETS_H

#include "Trig.h"
#include "ThingID.h"

#pragma pack(push)
#pragma pack(1)




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
enum
{
    PACKETTYPE_SERVER_CONFIG,       // first packet sent 
    PACKETTYPE_CLIENT_VERSION,      // client responds to PACKETTYPE_SERVER_CONFIG w/ this 
    PACKETTYPE_SERVER_ACKVERSION,   // server then responds to PACKETTYPE_CLIENT_VERSION w/ this and game begins
    PACKETTYPE_CLIENT_POS,          // sent to server as client moves
    PACKETTYPE_SERVER_ACKPOS        // sent to client as server acks the PACKETTYPE_CLIENT_POS packets
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Base class for packets sent from client to server
//-----------------------------------------------------------------------------
struct  ClientPacket
{
    ClientPacket();
    ClientPacket( WORD type ) :
        wType(type) {};

    WORD    wType;
    LONG    lSequence;
    DWORD   dwTimestamp;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  ClientPosPacket : public ClientPacket
{
    ClientPosPacket();
    ClientPosPacket( float x , float y, ANGLE cameraYaw )
        : ClientPacket( PACKETTYPE_CLIENT_POS ) , fX(x) , fY(y), aCameraYaw(cameraYaw) {};

    float   fX,fY;  
    ANGLE   aCameraYaw;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  ClientVersionPacket : public ClientPacket
{
    ClientVersionPacket();
    ClientVersionPacket( DWORD version )
        : ClientPacket( PACKETTYPE_CLIENT_VERSION ) , dwVersion(version) {};

    DWORD dwVersion;  
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Structure containing client net configuration data. 
//       The server sends this to clients
//-----------------------------------------------------------------------------
struct  ClientNetConfig
{
    DWORD dwMazeWidth;
    DWORD dwMazeHeight;
    BYTE  ubReliableRate;  // Percentage of packets to be transmitted reliably
    WORD  wUpdateRate;
    WORD  wTimeout;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Base class for packets sent from server to client
//-----------------------------------------------------------------------------
struct  ServerPacket
{
    ServerPacket();
    ServerPacket( WORD type ) : wType(type) {};

    WORD    wType;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Configuration data send from server to client
//-----------------------------------------------------------------------------
struct  ServerAckVersionPacket : public ServerPacket
{
    ServerAckVersionPacket();
    ServerAckVersionPacket( BOOL accepted, DWORD clientID ) :
        ServerPacket(PACKETTYPE_SERVER_ACKVERSION), bAccepted(accepted), dwClientID(clientID) {};

    BOOL bAccepted;
    DWORD dwClientID;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Configuration data send from server to client
//-----------------------------------------------------------------------------
struct  ServerConfigPacket : public ServerPacket
{
    ServerConfigPacket();
    ServerConfigPacket( const ClientNetConfig& config ) :
        ServerPacket(PACKETTYPE_SERVER_CONFIG) , Config(config) {};

    ClientNetConfig Config;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Chunk of thing data a server send to a client
//-----------------------------------------------------------------------------
struct ServerThingDataChunk
{
    ThingID ID;
    float   fX;
    float   fY;
    ANGLE   aCameraYaw;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  ServerAckPacket : public ServerPacket
{
    ServerAckPacket();
    ServerAckPacket( LONG seq , DWORD playercount )
        : ServerPacket(PACKETTYPE_SERVER_ACKPOS), 
          lSequence(seq), 
          wPlayerCount(WORD(playercount)) {};

    LONG    lSequence;      // Sequence number we're replying to
    WORD    wPlayerCount;   // Count of total players on server
    WORD    wThingCount;    // Count of following ServerThingDataChunk structures
};


#pragma pack(pop)

#endif
