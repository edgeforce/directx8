//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _DPLAY8_SERVER_H
#define _DPLAY8_SERVER_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include "NetAbstract.h"

interface IDirectPlay8Server;




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class CDPlay8Server : public IOutboundServer
{
public:
    CDPlay8Server();

    HRESULT Start();
    void    Shutdown();
    void    SetServer( INetServer* pServer ) { m_pServer = pServer; };

    // From IOutboundServer
    virtual HRESULT SendPacket( DWORD dwTo, void* pData, DWORD dwSize, BOOL bGuaranteed, DWORD dwTimeout );
    virtual HRESULT GetConnectionInfo( DWORD dwID, TCHAR* strConnectionInfo );
    virtual HRESULT RejectClient( DWORD dwID, HRESULT hrReason );

protected:
    IDirectPlay8Server*     m_pDPlay;
    INetServer*             m_pServer;

    static HRESULT WINAPI StaticReceiveHandler( void *pvContext, DWORD dwMessageType, void *pvMessage );
    HRESULT WINAPI ReceiveHandler( void *pvContext, DWORD dwMessageType, void *pvMessage );
};




#endif
