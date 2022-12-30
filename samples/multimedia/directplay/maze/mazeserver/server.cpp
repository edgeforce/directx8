//----------------------------------------------------------------------------
// File: server.cpp
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <DXErr8.h>
#include "DXUtil.h"
#include "server.h"
#include "StressMazeGUID.h"

#define DPMAZESERVER_PORT       2309
      



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CDPlay8Server::CDPlay8Server()
{
    m_pDPlay    = NULL;
    m_pServer   = NULL;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::Start()
{
    HRESULT hr;
    PDIRECTPLAY8ADDRESS   pDP8AddrLocal = NULL;
    DWORD dwPort;

    // Create DirectPlay object
    hr = CoCreateInstance( CLSID_DirectPlay8Server, NULL, 
                           CLSCTX_ALL, IID_IDirectPlay8Server,
                           (LPVOID*) &m_pDPlay );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("CoCreateInstance"), hr );
        goto LCleanup;
    }

    hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL, 
                           CLSCTX_ALL, IID_IDirectPlay8Address, 
                           (LPVOID*) &pDP8AddrLocal );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("CoCreateInstance"), hr );
        goto LCleanup;
    }

    hr = pDP8AddrLocal->SetSP( &CLSID_DP8SP_TCPIP );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("BuildLocalAddress"), hr );
        goto LCleanup;
    }

    dwPort = DPMAZESERVER_PORT;
    hr = pDP8AddrLocal->AddComponent( DPNA_KEY_PORT, 
                                      &dwPort, sizeof(dwPort),
                                      DPNA_DATATYPE_DWORD );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("BuildLocalAddress"), hr );
        goto LCleanup;
    }

    hr = m_pDPlay->Initialize( this, StaticReceiveHandler, 0 );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("Initialize"), hr );
        goto LCleanup;
    }

    DPN_APPLICATION_DESC dpnAppDesc;
    ZeroMemory( &dpnAppDesc, sizeof(DPN_APPLICATION_DESC) );
    dpnAppDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
    dpnAppDesc.dwFlags = DPNSESSION_CLIENT_SERVER;
    dpnAppDesc.guidApplication = StressMazeAppGUID;
    dpnAppDesc.pwszSessionName = L"StressMazeServer Session";

    // Set host player context to non-NULL so we can determine which player indication is 
    // the host's.
    hr = m_pDPlay->Host( &dpnAppDesc, &pDP8AddrLocal, 1, NULL, NULL, (void *) 1, 0  );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("Host"), hr );
        goto LCleanup;
    }

LCleanup:
    SAFE_RELEASE( pDP8AddrLocal );
    SAFE_RELEASE( pDP8AddrLocal );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CDPlay8Server::Shutdown()
{
    if( m_pDPlay != NULL )
        m_pDPlay->Close(0);

    SAFE_RELEASE( m_pDPlay );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::RejectClient( DWORD dwID, HRESULT hrReason )
{
    HRESULT hrRet = S_OK;

    if( m_pDPlay )
    {
        hrRet = m_pDPlay->DestroyClient( dwID, &hrReason, sizeof(hrReason), 0 );
    }

    return hrRet;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::StaticReceiveHandler( void *pvContext, DWORD dwMessageType, 
                                             void *pvMessage )
{
    CDPlay8Server* pThisObject = (CDPlay8Server*) pvContext;

    return pThisObject->ReceiveHandler( pvContext, dwMessageType, pvMessage );
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::ReceiveHandler( void *pvContext, DWORD dwMessageType, 
                                       void *pvMessage )
{
    switch( dwMessageType )
    {
        case DPN_MSGID_CREATE_PLAYER:
        {
            PDPNMSG_CREATE_PLAYER pCreatePlayer = (PDPNMSG_CREATE_PLAYER) pvMessage;

            if( pCreatePlayer->pvPlayerContext == NULL )
                m_pServer->OnAddConnection( pCreatePlayer->dpnidPlayer );
            break;
        }
        
        case DPN_MSGID_DESTROY_PLAYER:
        {
            PDPNMSG_DESTROY_PLAYER pDestroyPlayer = (PDPNMSG_DESTROY_PLAYER) pvMessage;

            if( pDestroyPlayer->pvPlayerContext == NULL )
                m_pServer->OnRemoveConnection( pDestroyPlayer->dpnidPlayer );
            break;
        }

        case DPN_MSGID_RECEIVE:
        {
            PDPNMSG_RECEIVE pRecvData = (PDPNMSG_RECEIVE) pvMessage;

            m_pServer->OnPacket( pRecvData->dpnidSender, pRecvData->pReceiveData, pRecvData->dwReceiveDataSize );
            break;
        }

        case DPN_MSGID_TERMINATE_SESSION:
        {
            m_pServer->OnSessionLost( DISCONNNECT_REASON_UNKNOWN );
            break;
        }
    }
    
    return DPN_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::SendPacket( DWORD dwTo, void* pData, 
                                   DWORD dwSize, BOOL bGuaranteed,
                                   DWORD dwTimeout )
{
    DPNHANDLE       hAsync;
    DPNHANDLE*      phAsync;
    DWORD           dwFlags = 0;
    DPN_BUFFER_DESC dpnBufferDesc;

    if( bGuaranteed )
    {
        // If we are guaranteed then we must specify
        // DPNSEND_NOCOMPLETE and pass in non-null for the 
        // pvAsyncContext
        dwFlags = DPNSEND_GUARANTEED;
    }
    else
    {
        // If we aren't guaranteed then we can
        // specify DPNSEND_NOCOMPLETE.  And when 
        // DPNSEND_NOCOMPLETE is on pvAsyncContext
        // must be NULL.
        dwFlags = DPNSEND_NOCOMPLETE;
    }
    //Must define an async handle for the SendTo call. 
	phAsync = &hAsync;

    dpnBufferDesc.dwBufferSize = dwSize;
    dpnBufferDesc.pBufferData = (PBYTE) pData;

    // DirectPlay will tell via the message handler 
    // if there are any severe errors, so ignore any errors 
    m_pDPlay->SendTo( dwTo, &dpnBufferDesc, 1, dwTimeout, NULL, phAsync, dwFlags );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Server::GetConnectionInfo( DWORD dwID, TCHAR* strConnectionInfo )
{
    HRESULT hr;

    // Call GetConnectionInfo and display results
    DPN_CONNECTION_INFO dpnConnectionInfo;
    ZeroMemory( &dpnConnectionInfo, sizeof(DPN_CONNECTION_INFO) );
    dpnConnectionInfo.dwSize = sizeof(DPN_CONNECTION_INFO);
    hr = m_pDPlay->GetConnectionInfo( dwID, &dpnConnectionInfo, 0 );

    if( SUCCEEDED(hr) )
    {
        DWORD dwHighPriMessages, dwHighPriBytes;
        DWORD dwNormalPriMessages, dwNormalPriBytes;
        DWORD dwLowPriMessages, dwLowPriBytes;

        hr = m_pDPlay->GetSendQueueInfo( dwID,
                                      &dwHighPriMessages, &dwHighPriBytes, 
                                      DPNGETSENDQUEUEINFO_PRIORITY_HIGH );

        hr = m_pDPlay->GetSendQueueInfo( dwID,
                                      &dwNormalPriMessages, &dwNormalPriBytes, 
                                      DPNGETSENDQUEUEINFO_PRIORITY_NORMAL );

        hr = m_pDPlay->GetSendQueueInfo( dwID,
                                      &dwLowPriMessages, &dwLowPriBytes, 
                                      DPNGETSENDQUEUEINFO_PRIORITY_LOW );

        _stprintf( strConnectionInfo, 
                   "     Round Trip Latency MS=%dms\n"                      \
                   "     Throughput BPS: Current=%d Peak=%d\n"              \
                                                                            \
                   "     Messages Received=%d\n"                            \
                                                                            \
                   "     Sent: GB=%d GP=%d NGB=%d NGP=%d\n"                 \
                   "     Received: GB=%d GP=%d NGB=%d NGP=%d\n"             \
                                                                            \
                   "     Messages Transmitted: HP=%d NP=%d LP=%d\n"         \
                   "     Messages Timed Out: HP=%d NP=%d LP=%d\n"           \
                                                                            \
                   "     Retried: GB=%d GP=%d\n"                            \
                   "     Dropped: NGB=%d NGP=%d\n"                          \
                                                                            \
                   "     Send Queue Messages: HP=%d NP=%d LP=%d\n"          \
                   "     Send Queue Bytes: HP=%d NP=%d LP=%d\n",            \
                                                                            \
                                                                            \
                   dpnConnectionInfo.dwRoundTripLatencyMS, 
                   dpnConnectionInfo.dwThroughputBPS, 
                   dpnConnectionInfo.dwPeakThroughputBPS,

                   dpnConnectionInfo.dwMessagesReceived,

                   dpnConnectionInfo.dwBytesSentGuaranteed,
                   dpnConnectionInfo.dwPacketsSentGuaranteed,
                   dpnConnectionInfo.dwBytesSentNonGuaranteed,
                   dpnConnectionInfo.dwPacketsSentNonGuaranteed,

                   dpnConnectionInfo.dwBytesReceivedGuaranteed,
                   dpnConnectionInfo.dwPacketsReceivedGuaranteed,
                   dpnConnectionInfo.dwBytesReceivedNonGuaranteed,
                   dpnConnectionInfo.dwPacketsReceivedNonGuaranteed,

                   dpnConnectionInfo.dwMessagesTransmittedHighPriority,
                   dpnConnectionInfo.dwMessagesTransmittedNormalPriority,
                   dpnConnectionInfo.dwMessagesTransmittedLowPriority,

                   dpnConnectionInfo.dwMessagesTimedOutHighPriority,
                   dpnConnectionInfo.dwMessagesTimedOutNormalPriority,
                   dpnConnectionInfo.dwMessagesTimedOutLowPriority,

                   dpnConnectionInfo.dwBytesRetried,
                   dpnConnectionInfo.dwPacketsRetried,

                   dpnConnectionInfo.dwBytesDropped,
                   dpnConnectionInfo.dwPacketsDropped,

                   dwHighPriMessages, dwNormalPriMessages, dwLowPriMessages, 
                   dwHighPriBytes, dwNormalPriBytes, dwLowPriBytes

                   );

    }
    else
    {
        strcpy( strConnectionInfo, "DPNID not found.\n" );
    }

    return S_OK;
}


