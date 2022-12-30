//------------------------------------------------------------------------------
// File: DShowUtil.cpp
//
// Desc: DirectShow sample code - utility functions.
//
// Copyright (c) 1996 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include "dshowutil.h"


HRESULT GetPin( IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin)
{
    CComPtr< IEnumPins > pEnum;
    *ppPin = NULL;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if(FAILED(hr)) 
        return hr;

    ULONG ulFound;
    IPin *pPin;
    hr = E_FAIL;
    while(S_OK == pEnum->Next(1, &pPin, &ulFound))
    {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;
        pPin->QueryDirection(&pindir);
        if(pindir == dirrequired)
        {
            if(iNum == 0)
            {
                *ppPin = pPin;
                break;
            }
            iNum--;
        } // if
        pPin->Release();
    } // while

    return hr;
}


IPin * GetInPin( IBaseFilter * pFilter, int Num )
{
    CComPtr< IPin > pComPin;
    GetPin(pFilter, PINDIR_INPUT, Num, &pComPin);
    return pComPin;
}


IPin * GetOutPin( IBaseFilter * pFilter, int Num )
{
    CComPtr< IPin > pComPin;
    GetPin(pFilter, PINDIR_OUTPUT, Num, &pComPin);
    return pComPin;
}


HRESULT FindOtherSplitterPin(IPin *pPinIn, GUID guid, int nStream, IPin **ppSplitPin)
{
    DbgLog((LOG_TRACE,1,TEXT("FindOtherSplitterPin")));

    CheckPointer(ppSplitPin, E_POINTER);
    CComPtr< IPin > pPinOut;
    pPinOut = pPinIn;

    while(pPinOut)
    {
        PIN_INFO ThisPinInfo;
        pPinOut->QueryPinInfo(&ThisPinInfo);
        if(ThisPinInfo.pFilter) ThisPinInfo.pFilter->Release();

        pPinOut = NULL;
        CComPtr< IEnumPins > pEnumPins;
        ThisPinInfo.pFilter->EnumPins(&pEnumPins);
        if(!pEnumPins)
        {
            return NULL;
        }

        // look at every pin on the current filter...
        //
        ULONG Fetched = 0;
        while(1)
        {
            CComPtr< IPin > pPin;
            Fetched = 0;
            ASSERT(!pPin); // is it out of scope?
            pEnumPins->Next(1, &pPin, &Fetched);
            if(!Fetched)
            {
                break;
            }

            PIN_INFO pi;
            pPin->QueryPinInfo(&pi);
            if(pi.pFilter) pi.pFilter->Release();

            // if it's an input pin...
            //
            if(pi.dir == PINDIR_INPUT)
            {
                // continue searching upstream from this pin
                //
                pPin->ConnectedTo(&pPinOut);

                // a pin that supports the required media type is the
                // splitter pin we are looking for!  We are done
                //
            }
            else
            {
                CComPtr< IEnumMediaTypes > pMediaEnum;
                pPin->EnumMediaTypes(&pMediaEnum);
                if(pMediaEnum)
                {
                    Fetched = 0;
                    AM_MEDIA_TYPE *pMediaType;
                    pMediaEnum->Next(1, &pMediaType, &Fetched);
                    if(Fetched)
                    {
                        if(pMediaType->majortype == guid)
                        {
                            if(nStream-- == 0)
                            {
                                DeleteMediaType(pMediaType);
                                *ppSplitPin = pPin;
                                (*ppSplitPin)->AddRef();
                                DbgLog((LOG_TRACE,1,TEXT("Found SPLIT pin")));
                                return S_OK;
                            }
                        }
                        DeleteMediaType(pMediaType);
                    }
                }
            }

            // go try the next pin

        } // while
    }
    ASSERT(FALSE);
    return E_FAIL;
}


HRESULT SeekNextFrame( IMediaSeeking * pSeeking, double FPS, long Frame )
{
    // try seeking by frames first
    //
    HRESULT hr = pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME);
    REFERENCE_TIME Pos = 0;
    if(!FAILED(hr))
    {
        pSeeking->GetCurrentPosition(&Pos);
        Pos++;
    }
    else
    {
        // couldn't seek by frames, use Frame and FPS to calculate time
        //
        Pos = REFERENCE_TIME(double( Frame * UNITS ) / FPS);

        // add a half-frame to seek to middle of the frame
        //
        Pos += REFERENCE_TIME(double( UNITS ) * 0.5 / FPS);
    }

    hr = pSeeking->SetPositions(&Pos, AM_SEEKING_AbsolutePositioning, 
                                NULL, AM_SEEKING_NoPositioning);
    return hr;

}

#ifdef DEBUG
    // for debugging purposes
    const INT iMAXLEVELS = 5;                // Maximum debug categories
    extern DWORD m_Levels[iMAXLEVELS];       // Debug level per category
#endif


void TurnOnDebugDllDebugging( )
{
#ifdef DEBUG
    for(int i = 0 ; i < iMAXLEVELS ; i++)
    {
        m_Levels[i] = 1;
    }
#endif
}


void DbgPrint( char * pText )
{
    DbgLog(( LOG_TRACE, 1, "%s", pText ));
}

void ErrPrint( char * pText )
{
    printf(pText);
    return;
}


HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
    IMoniker * pMoniker;
    IRunningObjectTable *pROT;
    WCHAR wsz[128];
    HRESULT hr;

    if (FAILED(GetRunningObjectTable(0, &pROT))) {
        return E_FAIL;
    }

    wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, 
              GetCurrentProcessId());

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) {
        hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
        pMoniker->Release();
    }
    pROT->Release();
    return hr;
}


void RemoveGraphFromRot(DWORD pdwRegister)
{
    IRunningObjectTable *pROT;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}


