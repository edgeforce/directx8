//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef	_MISC_UTILS_H
#define	_MISC_UTILS_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IDirectDrawSurface7*	LoadTextureFromFile( IDirect3DDevice7* device , const char* filename );
IDirectDrawSurface7*	LoadTextureFromResource( IDirect3DDevice7* device , DWORD resource_id );
IDirectDrawSurface7*	LoadAlphaTextureFromFile( IDirect3DDevice7* device , const char* filename );
IDirectDrawSurface7*	LoadAlphaTextureFromResource( IDirect3DDevice7* device , DWORD resource_id );




#endif
