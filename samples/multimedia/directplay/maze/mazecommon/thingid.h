//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _THINGID_H
#define _THINGID_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: Thing IDs are 32-bit values that refer to a particular thing. They are 
//       broken up into two bitfields, one of which can be used into an index 
//       of a list of thing 'slots', the other bitfield is a "uniqueness" value 
//       that is incremented each time a new thing is created. Hence, although
//       the same slot may be reused by different things are different times, 
//       it's possible to distinguish between the two by comparing uniqueness 
//       values (you can just compare the whole 32-bit id).
//-----------------------------------------------------------------------------
typedef DWORD   ThingID;

#define THING_SLOT_BITS 13
#define MAX_THINGS      (1<<THING_SLOT_BITS)
#define THING_SLOT_MASK (MAX_THINGS-1)




#endif
