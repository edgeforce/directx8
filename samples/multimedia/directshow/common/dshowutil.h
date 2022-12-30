//------------------------------------------------------------------------------
// File: DShowUtil.h
//
// Desc: DirectShow sample code - prototypes for utility functions.
//
// Copyright (c) 1996 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


//
// Function prototypes for util.cpp
//

HRESULT GetPin( 
    IBaseFilter * pFilter, 
    PIN_DIRECTION dirrequired, 
    int iNum, 
    IPin **ppPin);
    
HRESULT FindOtherSplitterPin(
    IPin *pPinIn, 
    GUID guid, 
    int nStream, 
    IPin **ppSplitPin);
    
HRESULT SeekNextFrame( 
    IMediaSeeking * pSeeking, 
    double FPS, 
    long Frame );
    
IPin * GetInPin( IBaseFilter * pFilter, int Num );
IPin * GetOutPin( IBaseFilter * pFilter, int Num );

void TurnOnDebugDllDebugging( );

void DbgPrint( char * pText );
void ErrPrint( char * pText );

