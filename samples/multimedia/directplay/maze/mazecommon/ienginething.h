//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _IENGINE_THING_H
#define _IENGINE_THING_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
interface   IEngineThing
{
public:
    DWORD   dwLastFrame;

    virtual const D3DXVECTOR3&  GetPos() const = 0;
    virtual IEngineThing*       GetNext() const = 0;
};




#endif  _IENGINE_THING_H
