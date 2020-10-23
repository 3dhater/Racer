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

#include <assert.h>
#include <math.h>

enum ResultAxes
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0,
    AXIS_A1,
    AXIS_A2,
    AXIS_A0xE0,
    AXIS_A0xE1,
    AXIS_A0xE2,
    AXIS_A1xE0,
    AXIS_A1xE1,
    AXIS_A1xE2,
    AXIS_A2xE0,
    AXIS_A2xE1,
    AXIS_A2xE2
};

char* tr3bx3Axis[14] =
{
    "intersection",
    "N",
    "A0",
    "A1",
    "A2",
    "A0xE0",
    "A0xE1",
    "A0xE2",
    "A1xE0",
    "A1xE1",
    "A1xE2",
    "A2xE0",
    "A2xE1",
    "A2xE2"
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
#define COMBO(combo,p,t,q) \
{ \
  combo.x=p.x+t*q.x; \
  combo.y=p.y+t*q.y; \
  combo.z=p.z+t*q.z; \
}
#ifdef OBS
    combo[0] = p[0]+t*q[0]; \
    combo[1] = p[1]+t*q[1]; \
    combo[2] = p[2]+t*q[2];
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define FABS(x) (float(fabs(x)))
//---------------------------------------------------------------------------
// compare [-r,r] to [NdD+dt*NdW]
#define FIND0(NdD,R,t,N,W,tmax,type,side,axis) \
{ \
    float tmp, NdW; \
    if ( NdD > R ) \
    { \
        NdW = DOT(N,W); \
        if ( NdD+t*NdW > R ) \
            return axis; \
        tmp = t*(R-NdD)/NdW; \
        if ( tmp > tmax ) \
        { \
            tmax = tmp; \
            type = axis; \
            side = +1; \
        } \
    } \
    else if ( NdD < -R ) \
    { \
        NdW = DOT(N,W); \
        if ( NdD+t*NdW < -R ) \
            return axis; \
        tmp = -t*(R+NdD)/NdW; \
        if ( tmp > tmax ) \
        { \
            tmax = tmp; \
            type = axis; \
            side = -1; \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d0,p+d1},max{p,p+d0,p+d1}]+t*w where t >= 0
#define FIND1(p,t,w,d0,d1,r,tmax,type,extr,side,axis) \
{ \
    float tmp; \
    if ( (p) > (r) ) \
    { \
        float min; \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (d1) >= 0.0f ) \
            { \
                if ( (p)+(t)*(w) > (r) ) \
                    return axis; \
                tmp = (t)*((r)-(p))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = +1; \
                } \
            } \
            else \
            { \
                min = (p)+(d1); \
                if ( min > (r) ) \
                { \
                    if ( min+(t)*(w) > (r) ) \
                        return axis; \
                    tmp = (t)*((r)-min)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 2; \
                        side = +1; \
                    } \
                } \
            } \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            min = (p)+(d1); \
            if ( min > (r) ) \
            { \
                if ( min+(t)*(w) > (r) ) \
                    return axis; \
                tmp = (t)*((r)-min)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 2; \
                    side = +1; \
                } \
            } \
        } \
        else \
        { \
            min = (p)+(d0); \
            if ( min > (r) ) \
            { \
                if ( min+(t)*(w) > (r) ) \
                    return axis; \
                tmp = (t)*((r)-min)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = +1; \
                } \
            } \
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
                tmp = -(t)*((r)+(p))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = -1; \
                } \
            } \
            else \
            { \
                max = (p)+(d1); \
                if ( max < -(r) ) \
                { \
                    if ( max+(t)*(w) < -(r) ) \
                        return axis; \
                    tmp = -(t)*((r)+max)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 2; \
                        side = -1; \
                    } \
                } \
            } \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            max = (p)+(d1); \
            if ( max < -(r) ) \
            { \
                if ( max+(t)*(w) < -(r) ) \
                    return axis; \
                tmp = -(t)*((r)+max)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 2; \
                    side = -1; \
                } \
            } \
        } \
        else \
        { \
            max = (p)+(d0); \
            if ( max < -(r) ) \
            { \
                if ( max+(t)*(w) < -(r) ) \
                    return axis; \
                tmp = -(t)*((r)+max)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = -1; \
                } \
            } \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d},max{p,p+d}]+t*q where t >= 0
#define FIND2(p,t,w,d,r,tmax,type,extr,side,axis) \
{ \
    float tmp; \
    if ( (p) > (r) ) \
    { \
        if ( (d) >= 0.0f ) \
        { \
            if ( (p)+(t)*(w) > (r) ) \
                return axis; \
            tmp = (t)*((r)-(p))/(w); \
            if ( tmp > tmax ) \
            { \
                tmax = tmp; \
                type = axis; \
                extr = 0; \
                side = +1; \
            } \
        } \
        else \
        { \
            float min = (p)+(d); \
            if ( min > (r) ) \
            { \
                if ( min+(t)*(w) > (r) ) \
                    return axis; \
                tmp = (t)*((r)-min)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = +1; \
                } \
            } \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        if ( (d) <= 0.0f ) \
        { \
            if ( (p)+(t)*(w) < -(r) ) \
                return axis; \
            tmp = -(t)*((r)+(p))/(w); \
            if ( tmp > tmax ) \
            { \
                tmax = tmp; \
                type = axis; \
                extr = 0; \
                side = -1; \
            } \
        } \
        else \
        { \
            float max = (p)+(d); \
            if ( max < -(r) ) \
            { \
                if ( max+(t)*(w) < -(r) ) \
                    return axis; \
                tmp = -(t)*((r)+max)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = -1; \
                } \
            } \
        } \
    } \
}
//---------------------------------------------------------------------------
#define GET_COEFF(coeff,sign0,sign1,side,cmat,ext) \
{ \
    if ( cmat > 0.0f ) \
    { \
        coeff = sign0##side*ext; \
    } \
    else if ( cmat < 0.0f ) \
    { \
        coeff = sign1##side*ext; \
    } \
    else \
    { \
        coeff = 0.0f; \
    } \
}
//---------------------------------------------------------------------------
#define GET_POINT(P,box,T,V,x) \
{ \
    DVector3* center = box.GetWorldCenter(); \
    DVector3* axis = box.GetWorldAxes(); \
    P.x=center->x+T*V.x+x[0]*axis[0].x+x[1]*axis[1].x+x[2]*axis[2].x; \
    P.y=center->y+T*V.y+x[0]*axis[0].y+x[1]*axis[1].y+x[2]*axis[2].y; \
    P.z=center->z+T*V.z+x[0]*axis[0].z+x[1]*axis[1].z+x[2]*axis[2].z; \
}
#ifdef OBS
    P.x=center.x+T*V.x+x[0]*axis[0].x+x[1]*axis[1].x+x[2]*axis[2].x; \
    P.y=center.y+T*V.y+x[0]*axis[0].y+x[1]*axis[1].y+x[2]*axis[2].y; \
    P.z=center.z+T*V.z+x[0]*axis[0].z+x[1]*axis[1].z+x[2]*axis[2].z; \
    for (int k = 0; k < 3; k++) \
    { \
        P[k] = center[k] + T*V[k] + x[0]*axis[0][k] + \
            x[1]*axis[1][k] + x[2]*axis[2][k]; \
    }
#endif
//---------------------------------------------------------------------------
#ifdef OBS
unsigned int d3FindTriObb(float dt,DVector3* tri[3],
  const DVector3& triVel,DOBB& box,const DVector3& boxVel,
  dfloat& T,DVector3& P)
#endif
unsigned int d3FindTriObb(float dt,/*const*/ DVector3* tri[3],
  const DVector3& triVel,/*const*/ DOBB& box,const DVector3& boxVel,
  dfloat& T,DVector3& P)
// Find an intersection. If not, a separating axis index is returned.
// If an intersection is found, 'P' contains the closest one.
{
//qdbg("\n=> d3FindTriOBB\n");
  // convenience variables
  /*const*/ DVector3 *A=box.GetWorldAxes();
  const float *extA=box.GetWorldExtent();

  // Compute relative velocity of triangle with respect to box so that box
  // may as well be stationary.
  DVector3 W;
  DIFF(W,triVel,boxVel);

  // construct triangle normal, difference of center and vertex
  DVector3 D, E[3], N;
  E[0]=*tri[1]; E[0].Subtract(tri[0]);
  E[1]=*tri[2]; E[1].Subtract(tri[0]);
#ifdef OBS
    E[0] = (*tri[1]) - (*tri[0]);
    E[1] = (*tri[2]) - (*tri[0]);
#endif
  CROSS(N,E[0],E[1]);
#ifdef OBS
tri[0]->DbgPrint("tri0");
tri[1]->DbgPrint("tri1");
tri[2]->DbgPrint("tri2");
E[0].DbgPrint("E[0]");
E[1].DbgPrint("E[1]");
N.DbgPrint("N");
qdbg("Body axes:\n");
A[0].DbgPrint("A0");
A[1].DbgPrint("A1");
A[2].DbgPrint("A2");
#endif

  //D = (*tri[0]) - *box.GetWorldCenter();
  D=*tri[0];
  D.Subtract(box.GetWorldCenter());
#ifdef OBS
D.DbgPrint("D (distance to box)");
box.GetWorldCenter()->DbgPrint("box center");
#endif

  // track maximum time of projection-intersection
  unsigned int type=INTERSECTION, extr=0;
  int side = 0;
  T = 0.0f;

  // axis C+t*N
  float AN[3];
  AN[0] = DOT(A[0],N);
  AN[1] = DOT(A[1],N);
  AN[2] = DOT(A[2],N);
#ifdef OBS
qdbg("Vector AN=%f,%f,%f\n",AN[0],AN[1],AN[2]);
qdbg("  extents: %f x %f x %f\n",extA[0],extA[1],extA[2]);
#endif
  float R = extA[0]*FABS(AN[0])+extA[1]*FABS(AN[1])+extA[2]*FABS(AN[2]);
  float NdD = DOT(N,D);
//qdbg("NdD=%f, R=%f\n",NdD,R);
  FIND0(NdD,R,dt,N,W,T,type,side,AXIS_N);

    // axis C+t*A0
    float AD[3], AE[3][3];
    AD[0] = DOT(A[0],D);
    AE[0][0] = DOT(A[0],E[0]);
    AE[0][1] = DOT(A[0],E[1]);
    float A0dW = DOT(A[0],W);
    FIND1(AD[0],dt,A0dW,AE[0][0],AE[0][1],extA[0],T,type,extr,side,AXIS_A0);

    // axis C+t*A1
    AD[1] = DOT(A[1],D);
    AE[1][0] = DOT(A[1],E[0]);
    AE[1][1] = DOT(A[1],E[1]);
    float A1dW = DOT(A[1],W);
    FIND1(AD[1],dt,A1dW,AE[1][0],AE[1][1],extA[1],T,type,extr,side,AXIS_A1);

    // axis C+t*A2
    AD[2] = DOT(A[2],D);
    AE[2][0] = DOT(A[2],E[0]);
    AE[2][1] = DOT(A[2],E[1]);
//qdbg(" AD[2]=%f, AE[2][0]=%f, AE[2][1]=%f\n",AD[2],AE[2][0],AE[2][1]);
    float A2dW = DOT(A[2],W);
//qdbg("  A2dW=%f\n",A2dW);
    FIND1(AD[2],dt,A2dW,AE[2][0],AE[2][1],extA[2],T,type,extr,side,AXIS_A2);
//qdbg("type=%d, extr=%d, side=%d\n",type,extr,side);

    // axis C+t*A0xE0
    DVector3 A0xE0;
    CROSS(A0xE0,A[0],E[0]);
    float A0xE0dD = DOT(A0xE0,D);
    float A0xE0dW = DOT(A0xE0,W);
    R = extA[1]*FABS(AE[2][0])+extA[2]*FABS(AE[1][0]);
    FIND2(A0xE0dD,dt,A0xE0dW,AN[0],R,T,type,extr,side,AXIS_A0xE0);

    // axis C+t*A0xE1
    DVector3 A0xE1;
    CROSS(A0xE1,A[0],E[1]);
    float A0xE1dD = DOT(A0xE1,D);
    float A0xE1dW = DOT(A0xE1,W);
    R = extA[1]*FABS(AE[2][1])+extA[2]*FABS(AE[1][1]);
    FIND2(A0xE1dD,dt,A0xE1dW,-AN[0],R,T,type,extr,side,AXIS_A0xE1);

    // axis C+t*A0xE2
    AE[1][2] = AE[1][1]-AE[1][0];
    AE[2][2] = AE[2][1]-AE[2][0];
    float A0xE2dD = A0xE1dD-A0xE0dD;
    float A0xE2dW = A0xE1dW-A0xE0dW;
    R = extA[1]*FABS(AE[2][2])+extA[2]*FABS(AE[1][2]);
    FIND2(A0xE2dD,dt,A0xE2dW,-AN[0],R,T,type,extr,side,AXIS_A0xE2);

    // axis C+t*A1xE0
    DVector3 A1xE0;
    CROSS(A1xE0,A[1],E[0]);
    float A1xE0dD = DOT(A1xE0,D);
    float A1xE0dW = DOT(A1xE0,W);
    R = extA[0]*FABS(AE[2][0])+extA[2]*FABS(AE[0][0]);
    FIND2(A1xE0dD,dt,A1xE0dW,AN[1],R,T,type,extr,side,AXIS_A1xE0);

    // axis C+t*A1xE1
    DVector3 A1xE1;
    CROSS(A1xE1,A[1],E[1]);
    float A1xE1dD = DOT(A1xE1,D);
    float A1xE1dW = DOT(A1xE1,W);
    R = extA[0]*FABS(AE[2][1])+extA[2]*FABS(AE[0][1]);
    FIND2(A1xE1dD,dt,A1xE1dW,-AN[1],R,T,type,extr,side,AXIS_A1xE1);

    // axis C+t*A1xE2
    AE[0][2] = AE[0][1]-AE[0][0];
    float A1xE2dD = A1xE1dD-A1xE0dD;
    float A1xE2dW = A1xE1dW-A1xE0dW;
    R = extA[0]*FABS(AE[2][2])+extA[2]*FABS(AE[0][2]);
    FIND2(A1xE2dD,dt,A1xE2dW,-AN[1],R,T,type,extr,side,AXIS_A1xE2);

    // axis C+t*A2xE0
    DVector3 A2xE0;
    CROSS(A2xE0,A[2],E[0]);
    float A2xE0dD = DOT(A2xE0,D);
    float A2xE0dW = DOT(A2xE0,W);
    R = extA[0]*FABS(AE[1][0])+extA[1]*FABS(AE[0][0]);
    FIND2(A2xE0dD,dt,A2xE0dW,AN[2],R,T,type,extr,side,AXIS_A2xE0);

    // axis C+t*A2xE1
    DVector3 A2xE1;
    CROSS(A2xE1,A[2],E[1]);
    float A2xE1dD = DOT(A2xE1,D);
    float A2xE1dW = DOT(A2xE1,W);
    R = extA[0]*FABS(AE[1][1])+extA[1]*FABS(AE[0][1]);
    FIND2(A2xE1dD,dt,A2xE1dW,-AN[2],R,T,type,extr,side,AXIS_A2xE1);

    // axis C+t*A2xE2
    float A2xE2dD = A2xE1dD-A2xE0dD;
    float A2xE2dW = A2xE1dW-A2xE0dW;
    R = extA[0]*FABS(AE[1][2])+extA[1]*FABS(AE[0][2]);
    FIND2(A2xE2dD,dt,A2xE2dW,-AN[2],R,T,type,extr,side,AXIS_A2xE2);

    // determine the point of intersection
    int i;
    float x[3], numer, NDE[3], NAE[3][3];
    DVector3 DxE[3], A0xE2, A1xE2, A2xE2;

//qdbg("type=%d\n",type);
  // No axis found?
  //if(type==0)return -1;
//else qdbg("COLLISION!\n");

    switch ( type )
    {
        case AXIS_N:
        {
            for (i = 0; i < 3; i++)
            {
                GET_COEFF(x[i],+,-,side,AN[i],extA[i]);
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A0:
        case AXIS_A1:
        case AXIS_A2:
        {
            if ( extr == 0 )
            {
                // y0 = 0, y1 = 0
#ifdef OBS
                P.x=tri[0].x+T*triVel.x;
                P.y=tri[0].y+T*triVel.y;
                P.z=tri[0].z+T*triVel.z;
#endif
                P.x=(*tri[0]).x+T*triVel.x;
                P.y=(*tri[0]).y+T*triVel.y;
                P.z=(*tri[0]).z+T*triVel.z;
#ifdef OBS
                for (i = 0; i < 3; i++)
                    P[i] = (*tri[0])[i]+T*triVel[i];
#endif
            }
            else if ( extr == 1 )
            {
                // y0 = 1, y1 = 0
                P.x=(*tri[0]).x+T*triVel.x+E[0].x;
                P.y=(*tri[0]).y+T*triVel.y+E[0].y;
                P.z=(*tri[0]).z+T*triVel.z+E[0].z;
#ifdef OBS
                for (i = 0; i < 3; i++)
                    P[i] = (*tri[0])[i]+T*triVel[i]+E[0][i];
#endif
            }
            else  // extr == 2
            {
                // y0 = 0, y1 = 1
                P.x=(*tri[0]).x+T*triVel.x+E[1].x;
                P.y=(*tri[0]).y+T*triVel.y+E[1].y;
                P.z=(*tri[0]).z+T*triVel.z+E[1].z;
#ifdef OBS
                for (i = 0; i < 3; i++)
                    P[i] = (*tri[0])[i]+T*triVel[i]+E[1][i];
#endif
            }
            break;
        }
        case AXIS_A0xE0:
        {
            GET_COEFF(x[1],-,+,side,AE[2][0],extA[1]);
            GET_COEFF(x[2],+,-,side,AE[1][0],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[0],D,E[0]);
            NDE[0] = DOT(N,DxE[0]);
            NAE[0][0] = DOT(N,A0xE0);
            NAE[1][0] = DOT(N,A1xE0);
            NAE[2][0] = DOT(N,A2xE0);
            numer = NDE[0]-NAE[1][0]*x[1]-NAE[2][0]*x[2];
            if ( extr == 0 )
            {
                // y1 = 0
                x[0] = numer/NAE[0][0];
            }
            else  // extr == 1
            {
                // y1 = 1
                x[0] = (numer-DOT(N,N))/NAE[0][0];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A0xE1:
        {
            GET_COEFF(x[1],-,+,side,AE[2][1],extA[1]);
            GET_COEFF(x[2],+,-,side,AE[1][1],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[1],D,E[1]);
            NDE[1] = DOT(N,DxE[1]);
            NAE[0][1] = DOT(N,A0xE1);
            NAE[1][1] = DOT(N,A1xE1);
            NAE[2][1] = DOT(N,A2xE1);
            numer = NDE[1]-NAE[1][1]*x[1]-NAE[2][1]*x[2];
            if ( extr == 0 )
            {
                // y0 = 0
                x[0] = numer/NAE[0][1];
            }
            else  // extr == 1
            {
                // y0 = 1
                x[0] = (numer+DOT(N,N))/NAE[0][1];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A0xE2:
        {
            DIFF(E[2],E[1],E[0]);
            CROSS(A0xE2,A[0],E[2]);
            CROSS(A1xE2,A[1],E[2]);
            CROSS(A2xE2,A[2],E[2]);

            GET_COEFF(x[1],-,+,side,AE[2][2],extA[1]);
            GET_COEFF(x[2],+,-,side,AE[1][2],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[2],D,E[2]);
            NDE[2] = DOT(N,DxE[2]);
            NAE[0][2] = DOT(N,A0xE2);
            NAE[1][2] = DOT(N,A1xE2);
            NAE[2][2] = DOT(N,A2xE2);
            numer = NDE[2]-NAE[1][2]*x[1]-NAE[2][2]*x[2];
            if ( extr == 0 )
            {
                // y0+y1 = 0
                x[0] = numer/NAE[0][2];
            }
            else  // extr == 1
            {
                // y0+y1 = 1
                x[0] = (numer+DOT(N,N))/NAE[0][2];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A1xE0:
        {
            GET_COEFF(x[0],+,-,side,AE[2][0],extA[0]);
            GET_COEFF(x[2],-,+,side,AE[0][0],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[0],D,E[0]);
            NDE[0] = DOT(N,DxE[0]);
            NAE[0][0] = DOT(N,A0xE0);
            NAE[1][0] = DOT(N,A1xE0);
            NAE[2][0] = DOT(N,A2xE0);
            numer = NDE[0]-NAE[0][0]*x[0]-NAE[2][0]*x[2];
            if ( extr == 0 )
            {
                // y1 = 0
                x[1] = numer/NAE[1][0];
            }
            else  // extr == 1
            {
                // y1 = 1
                x[1] = (numer-DOT(N,N))/NAE[1][0];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A1xE1:
        {
            GET_COEFF(x[0],+,-,side,AE[2][1],extA[0]);
            GET_COEFF(x[2],-,+,side,AE[0][1],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[1],D,E[1]);
            NDE[1] = DOT(N,DxE[1]);
            NAE[0][1] = DOT(N,A0xE1);
            NAE[1][1] = DOT(N,A1xE1);
            NAE[2][1] = DOT(N,A2xE1);
            numer = NDE[1]-NAE[0][1]*x[0]-NAE[2][1]*x[2];
            if ( extr == 0 )
            {
                // y0 = 0
                x[1] = numer/NAE[1][1];
            }
            else  // extr == 1
            {
                // y0 = 1
                x[1] = (numer+DOT(N,N))/NAE[1][1];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A1xE2:
        {
            DIFF(E[2],E[1],E[0]);
            CROSS(A0xE2,A[0],E[2]);
            CROSS(A1xE2,A[1],E[2]);
            CROSS(A2xE2,A[2],E[2]);

            GET_COEFF(x[0],+,-,side,AE[2][2],extA[0]);
            GET_COEFF(x[2],-,+,side,AE[0][2],extA[2]);
            COMBO(D,D,T,W);
            CROSS(DxE[2],D,E[2]);
            NDE[2] = DOT(N,DxE[2]);
            NAE[0][2] = DOT(N,A0xE2);
            NAE[1][2] = DOT(N,A1xE2);
            NAE[2][2] = DOT(N,A2xE2);
            numer = NDE[2]-NAE[0][2]*x[0]-NAE[2][2]*x[2];
            if ( extr == 0 )
            {
                // y0+y1 = 0
                x[1] = numer/NAE[1][2];
            }
            else  // extr == 1
            {
                // y0+y1 = 1
                x[1] = (numer+DOT(N,N))/NAE[1][2];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A2xE0:
        {
            GET_COEFF(x[0],-,+,side,AE[1][0],extA[0]);
            GET_COEFF(x[1],+,-,side,AE[0][0],extA[1]);
            COMBO(D,D,T,W);
            CROSS(DxE[0],D,E[0]);
            NDE[0] = DOT(N,DxE[0]);
            NAE[0][0] = DOT(N,A0xE0);
            NAE[1][0] = DOT(N,A1xE0);
            NAE[2][0] = DOT(N,A2xE0);
            numer = NDE[0]-NAE[0][0]*x[0]-NAE[1][0]*x[1];
            if ( extr == 0 )
            {
                // y1 = 0
                x[2] = numer/NAE[2][0];
            }
            else  // extr == 1
            {
                // y1 = 1
                x[2] = (numer-DOT(N,N))/NAE[2][0];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A2xE1:
        {
            GET_COEFF(x[0],-,+,side,AE[1][1],extA[0]);
            GET_COEFF(x[1],+,-,side,AE[0][1],extA[1]);
            COMBO(D,D,T,W);
            CROSS(DxE[1],D,E[1]);
            NDE[1] = DOT(N,DxE[1]);
            NAE[0][1] = DOT(N,A0xE1);
            NAE[1][1] = DOT(N,A1xE1);
            NAE[2][1] = DOT(N,A2xE1);
            numer = NDE[1]-NAE[0][1]*x[0]-NAE[1][1]*x[1];
            if ( extr == 0 )
            {
                // y0 = 0
                x[2] = numer/NAE[2][1];
            }
            else  // extr == 1
            {
                // y0 = 1
                x[2] = (numer+DOT(N,N))/NAE[2][1];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
        case AXIS_A2xE2:
        {
            DIFF(E[2],E[1],E[0]);
            CROSS(A0xE2,A[0],E[2]);
            CROSS(A1xE2,A[1],E[2]);
            CROSS(A2xE2,A[2],E[2]);

            GET_COEFF(x[0],-,+,side,AE[1][2],extA[0]);
            GET_COEFF(x[1],+,-,side,AE[0][2],extA[1]);
            COMBO(D,D,T,W);
            CROSS(DxE[2],D,E[2]);
            NDE[2] = DOT(N,DxE[2]);
            NAE[0][2] = DOT(N,A0xE2);
            NAE[1][2] = DOT(N,A1xE2);
            NAE[2][2] = DOT(N,A2xE2);
            numer = NDE[2]-NAE[0][2]*x[0]-NAE[1][2]*x[1];
            if ( extr == 0 )
            {
                // y0+y1 = 0
                x[2] = numer/NAE[2][2];
            }
            else  // extr == 1
            {
                // y0+y1 = 1
                x[2] = (numer+DOT(N,N))/NAE[2][2];
            }
            GET_POINT(P,box,T,boxVel,x);
            break;
        }
    }

  if(type==0)
  {
    // Was already colliding, but no change in collision
//qdbg("Was already colliding\n");
    //assert( type != 0 );
#ifdef OBS
  } else
  {
    P.DbgPrint("Collision point");
#endif
  }

  return 0;
}
//---------------------------------------------------------------------------
