/*
 * D3 - intersection of triangle and OBB
 * 22-04-2001: Created! (19:40:16)
 * NOTES:
 * - Source code taken from Magic Software website, see header below
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/intersect.h>
DEBUG_ENABLE

// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000, 2001.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

#include <math.h>

enum ResultAxes
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_A0xE0, AXIS_A0xE1, AXIS_A0xE2,
    AXIS_A1xE0, AXIS_A1xE1, AXIS_A1xE2,
    AXIS_A2xE0, AXIS_A2xE1, AXIS_A2xE2
};

//---------------------------------------------------------------------------
// macros for fast arithmetic
//---------------------------------------------------------------------------
#define DIFF(diff,p,q) \
{ \
    diff.x=p.x-q.x; \
    diff.y=p.y-q.y; \
    diff.z=p.z-q.z; \
}
#ifdef OBS
    diff[0] = p[0]-q[0]; \
    diff[1] = p[1]-q[1]; \
    diff[2] = p[2]-q[2];
#endif
//---------------------------------------------------------------------------
#define DOT(p,q) \
    (p.x*q.x+p.y*q.y+p.z*q.z)
    //(p[0]*q[0]+p[1]*q[1]+p[2]*q[2])
//---------------------------------------------------------------------------
#define CROSS(cross,p,q) \
{ \
  cross.x=p.y*q.z-p.z*q.y; \
  cross.y=p.z*q.x-p.x*q.z; \
  cross.z=p.x*q.y-p.y*q.x; \
}
#ifdef OBS
    cross[0] = p[1]*q[2]-p[2]*q[1]; \
    cross[1] = p[2]*q[0]-p[0]*q[2]; \
    cross[2] = p[0]*q[1]-p[1]*q[0];
#endif
//---------------------------------------------------------------------------
#define XXXCOMBO(combo,p,t,q) \
{ \
    combo[0] = p[0]+t*q[0]; \
    combo[1] = p[1]+t*q[1]; \
    combo[2] = p[2]+t*q[2]; \
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define FABS(x) ((float)fabs(x))
//---------------------------------------------------------------------------
// compare [-r,r] to [NdD+dt*NdW]
#define TESTV0(NdD,R,dt,N,W,axis) \
{ \
    if ( NdD > R ) \
    { \
        if ( NdD+dt*DOT(N,W) > R ) \
            return axis; \
    } \
    else if ( NdD < -R ) \
    { \
        if ( NdD+dt*DOT(N,W) < -R ) \
            return axis; \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d0,p+d1},max{p,p+d0,p+d1}]+t*w where t >= 0
#define TESTV1(p,t,w,d0,d1,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        float min; \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (d1) >= 0.0f ) \
            { \
                if ( (p)+(t)*(w) > (r) ) \
                    return axis; \
            } \
            else \
            { \
                min = (p)+(d1); \
                if ( min > (r) && min+(t)*(w) > (r) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            min = (p)+(d1); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
        else \
        { \
            min = (p)+(d0); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        float max; \
        if ( (d0) <= 0.0f ) \
        { \
            if ( (d1) <= 0.0f ) \
            { \
                if ( (p)+(t)*(w) < -(r) ) \
                    return axis; \
            } \
            else \
            { \
                max = (p)+(d1); \
                if ( max < -(r) && max+(t)*(w) < -(r) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            max = (p)+(d1); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
        else \
        { \
            max = (p)+(d0); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d},max{p,p+d}]+t*w where t >= 0
#define TESTV2(p,t,w,d,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        if ( (d) >= 0.0f ) \
        { \
            if ( (p)+(t)*(w) > (r) ) \
                return axis; \
        } \
        else \
        { \
            float min = (p)+(d); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        if ( (d) <= 0.0f ) \
        { \
            if ( (p)+(t)*(w) < -(r) ) \
                return axis; \
        } \
        else \
        { \
            float max = (p)+(d); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------

unsigned int d3TestTriObb(float dt,DVector3* tri[3],
  const DVector3& triVel,DOBB& box,const DVector3& boxVel)
{
    // convenience variables
    DVector3* A = box.GetWorldAxes();
    float* extA = box.GetWorldExtent();

    // Compute relative velocity of triangle with respect to box so that box
    // may as well be stationary.
    DVector3 W;
    DIFF(W,triVel,boxVel);

    // construct triangle normal, difference of center and vertex (18 ops)
    DVector3 D, E[2], N;
    E[0]=*tri[1];
    E[0].Subtract(tri[0]);
    E[1]=*tri[2];
    E[1].Subtract(tri[0]);
#ifdef OBS
    E[0] = (*tri[1])-(*tri[0]);
    E[1] = (*tri[2])-(*tri[0]);
#endif
    CROSS(N,E[0],E[1]);
    //D = (*tri[0]) - box.GetWorldCenter();
    D=*tri[0];
    D.Subtract(box.GetWorldCenter());

    // axis C+t*N
    float A0dN = DOT(A[0],N);
    float A1dN = DOT(A[1],N);
    float A2dN = DOT(A[2],N);
    float R = FABS(extA[0]*A0dN)+FABS(extA[1]*A1dN)+FABS(extA[2]*A2dN);
    float NdD = DOT(N,D);
    TESTV0(NdD,R,dt,N,W,AXIS_N);

    // axis C+t*A0
    float A0dD = DOT(A[0],D);
    float A0dE0 = DOT(A[0],E[0]);
    float A0dE1 = DOT(A[0],E[1]);
    float A0dW = DOT(A[0],W);
    TESTV1(A0dD,dt,A0dW,A0dE0,A0dE1,extA[0],AXIS_A0);

    // axis C+t*A1
    float A1dD = DOT(A[1],D);
    float A1dE0 = DOT(A[1],E[0]);
    float A1dE1 = DOT(A[1],E[1]);
    float A1dW = DOT(A[1],W);
    TESTV1(A1dD,dt,A1dW,A1dE0,A1dE1,extA[1],AXIS_A1);

    // axis C+t*A2
    float A2dD = DOT(A[2],D);
    float A2dE0 = DOT(A[2],E[0]);
    float A2dE1 = DOT(A[2],E[1]);
    float A2dW = DOT(A[2],W);
    TESTV1(A2dD,dt,A2dW,A2dE0,A2dE1,extA[2],AXIS_A2);

    // axis C+t*A0xE0
    DVector3 A0xE0;
    CROSS(A0xE0,A[0],E[0]);
    float A0xE0dD = DOT(A0xE0,D);
    float A0xE0dW = DOT(A0xE0,W);
    R = FABS(extA[1]*A2dE0)+FABS(extA[2]*A1dE0);
    TESTV2(A0xE0dD,dt,A0xE0dW,A0dN,R,AXIS_A0xE0);

    // axis C+t*A0xE1
    DVector3 A0xE1;
    CROSS(A0xE1,A[0],E[1]);
    float A0xE1dD = DOT(A0xE1,D);
    float A0xE1dW = DOT(A0xE1,W);
    R = FABS(extA[1]*A2dE1)+FABS(extA[2]*A1dE1);
    TESTV2(A0xE1dD,dt,A0xE1dW,-A0dN,R,AXIS_A0xE1);

    // axis C+t*A0xE2
    float A1dE2 = A1dE1-A1dE0;
    float A2dE2 = A2dE1-A2dE0;
    float A0xE2dD = A0xE1dD-A0xE0dD;
    float A0xE2dW = A0xE1dW-A0xE0dW;
    R = FABS(extA[1]*A2dE2)+FABS(extA[2]*A1dE2);
    TESTV2(A0xE2dD,dt,A0xE2dW,-A0dN,R,AXIS_A0xE2);

    // axis C+t*A1xE0
    DVector3 A1xE0;
    CROSS(A1xE0,A[1],E[0]);
    float A1xE0dD = DOT(A1xE0,D);
    float A1xE0dW = DOT(A1xE0,W);
    R = FABS(extA[0]*A2dE0)+FABS(extA[2]*A0dE0);
    TESTV2(A1xE0dD,dt,A1xE0dW,A1dN,R,AXIS_A1xE0);

    // axis C+t*A1xE1
    DVector3 A1xE1;
    CROSS(A1xE1,A[1],E[1]);
    float A1xE1dD = DOT(A1xE1,D);
    float A1xE1dW = DOT(A1xE1,W);
    R = FABS(extA[0]*A2dE1)+FABS(extA[2]*A0dE1);
    TESTV2(A1xE1dD,dt,A1xE1dW,-A1dN,R,AXIS_A1xE1);

    // axis C+t*A1xE2
    float A0dE2 = A0dE1-A0dE0;
    float A1xE2dD = A1xE1dD-A1xE0dD;
    float A1xE2dW = A1xE1dW-A1xE0dW;
    R = FABS(extA[0]*A2dE2)+FABS(extA[2]*A0dE2);
    TESTV2(A1xE2dD,dt,A1xE2dW,-A1dN,R,AXIS_A1xE2);

    // axis C+t*A2xE0
    DVector3 A2xE0;
    CROSS(A2xE0,A[2],E[0]);
    float A2xE0dD = DOT(A2xE0,D);
    float A2xE0dW = DOT(A2xE0,W);
    R = FABS(extA[0]*A1dE0)+FABS(extA[1]*A0dE0);
    TESTV2(A2xE0dD,dt,A2xE0dW,A2dN,R,AXIS_A2xE0);

    // axis C+t*A2xE1
    DVector3 A2xE1;
    CROSS(A2xE1,A[2],E[1]);
    float A2xE1dD = DOT(A2xE1,D);
    float A2xE1dW = DOT(A2xE1,W);
    R = FABS(extA[0]*A1dE1)+FABS(extA[1]*A0dE1);
    TESTV2(A2xE1dD,dt,A2xE1dW,-A2dN,R,AXIS_A2xE1);

    // axis C+t*A2xE2
    float A2xE2dD = A2xE1dD-A2xE0dD;
    float A2xE2dW = A2xE1dW-A2xE0dW;
    R = FABS(extA[0]*A1dE2)+FABS(extA[1]*A0dE2);
    TESTV2(A2xE2dD,dt,A2xE2dW,-A2dN,R,AXIS_A2xE2);

    // intersection occurs
    return INTERSECTION;
}
//---------------------------------------------------------------------------
