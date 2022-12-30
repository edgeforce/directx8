//------------------------------------------------------------------------------
// File: Graph.cpp
//
// Desc: Sample code for BDA graph building.
//
// Copyright (c) 2000, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "graph.h"

// Constructor, initializes member variables
// and calls InitializeGraphBuilder
CBDAFilterGraph::CBDAFilterGraph() :
    m_pFilterGraph(NULL),
    m_pITuningSpace(NULL),
    m_pITuner(NULL),
    m_pITuneRequest(NULL),
	m_pIMediaControl(NULL),
    m_pNetworkProvider(NULL),
    m_pTunerDevice(NULL),
    m_pCaptureDevice(NULL),
    m_pDemux(NULL),
    m_pVideoDecoder(NULL),
    m_pAudioDecoder(NULL),
    m_pMPE(NULL),
    m_pIPSink(NULL),
    m_pTIF(NULL),
    m_pOVMixer(NULL),
    m_pVRenderer(NULL),
    m_pDDSRenderer(NULL),
    m_fGraphBuilt(FALSE),
    m_fGraphRunning(FALSE),
    m_NetworkType(ATSC),
    m_MajorChannel(46L), // 46 is an in house test channel - go ahead and change it 
    m_MinorChannel(-1L)
{
    if(FAILED(InitializeGraphBuilder()))
    {
        m_fGraphFailure = TRUE;
    }
    else
        m_fGraphFailure = FALSE;
}

// Destructor
CBDAFilterGraph::~CBDAFilterGraph()
{
	if(m_fGraphRunning)
	{	
		StopGraph();
	}
	
	if(m_fGraphBuilt || m_fGraphFailure)
	{
		TearDownGraph();
	}

    SAFE_RELEASE(m_pFilterGraph);

    CoUninitialize();
}


// Instantiate graph object for filter graph building
HRESULT 
CBDAFilterGraph::InitializeGraphBuilder()
{
    HRESULT hr = S_OK;
  
    // we have a graph already, bail
    if(m_pFilterGraph)
        return hr;

    // initialize com library.
    if (hr = CoInitialize(NULL) != S_OK)
    {
        OutputDebugString("BDASampl cannot initialize COM library\n");
        goto err;
    }

    // create the filter graph
    if(hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                IID_IGraphBuilder, reinterpret_cast<void**>(&m_pFilterGraph)) != S_OK)
    {
        OutputDebugString("Couldn't CoCreate IGraphBuilder\n");
        goto err;
    }

    return hr;

err:    
	SAFE_RELEASE(m_pFilterGraph);
	m_fGraphFailure = TRUE;

    return hr;
}

// BuildGraph sets up devices, adds and connects filters
HRESULT
CBDAFilterGraph::BuildGraph(NETWORK_TYPE NetType)
{
    m_NetworkType = NetType;
    
	// if we have already have a filter graph, tear it down
    if(m_fGraphBuilt)
    {
        if(m_fGraphRunning)
        {
            StopGraph();
        }   

        TearDownGraph();
    }

    // load network provider first so that it can configure other
    // filters such as configuring the demux to sprout output pins
    if(FAILED(LoadNetworkProvider()))
    {
        OutputDebugString("Cannot load network provider\n");
        goto err;
    }

    // create a tune request to initialize the network provider 
    // before connecting other filters 
    if(FAILED(CreateATSCTuneRequest(m_MajorChannel, m_MinorChannel)))
    {
        OutputDebugString("Cannot create tune request\n");
        goto err;
    }

	// load tuner device and connect to network provider
    if(FAILED(LoadFilter(KSCATEGORY_BDA_RECEIVER_COMPONENT, &m_pTunerDevice, 
				m_pNetworkProvider, TRUE)))
    {
        OutputDebugString("Cannot load tuner device and connect network provider\n");
        goto err;
    }

    // load capture device and connect to tuner device
    if(FAILED(LoadFilter(CLSID_VideoInputDeviceCategory, &m_pCaptureDevice, 
                m_pTunerDevice, TRUE)))
    {
        OutputDebugString("Cannot load capture device and connect tuner\n");
        goto err;
    }
	
    // load demux
    if(FAILED(LoadDemux()))
    {
        OutputDebugString("Cannot load demux\n");
        goto err;
    }

	// this next call loads and connects filters associated with
	// the demultiplexor. if you want to manually load individual
	// filters such as audio and video decoders, use the code at 
	// the bottom of this file
	
	
	// render demux pins
    if(FAILED(RenderDemux()))
    {
        OutputDebugString("Cannot load demux\n");
        goto err;
    }

    m_fGraphBuilt = TRUE;

    return S_OK;

err:
    TearDownGraph();
    
    m_fGraphFailure = TRUE;

    return E_FAIL;
}


// Loads the correct tuning space based on NETWORK_TYPE that got
// passed into BuildGraph()
HRESULT
CBDAFilterGraph::LoadTuningSpace()
{
    HRESULT                 hr                      = S_OK;
    ITuningSpaceContainer*  pITuningSpaceContainer  = NULL;

    // get the tuningspace container for all the tuning spaces from SYSTEM_TUNING_SPACES
    hr = CoCreateInstance(CLSID_SystemTuningSpaces, NULL, CLSCTX_INPROC_SERVER, 
            IID_ITuningSpaceContainer, reinterpret_cast<void**>(&pITuningSpaceContainer)); 

    if(SUCCEEDED(hr) && pITuningSpaceContainer) 
    {
        VARIANT var;
        var.vt = VT_I2;

        var.iVal = m_NetworkType; 
        
        hr = pITuningSpaceContainer->get_Item(var, &m_pITuningSpace);

        if(FAILED(hr) || !m_pITuningSpace)
        {
            OutputDebugString("Unable to retrieve Tuning Space\n");
        }
    }
    else
    {
        OutputDebugString("Could not CoCreate SystemTuningSpaces\n");
    }

    SAFE_RELEASE(pITuningSpaceContainer);

    return hr;
}


// Creates and puts an ATSC Tune Request
HRESULT
CBDAFilterGraph::CreateATSCTuneRequest(LONG lMajorChannel, LONG lMinorChannel)
{
    HRESULT                     hr = S_OK;
    IATSCTuningSpace*           pATSCTuningSpace;
    IATSCChannelTuneRequest*    pATSCTuneRequest;
    
    // Making sure we have a valid tuning space 
    if(m_pITuningSpace == NULL)
    {
        OutputDebugString("Tuning Space is NULL\n");
        hr = LoadTuningSpace();

        if(FAILED(hr))
        {
            goto err;
        }
    }
    
    hr = m_pNetworkProvider->QueryInterface(IID_ITuner, reinterpret_cast<void**>(&m_pITuner));

    if(FAILED(hr) || !m_pITuner)
    {
        OutputDebugString("pNetworkProvider->QI: Can't QI for ITuner.\n");
        hr = E_NOINTERFACE;
        goto err;
    }

    //  Create an instance of the ATSC tuning space
    hr = m_pITuningSpace->QueryInterface(IID_IATSCTuningSpace, 
                        reinterpret_cast<void**>(&pATSCTuningSpace));

    if(FAILED(hr) || !pATSCTuningSpace)
    {
        OutputDebugString("pITuningSpace->QI: Can't QI for ATSC Tuning Space.\n");
        hr = E_NOINTERFACE;
        goto err;
    }
    
    //  Create an empty tune request.
    hr = pATSCTuningSpace->CreateTuneRequest(&m_pITuneRequest);

    if(FAILED(hr) || !m_pITuneRequest)
    {
        OutputDebugString("CreateTuneRequest: Can't create tune request.\n");
        hr = E_NOINTERFACE;
        goto err;
    }

    hr = m_pITuneRequest->QueryInterface(IID_IATSCChannelTuneRequest,
                        reinterpret_cast<void**>(&pATSCTuneRequest));

    if(FAILED(hr) || !pATSCTuneRequest)
    {
        OutputDebugString("CreateATSCTuneRequest: Can't create ATSC tune request.\n");
        hr = E_NOINTERFACE;
        goto err;
    }
    
    //  Set the initial major and minor channels
    hr = pATSCTuneRequest->put_Channel(lMajorChannel);
    
    if(FAILED(hr))
    {
        OutputDebugString("put_Channel failed\n");
        goto err;
    }

    hr = pATSCTuneRequest->put_MinorChannel(lMinorChannel);

    if(FAILED(hr))
    {
        OutputDebugString("put_MinorChannel failed\n");
        goto err;
    }
    
    hr = m_pITuner->put_TuneRequest(pATSCTuneRequest);

    if(FAILED(hr))
    {
        OutputDebugString("Tune Request failed\n");
        goto err;
    }
    
err:
    SAFE_RELEASE(pATSCTuningSpace);
    SAFE_RELEASE(pATSCTuneRequest);
	SAFE_RELEASE(m_pITuneRequest);
	SAFE_RELEASE(pATSCTuneRequest);
	SAFE_RELEASE(m_pITuner);

    return hr;
}

/*
DVB not implemented for DX8 RC0

// Creates and puts a DVB Tune Request
HRESULT
CBDAFilterGraph::CreateDVBTuneRequest()
{
    HRESULT         hr      = S_OK;

    return hr;
}
*/

// LoadNetworkProvider loads network provider
HRESULT
CBDAFilterGraph::LoadNetworkProvider()
{
    HRESULT hr                      = S_OK;
    BSTR    bstrNetworkType         = NULL;
    CLSID   CLSIDNetworkType;

    // obtain tuning space then load network provider
    if(m_pITuningSpace == NULL)
    {
        hr = LoadTuningSpace();

        if(FAILED(hr))
        {
	        OutputDebugString("Cannot load TuningSpace\n");
            goto err;
        }
    }

    // Get the current Network Type clsid
    hr = m_pITuningSpace->get_NetworkType(&bstrNetworkType);

    if(SUCCEEDED(hr) && bstrNetworkType)
    {
        if(FAILED(CLSIDFromString(bstrNetworkType, &CLSIDNetworkType)))
        {
            OutputDebugString("Couldn't get CLSIDFromString\n");
            goto err;
        }

		SysFreeString(bstrNetworkType);
    }
    else
    {
        OutputDebugString("ITuningSpace::Get Network Type failed\n");
	    goto err;
    }

    // create the network provider based on the clsid obtained from the tuning space
    hr = CoCreateInstance(CLSIDNetworkType, NULL, CLSCTX_INPROC_SERVER, 
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pNetworkProvider));
                    
    if(SUCCEEDED(hr) && m_pNetworkProvider)
    {
        hr = m_pFilterGraph->AddFilter(m_pNetworkProvider, L"Network Provider");
    }
    else
    {   
        OutputDebugString("Couldn't CoCreate Network Provider\n");
        goto err;
    }

    return hr;
    
err:
    SysFreeString(bstrNetworkType);
	SAFE_RELEASE(m_pNetworkProvider);
    
    return hr;
}


// enumerates through registered filters 
// instantiates the the filter object and adds it to the graph
// it checks to see if it connects to upstream filter  
// if not,  on to the next enumerated filter
// used for tuner, capture, MPE Data Filters and decoders that 
// could have more than one filter object 
// if pUpstreamFilter is NULL don't bother connecting
HRESULT
CBDAFilterGraph::LoadFilter(REFCLSID clsid, IBaseFilter** ppFilter, 
                    IBaseFilter* pConnectFilter, BOOL fIsUpstream)
{

    HRESULT             hr;
    BOOL                fFoundFilter	= FALSE;
    IMoniker*           pIMoniker       = NULL;
    IEnumMoniker*       pIEnumMoniker   = NULL;
    IBaseFilter*        pFilter         = NULL;
    IPropertyBag*       pBag            = NULL;
    
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
           IID_ICreateDevEnum, reinterpret_cast<void**>(&m_pICreateDevEnum));

	if(FAILED(hr))
	{
		OutputDebugString("LoadFilter(): Cannot CoCreate ICreateDevEnum");
		goto err;
	}

    // obtain the enumerator
    hr = m_pICreateDevEnum->CreateClassEnumerator(clsid, &pIEnumMoniker, 0);

    if(FAILED(hr))
    {
		OutputDebugString("LoadFilter(): Cannot CreateClassEnumerator");
        goto err;
    }
    
    // next filter
    while(pIEnumMoniker->Next(1, &pIMoniker, 0) == S_OK)
    {
		// obtain filter's friendly name
        hr = pIMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, 
                reinterpret_cast<void**>(&pBag));

        if(FAILED(hr) && !pBag) 
        {
            goto err;
        }

        VARIANT var;
        var.vt = VT_BSTR;
        hr = pBag->Read(L"FriendlyName", &var, NULL);

        if(FAILED(hr)) 
        {
            goto err;
        }
            
        SAFE_RELEASE(pBag);

		// bind the filter
        hr = pIMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, 
                            reinterpret_cast<void**>(&pFilter));

        SAFE_RELEASE(pIMoniker);

        if(FAILED(hr) || !pFilter)
        {
            SAFE_RELEASE(pIMoniker);
            SAFE_RELEASE(pFilter);
            continue;
        }

		hr = m_pFilterGraph->AddFilter(pFilter, var.bstrVal);

		SysFreeString(var.bstrVal);

        if(FAILED(hr))
        {
            OutputDebugString("Can not add filter\n");
			goto err;
        }

        // test connections
        // to upstream filter
        if(pConnectFilter)
        {
            if(fIsUpstream)
            {
                hr = ConnectFilters(pConnectFilter, pFilter);
            }
            else
            {
                hr = ConnectFilters(pFilter, pConnectFilter);
            }

            if(SUCCEEDED(hr))
            {
                // that's the filter we want
                fFoundFilter = TRUE;
                *ppFilter = pFilter;
                break;
            }
            else
            {   
                fFoundFilter = FALSE;
                // that wasn't the the filter we wanted
                // so unload and try the next one
                hr = m_pFilterGraph->RemoveFilter(pFilter);

                if(FAILED(hr))
                {
                    OutputDebugString("Failed unloading Filter\n");
                }
            }
        }
        else
        {
            fFoundFilter = TRUE;
            *ppFilter = pFilter;
            break;
        }

        SAFE_RELEASE(pFilter);
    } // while

    SAFE_RELEASE(pIEnumMoniker);

err:
	SAFE_RELEASE(pBag);
	SAFE_RELEASE(m_pICreateDevEnum);

    if(fFoundFilter)
    {
        return S_OK;
    }
    else
    {
        return hr;
    }
}

// loads the demux into the FilterGraph
HRESULT
CBDAFilterGraph::LoadDemux()
{
    HRESULT         hr;
    
    hr = CoCreateInstance(CLSID_MPEG2Demultiplexer, NULL, CLSCTX_INPROC_SERVER, 
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pDemux));
            
    if(SUCCEEDED(hr) && m_pDemux)
    {
        hr = m_pFilterGraph->AddFilter(m_pDemux, L"Demux");

        if(FAILED(hr))
        {
            OutputDebugString("Unable to add demux filter to graph\n");
        }
    }
    else
    {
         OutputDebugString("Could not CoCreateInstance CLSID_MPEG2Demultiplexer\n");
    }

    return hr;
}


// renders demux output pins
HRESULT
CBDAFilterGraph::RenderDemux()
{

    HRESULT         hr;
    IPin*           pIPin;
	IPin*			pDownstreamPin;
    IEnumPins*      pIEnumPins;
    PIN_DIRECTION   direction;
    
    if(!m_pDemux)
    {
        goto err;
    }

    // connect the demux to the capture device
    hr = ConnectFilters(m_pCaptureDevice, m_pDemux);

    if(FAILED(hr))
    {
        OutputDebugString("Cannot connect demux to capture filter\n");
        goto err;
    }

    // load transform information filter and connect it to the demux
    hr = LoadFilter(KSCATEGORY_BDA_TRANSPORT_INFORMATION, &m_pTIF, m_pDemux, TRUE);

    if(FAILED(hr))
    {
        OutputDebugString("Cannot load TIF\n");
        goto err;
    }

    // load multi protocol encapsulator
    hr = LoadFilter(KSCATEGORY_BDA_RECEIVER_COMPONENT, &m_pMPE, m_pDemux, TRUE);
    
    if(FAILED(hr))
    {
        OutputDebugString("Cannot load MPE\n");
        goto err;
    }

    // load IP Sink
	hr = LoadFilter(KSCATEGORY_IP_SINK, &m_pIPSink, m_pMPE, TRUE);
    
	if(FAILED(hr))
    {
        OutputDebugString("Cannot load IP Sink\n");
        goto err;
    }

    // render/connect the rest of the demux pins
    hr = m_pDemux->EnumPins(&pIEnumPins);

    ASSERT(pIEnumPins);

    while(pIEnumPins->Next(1, &pIPin, 0) == S_OK)
    {
        hr = pIPin->QueryDirection(&direction);

        if(direction == PINDIR_OUTPUT)
        {
			pIPin->ConnectedTo(&pDownstreamPin);

            if(pDownstreamPin == NULL) 
			{
				m_pFilterGraph->Render(pIPin);
			}
			
			SAFE_RELEASE(pDownstreamPin);
		}

		SAFE_RELEASE(pIPin);
    }

    SAFE_RELEASE(pIEnumPins);
			
    return hr;
    
err:    
    m_pFilterGraph->RemoveFilter(m_pDemux);
    m_pFilterGraph->RemoveFilter(m_pMPE);
    m_pFilterGraph->RemoveFilter(m_pTIF);
    m_pFilterGraph->RemoveFilter(m_pIPSink);

    return hr;
}


// removes each filter from the graph
HRESULT
CBDAFilterGraph::TearDownGraph()
{
    HRESULT         hr              = S_OK;
    
    IBaseFilter     *pFilter        = NULL;
    IEnumFilters    *pIFilterEnum   = NULL;

	SAFE_RELEASE(m_pITuningSpace);
    
    if(m_fGraphBuilt || m_fGraphFailure)
    {
		// unload manually added filters
		m_pFilterGraph->RemoveFilter(m_pIPSink);
		m_pFilterGraph->RemoveFilter(m_pMPE);
		m_pFilterGraph->RemoveFilter(m_pTIF);
		m_pFilterGraph->RemoveFilter(m_pDemux);
		m_pFilterGraph->RemoveFilter(m_pNetworkProvider); 
		m_pFilterGraph->RemoveFilter(m_pTunerDevice);
		m_pFilterGraph->RemoveFilter(m_pCaptureDevice); 
		
		SAFE_RELEASE(m_pIPSink);
		SAFE_RELEASE(m_pMPE);
		SAFE_RELEASE(m_pTIF);
		SAFE_RELEASE(m_pDemux); 
		SAFE_RELEASE(m_pNetworkProvider); 
		SAFE_RELEASE(m_pTunerDevice);
		SAFE_RELEASE(m_pCaptureDevice); 
		
		// now go unload rendered filters
        hr = m_pFilterGraph->EnumFilters(&pIFilterEnum);

        if(FAILED(hr))
        {
            OutputDebugString("TearDownGraph: cannot EnumFilters\n");
            goto err;
        }
        
        pIFilterEnum->Reset();
        
        while(pIFilterEnum->Next(1, &pFilter, 0) == S_OK) // addrefs filter
        {
            if(hr = m_pFilterGraph->RemoveFilter(pFilter) != S_OK)
            {
                goto err;
            }

			SAFE_RELEASE(pFilter);
            pIFilterEnum->Reset();
        }
    }

    m_fGraphBuilt = FALSE;

err:
    SAFE_RELEASE(pFilter);
	SAFE_RELEASE(pIFilterEnum);
	
    return hr;
}


//ConnectFilters is called from BuildGraph
//it enumerates and connects pins 
HRESULT
CBDAFilterGraph::ConnectFilters(IBaseFilter* pFilterUpstream, IBaseFilter* pFilterDownstream)
{
    HRESULT         hr                  = E_FAIL;

    IPin*           pIPinUpstream       = NULL;
    IPin*           pPinDown              = NULL;
    IPin*           pIPinDownstream     = NULL;
    IPin*           pPinUp            = NULL;

    IEnumPins*      pIEnumPinsUpstream  = NULL;
    IEnumPins*      pIEnumPinsDownstream = NULL;

    PIN_INFO        PinInfoUpstream;
    PIN_INFO        PinInfoDownstream;

    BOOL            bConnected          = FALSE;
    
    // validate passed in filters
    ASSERT(pFilterUpstream);
    ASSERT(pFilterDownstream);
    
    // grab upstream filter's enumerator
    hr = pFilterUpstream->EnumPins(&pIEnumPinsUpstream);

    if(FAILED(hr))
    {
        OutputDebugString("Cannot Enumerate Upstream Filter's Pins\n");
        goto err;
    }
    
    // iterate through upstream filter's pins
    while(pIEnumPinsUpstream->Next(1, &pIPinUpstream, 0) == S_OK) 
    {
        hr = pIPinUpstream->QueryPinInfo(&PinInfoUpstream);

        if(FAILED(hr))
        {
            OutputDebugString("Cannot Obtain Upstream Filter's PIN_INFO\n");
            goto err;
        }

        pIPinUpstream->ConnectedTo(&pPinDown);

        // bail if pins are connected
        // otherwise check direction and connect
        if(PINDIR_OUTPUT == PinInfoUpstream.dir && pPinDown == NULL) 
        {
            // grab downstream filter's enumerator
            hr = pFilterDownstream->EnumPins(&pIEnumPinsDownstream);

            // iterate through downstream filter's pins
            while(pIEnumPinsDownstream->Next(1, &pIPinDownstream, 0) == S_OK)
            {
                // make sure it is an input pin
                hr = pIPinDownstream->QueryPinInfo(&PinInfoDownstream);

                if(SUCCEEDED(hr)) 
                {
                    pIPinDownstream->ConnectedTo(&pPinUp);

                    if(PINDIR_INPUT == PinInfoDownstream.dir && pPinUp == NULL) 
                    {
                        if(SUCCEEDED(m_pFilterGraph->Connect(pIPinUpstream, 
                            pIPinDownstream)))
                        {
                            PinInfoDownstream.pFilter->Release();
							PinInfoUpstream.pFilter->Release();
                            bConnected = TRUE;
							SAFE_RELEASE(pIPinDownstream);

                            break;
                        }
                    }
	                SAFE_RELEASE(pPinUp);
                }

				PinInfoDownstream.pFilter->Release();
				SAFE_RELEASE(pIPinDownstream);
            } // while next downstream filter pin
                
            SAFE_RELEASE(pIEnumPinsDownstream);
            SAFE_RELEASE(pPinUp);
			SAFE_RELEASE(pIPinDownstream);
			SAFE_RELEASE(pIPinUpstream);
            //We are now back into the upstream pin loop
            if(bConnected)
            {
				break;
            }
        } // if output pin
        
        PinInfoUpstream.pFilter->Release();
		SAFE_RELEASE(pIPinUpstream);
		SAFE_RELEASE(pPinDown);
        SAFE_RELEASE(pPinUp);
		SAFE_RELEASE(pIPinDownstream);
	    SAFE_RELEASE(pIEnumPinsDownstream);

    } // while next upstream filter pin
	
	SAFE_RELEASE(pIEnumPinsUpstream);
    
	if(bConnected)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

err:
    
    SAFE_RELEASE(pPinDown);
    SAFE_RELEASE(pPinUp);
    SAFE_RELEASE(pIPinUpstream);
    SAFE_RELEASE(pIPinDownstream);
    SAFE_RELEASE(pIEnumPinsDownstream);
    SAFE_RELEASE(pIEnumPinsUpstream);
    
    return hr;
}


// RunGraph checks to see if a graph has been built
// if not it calls BuildGraph
// RunGraph then calls MediaCtrl-Run
HRESULT 
CBDAFilterGraph::RunGraph()
{
    HRESULT hr = S_OK;
    
    // check to see if the graph is already running
    if(m_fGraphRunning)
    {
        return hr;
    }

	hr = m_pFilterGraph->QueryInterface(IID_IMediaControl, 
                                reinterpret_cast<void**>(&m_pIMediaControl));

    if(SUCCEEDED(hr))        
    {
        // run the graph
        hr = m_pIMediaControl->Run();

        if(SUCCEEDED(hr))
        {
            m_fGraphRunning = TRUE;
        }
        else
        {
            // stop parts of the graph that ran
            m_pIMediaControl->Stop();
            OutputDebugString("Cannot run graph\n");
        }
    }

	SAFE_RELEASE(m_pIMediaControl);

    return hr;
}


// StopGraph calls MediaCtrl - Stop
HRESULT 
CBDAFilterGraph::StopGraph()
{
    HRESULT hr = S_OK;

	// check to see if the graph is already stopped
    if(!m_fGraphRunning) 
    {
        return hr;
    }

	hr = m_pFilterGraph->QueryInterface(IID_IMediaControl, 
                                reinterpret_cast<void**>(&m_pIMediaControl));
    
    if(SUCCEEDED(hr))        
    {
		// pause before stopping
		hr = m_pIMediaControl->Pause();

		// stop the graph
		hr = m_pIMediaControl->Stop();
    
		if(SUCCEEDED(hr))
		{
			m_fGraphRunning = FALSE;
		}
		else
		{
			OutputDebugString("Cannot stop graph\n");
		}
	}

	SAFE_RELEASE(m_pIMediaControl);

    return hr;
}

// Set our client area for viewing
// 
// Note, what your not seeing here is a call to
// IAMSreamCconfig's GetFormat to obtain the video
// format properties that would enable us to set 
// the viewing window's size  
HRESULT
CBDAFilterGraph::SetVideoWindow(HWND hwndMain)
{
    HRESULT hr = S_OK;

    IVideoWindow*   pVideoWindow = NULL;
    RECT            rc;
    INT             cyBorder;
    INT             cy;

    // get IVideoWindow interface
    hr = m_pFilterGraph->QueryInterface(IID_IVideoWindow, 
            reinterpret_cast<void**>(&pVideoWindow));

    if(FAILED(hr))
    {
        OutputDebugString("QueryInterface IVideoWindow Failed\n");
        goto err;
    }

    hr = pVideoWindow->put_Owner((LONG) hwndMain);

    if(FAILED(hr))
    {
        OutputDebugString("Unable to set video window\n");
        goto err;
    }
    
    hr = pVideoWindow->put_WindowStyle(WS_CHILD);

    GetClientRect(hwndMain, &rc);
    cyBorder = GetSystemMetrics(SM_CYBORDER);
    cy = cyBorder;
    rc.bottom -= cy;
    pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
    pVideoWindow->put_Visible(OATRUE);
    
  err:
    SAFE_RELEASE(pVideoWindow);
    
    return hr;
}

HRESULT
CBDAFilterGraph::ChangeChannel(LONG lMajorChannel, LONG lMinorChannel)
{
    HRESULT         hr              = E_FAIL;
    IScanningTuner* pIScanningTuner = NULL;

    if(!m_pNetworkProvider)  
    {
        OutputDebugString("The FilterGraph is not Built Yet\n");
        hr = E_NOINTERFACE;
        goto err;
    }

    hr = m_pNetworkProvider->QueryInterface(IID_IScanningTuner,
            reinterpret_cast<void**>(&pIScanningTuner));

    if(FAILED(hr) && !pIScanningTuner)
    {
        OutputDebugString("Cannot QI for IScanningTuner\n");
        goto err;
    }

    // check to see if we're going for major channel change
    // if so, set minor channel to -1, unless the minor has changed as well
    if(lMajorChannel == m_MajorChannel)
    {
        switch(lMinorChannel)
        {
        case CHANNEL_UP: // up channel
            m_MinorChannel++;

            break;
    
        case CHANNEL_DOWN: // down channel
            m_MinorChannel--;

            break;
    
        default: 
             if(lMajorChannel < CHANNEL_UPPER_LIMIT         &&
                lMajorChannel > CHANNEL_LOWER_LIMIT         &&
                lMinorChannel < MINOR_CHANNEL_UPPER_LIMIT   &&
                lMinorChannel > MINOR_CHANNEL_LOWER_LIMIT)

             {
                 m_MinorChannel = lMinorChannel;
             }

            break;
        }
    }
    else
    {
        // major channel change, and did minor change?
        // if not set it to "unset" (-1)
        if(m_MinorChannel == lMinorChannel)
        {
            m_MinorChannel = -1;
        }
        else
        {
            m_MinorChannel = lMinorChannel;
        }
    }

    switch(lMajorChannel)
    {
    case CHANNEL_UP: // up channel
        hr = pIScanningTuner->SeekUp();

        if(SUCCEEDED(hr))
        {
            m_MajorChannel++;
        }
        else
        {
            OutputDebugString("pIScanningTuner->SeekUp Failed\n");
        }

        break;
    
    case CHANNEL_DOWN: // down channel
        hr = pIScanningTuner->SeekDown();

        if(SUCCEEDED(hr))
        {
            m_MajorChannel--;
        }
        else
        {
            OutputDebugString("pIScanningTuner->SeekDown Failed\n");
        }
    
        break;
    
    default: 
        if( lMajorChannel  < CHANNEL_UPPER_LIMIT        &&
            lMajorChannel  > CHANNEL_LOWER_LIMIT        &&
            m_MinorChannel < MINOR_CHANNEL_UPPER_LIMIT  &&
            m_MinorChannel > MINOR_CHANNEL_LOWER_LIMIT)

        {
            // create tune request
            hr = CreateATSCTuneRequest(lMajorChannel, m_MinorChannel);

            if(SUCCEEDED(hr))
            {
                m_MajorChannel = lMajorChannel;
            }
            else
            {
                OutputDebugString("Cannot Change Channels\n");
            }
        }

        break;
    }
    
err:
    SAFE_RELEASE(pIScanningTuner);
    
    return hr;
}


void
CBDAFilterGraph::Refresh(HWND hDlg)
{
    if(hDlg)
    {
        SetDlgItemInt(hDlg, IDC_MAJOR_CHANNEL, m_MajorChannel, FALSE);
        SetDlgItemInt(hDlg, IDC_MINOR_CHANNEL, m_MinorChannel, TRUE);
    }
}



// USE THE BELOW CODE IF YOU WANT TO MANUALLY LOAD AND 
// CONNECT A/V DECODERS TO THE DEMUX OUTPUT PINS

/*
To use this code:
1) in LoadAudioDecoder() and LoadVideoDecoder(), fill in decoder specific information (clsid)
2) goto BuildGraph() and replace RenderDemux() with BuildAVSegment()
*/

/*
// Builds the Audio, Video segment of the digital TV graph.   
// Demux -> AV Decoder -> OVMixer -> Video Renderer
HRESULT 
CBDAFilterGraph::BuildAVSegment()
{
    HRESULT hr = E_FAIL;

    // connect the demux to the capture device
    hr = ConnectFilters(m_pCaptureDevice, m_pDemux);

    hr = LoadVideoDecoder(); 

    if(SUCCEEDED(hr) && m_pVideoDecoder)        
    {                                           
        // Connect the demux & video decoder
        hr = ConnectFilters(m_pDemux, m_pVideoDecoder);  

        if(FAILED(hr))
        {
            OutputDebugString("Connecting Demux & Video Decoder Failed\n");
            goto err;
        }
    }
    else
    {
        //OutputDebugString("Unable to load Video Decoder\n");
        goto err;
    }

    //Audio
    hr = LoadAudioDecoder();

    if(SUCCEEDED(hr) && m_pAudioDecoder)
    {
        hr = ConnectFilters(m_pDemux, m_pAudioDecoder);

        if(FAILED(hr))
        {
            OutputDebugString("Connecting Deumx & Audio Decoder Failed\n");
            goto err;
        }
    }
    else
    {
        OutputDebugString("Unable to load Audio Decoder\n");
        goto err;
    }
    
    // Create the OVMixer & Video Renderer for the video segment
    hr = CoCreateInstance(CLSID_OverlayMixer, NULL, CLSCTX_INPROC_SERVER, 
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pOVMixer));

    if(SUCCEEDED(hr) && m_pOVMixer)
    {
        hr = m_pFilterGraph->AddFilter(m_pOVMixer, L"OVMixer");
        
        if(FAILED(hr))
        {
            OutputDebugString("Adding OVMixer to the FilterGraph Failed\n");
            goto err;
        }
    }
    else
    {
        OutputDebugString("Loading OVMixer Failed\n");
        goto err;
    }

    hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, 
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pVRenderer));

    if(SUCCEEDED(hr) && m_pVRenderer)
    {
        hr = m_pFilterGraph->AddFilter(m_pVRenderer, L"Video Renderer");
        
        if(FAILED(hr))
        {
            OutputDebugString("Adding Video Renderer to the FilterGraph Failed\n");
            goto err;
        }
    }
    else
    {
        OutputDebugString("Loading Video Renderer Failed\n");
        goto err;
    }

    // Split AV Decoder? Then add Default DirectSound Renderer to the filtergraph
    if(m_pVideoDecoder != m_pAudioDecoder) 
    {
        hr = CoCreateInstance(CLSID_DSoundRender, NULL, 
                        CLSCTX_INPROC_SERVER, IID_IBaseFilter, 
                        reinterpret_cast<void**>(&m_pDDSRenderer));
                        
        if(SUCCEEDED(hr) && m_pDDSRenderer)
        {
            hr = m_pFilterGraph->AddFilter(m_pDDSRenderer, L"Sound Renderer");

            if(FAILED(hr))
            {
                OutputDebugString("Adding DirectSound Device to the FilterGraph Failed\n");
                goto err;
            }
        }
        else
        {
            OutputDebugString("Loading DirectSound Device Failed\n");
            goto err;
        }
    }

    hr = ConnectFilters(m_pVideoDecoder, m_pOVMixer);
    
    if(FAILED(hr))
    {
        OutputDebugString("Connecting Capture & OVMixer Failed\n");
        goto err;
    }
    
    hr = ConnectFilters(m_pOVMixer, m_pVRenderer);

    if(FAILED(hr))
    {
        OutputDebugString("Connecting OVMixer & Video Renderer Failed\n");
        goto err;
    }
    
    // Split AV Decoder & if you need audio too ?? then connect Audio decoder to Sound Renderer
    if(m_pVideoDecoder != m_pAudioDecoder) 
    {
        hr = ConnectFilters(m_pAudioDecoder, m_pDDSRenderer);
        
        if(FAILED(hr))
        {
            OutputDebugString("Connecting AudioDecoder & DirectSound Device Failed\n");
            goto err;
        }
    }

err:
    return hr;
}

// placeholders for real decoders
DEFINE_GUID(CLSID_FILL_IN_NAME_AUDIO_DECODER, 0xFEEDFEED, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00);
DEFINE_GUID(CLSID_FILL_IN_NAME_VIDEO_DECODER, 0xFEEDFEED, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00);

HRESULT
CBDAFilterGraph::LoadVideoDecoder()
{
    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(CLSID_FILL_IN_NAME_VIDEO_DECODER, NULL, 
            CLSCTX_INPROC_SERVER, IID_IBaseFilter, 
            reinterpret_cast<void**>(&m_pVideoDecoder));

    if(SUCCEEDED(hr) && m_pVideoDecoder)
    {
        hr = m_pFilterGraph->AddFilter(m_pVideoDecoder, L"Video Decoder");
        
        if(FAILED(hr))
        {
            OutputDebugString("Unable to add Video Decoder filter to graph\n");
        }
    }
    
    return hr;
}


HRESULT
CBDAFilterGraph::LoadAudioDecoder()
{
    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(CLSID_FILL_IN_NAME_AUDIO_DECODER, NULL, 
            CLSCTX_INPROC_SERVER, IID_IBaseFilter, 
            reinterpret_cast<void**>(&m_pAudioDecoder));

    if(SUCCEEDED(hr) && m_pAudioDecoder)
    {
        hr = m_pFilterGraph->AddFilter(m_pAudioDecoder, L"Audio Decoder");
        
        if(FAILED(hr))
        {
            OutputDebugString("Unable to add Audio filter to graph\n");
        }
    }
    
    return hr;
}

*/

