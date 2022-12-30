//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _SYNC_OBJECTS_H
#define _SYNC_OBJECTS_H



//-----------------------------------------------------------------------------
// Name: 
// Desc: Simple wrapper for critical section
//-----------------------------------------------------------------------------
class CCriticalSection
{
public:
    CCriticalSection( DWORD spincount = 2000 )
    {
        InitializeCriticalSection( &m_CritSec );
    };

    ~CCriticalSection()
    {
        DeleteCriticalSection( &m_CritSec );
    };

    void    Enter()
    {
        EnterCriticalSection( &m_CritSec );
    };

    void    Leave()
    {
        LeaveCriticalSection( &m_CritSec );
    };

private:
    CRITICAL_SECTION    m_CritSec;
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Array of critical section objects. The methods allow locking single 
//       elements, rectangular regions, or a combination. Using these methods 
//       ensures the cells are locked/unlocked in a consistent order
//       which prevents deadlocks.
//-----------------------------------------------------------------------------
template< DWORD width , DWORD height > class    CLockArray
{
public:
    #define CS_RESOLUTION 4

    // Lock/Unlock a single cell
    void    LockCell( DWORD x , DWORD y )
    {
        x /= CS_RESOLUTION;
        y /= CS_RESOLUTION;

        m_Grid[y][x].Enter();
    };

    void    UnlockCell( DWORD x , DWORD y )
    {
        x /= CS_RESOLUTION;
        y /= CS_RESOLUTION;

        m_Grid[y][x].Leave();
    };

    // Lock/Unlock a rectangular range of cells
    void    LockRange( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 )
    {
        if ( x1 > x2 ) { DWORD t = x1; x1 = x2; x2 = t; };
        if ( y1 > y2 ) { DWORD t = y1; y1 = y2; y2 = t; };

        x1 /= CS_RESOLUTION;
        y1 /= CS_RESOLUTION;
        x2 /= CS_RESOLUTION;
        y2 /= CS_RESOLUTION;

        for ( INT y = y1 ; y <= (INT) y2 ; y++ )
            for ( INT x = x1 ; x <= (INT) x2 ; x++ )
                m_Grid[y][x].Enter();
    };

    void    UnlockRange( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 )
    {
        if ( x1 > x2 ) { DWORD t = x1; x1 = x2; x2 = t; };
        if ( y1 > y2 ) { DWORD t = y1; y1 = y2; y2 = t; };

        x1 /= CS_RESOLUTION;
        y1 /= CS_RESOLUTION;
        x2 /= CS_RESOLUTION;
        y2 /= CS_RESOLUTION;

        for ( INT y = y2 ; y >= (INT) y1 ; y-- )
            for ( INT x = x2 ; x >= (INT) x1 ; x-- )
                m_Grid[y][x].Leave();
    };

    // Lock/Unlock a rectangular range of cells plus a single other cell
    void    LockRangeAndCell( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 , DWORD cellx , DWORD celly )
    {
        if ( x1 > x2 ) { DWORD t = x1; x1 = x2; x2 = t; };
        if ( y1 > y2 ) { DWORD t = y1; y1 = y2; y2 = t; };

        x1 /= CS_RESOLUTION;
        y1 /= CS_RESOLUTION;
        x2 /= CS_RESOLUTION;
        y2 /= CS_RESOLUTION;
        cellx /= CS_RESOLUTION;
        celly /= CS_RESOLUTION;

        if ( (celly < y1) || ((celly == y1) && (cellx < x1)) )
            m_Grid[celly][cellx].Enter();

        for ( INT y = y1 ; y <= (INT) y2 ; y++ )
            for ( INT x = x1 ; x <= (INT) x2 ; x++ )
                m_Grid[y][x].Enter();

        if ( (celly > y2) || ((celly == y2) && (cellx > x2)) )
            m_Grid[celly][cellx].Enter();
    };

    void    UnlockRangeAndCell( DWORD x1 , DWORD y1 , DWORD x2 , DWORD y2 , DWORD cellx , DWORD celly )
    {
        if ( x1 > x2 ) { DWORD t = x1; x1 = x2; x2 = t; };
        if ( y1 > y2 ) { DWORD t = y1; y1 = y2; y2 = t; };

        x1 /= CS_RESOLUTION;
        y1 /= CS_RESOLUTION;
        x2 /= CS_RESOLUTION;
        y2 /= CS_RESOLUTION;
        cellx /= CS_RESOLUTION;
        celly /= CS_RESOLUTION;

        if ( (celly > y2) || ((celly == y2) && (cellx > x2)) )
            m_Grid[celly][cellx].Leave();

        for ( INT y = y2 ; y >= (INT) y1 ; y-- )
            for ( INT x = x2 ; x >= (INT) x1 ; x-- )
                m_Grid[y][x].Leave();

        if ( (celly < y1) || ((celly == y1) && (cellx < x1)) )
            m_Grid[celly][cellx].Leave();
    };

private:
    CCriticalSection    m_Grid[height][width];
};




#endif
