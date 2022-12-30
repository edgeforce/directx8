//----------------------------------------------------------------------------
// File: 
//
// Desc: 
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _VECTOR2_H
#define _VECTOR2_H




//-----------------------------------------------------------------------------
// Name: 
// Desc: Just what the world needs, another 2d vector class...
//-----------------------------------------------------------------------------
struct Vector2
{
    Vector2() {};
    Vector2( float X , float Y ) : x(X),y(Y) {};

    float   x,y;

    Vector2 operator+( const Vector2& v ) const { return Vector2(x+v.x,y+v.y); };
    Vector2 operator-( const Vector2& v ) const { return Vector2(x-v.x,y-v.y); };
    void operator+=( const Vector2& v ) { x +=v.x; y += v.y; };
    void operator-=( const Vector2& v ) { x -=v.x; y -= v.y; };

    Vector2 operator*( float f ) const { return Vector2(x*f,y*f); };
    Vector2 operator/( float f ) const { float s = 1/f; return Vector2(x*s,y*s); };
    void    operator*=( float f ) { x*=f; y*=f; };
    void    operator/=( float f ) { float s = 1/f; x*=s; y*=f; };

    float   operator*( const Vector2& v ) const { return ((x*v.x)+(y*v.y)); };
    float   operator^( const Vector2& v ) const { return ((x*v.y)-(y*v.x)); };

    BOOL    operator==( const Vector2& v ) const { return ((x==v.x)&&(y==v.y)); };
    BOOL    operator!=( const Vector2& v ) const { return ((x!=v.x)||(y!=v.y)); };
};

inline Vector2 operator*( float f , const Vector2& v ) { return Vector2(f*v.x,f*v.y); }




#endif
