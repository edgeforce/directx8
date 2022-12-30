//------------------------------------------------------------------------------
// File: JukeboxDlg.cpp
//
// Desc: DirectShow sample code - implementation of CJukeboxDlg class.
//
// Copyright (c) 1998 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Jukebox.h"
#include "JukeboxDlg.h"
#include "playvideo.h"
#include "mediatypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJukeboxDlg dialog

CJukeboxDlg::CJukeboxDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJukeboxDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJukeboxDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CJukeboxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJukeboxDlg)
	DDX_Control(pDX, IDC_STATIC_DURATION, m_StrDuration);
	DDX_Control(pDX, IDC_EDIT_MEDIADIR, m_EditMediaDir);
	DDX_Control(pDX, IDC_SPIN_FILES, m_SpinFiles);
	DDX_Control(pDX, IDC_BUTTON_FRAMESTEP, m_ButtonFrameStep);
	DDX_Control(pDX, IDC_LIST_EVENTS, m_ListEvents);
	DDX_Control(pDX, IDC_CHECK_EVENTS, m_CheckEvents);
	DDX_Control(pDX, IDC_BUTTON_PROPPAGE, m_ButtonProperties);
	DDX_Control(pDX, IDC_STATUS_DIRECTORY, m_StrMediaPath);
	DDX_Control(pDX, IDC_CHECK_MUTE, m_CheckMute);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_ButtonStop);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_ButtonPlay);
	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_ButtonPause);
	DDX_Control(pDX, IDC_CHECK_PLAYTHROUGH, m_CheckPlaythrough);
	DDX_Control(pDX, IDC_CHECK_LOOP, m_CheckLoop);
	DDX_Control(pDX, IDC_STATIC_FILEDATE, m_StrFileDate);
	DDX_Control(pDX, IDC_STATIC_FILESIZE, m_StrFileSize);
	DDX_Control(pDX, IDC_LIST_PINS_OUTPUT, m_ListPinsOutput);
	DDX_Control(pDX, IDC_LIST_PINS_INPUT, m_ListPinsInput);
	DDX_Control(pDX, IDC_STATIC_FILELIST, m_StrFileList);
	DDX_Control(pDX, IDC_STATUS, m_Status);
	DDX_Control(pDX, IDC_MOVIE_SCREEN, m_Screen);
	DDX_Control(pDX, IDC_LIST_FILTERS, m_ListFilters);
	DDX_Control(pDX, IDC_LIST_FILES, m_ListFiles);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJukeboxDlg, CDialog)
	//{{AFX_MSG_MAP(CJukeboxDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_LBN_SELCHANGE(IDC_LIST_FILES, OnSelectFile)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, OnPause)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, OnPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnStop)
	ON_WM_MOVE()
	ON_BN_CLICKED(IDC_CHECK_MUTE, OnCheckMute)
	ON_BN_CLICKED(IDC_CHECK_LOOP, OnCheckLoop)
	ON_BN_CLICKED(IDC_CHECK_PLAYTHROUGH, OnCheckPlaythrough)
	ON_LBN_SELCHANGE(IDC_LIST_FILTERS, OnSelchangeListFilters)
	ON_LBN_DBLCLK(IDC_LIST_FILTERS, OnDblclkListFilters)
	ON_BN_CLICKED(IDC_BUTTON_PROPPAGE, OnButtonProppage)
	ON_BN_CLICKED(IDC_CHECK_EVENTS, OnCheckEvents)
	ON_BN_CLICKED(IDC_BUTTON_FRAMESTEP, OnButtonFramestep)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_EVENTS, OnButtonClearEvents)
	ON_LBN_DBLCLK(IDC_LIST_FILES, OnDblclkListFiles)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_FILES, OnDeltaposSpinFiles)
	ON_BN_CLICKED(IDC_BUTTON_SET_MEDIADIR, OnButtonSetMediadir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CJukeboxDlg message handlers

void CJukeboxDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CJukeboxDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CJukeboxDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


/////////////////////////////////////////////////////////////////////////////
// CJukeboxDlg DirectShow code and message handlers


BOOL CJukeboxDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    // Initialize COM
    CoInitialize(NULL);
	
    // Initialize DirectShow and query for needed interfaces
    HRESULT hr = InitDirectShow();
    if(FAILED(hr))
    {
        RetailOutput(TEXT("Failed to initialize DirectShow!  hr=0x%x\r\n"), hr);
        return FALSE;
    }

    // IMPORTANT
    // Since we're embedding video in a child window of a dialog,
    // we must set the WS_CLIPCHILDREN style to prevent the bounding
    // rectangle from drawing over our video frames.
    //
    // Neglecting to set this style can lead to situations when the video
    // is erased and replaced with black (or the default color of the 
    // bounding rectangle).
    m_Screen.ModifyStyle(0, WS_CLIPCHILDREN);

    // Fill the media file list, starting with the directory passed
    // on the command line.  If no directory is passed, then read the
    // default media path for the DirectX SDK.
    TCHAR *szRootDir;

    if (theApp.m_lpCmdLine[0] == L'\0')
        szRootDir = GetDXSDKMediaPath();
    else
        szRootDir = theApp.m_lpCmdLine;              

    TCHAR szPathMsg[MAX_PATH];
    wsprintf(szPathMsg, "Media directory: %s\0", szRootDir);
    m_StrMediaPath.SetWindowText(szPathMsg);

    m_EditMediaDir.SetLimitText(MAX_PATH);
    m_EditMediaDir.SetWindowText(szRootDir);

    // Propagate the files list and select the first item
    FillFileList(szRootDir);
    
    return TRUE;  // return TRUE  unless you set the focus to a control
}


TCHAR* CJukeboxDlg::GetDXSDKMediaPath()
{
    static TCHAR strNull[2] = _T("");
    static TCHAR strPath[MAX_PATH];
    DWORD dwType;
    DWORD dwSize = MAX_PATH;
    HKEY  hKey;

    // Open the appropriate registry key
    LONG lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                _T("Software\\Microsoft\\DirectX"),
                                0, KEY_READ, &hKey );
    if( ERROR_SUCCESS != lResult )
        return strNull;

    lResult = RegQueryValueEx( hKey, _T("DX8SDK Samples Path"), NULL,
                              &dwType, (BYTE*)strPath, &dwSize );
    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult )
        return strNull;

    _tcscat( strPath, _T("\\Media\\") );

    return strPath;
}


void CJukeboxDlg::FillFileList(LPTSTR pszRootDir)
{
    UINT attr = 0;

    m_ListFiles.ResetContent();

    ::SetCurrentDirectory(pszRootDir);
    Say(TEXT("Building file list..."));

    // Add all of our known supported media types to the file list.
    // Add files of each type in order.
    for (int i=0; i < NUM_MEDIA_TYPES; i++)
    {
        m_ListFiles.Dir(attr, TypeInfo[i].pszType);
    }
    Say(TEXT("File list complete."));

    // Update list box title with number of items added
    int nItems  = m_ListFiles.GetCount();
    TCHAR szTitle[64];
    wsprintf(szTitle, TEXT("Media files (%d found)"), nItems);
    m_StrFileList.SetWindowText(szTitle);
    
    // Automatically select the first file in the list once
    // the dialog is displayed.
    PostMessage(WM_FIRSTFILE, 0, 0L);
    m_nCurrentFileSelection = -1;     // No selection yet
}


void CJukeboxDlg::OnClose() 
{
    // Release DirectShow interfaces
    StopMedia();
    FreeDirectShow();

    // Release COM
    CoUninitialize();

	CDialog::OnClose();
}


void CJukeboxDlg::OnSelectFile() 
{
    HRESULT hr;
    TCHAR szFilename[MAX_PATH];

    // If this is the currently selected file, do nothing
    int nItem = m_ListFiles.GetCurSel();
    if (nItem == m_nCurrentFileSelection)
        return;

    // Remember the current selection to speed double-click processing
    m_nCurrentFileSelection = nItem;

    // Read file name from list box
    m_ListFiles.GetText(nItem, szFilename);

    // Remember current play state to restart playback
    int nCurrentState = g_psCurrent;

    // First release any existing interfaces
    ResetDirectShow();

    // Clear filter/pin/event information listboxes
    m_ListFilters.ResetContent();
    m_ListPinsInput.ResetContent();
    m_ListPinsOutput.ResetContent();
    m_ListEvents.ResetContent();

    // Load the selected media file
    hr = PrepareMedia(szFilename);
    if (FAILED(hr))
    {
        // Error - disable play button and give feedback
        Say(TEXT("File failed to render!"));
        m_ButtonPlay.EnableWindow(FALSE);
        MessageBeep(0);
        return;
    }
    else
    {
        m_ButtonPlay.EnableWindow(TRUE);
    }

    // Display useful information about this file
    DisplayFileInfo(szFilename);
    DisplayFileDuration();

    // Enumerate and display filters in graph
    hr = EnumFilters();

    // Select the first filter in the list to display pin info
    m_ListFilters.SetCurSel(0);
    OnSelchangeListFilters();

    // See if the renderer supports frame stepping on this file.
    // Enable/disable frame stepping button accordingly
    m_ButtonFrameStep.EnableWindow(CanStep());

    // If the user has asked to mute audio then we
    // need to mute this new clip before continuing.
    if (g_bGlobalMute)
        MuteAudio();

    // If we were running when the user changed selection,
    // start running the newly selected clip
    if (nCurrentState == State_Running)
    {
        OnPlay();
    }
    else
    {
        // Cue the first video frame
        OnStop();
    }
}


HRESULT CJukeboxDlg::PrepareMedia(LPTSTR lpszMovie)
{
    HRESULT hr = S_OK;
    WCHAR wFile[MAX_PATH];

    Say(TEXT("Loading..."));

#ifndef UNICODE
    MultiByteToWideChar(CP_ACP, 0, lpszMovie, -1, wFile, MAX_PATH);
#else
    lstrcpy(wFile, lpszMovie);
#endif

    // Allow DirectShow to create the FilterGraph for this media file
    hr = pGB->RenderFile(wFile, NULL);
    if (FAILED(hr)) {
        RetailOutput(TEXT("*** Failed(%08lx) in RenderFile(%s)!\r\n"),
                 hr, lpszMovie);
        return hr;
    }

    // We'll manually set the video to be visible, so disable autoshow
    hr = pVW->put_AutoShow(OAFALSE);

    // Set the message drain of the video window to point to our main
    // application window.
    //
    // If this is an audio-only or MIDI file, then put_MessageDrain will fail.
    //
    hr = pVW->put_MessageDrain((OAHWND) m_hWnd);
    if (FAILED(hr))
    {
        g_bAudioOnly = TRUE;
    }

    if (!g_bAudioOnly)
    {
        hr = pVW->put_Owner((OAHWND) m_Screen.GetSafeHwnd());
        hr = pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    }

    // Have the graph signal event via window callbacks
    hr = pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0);

    // Place video window within the bounding rectangle
    CenterVideo();

    // Make the video window visible within the screen window.
    // If this is an audio-only file, then there won't be a video interface.
    if (!g_bAudioOnly)
    {
        hr = pVW->put_Visible(OATRUE);
        hr = pVW->SetWindowForeground(-1);
    }

    Say(TEXT("Ready"));
    return hr;
}


//
//  Displays a text string in a status line near the bottom of the dialog
//
CJukeboxDlg::Say(LPTSTR szText)
{
    m_Status.SetWindowText(szText);
}

void CJukeboxDlg::OnPause() 
{  
    if (g_psCurrent == State_Paused)
    {
        RunMedia();
        Say(TEXT("Running"));
    }
    else
    {
        PauseMedia();
        Say(TEXT("PAUSED"));
    }
}

void CJukeboxDlg::OnPlay() 
{
  	RunMedia();
    Say(TEXT("Running"));
}

void CJukeboxDlg::ShowState()
{
    HRESULT hr;

    OAFilterState fs;
    hr = pMC->GetState(500, &fs);
    if (FAILED(hr))
    {
        RetailOutput(TEXT("Failed to read graph state!  hr=0x%x\r\n"), hr);
        return;
    }

    // Show debug output for current media state
    switch (fs)
    {
        case State_Stopped:
            RetailOutput(TEXT("State_Stopped\r\n"));
            break;
        case State_Paused:
            RetailOutput(TEXT("State_Paused\r\n"));
            break;
        case State_Running:
            RetailOutput(TEXT("State_Running\r\n"));
            break;
    }
}

void CJukeboxDlg::OnStop() 
{
    HRESULT hr;

    // Stop playback immediately with IMediaControl::Stop().
    StopMedia();	

    // Wait for the stop to propagate to all filters
    OAFilterState fs;
    hr = pMC->GetState(500, &fs);
    if (FAILED(hr))
    {
        RetailOutput(TEXT("Failed to read graph state!  hr=0x%x\r\n"), hr);
    }

    // Reset to beginning of media clip
    LONGLONG pos=0;
    hr = pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning ,
                           NULL, AM_SEEKING_NoPositioning);
    if (FAILED(hr))
    {
        RetailOutput(TEXT("Failed to seek to beginning of media!  hr=0x%x\r\n"), hr);
    }
  
    // Display the first frame of the media clip, if it contains video.
    // StopWhenReady() pauses all filters internally (which allows the video
    // renderer to queue and display the first video frame), after which
    // it sets the filters to the stopped state.  This enables easy preview
    // of the video's poster frame.
    hr = pMC->StopWhenReady();
    if (FAILED(hr))
    {
        RetailOutput(TEXT("Failed in StopWhenReady!  hr=0x%x\r\n"), hr);
    }

    Say(TEXT("Stopped"));
}

HRESULT CJukeboxDlg::InitDirectShow(void)
{
    HRESULT hr = S_OK;

    g_bAudioOnly = FALSE;

    // Zero interfaces (sanity check)
    pVW = NULL;
    pBV = NULL;

    JIF(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGB));
    JIF(pGB->QueryInterface(IID_IMediaControl,  (void **)&pMC));
    JIF(pGB->QueryInterface(IID_IMediaSeeking,  (void **)&pMS));
    JIF(pGB->QueryInterface(IID_IBasicVideo,    (void **)&pBV));
    JIF(pGB->QueryInterface(IID_IVideoWindow,   (void **)&pVW));
    JIF(pGB->QueryInterface(IID_IMediaEventEx,  (void **)&pME));

    return S_OK;

CLEANUP:
    FreeDirectShow();
    return(hr);
}

HRESULT CJukeboxDlg::FreeDirectShow(void)
{
    HRESULT hr=S_OK;

    StopMedia();

    // Hide video window and remove owner.  This is not necessary here,
    // since we are about to destroy the filter graph, but it is included
    // for demonstration purposes.  Remember to hide the video window and
    // clear its owner when destroying a window that plays video.
    if(pVW)
    {
        hr = pVW->put_Visible(OAFALSE);
        hr = pVW->put_Owner(NULL);
    }

    SAFE_RELEASE(pMC);
    SAFE_RELEASE(pMS);
    SAFE_RELEASE(pVW);
    SAFE_RELEASE(pBV);
    SAFE_RELEASE(pME);
    SAFE_RELEASE(pGB);

    return hr;
}

void CJukeboxDlg::ResetDirectShow(void)
{
    // Destroy the current filter graph its filters.
    FreeDirectShow();

    // Reinitialize graph builder and query for interfaces
    InitDirectShow();
}

void CJukeboxDlg::CenterVideo(void)
{
    LONG width, height;
    HRESULT hr;

    if ((g_bAudioOnly) || (!pVW))
        return;

    // Read coordinates of video container window
    RECT rc;
    m_Screen.GetClientRect(&rc);
    width =  rc.right - rc.left;
    height = rc.bottom - rc.top;

    // Ignore the video's original size and stretch to fit bounding rectangle
    hr = pVW->SetWindowPosition(rc.left, rc.top, width, height);
    if (FAILED(hr))
    {
        RetailOutput(TEXT("Failed to set window position!  hr=0x%x\r\n"), hr);
        return;
    }
}


LRESULT CJukeboxDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// Field notifications from the DirectShow filter graph manager
    // and those posted by the application
    switch (message)
    {
        case WM_GRAPHNOTIFY:
            HandleGraphEvent();
            break;

        //
        // If the media is not running and contains a video component,
        // the following code helps to keep the video window properly
        // painted when the main window moves or returns from being
        // minimized.  This allows the current video frame to be smoothly 
        // redrawn.  Otherwise, the video portion could "lag" behind the 
        // rest of the application window as it moves.
        //
        case WM_WINDOWPOSCHANGED:
            if (pVW && (g_psCurrent != State_Running))
            {
                pVW->put_Visible(OAFALSE);
                pVW->put_Visible(OATRUE);
            }
            break;

        case WM_PLAYFILE:
            PlaySelectedFile();
            break;

        case WM_NEXTFILE:
            PlayNextFile();
            break;

        case WM_PREVIOUSFILE:
            PlayPreviousFile();
            break;

        case WM_FIRSTFILE:
            // Select the first item in the list
            m_ListFiles.SetCurSel(0);
            OnSelectFile();
            break;
    }

	return CDialog::WindowProc(message, wParam, lParam);
}


HRESULT CJukeboxDlg::HandleGraphEvent(void)
{
    LONG evCode, evParam1, evParam2;
    HRESULT hr=S_OK;

    while(SUCCEEDED(pME->GetEvent(&evCode, &evParam1, &evParam2, 0)))
    {
        // Spin through the events
        hr = pME->FreeEventParams(evCode, evParam1, evParam2);

        if(EC_COMPLETE == evCode)
        {
            // If looping, reset to beginning and continue playing
            if (g_bLooping)
            {
                LONGLONG pos=0;

                // Reset to first frame of movie
                hr = pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning ,
                                       NULL, AM_SEEKING_NoPositioning);
                if (FAILED(hr))
                {
                    // Some custom filters (like the Windows CE MIDI filter) 
                    // may not implement seeking interfaces (IMediaSeeking)
                    // to allow seeking to the start.  In that case, just stop 
                    // and restart for the same effect.  This should not be
                    // necessary in most cases.
                    StopMedia();
                    RunMedia();
                }
            }
            else if (g_bPlayThrough)
            {
                // Tell the app to select the next file in the list
                PostMessage(WM_NEXTFILE, 0, 0);
            }
            else
            {
                // Stop playback and display first frame of movie
                OnStop();
            }
        }

        //  If requested, display DirectShow events received
        if (g_bDisplayEvents)
        {
            DisplayECEvent(evCode, evParam1, evParam2);
        }
    }

    return hr;
}

void CJukeboxDlg::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	
	// Reposition the video window when the main window moves
    CenterVideo();	
}

void CJukeboxDlg::OnCheckMute() 
{
    // Remember global mute status for next file.  When you destroy a
    // filtergraph, you lose all of its audio settings.  Therefore, when
    // we create the next graph, we will mute the audio before running
    // the graph if this global variable is set.
    g_bGlobalMute ^= 1; 

    if (g_bGlobalMute)
        MuteAudio();
    else
        ResumeAudio();
}

void CJukeboxDlg::OnCheckLoop() 
{
    g_bLooping ^= 1;

    // Looping and play-through are mutually exclusive
    if ((g_bLooping) && (g_bPlayThrough))
    {
        // Disable play-through and uncheck button
        g_bPlayThrough = 0;
        m_CheckPlaythrough.SetCheck(0);
    }
}

void CJukeboxDlg::OnCheckPlaythrough() 
{
    g_bPlayThrough ^= 1;	

    // Looping and play-through are mutually exclusive
    if ((g_bPlayThrough) && (g_bLooping) )
    {
        // Disable play-through and uncheck button
        g_bLooping = 0;
        m_CheckLoop.SetCheck(0);
    }
}

void CJukeboxDlg::OnCheckEvents() 
{
    g_bDisplayEvents ^= 1;	
}

void CJukeboxDlg::OnButtonClearEvents() 
{
    m_ListEvents.ResetContent();
}

void CJukeboxDlg::OnButtonSetMediadir() 
{
    // Make sure that we're not playing media
    OnStop();

    // Read the string in the media directory edit box.
    TCHAR szEditPath[MAX_PATH];
    DWORD dwAttr;

    m_EditMediaDir.GetWindowText(szEditPath, MAX_PATH);

    // Is this a valid directory name?
    dwAttr = GetFileAttributes(szEditPath);
    if ((dwAttr == (DWORD) -1) || (! (dwAttr & FILE_ATTRIBUTE_DIRECTORY)))
    {
        MessageBox(TEXT("Please enter a valid directory name."), TEXT("Media error"));
        return;
    }

    // User has specified a valid media directory.  
    // Update the current path string.
    TCHAR szPathMsg[MAX_PATH];
    wsprintf(szPathMsg, "Media directory: %s\0", szEditPath);
    m_StrMediaPath.SetWindowText(szPathMsg);

    // Propagate the files list and select the first item
    FillFileList(szEditPath);
}


void CJukeboxDlg::OnDblclkListFiles() 
{
    // Because it might take time to render the file and display
    // its first frame, it's better to post a message that tells
    // the app to play the selected file when ready.
    PostMessage(WM_PLAYFILE, 0, 0L);
}

void CJukeboxDlg::PlaySelectedFile() 
{
    OnPlay();
}

void CJukeboxDlg::OnDeltaposSpinFiles(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

    if (pNMUpDown->iDelta > 0)
        PostMessage(WM_NEXTFILE, 0, 0L);
    else
        PostMessage(WM_PREVIOUSFILE, 0, 0L);

	*pResult = 0;
}

void CJukeboxDlg::PlayNextFile(void)
{
    int nItems  = m_ListFiles.GetCount();

    // Return if the list is empty
    if (!nItems)
        return;
        
    int nCurSel = m_ListFiles.GetCurSel();
    int nNewSel = (nCurSel + 1) % nItems;

    // Select the next item in the list, wrapping to top if needed
    m_ListFiles.SetCurSel(nNewSel);
    OnSelectFile();
}


void CJukeboxDlg::PlayPreviousFile(void)
{
    int nItems  = m_ListFiles.GetCount();

    // Return if the list is empty
    if (!nItems)
        return;
        
    int nCurSel = m_ListFiles.GetCurSel();
    int nNewSel = nCurSel - 1;

    // If moved off top of list, select last item in list
    if (nNewSel < 0)
        nNewSel = nItems - 1;

    // Select the next item in the list, wrapping to top if needed
    m_ListFiles.SetCurSel(nNewSel);
    OnSelectFile();
}


HRESULT CJukeboxDlg::EnumFilters (void) 
{
    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    ULONG cFetched;

    // Clear filters list box
    m_ListFilters.ResetContent();
    
    // Get filter enumerator
    hr = pGB->EnumFilters(&pEnum);
    if (FAILED(hr))
    {
        m_ListFilters.AddString(TEXT("<ERROR>"));
        return hr;
    }

    // Enumerate all filters in the graph
    while(pEnum->Next(1, &pFilter, &cFetched) == S_OK)
    {
        FILTER_INFO FilterInfo;
        TCHAR szName[256];
        
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
        {
            m_ListFilters.AddString(TEXT("<ERROR>"));
        }
        else
        {
            // Add the filter name to the filters listbox
            WideCharToMultiByte(CP_ACP, 0, FilterInfo.achName, -1, szName, 256, 0, 0);
            m_ListFilters.AddString(szName);

            FilterInfo.pGraph->Release();
        }       
        pFilter->Release();
    }
    pEnum->Release();

    return hr;
}


//
// The GraphBuilder interface provides a FindFilterByName() method,
// which provides similar functionality to the method below.
// This local method is provided for educational purposes.
//
IBaseFilter *CJukeboxDlg::FindFilterFromName(LPTSTR szNameToFind)
{
    HRESULT hr;
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    ULONG cFetched;
    BOOL bFound = FALSE;

    // Get filter enumerator
    hr = pGB->EnumFilters(&pEnum);
    if (FAILED(hr))
        return NULL;

    // Enumerate all filters in the graph
    while((pEnum->Next(1, &pFilter, &cFetched) == S_OK) && (!bFound))
    {
        FILTER_INFO FilterInfo;
        TCHAR szName[256];
        
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
        {
            pFilter->Release();
            pEnum->Release();
            return NULL;
        }

        // Compare this filter's name with the one we want
        WideCharToMultiByte(CP_ACP, 0, FilterInfo.achName, -1, szName, 256, 0, 0);
        if (! lstrcmp(szName, szNameToFind))
        {
            bFound = TRUE;
        }

        FilterInfo.pGraph->Release();

        // If we found the right filter, don't release its interface.
        // The caller will use it and release it later.
        if (!bFound)
            pFilter->Release();
        else
            break;
    }
    pEnum->Release();

    return (bFound ? pFilter : NULL);
}


HRESULT CJukeboxDlg::EnumPins(IBaseFilter *pFilter, PIN_DIRECTION PinDir,
                              CListBox& Listbox)
{
    HRESULT hr;
    IEnumPins  *pEnum = NULL;
    IPin *pPin = NULL;

    // Clear the specified listbox (input or output)
    Listbox.ResetContent();

    // Get pin enumerator
    hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        Listbox.AddString(TEXT("<ERROR>"));
        return hr;
    }

    // Enumerate all pins on this filter
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;

        hr = pPin->QueryDirection(&PinDirThis);
        if (FAILED(hr))
        {
            Listbox.AddString(TEXT("<ERROR>"));
            pPin->Release();
            continue;
        }

        // Does the pin's direction match the requested direction?
        if (PinDir == PinDirThis)
        {
            PIN_INFO pininfo={0};

            // Direction matches, so add pin name to listbox
            hr = pPin->QueryPinInfo(&pininfo);
            if (SUCCEEDED(hr))
            {
                CString str(pininfo.achName);
                Listbox.AddString(str);
            }

            // The pininfo structure contains a reference to an IBaseFilter,
            // so you must release its reference to prevent resource a leak.
            pininfo.pFilter->Release();
        }
        pPin->Release();
    }
    pEnum->Release();

    return hr;
}


void CJukeboxDlg::OnSelchangeListFilters() 
{
    HRESULT hr;
    IBaseFilter *pFilter = NULL;
    TCHAR szNameToFind[128];

    // Read the current filter name from the list box
    int nCurSel = m_ListFilters.GetCurSel();
    m_ListFilters.GetText(nCurSel, szNameToFind);

    // Read the current list box name and find it in the graph
    pFilter = FindFilterFromName(szNameToFind);
    if (!pFilter)
        return;

    // Now that we have filter information, enumerate pins by direction
    // and add their names to the appropriate listboxes
    hr = EnumPins(pFilter, PINDIR_INPUT,  m_ListPinsInput);
    hr = EnumPins(pFilter, PINDIR_OUTPUT, m_ListPinsOutput);

    // Find out if this filter supports a property page
    if (SupportsPropertyPage(pFilter))
        m_ButtonProperties.EnableWindow(TRUE);
    else
        m_ButtonProperties.EnableWindow(FALSE);
    
    // Must release the filter interface returned from FindFilterByName()
    pFilter->Release();
}


BOOL CJukeboxDlg::DisplayFileInfo(LPTSTR szFile)
{
    HANDLE hFile;
    LONGLONG llSize=0;
    DWORD dwSizeLow=0, dwSizeHigh=0;
    TCHAR szScrap[64];

    // Open the specified file to read size and date information
    hFile = CreateFile(szFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                       (DWORD) 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        RetailOutput(TEXT("*** Failed(0x%x) to open file (to read size)!\r\n"),
                     GetLastError());
        return FALSE;
    }

    dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
    if ((dwSizeLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    {
        RetailOutput(TEXT("*** Error(0x%x) reading file size!\r\n"),
                     GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    // Large files will also use the upper DWORD to report size.
    // Add them together for the true size if necessary.
    if (dwSizeHigh)
        llSize = (dwSizeHigh << 16) + dwSizeLow;
    else
        llSize = dwSizeLow;

    // Read date information
    BY_HANDLE_FILE_INFORMATION fi;
    if (GetFileInformationByHandle(hFile, &fi))
    {
        CTime time(fi.ftLastWriteTime);

        wsprintf(szScrap, "File date: %02d/%02d/%d\0", 
                 time.GetMonth(), time.GetDay(), time.GetYear());
        m_StrFileDate.SetWindowText(szScrap);
    }

    CloseHandle(hFile);

    // Update size/date windows
    wsprintf(szScrap, "Size: %d bytes\0", dwSizeLow);
    m_StrFileSize.SetWindowText(szScrap);

    return TRUE;
}


HRESULT CJukeboxDlg::DisplayFileDuration(void)
{
    HRESULT hr;

    if (!pMS)
        return E_NOINTERFACE;

    // Initialize the display in case we can't read the duration
    m_StrDuration.SetWindowText("<00:00.000>");

    // Is media time supported for this file?
    if (S_OK != pMS->IsFormatSupported(&TIME_FORMAT_MEDIA_TIME))
        return E_NOINTERFACE;

    // Read the time format to restore later
    GUID guidOriginalFormat;
    hr = pMS->GetTimeFormat(&guidOriginalFormat);
    if (FAILED(hr))
        return hr;

    // Set to time format for easy display
    hr = pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    if (FAILED(hr))
        return hr;

    // Read the file's duration
    LONGLONG llDuration;
    hr = pMS->GetDuration(&llDuration);
    if (FAILED(hr))
        return hr;

    // Return to the original format
    if (guidOriginalFormat != TIME_FORMAT_MEDIA_TIME)
    {
        hr = pMS->SetTimeFormat(&guidOriginalFormat);
        if (FAILED(hr))
            return hr;
    }

    // Convert the LONGLONG duration into human-readable format
    unsigned long nTotalMS = (unsigned long) llDuration / 10000; // 100ns -> ms
    int nMS = nTotalMS % 1000;
    int nSeconds = nTotalMS / 1000;
    int nMinutes = nSeconds / 60;
    nSeconds %= 60;

    // Update the display
    TCHAR szDuration[24];
    wsprintf(szDuration, _T("%02dm:%02d.%03ds\0"), nMinutes, nSeconds, nMS);
    m_StrDuration.SetWindowText(szDuration);

    return hr;
}


BOOL CJukeboxDlg::SupportsPropertyPage(IBaseFilter *pFilter) 
{
    HRESULT hr;
    TCHAR szNameToFind[128];
    ISpecifyPropertyPages *pSpecify;

    // Read the current filter name from the list box
    int nCurSel = m_ListFilters.GetCurSel();
    m_ListFilters.GetText(nCurSel, szNameToFind);

    // Discover if this filter contains a property page
    hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
    if (SUCCEEDED(hr)) 
    {
        pSpecify->Release();
        return TRUE;
    }
    else
        return FALSE;
}


void CJukeboxDlg::OnButtonProppage() 
{
    HRESULT hr;
    IBaseFilter *pFilter = NULL;
    TCHAR szNameToFind[128];
    ISpecifyPropertyPages *pSpecify;

    // Read the current filter name from the list box
    int nCurSel = m_ListFilters.GetCurSel();
    m_ListFilters.GetText(nCurSel, szNameToFind);

    // Read the current list box name and find it in the graph
    pFilter = FindFilterFromName(szNameToFind);
    if (!pFilter)
        return;

    // Discover if this filter contains a property page
    hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
    if (SUCCEEDED(hr)) 
    {
        do 
        {
            FILTER_INFO FilterInfo;
            hr = pFilter->QueryFilterInfo(&FilterInfo);
            if (FAILED(hr))
                break;

            CAUUID caGUID;
            hr = pSpecify->GetPages(&caGUID);
            if (FAILED(hr))
                break;

            pSpecify->Release();
        
            // Display the filter's property page
            OleCreatePropertyFrame(
                m_hWnd,                 // Parent window
                0,                      // x (Reserved)
                0,                      // y (Reserved)
                FilterInfo.achName,     // Caption for the dialog box
                1,                      // Number of filters
                (IUnknown **)&pFilter,  // Pointer to the filter 
                caGUID.cElems,          // Number of property pages
                caGUID.pElems,          // Pointer to property page CLSIDs
                0,                      // Locale identifier
                0,                      // Reserved
                NULL                    // Reserved
            );
            CoTaskMemFree(caGUID.pElems);
            FilterInfo.pGraph->Release(); 

        } while(0);
    }

    pFilter->Release();
}


void CJukeboxDlg::OnDblclkListFilters() 
{
    OnButtonProppage();
}


void CJukeboxDlg::DisplayECEvent(long lEventCode, long lParam1, long lParam2)
{
    static TCHAR szMsg[256];
    BOOL bMatch = TRUE;

#define HANDLE_EC(c)                              \
    case c:                                       \
        wsprintf(szMsg, TEXT("%s\0"), TEXT(#c));  \
        break;

    switch (lEventCode)
    {
        HANDLE_EC(EC_ACTIVATE);
        HANDLE_EC(EC_BUFFERING_DATA);
        HANDLE_EC(EC_CLOCK_CHANGED);
        HANDLE_EC(EC_COMPLETE);
        HANDLE_EC(EC_DEVICE_LOST);
        HANDLE_EC(EC_DISPLAY_CHANGED);
        HANDLE_EC(EC_END_OF_SEGMENT);
        HANDLE_EC(EC_ERROR_STILLPLAYING);
        HANDLE_EC(EC_ERRORABORT);
        HANDLE_EC(EC_EXTDEVICE_MODE_CHANGE);
        HANDLE_EC(EC_FULLSCREEN_LOST);
        HANDLE_EC(EC_GRAPH_CHANGED);
        HANDLE_EC(EC_LENGTH_CHANGED);
        HANDLE_EC(EC_NEED_RESTART);
        HANDLE_EC(EC_NOTIFY_WINDOW);
        HANDLE_EC(EC_OLE_EVENT);
        HANDLE_EC(EC_OPENING_FILE);
        HANDLE_EC(EC_PALETTE_CHANGED);
        HANDLE_EC(EC_PAUSED);
        HANDLE_EC(EC_QUALITY_CHANGE);
        HANDLE_EC(EC_REPAINT);
        HANDLE_EC(EC_SEGMENT_STARTED);
        HANDLE_EC(EC_SHUTTING_DOWN);
        HANDLE_EC(EC_SNDDEV_IN_ERROR);
        HANDLE_EC(EC_SNDDEV_OUT_ERROR);
        HANDLE_EC(EC_STARVATION);
        HANDLE_EC(EC_STEP_COMPLETE);
        HANDLE_EC(EC_STREAM_CONTROL_STARTED);
        HANDLE_EC(EC_STREAM_CONTROL_STOPPED);
        HANDLE_EC(EC_STREAM_ERROR_STILLPLAYING);
        HANDLE_EC(EC_STREAM_ERROR_STOPPED);
        HANDLE_EC(EC_TIMECODE_AVAILABLE);
        HANDLE_EC(EC_USERABORT);
        HANDLE_EC(EC_VIDEO_SIZE_CHANGED);
        HANDLE_EC(EC_WINDOW_DESTROYED);

    default:
        bMatch = FALSE;
        RetailOutput(TEXT("  Received unknown event code (0x%x)\r\n"), lEventCode);
        break;
    }

    // If a recognized event was found, add its name to the events list box
    if (bMatch)
        m_ListEvents.AddString(szMsg);
}


void CJukeboxDlg::OnButtonFramestep() 
{
    StepFrame();
}


//
// Some hardware decoders and video renderers support stepping media
// frame by frame with the IVideoFrameStep interface.  See the interface
// documentation for more details on frame stepping.
//
BOOL CJukeboxDlg::CanStep(void)
{
    HRESULT hr;
    IVideoFrameStep* pFS;

    hr = pGB->QueryInterface(__uuidof(IVideoFrameStep), (PVOID *)&pFS);
    if (FAILED(hr))
        return FALSE;

    // Check if this decoder can step
    hr = pFS->CanStep(0L, NULL); 

    pFS->Release();

    if (hr == S_OK)
        return TRUE;
    else
        return FALSE;
}

HRESULT CJukeboxDlg::StepFrame(void)
{
    // Get the Frame Stepping Interface
    HRESULT hr;
    IVideoFrameStep* pFS;

    hr = pGB->QueryInterface(__uuidof(IVideoFrameStep), (PVOID *)&pFS);
    if (FAILED(hr))
        return hr;

    // The graph must be paused for frame stepping to work
    if (g_psCurrent != State_Paused)
        OnPause();

    // Step one frame
    hr = pFS->Step(1, NULL); 

    // Clean up
    pFS->Release();
    return hr;
}
