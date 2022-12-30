//----------------------------------------------------------------------------
// File: dplay8client.cpp
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <assert.h>
#include <dxerr8.h>
#include <stdio.h>
#include "dplay8.h"
#include "dpaddr.h"
#include "DPlay8Client.h"
#include "StressMazeGUID.h"
#include "DXUtil.h"

#define DPMAZESERVER_PORT       2309
      



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CDPlay8Client::CDPlay8Client()
{
    m_dwNumSessions = 0;
    m_pClient       = NULL;
    m_pDPlay        = NULL;
    m_dpnhEnum      = NULL;
    m_bSessionLost  = TRUE;
    m_dwSessionLostReason = DISCONNNECT_REASON_UNKNOWN;

    for( DWORD dwIndex = 0; dwIndex < MAX_SESSIONS; dwIndex++ )
    {
        m_pHostAddresses[dwIndex] = NULL;
        m_pDeviceAddresses[dwIndex] = NULL;
    }

    InitializeCriticalSection( &m_csLock );
}

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
CDPlay8Client::~CDPlay8Client()
{
    DeleteCriticalSection( &m_csLock );
}


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::Init()
{
    HRESULT hr;
    
    if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Client, NULL, 
                                       CLSCTX_ALL, IID_IDirectPlay8Client,
                                       (LPVOID*) &m_pDPlay ) ) )
        return DXTRACE_ERR( TEXT("CoCreateInstance"), hr );

    if( FAILED( hr = m_pDPlay->Initialize( this, StaticReceiveHandler, 0 ) ) )
        return DXTRACE_ERR( TEXT("Initialize"), hr );

    m_fLastUpdateConnectInfoTime = DXUtil_Timer( TIMER_GETAPPTIME );
    m_bSessionLost  = TRUE;
    m_dwNumSessions = 0;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void CDPlay8Client::Shutdown()
{
    for( DWORD dwIndex = 0; dwIndex < MAX_SESSIONS; dwIndex++ )
    {
        SAFE_RELEASE( m_pHostAddresses[dwIndex] );
        SAFE_RELEASE( m_pDeviceAddresses[dwIndex] );
    }

    if( m_dpnhEnum != NULL )
    {
        // Cancel the enumeration if its in progress, and ignore any errors
        m_pDPlay->CancelAsyncOperation( m_dpnhEnum, 0 );
        m_dpnhEnum = NULL;
    }

    if( m_pDPlay != NULL )
        m_pDPlay->Close(0);

    SAFE_RELEASE( m_pDPlay );
    m_bSessionLost  = TRUE;
    m_dwNumSessions = 0;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::StartSessionEnum( const TCHAR* ipaddress )
{
    if( NULL == m_pDPlay )
        return E_FAIL;

    DPN_APPLICATION_DESC   dpnAppDesc;
    IDirectPlay8Address*   pDP8AddressHost  = NULL;
    IDirectPlay8Address*   pDP8AddressLocal = NULL;
    WCHAR*                 wszHostName      = NULL;
    HRESULT                hr;
    DWORD                  dwPort;

    m_dwNumSessions = 0;

    if( m_dpnhEnum != NULL )
    {
        // If an enumeration is already running, cancel 
        // it and start a new one.  Ignore any errors from CancelAsyncOperation
        m_pDPlay->CancelAsyncOperation( m_dpnhEnum, 0 );
        m_dpnhEnum = NULL;
    }

    // Create the local device address object
    if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL, 
                                       CLSCTX_ALL, IID_IDirectPlay8Address,
                                       (LPVOID*) &pDP8AddressLocal ) ) )
    {
        DXTRACE_ERR( TEXT("CoCreateInstance"), hr );
        goto LCleanup;
    }

    // Set IP service provider
    if( FAILED( hr = pDP8AddressLocal->SetSP( &CLSID_DP8SP_TCPIP ) ) )
    {
        DXTRACE_ERR( TEXT("SetSP"), hr );
        goto LCleanup;
    }


    // Create the remote host address object
    if( FAILED( hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL, 
                                       CLSCTX_ALL, IID_IDirectPlay8Address,
                                       (LPVOID*) &pDP8AddressHost ) ) )
    {
        DXTRACE_ERR( TEXT("CoCreateInstance"), hr );
        goto LCleanup;
    }

    // Set IP service provider
    if( FAILED( hr = pDP8AddressHost->SetSP( &CLSID_DP8SP_TCPIP ) ) )
    {
        DXTRACE_ERR( TEXT("SetSP"), hr );
        goto LCleanup;
    }

    // Maze uses a fixed port, so add it to the host address
    //dwPort = DPNA_DPNSVR_PORT;
	dwPort = DPMAZESERVER_PORT;
 	hr = pDP8AddressHost->AddComponent( DPNA_KEY_PORT, 
                                      &dwPort, sizeof(dwPort),
                                      DPNA_DATATYPE_DWORD );
    if( FAILED(hr) )
    {
        DXTRACE_ERR( TEXT("AddComponent"), hr );
        goto LCleanup;
    }

    // Set the remote host name (if provided)
    if( ipaddress != NULL && ipaddress[0] != 0 )
    {
        wszHostName = new WCHAR[_tcslen(ipaddress)+1];

        DXUtil_ConvertGenericStringToWide( wszHostName, ipaddress );

        hr = pDP8AddressHost->AddComponent( DPNA_KEY_HOSTNAME, wszHostName, 
                                            (wcslen(wszHostName)+1)*sizeof(WCHAR), 
                                            DPNA_DATATYPE_STRING );
        if( FAILED(hr) )
        {
            DXTRACE_ERR( TEXT("AddComponent"), hr );
            goto LCleanup;
        }
    }

    ZeroMemory( &dpnAppDesc, sizeof( DPN_APPLICATION_DESC ) );
    dpnAppDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
    dpnAppDesc.guidApplication = StressMazeAppGUID;

    // Enumerate all StressMazeApp hosts running on IP service providers
    hr = m_pDPlay->EnumHosts( &dpnAppDesc, pDP8AddressHost, 
                              pDP8AddressLocal, NULL, 
                              0, INFINITE, 0, INFINITE, NULL, 
                              &m_dpnhEnum, 0 );
    if( FAILED(hr) )
    {
        DXTRACE_ERR_NOMSGBOX( TEXT("EnumHosts"), hr );
        goto LCleanup;
    }

LCleanup:
    SAFE_RELEASE( pDP8AddressHost);
    SAFE_RELEASE( pDP8AddressLocal );
    SAFE_DELETE( wszHostName );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::StopSessionEnum()
{
    if( NULL == m_pDPlay )
        return E_FAIL;

    // If an enumeration is already running, cancel it and ignore
    // any errors from CancelAsyncOperation
    if( m_dpnhEnum != NULL )
    {
        m_pDPlay->CancelAsyncOperation( m_dpnhEnum, 0 );
        m_dpnhEnum = NULL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CDPlay8Client::EnumSessionCallback( const DPN_APPLICATION_DESC *pdesc, 
                                         IDirectPlay8Address* pDP8AddressHost,
                                         IDirectPlay8Address* pDP8AddressDevice )
{
    if( NULL == m_pDPlay )
        return DPN_OK;

    EnterCriticalSection( &m_csLock );

    if( m_dwNumSessions < MAX_SESSIONS )
    {
        // Search for existing record for this session, if 
        // there is one, break this loop so we just update
        // the current entry.
        for( DWORD dwIndex = 0; dwIndex < m_dwNumSessions; dwIndex++ )
        {
            if( m_Sessions[dwIndex].guidInstance == pdesc->guidInstance )
                break;
        }
        
        memcpy( &m_Sessions[dwIndex], pdesc, sizeof( DPN_APPLICATION_DESC ) );

        // Copy pDP8AddressHost to m_pHostAddresses[dwIndex]
        SAFE_RELEASE( m_pHostAddresses[dwIndex] );
        pDP8AddressHost->QueryInterface( IID_IDirectPlay8Address, 
                                         (LPVOID*) &m_pHostAddresses[dwIndex] );

        // Copy pDP8AddressDevice to m_pDeviceAddresses[dwIndex]
        SAFE_RELEASE( m_pDeviceAddresses[dwIndex] );
        pDP8AddressDevice->QueryInterface( IID_IDirectPlay8Address, 
                                         (LPVOID*) &m_pDeviceAddresses[dwIndex] );

        if( pdesc->pwszSessionName != NULL )
        {
            DXUtil_ConvertWideStringToGeneric( m_szSessionNames[dwIndex], 
                                               pdesc->pwszSessionName );
        }
        else
        {
            _tcscpy( m_szSessionNames[dwIndex], TEXT("Untitled") );
        }

        if( m_dwNumSessions == dwIndex )
            m_dwNumSessions++;
    }

    LeaveCriticalSection( &m_csLock );

    return DPN_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::JoinSession( DWORD num )
{
    HRESULT hr;

    if( m_pDPlay == NULL )
        return E_FAIL;

    DXTRACE( "MazeClient: Trying to connect to server\n" );

    DPN_APPLICATION_DESC dpnAppDesc;
    ZeroMemory( &dpnAppDesc, sizeof( DPN_APPLICATION_DESC ) );
    dpnAppDesc.dwSize          = sizeof( DPN_APPLICATION_DESC );
    dpnAppDesc.guidApplication = StressMazeAppGUID;
    dpnAppDesc.guidInstance    = m_Sessions[num].guidInstance;

    // Connect to the remote host
    // The enumeration is automatically canceled after Connect is called 
    if( FAILED( hr = m_pDPlay->Connect( &dpnAppDesc,        // Application description
                                        m_pHostAddresses[num],   // Session host address
                                        m_pDeviceAddresses[num], // Address of device used to connect to the host
                                        NULL, NULL,         // Security descriptions & credientials (MBZ in DPlay8)
                                        NULL, 0,            // User data & its size
                                        NULL,               // Asynchronous connection context (returned with DPNMSG_CONNECT_COMPLETE in async handshaking)
                                        NULL,               // Asynchronous connection handle (used to cancel connection process)
                                        DPNOP_SYNC ) ) )    // Connect synchronously
    {
        if( hr == DPNERR_NORESPONSE || hr == DPNERR_ABORTED )
            goto LCleanup; // These are possible if the server exits while joining 

        if( hr == DPNERR_INVALIDINSTANCE )
            goto LCleanup; // This is possible if the original server exits and another server comes online while we are connecting

        DXTRACE_ERR_NOMSGBOX( TEXT("Connect"), hr );
        goto LCleanup;
    }

    m_bSessionLost = FALSE;
    
    DXTRACE( "MazeClient: Connected to server.  Enum automatically canceled\n" );

    UpdateConnectionInfo();
    m_fLastUpdateConnectInfoTime = DXUtil_Timer( TIMER_GETAPPTIME );


LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::SendPacket( void* pData, DWORD dwSize, 
                                   BOOL bGuaranteed, DWORD dwTimeout )
{
    if( NULL == m_pDPlay )
        return S_OK;

    DPNHANDLE  hAsync;
    DWORD      dwFlags;
    DPNHANDLE* phAsync;

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
	
	//Must pass in a value for the Asyn handle. Will be thrown when proc completes.
	phAsync = &hAsync;

    DPN_BUFFER_DESC dpnBufferDesc;
    dpnBufferDesc.dwBufferSize = dwSize;
    dpnBufferDesc.pBufferData = (PBYTE) pData;

    // Update the throughput counter
    m_dwThroughputBytes += dwSize;

    // DirectPlay will tell via the message handler 
    // if there are any severe errors, so ignore any errors 
    m_pDPlay->Send( &dpnBufferDesc, 1, dwTimeout, 
                    NULL, phAsync, dwFlags );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::StaticReceiveHandler( void *pvContext, DWORD dwMessageType, 
                                             void *pvMessage )
{
    CDPlay8Client* pThisObject = (CDPlay8Client*) pvContext;

    return pThisObject->ReceiveHandler( pvContext, dwMessageType, pvMessage );
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::ReceiveHandler( void *pvContext, DWORD dwMessageType, 
                                       void *pvMessage )
{
    switch( dwMessageType )
    {
        case DPN_MSGID_RECEIVE:
        {
            PDPNMSG_RECEIVE pRecvData = (PDPNMSG_RECEIVE) pvMessage;

            // Update the throughput counter
            m_dwThroughputBytes += pRecvData->dwReceiveDataSize;

            if( m_pClient != NULL )
            {
                m_pClient->OnPacket( pRecvData->dpnidSender, 
                                     pRecvData->pReceiveData, 
                                     pRecvData->dwReceiveDataSize );
            }
            break;
        }
        
        case DPN_MSGID_TERMINATE_SESSION:
        {
            m_dwSessionLostReason = DISCONNNECT_REASON_UNKNOWN;
            PDPNMSG_TERMINATE_SESSION pTermMsg = (PDPNMSG_TERMINATE_SESSION) pvMessage;

            // The MazeServer passes a DWORD in pvTerminateData if 
            // it disconnected us, otherwise it will be null.
            if( pTermMsg->pvTerminateData != NULL )
            {
                DWORD* pdw = (DWORD*) pTermMsg->pvTerminateData;
                m_dwSessionLostReason = *pdw;
            }

            if( m_pClient != NULL )
                m_pClient->OnSessionLost( m_dwSessionLostReason );

            // Now that the session is lost we need to restart DirectPlay by calling
            // Close() and Init() on m_pDPlay, however this can not be 
            // done in the DirectPlay message callback, so the main thread will
            // do this when IsSessionLost() returns TRUE
            m_bSessionLost = TRUE;
            break;
        }

        case DPN_MSGID_ENUM_HOSTS_RESPONSE:
        {
            PDPNMSG_ENUM_HOSTS_RESPONSE pEnumResponse = (PDPNMSG_ENUM_HOSTS_RESPONSE) pvMessage;

            EnumSessionCallback( pEnumResponse->pApplicationDescription, 
                                 pEnumResponse->pAddressSender,
                                 pEnumResponse->pAddressDevice );
            break;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::UpdateConnectionInfo()
{
    if( NULL == m_pDPlay )
        return E_FAIL;

    // Update the DPN_CONNECTION_INFO every 1/2 second...
    float fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
    if( fCurTime - m_fLastUpdateConnectInfoTime > 0.5f )
    {
        // Call GetConnectionInfo to get DirectPlay stats about connection 
        ZeroMemory( &m_dpnConnectionInfo, sizeof(DPN_CONNECTION_INFO) );
        m_dpnConnectionInfo.dwSize = sizeof(DPN_CONNECTION_INFO);
        m_pDPlay->GetConnectionInfo( &m_dpnConnectionInfo, 0 );

        // Call GetSendQueueInfo to get DirectPlay stats about messages
        m_pDPlay->GetSendQueueInfo( &m_dwHighPriMessages, &m_dwHighPriBytes, 
                                    DPNGETSENDQUEUEINFO_PRIORITY_HIGH );
        m_pDPlay->GetSendQueueInfo( &m_dwNormalPriMessages, &m_dwNormalPriBytes, 
                                    DPNGETSENDQUEUEINFO_PRIORITY_NORMAL );
        m_pDPlay->GetSendQueueInfo( &m_dwLowPriMessages, &m_dwLowPriBytes, 
                                    DPNGETSENDQUEUEINFO_PRIORITY_LOW );

        m_fLastUpdateConnectInfoTime = fCurTime;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
DWORD CDPlay8Client::GetThroughputBPS()
{
    static float s_fLastThroughputBPSTime = DXUtil_Timer( TIMER_GETAPPTIME );
    float fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
    if( fCurTime - s_fLastThroughputBPSTime > 1.0f )
    {
        m_fThroughputBPS         = (float) m_dwThroughputBytes / (fCurTime - s_fLastThroughputBPSTime);

        s_fLastThroughputBPSTime = fCurTime;
        m_dwThroughputBytes      = 0;
    }

    return (DWORD) m_fThroughputBPS;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
DWORD CDPlay8Client::GetRoundTripLatencyMS()
{
    UpdateConnectionInfo();
    return m_dpnConnectionInfo.dwRoundTripLatencyMS;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CDPlay8Client::GetConnectionInfo( TCHAR* strConnectionInfo )
{
    UpdateConnectionInfo();

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
               m_dpnConnectionInfo.dwRoundTripLatencyMS, 
               m_dpnConnectionInfo.dwThroughputBPS, 
               m_dpnConnectionInfo.dwPeakThroughputBPS,

               m_dpnConnectionInfo.dwMessagesReceived,

               m_dpnConnectionInfo.dwBytesSentGuaranteed,
               m_dpnConnectionInfo.dwPacketsSentGuaranteed,
               m_dpnConnectionInfo.dwBytesSentNonGuaranteed,
               m_dpnConnectionInfo.dwPacketsSentNonGuaranteed,

               m_dpnConnectionInfo.dwBytesReceivedGuaranteed,
               m_dpnConnectionInfo.dwPacketsReceivedGuaranteed,
               m_dpnConnectionInfo.dwBytesReceivedNonGuaranteed,
               m_dpnConnectionInfo.dwPacketsReceivedNonGuaranteed,

               m_dpnConnectionInfo.dwMessagesTransmittedHighPriority,
               m_dpnConnectionInfo.dwMessagesTransmittedNormalPriority,
               m_dpnConnectionInfo.dwMessagesTransmittedLowPriority,

               m_dpnConnectionInfo.dwMessagesTimedOutHighPriority,
               m_dpnConnectionInfo.dwMessagesTimedOutNormalPriority,
               m_dpnConnectionInfo.dwMessagesTimedOutLowPriority,

               m_dpnConnectionInfo.dwBytesRetried,
               m_dpnConnectionInfo.dwPacketsRetried,

               m_dpnConnectionInfo.dwBytesDropped,
               m_dpnConnectionInfo.dwPacketsDropped,

               m_dwHighPriMessages, m_dwNormalPriMessages, m_dwLowPriMessages, 
               m_dwHighPriBytes, m_dwNormalPriBytes, m_dwLowPriBytes

               );

    return S_OK;
}
