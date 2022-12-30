//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef	_MODULE_H
#define	_MODULE_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#include	"FrameRate.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
#define	MOD_DISP_WINDOWED		(1<<0)		// Windowed display
#define	MOD_DISP_REFRAST		(1<<1)		// Force refrast
#define	MOD_DISP_SWTL			(1<<2)		// Force software T&L
#define	MOD_DISP_PREFER32BIT	(1<<3)		// Prefer 32-bit screen modes for fullscreen
#define	MOD_DISP_SWRASTER		(1<<4)		// Force software (RGB) rasterisation




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
struct	CommonStateBlocks
{
	DWORD	ModulateTexture;
	DWORD	CopyTexture;
	DWORD	CopyColour;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class	CModule
{
public:
	// Construction and destruction
	CModule();
	virtual	~CModule();

	// Init methods called externally
	virtual	HRESULT	OneTimeInit( HWND hWnd );
	virtual	void	OneTimeShutdown();
	virtual	HRESULT	DisplayInit( DWORD flags , IDirectDrawSurface7* target , RECT* rect = NULL );
	virtual	void	DisplayShutdown();

	virtual	HRESULT	OnSetRenderTarget( IDirectDrawSurface7* target , RECT* rect );

	// Do a frame of stuff
	virtual	void	RenderFrame( FLOAT fElapsed );
	
	// Repaint window
			void	OnPaint();

	// Control messages
	virtual	void	OnChar( TCHAR ch );
	virtual	void	OnMouseMove( DWORD keys , DWORD x , DWORD y );
	virtual	void	OnLButtonDown( DWORD keys , DWORD x , DWORD y );
	virtual	void	OnLButtonUp( DWORD keys , DWORD x , DWORD y );
	virtual	void	OnRButtonDown( DWORD keys , DWORD x , DWORD y );
	virtual	void	OnRButtonUp( DWORD keys , DWORD x , DWORD y );

	// Check for the main loop to see if it should go
	BOOL	OkayToProceed();

	// Overrideable, possibly called by OkayToProceed()
	virtual	void	RestoreSurfaceContent();

protected:
	IDirect3DDevice7*		m_pDevice;
	IDirectDraw7*			m_pDDraw;
	ID3DXContext*			m_pXContext;
	IDirect3D7*				m_pD3D;
	IDirectDrawSurface7*	m_pBackBuffer;
	DWORD					m_dwWidth;				// These are the size of the subrect
	DWORD					m_dwHeight;				// we're hitting
	DWORD					m_dwEntireTargetWidth;	// These are the size of the entire
	DWORD					m_dwEntireTargetHeight;	// target surface
	HWND					m_hWnd;
	DWORD					m_dwFlags;
	BOOL					m_bSoftwareRasterisation;

	DWORD					m_dwBPP;
	DWORD					m_dwZBits;
	DWORD					m_dwStencilBits;
	BOOL					m_bNeedToRestore;

	CFrameRate*				m_pFrameRate;
	CommonStateBlocks*		m_pStateBlocks;

	D3DDEVICEDESC7			m_DeviceCaps;
	DWORD					m_dwVBMemType;

	HRESULT	InitStateBlocks();

	// Call at the start and end of RenderFrame
	HRESULT	BeginFrame();
	void	EndFrame();

	virtual void Clear( DWORD flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER );

	void	DrawStats();

private:
	CModule( const CModule& );
	void operator=( const CModule& );
};




#endif
