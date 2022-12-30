//------------------------------------------------------------------------------
// File: Bdasampl.h
//
// Desc: Sample code header file for BDA graph building.
//
// Copyright (c) 2000, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#ifndef BDASAMPL_H_INCLUDED_
#define BDASAMPL_H_INCLUDED_

//-----------------------------------------------------------------------------
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       DlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK       AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK       SelectChannelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID                ErrorMessageBox(LPTSTR sz,...);

//-----------------------------------------------------------------------------
// index of tuning spaces 
enum NETWORK_TYPE 
{
    CABLE           = 0x0001,
    ANTENNA         = 0x0002,
    ATSC            = 0x0003,
    DIGITAL_CABLE   = 0x0004,
    DVB             = 0x0005
};

#endif // BDASAMPL_H_INCLUDED_

