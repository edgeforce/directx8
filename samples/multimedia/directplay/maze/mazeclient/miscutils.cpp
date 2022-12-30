//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#define D3D_OVERLOADS
#include <windows.h>
#include <basetsd.h>
#include <d3dx.h>
#include <stdio.h>
#include <math.h>
#include <dplay8.h>
#include <dpaddr.h>
#include <dxerr8.h>
#include "DXUtil.h"
#include "SyncObjects.h"
#include "MiscUtils.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IDirectDrawSurface7* LoadTextureFromFile( IDirect3DDevice7* device, const TCHAR* filename )
{
    HRESULT                 hr;
    DWORD                   width = D3DX_DEFAULT;
    DWORD                   height = D3DX_DEFAULT;
    D3DX_SURFACEFORMAT      format = D3DX_SF_UNKNOWN;
    IDirectDrawSurface7*    pTexture = NULL;
    DWORD                   num_mips = 0;
    CHAR                    strFileName[MAX_PATH];
    CHAR*                   pstrFileName;

    if(!IsBadReadPtr((BYTE*)filename,1) )
    {
        DXUtil_ConvertGenericStringToAnsi( strFileName, filename );
        pstrFileName = strFileName;
    }
    else
        pstrFileName = (CHAR*) (BYTE*)filename;

    hr = D3DXCreateTextureFromFile( device, NULL, &width, &height, &format ,
                                    NULL, &pTexture, &num_mips, pstrFileName,
                                    D3DX_FT_LINEAR );

    if( FAILED( hr ) )
    {
        DXTRACE_ERR_NOMSGBOX( TEXT("D3DXCreateTextureFromFile"), hr );
        return NULL;
    }
    else
        return pTexture;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IDirectDrawSurface7* LoadAlphaTextureFromFile( IDirect3DDevice7* device, const TCHAR* filename )
{
    // Load file into pTexture with an alpha channel. Don't create mipmaps at this point
    HRESULT                 hr;
    DWORD                   flags = D3DX_TEXTURE_NOMIPMAP;
    DWORD                   width = D3DX_DEFAULT;
    DWORD                   height = D3DX_DEFAULT;
    D3DX_SURFACEFORMAT      format = D3DX_SF_A1R5G5B5;
    IDirectDrawSurface7*    pTexture = NULL;
    DWORD                   num_mips = 0;
    CHAR                    strFileName[MAX_PATH];
    CHAR*                   pstrFileName;

    if(!IsBadReadPtr((BYTE*)filename,1) )
    {
        DXUtil_ConvertGenericStringToAnsi( strFileName, filename );
        pstrFileName = strFileName;
    }
    else
        pstrFileName = (CHAR*) (BYTE*)filename;

    hr = D3DXCreateTextureFromFile( device, &flags, &width, &height, &format,
                                    NULL, &pTexture, &num_mips, pstrFileName,
                                    D3DX_FT_LINEAR );
    if( FAILED( hr ) )
    {
        DXTRACE_ERR_NOMSGBOX( TEXT("D3DXCreateTextureFromFile"), hr );
        return NULL;
    }

    // Check we got a format we can cope with
    if( format != D3DX_SF_A1R5G5B5 && format != D3DX_SF_A4R4G4B4 && format != D3DX_SF_A8R8G8B8 )
    {
        DXTRACE( TEXT("Unable to create alpha pTexture with supported format\n") );
        DXTRACE_ERR_NOMSGBOX( TEXT("format check"), hr );
        pTexture->Release();
        return NULL;
    }

    // Lock the pTexture
    DDSURFACEDESC2  desc = {sizeof(desc)};
    if( FAILED( hr = pTexture->Lock( NULL, &desc, 
                                    DDLOCK_WAIT|DDLOCK_NOSYSLOCK|DDLOCK_SURFACEMEMORYPTR, 
                                    NULL ) ) )
    {
        DXTRACE_ERR_NOMSGBOX( TEXT("Lock"), hr );
        pTexture->Release();
        return NULL;
    }

    // Now loop through all the pixels and set the alpha channel. 0 if pixel is black, otherwise 255
    if( format == D3DX_SF_A1R5G5B5 )
    {
        for( DWORD y = 0; y < desc.dwHeight; y++ )
        {
            WORD*   ptr = (WORD*)desc.lpSurface;
            for( DWORD x = 0; x < desc.dwWidth; x++ )
            {
                WORD    pixel = *ptr;
                if( (pixel & 0x7fff) == 0 )
                    *ptr++ = 0;
                else
                    *ptr++ = 0x8000|pixel;
            }
            desc.lpSurface = ((TCHAR*)desc.lpSurface) + desc.lPitch;
        }
    }
    else if( format == D3DX_SF_A4R4G4B4 )
    {
        for( DWORD y = 0; y < desc.dwHeight; y++ )
        {
            WORD* ptr = (WORD*)desc.lpSurface;
            for( DWORD x = 0; x < desc.dwWidth; x++ )
            {
                WORD pixel = *ptr;
                if( (pixel & 0x0fff) == 0 )
                    *ptr++ = 0;
                else
                    *ptr++ = 0xf000|pixel;
            }
            desc.lpSurface = ((TCHAR*)desc.lpSurface) + desc.lPitch;
        }
    }
    else
    {
        for( DWORD y = 0; y < desc.dwHeight; y++ )
        {
            DWORD*  ptr = (DWORD*)desc.lpSurface;
            for( DWORD x = 0; x < desc.dwWidth; x++ )
            {
                DWORD pixel = *ptr;
                if( (pixel & 0x00ffffff) == 0 )
                    *ptr++ = 0;
                else
                    *ptr++ = 0xff000000|pixel;
            }
            desc.lpSurface = ((TCHAR*)desc.lpSurface) + desc.lPitch;
        }
    }
    
    // Unlock the pTexture
    pTexture->Unlock( NULL );

    // Done
    return pTexture;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IDirectDrawSurface7* LoadTextureFromResource( IDirect3DDevice7* device, DWORD resource_id )
{
    return LoadTextureFromFile( device, (const TCHAR*)(INT_PTR)resource_id );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
IDirectDrawSurface7* LoadAlphaTextureFromResource( IDirect3DDevice7* device, DWORD resource_id )
{
    return LoadAlphaTextureFromFile( device, (const TCHAR*)(INT_PTR)resource_id );
}

