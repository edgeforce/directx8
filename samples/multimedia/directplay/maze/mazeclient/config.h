//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _CONFIG_H
#define _CONFIG_H



#define MICROSOFT_SERVER TEXT("DirectPlayMaze.rte.microsoft.com")

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct  Config
{
    BOOL    bConnectToMicrosoftSite;
    BOOL    bConnectToLocalServer;
    BOOL    bConnectToRemoteServer;
    DWORD   dwNetworkRetryDelay;
    BOOL    bShowFramerate;
    BOOL    bShowIndicators;
    BOOL    bDrawMiniMap;
    BOOL    bFullScreen;
    BOOL    bReflections;
    BOOL    bPrefer32Bit;
    BOOL    bFileLogging;
    TCHAR   szIPAddress[64];
};




#endif
