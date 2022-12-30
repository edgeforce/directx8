//------------------------------------------------------------------------------
// File: TextOut.h
//
// Desc: DirectShow sample code - header file for TextOut renderer.
//
// Copyright (c) 1992 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// You MUST change this when adapting this sample!
// Run uuidgen.exe from any SDK to get a new GUID

DEFINE_GUID(CLSID_PlainText,
0x48025242, 0x2d39, 0x11ce, 0x87, 0x5d, 0x0, 0x60, 0x8c, 0xb7, 0x80, 0x66);

class CTextOutFilter;
class CTextOutWindow;

// These are our video window styles

const LPTSTR TextClassName = TEXT("TextRendererBaseClass");
const DWORD TextClassStyles = (CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT);
const DWORD TextWindowStyles = (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);

// Derived class handling window interactions. We did have the main renderer
// object inheriting from CBaseControlWindow so that we didn't have to have
// a separate class but that means there are two many classes derived from
// CUnknown, so when in the final text out filter class you call something
// like GetOwner it gets really confusing to know who is actually going to
// be called. So in the end we made it a separate class for the window. We
// have to specialise the base class to provide the PURE virtual method that
// returns the class and window information (GetClassWindowStyles). We are
// also interested in certain window messages like WM_PAINT and WM_NCHITTEST

class CTextOutWindow : public CBaseControlWindow
{
    CTextOutFilter *m_pRenderer;

public:

    CTextOutWindow(TCHAR *pName,                 // Object description
                   LPUNKNOWN pUnk,               // Normal COM ownership
                   HRESULT *phr,                 // OLE failure code
                   CCritSec *pLock,              // Our interface Lock
                   CTextOutFilter *pRenderer);   // Delegates locking to

    ~CTextOutWindow();
    BOOL OnClose();

    LRESULT OnReceiveMessage(HWND hwnd,             // Window handle
                             UINT uMsg,             // Message ID
                             WPARAM wParam,         // First parameter
                             LPARAM lParam);        // Other parameter

    LPTSTR GetClassWindowStyles(DWORD *pClassStyles,
                                DWORD *pWindowStyles,
                                DWORD *pWindowStylesEx);

}; // CTextOutWindow

// Overall filter object for the text renderer. We have to provide our own
// version of NonDelegatingQueryInterface so that we can expose not only the
// interfaces supported by the base renderer but also pass on queries for
// IVideoWindow to our window handling class (m_TextWindow). The rest of the
// methods we override are pretty dull, dealing with type checking and so on

class CTextOutFilter : public CBaseRenderer
{
    CTextOutWindow m_TextWindow;

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    CTextOutFilter(LPUNKNOWN pUnk,HRESULT *phr);
    ~CTextOutFilter();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    STDMETHODIMP Pause();
    HRESULT BreakConnect();
    HRESULT CheckMediaType(const CMediaType *pmt);
    BOOL OnPaint(COLORREF WindowColour);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    void DrawText(IMediaSample *pMediaSample);

}; // CTextOutFilter

