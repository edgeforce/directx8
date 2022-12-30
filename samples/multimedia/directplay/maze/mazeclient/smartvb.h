//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef	_SMARTVB_H
#define	_SMARTVB_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
// Smart vertex buffer template class. This performs the usual NOOVERWRITE/DISCARDCONTENTS
// filling loop. Simply call Begin(), then multiple instances of MakeRoom() to be given
// pointers to where to store vertex and index data, then End() when done. The class
// automatically flushes rendering as the buffer fills.
//
// By default rendering is performed by calling DrawIndexedPrimitive with a TRIANGELIST
// consisting of the submitted data, but the client can supply a callback if more complex
// rendering is required (perhaps for multipass?).
typedef void (__stdcall *SmartVBRenderCallback)( IDirect3DVertexBuffer7* pVB ,
												 DWORD dwStartVertex ,
												 DWORD dwNumVertices ,
												 LPWORD pwIndicies ,
												 DWORD dwIndexCount ,
												 void* pParam );




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
template< class VertType , DWORD VertFVF , DWORD NumIndex > class SmartVB
{
public:
	// Construction and destruction
	SmartVB() : m_pVB(NULL) , m_pDevice(NULL) { SetRenderCallback(); };
	~SmartVB()
	{
		Uninit();
	};

	// (Un)Initialisation
	HRESULT	Init( IDirect3D7* pD3D , IDirect3DDevice7* pDevice , DWORD dwNumVerts , DWORD dwCaps )
	{
		Uninit();
		m_dwNumVerts = dwNumVerts;
		m_pDevice = pDevice;
		if ( m_pDevice )
			m_pDevice->AddRef();
		else
			return E_INVALIDARG;
		m_pNextVert = NULL;
		m_dwNextVert = 0;
		m_dwNextIndex = 0;
		D3DVERTEXBUFFERDESC	vbdesc;
		vbdesc.dwSize = sizeof(vbdesc);
		vbdesc.dwCaps = dwCaps;
		vbdesc.dwNumVertices = dwNumVerts;
		vbdesc.dwFVF = VertFVF;
		return pD3D->CreateVertexBuffer( &vbdesc , &m_pVB , 0 );
	};
	void	Uninit()
	{
		if ( m_pVB )
		{
			m_pVB->Release();
			m_pVB = NULL;
		}
		if ( m_pDevice )
		{
			m_pDevice->Release();
			m_pDevice = NULL;
		}
	};

	// Set rendering callback. Passing NULL for the callback gets you a default
	// call to DrawIndexedPrimitive. pParam is passed through to the callback.
	void	SetRenderCallback( SmartVBRenderCallback pCallback = NULL , void* pParam = NULL )
	{
		if ( pCallback != NULL )
		{
			m_pCallback = pCallback;
			m_pCallbackParam = pParam;
		}
		else
		{
			m_pCallback = DefaultRenderCallback;
			m_pCallbackParam = this;
		}
	};

	// Begin data filling
	HRESULT	Begin()
	{
		// Check we haven't already begun
		if ( m_pNextVert != NULL )
			return E_FAIL;

		// Lock buffer, use NOOVERWRITE flag
		HRESULT	rc;
		rc = m_pVB->Lock( DDLOCK_NOSYSLOCK|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT|DDLOCK_WRITEONLY|
					      DDLOCK_NOOVERWRITE , (void**)&m_pNextVert , 0 );
		m_pNextVert += m_dwNextVert;
		m_dwFirstVert = m_dwNextVert;
		m_dwVertexCount = 0;
		m_dwIndexCount = 0;
		m_dwNextIndex = 0;

		return rc;
	};

	// Request space to submit data, may cause a 'flush' for rendering
	HRESULT	MakeRoom( DWORD dwNumVert , DWORD dwNumIndex , VertType** pVertPtr ,
					  WORD** pIndexPtr , WORD* wIndexOffset )
	{
		// Have we room left in the buffer?
		if ( (dwNumVert + m_dwNextVert >= m_dwNumVerts) ||
			 (dwNumIndex + m_dwNextIndex >= NumIndex) )
		{
			// Nope, so flush current batch
			Flush();

			// Check we could fit this in at all
			if ( (dwNumVert > m_dwNumVerts) || (dwNumIndex > NumIndex) )
				return E_INVALIDARG;
		}

		// Got room, so just return position in buffer
		*pVertPtr = m_pNextVert;
		*pIndexPtr = m_wIndicies + m_dwNextIndex;
		*wIndexOffset = WORD(m_dwVertexCount);

		// Update position
		m_pNextVert += dwNumVert;
		m_dwNextVert += dwNumVert;
		m_dwNextIndex += dwNumIndex;
		m_dwVertexCount += dwNumVert;
		m_dwIndexCount += dwNumIndex;

		// Done
		return S_OK;
	};

	// End data filling, and submit for rendering via callback
	void	End()
	{
		// Unlock VB
		m_pVB->Unlock();

		// Submit for rendering
		if ( m_dwVertexCount > 0 && m_dwIndexCount > 0 )
			m_pCallback( m_pVB , m_dwFirstVert , m_dwVertexCount , m_wIndicies ,
						 m_dwIndexCount , m_pCallbackParam );

		m_pNextVert = NULL;
	};

	// Flush data if we overflowed
	void	Flush()
	{
		// Unlock VB
		m_pVB->Unlock();

		// Submit for rendering
		if ( m_dwVertexCount > 0 && m_dwIndexCount > 0 )
			m_pCallback( m_pVB , m_dwFirstVert , m_dwVertexCount , m_wIndicies ,
						 m_dwIndexCount , m_pCallbackParam );

		// Lock VB again, this time use DISCARDCONTENTS flag
		m_pVB->Lock( DDLOCK_NOSYSLOCK|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT|DDLOCK_WRITEONLY|
					 DDLOCK_DISCARDCONTENTS , (void**)&m_pNextVert , 0 );
		m_dwFirstVert = 0;
		m_dwVertexCount = 0;
		m_dwIndexCount = 0;
		m_dwNextIndex = 0;
		m_dwNextVert = 0;
	};

protected:
	IDirect3DVertexBuffer7*	m_pVB;
	IDirect3DDevice7*		m_pDevice;
	DWORD					m_dwNumVerts;
	WORD					m_wIndicies[NumIndex];
	SmartVBRenderCallback	m_pCallback;
	void*					m_pCallbackParam;

	// Current position of 'write cursor' in buffer
	VertType*				m_pNextVert;
	DWORD					m_dwNextVert;
	DWORD					m_dwNextIndex;

	// Counts of vertices and indicies in the current batch
	DWORD					m_dwVertexCount;
	DWORD					m_dwIndexCount;
	DWORD					m_dwFirstVert;

	static void	__stdcall DefaultRenderCallback( IDirect3DVertexBuffer7* pVB ,
												 DWORD dwStartVertex ,
												 DWORD dwNumVertices ,
												 LPWORD pwIndicies ,
												 DWORD dwIndexCount ,
												 void* pParam )
	{
		((SmartVB*)pParam)->m_pDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST ,
			pVB , dwStartVertex , dwNumVertices , pwIndicies , dwIndexCount , 0 );
	};

private:
	SmartVB( const SmartVB& );
	void operator=( const SmartVB& );
};




#endif
