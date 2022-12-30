//------------------------------------------------------------------------------
// File: Graph.h
//
// Desc: Sample code for BDA graph building.
//
// Copyright (c) 2000, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#ifndef GRAPH_H_INCLUDED_
#define GRAPH_H_INCLUDED_

#include <streams.h>
#include <mmreg.h>
#include <msacm.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <ks.h>     
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>   
#include <bdaiface.h>
#include <uuids.h>      
#include <tuner.h>      

#include "bdasampl.h"
#include "resource.h"

class CBDAFilterGraph
{
private:
    ITuningSpace*   m_pITuningSpace;

    ITuner*         m_pITuner;
    ITuneRequest*   m_pITuneRequest;

    IGraphBuilder*  m_pFilterGraph;         // for current graph
    IMediaControl*  m_pIMediaControl;       // for controlling graph state
    ICreateDevEnum* m_pICreateDevEnum;      // for enumerating system devices

    IBaseFilter*    m_pNetworkProvider;     // for network provider filter
    IBaseFilter*    m_pTunerDevice;         // for tuner device filter
    IBaseFilter*    m_pCaptureDevice;       // for capture device filter
    IBaseFilter*    m_pDemux;               // for demux filter
    IBaseFilter*    m_pVideoDecoder;        // for mpeg video decoder filter
    IBaseFilter*    m_pAudioDecoder;        // for mpeg audio decoder filter
    IBaseFilter*    m_pTIF;                 // for transport information filter
    IBaseFilter*    m_pMPE;                 // for multiple protocol encapsulator
    IBaseFilter*    m_pIPSink;              // for ip sink filter
    IBaseFilter*    m_pOVMixer;             // for overlay mixer filter
    IBaseFilter*    m_pVRenderer;           // for video renderer filter
    IBaseFilter*    m_pDDSRenderer;         // for sound renderer filter
    
    HRESULT InitializeGraphBuilder();
    HRESULT LoadTuningSpace();
    HRESULT LoadNetworkProvider();
    HRESULT LoadDemux();
    HRESULT RenderDemux();
    HRESULT LoadFilter(REFCLSID clsid, IBaseFilter** ppFilter, IBaseFilter* pConnectFilter, BOOL fIsUpstream);
    HRESULT ConnectFilters(IBaseFilter* pFilterUpstream, IBaseFilter* pFilterDownstream);
    HRESULT CreateATSCTuneRequest(LONG lMajorChannel, LONG lMinorChannel);
    HRESULT CreateDVBTuneRequest();

public:
    LONG            m_MajorChannel;
    LONG            m_MinorChannel;
    BOOL            m_fGraphBuilt;
    BOOL            m_fGraphRunning;
    BOOL            m_fGraphFailure;
    NETWORK_TYPE    m_NetworkType;

    CBDAFilterGraph();
    ~CBDAFilterGraph();

    HRESULT     BuildGraph(NETWORK_TYPE NetworkType);
    HRESULT     RunGraph();
    HRESULT     StopGraph();
    HRESULT     TearDownGraph();
    HRESULT     SetVideoWindow(HWND hwndMain);
    HRESULT     ChangeChannel(LONG lMajorChannel, LONG lMinorChannel);
    void        Refresh(HWND hDlg);
 };

#define SAFE_RELEASE(pObject) if(pObject){ (pObject)->Release(); pObject = NULL;}

#define CHANNEL_UP                  10001
#define CHANNEL_DOWN                10002
#define CHANNEL_LOWER_LIMIT         1
#define CHANNEL_UPPER_LIMIT         126
#define MINOR_CHANNEL_LOWER_LIMIT   -2
#define MINOR_CHANNEL_UPPER_LIMIT   126

#endif // GRAPH_H_INCLUDED_
