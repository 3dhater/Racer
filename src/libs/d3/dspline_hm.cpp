/*
 * DHermiteSpline - definition/implementation
 * NOTES:
 * - Based on QLib's cmspline.cpp (Catmull-Rom)
 * FUTURE:
 * - Include support for tension/bias for the tangent vectors
 * (C) MarketGraph/RvG 3-6-2001
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
#include <d3/spline.h>
DEBUG_ENABLE

#define DEF_PTS     4          // Starting point

#undef  DBG_CLASS
#define DBG_CLASS "DHermiteSpline"

/********
* Ctors *
********/
DHermiteSpline::DHermiteSpline()
{
  points=0;
  pointsAllocated=0;
  p=0;
  pType=0;
  m=0;
  // Default is to treat spline as a circular entity
  flags=LOOP_SPLINE;
  AllocatePoints(DEF_PTS);
}

DHermiteSpline::~DHermiteSpline()
{
  QFREE(p);
  QFREE(pType);
  QFREE(m);
}

/********************
* Memory allocation *
********************/
void DHermiteSpline::AllocatePoints(int n)
{
  dfloat *oldP,*oldM;
  int    *oldPType;
  int     oldPoints;
  
//qdbg("DHermiteSpline::AllocatePoints(%d)\n",n);
  oldP=p;
  oldM=m;
  oldPType=pType;
  oldPoints=pointsAllocated;
  
  pointsAllocated=n;
  
  p=(dfloat*)qcalloc(pointsAllocated*sizeof(dfloat));
  if(!p)qerr("DHermiteSpline: out of memory for point array (%d pts)\n",
    points);
  pType=(int*)qcalloc(pointsAllocated*sizeof(int));
  if(!pType)qerr("DHermiteSpline: out of memory for type array (%d pts)\n",
    points);
  m=(dfloat*)qcalloc(pointsAllocated*sizeof(dfloat));
//qdbg("  m=%p\n",m);
  if(!p)qerr("DHermiteSpline: out of memory for tangent array (%d pts)\n",
    points);
  
  // Copy over old elements
  if(oldP)memcpy(p,oldP,oldPoints*sizeof(dfloat));
  if(oldM)memcpy(m,oldM,oldPoints*sizeof(dfloat));
  if(oldPType)memcpy(pType,oldPType,oldPoints*sizeof(int));
  
  // Free old arrays
  QFREE(oldP);
  QFREE(oldPType);
//qdbg("  QFREE(oldM=%p)\n",oldM);
  QFREE(oldM);
}

void DHermiteSpline::Reset()
{
  points=0;
}

/**********
* Attribs *
**********/
int DHermiteSpline::GetPointType(int n)
{
  if(n<0)return 0;
  if(n>=points)return 0;
  return pType[n];
}

dfloat DHermiteSpline::GetPoint(int n)
{
  if(points==0)return 0;
  if(n<0)return 0;
  if(n>=points)return 0;
  return p[n];
}

/********************
* Add/remove points *
********************/
void DHermiteSpline::AddPoint(dfloat v,int n)
// if n==-1, the next point is sel'd
{
  if(n==-1)n=points;
  if(n>=pointsAllocated)
  { // Resize array
    AllocatePoints(n+50);
  }
  if(p)
  {
    p[n]=v;
    // New point added?
    if(n==points)
      points++;
  }
}
void DHermiteSpline::RemovePoint(int n)
{
  if(points==0)return;
  if(n<0)return;
  if(n>=points)return;
  // Shift array
  int i;
  for(i=n;i<points-1;i++)
  { p[i]=p[i+1];
  }
  points--;
}
void DHermiteSpline::SetPointType(int n,int type)
{
  if(n<0)return;
  if(n>=points)return;
  pType[n]=type;
}

/********************
* Retrieving values *
********************/
dfloat DHermiteSpline::GetValue(dfloat t)
// t=0..points
// Automatically moves through control points
// Catmull-Rom splines takes 4 points; P(i-1) upto P(i+2)
{
  int    i;
  dfloat ti,t2,t3;
  dfloat p0,p1,p2,pm1;
  dfloat pt;
  
  if(!points)return 0.0;

//qdbg("GetValue(%f) (this=%p, m=%p)\n",t,this,m);
  // Calc base control point index
  // Note; ftrunc() doesn't exist on Linux
  //ti=ftrunc(t);
  ti=floor(t);
  i=(int)ti;
//qdbg("  t=%f, ti=%f, i=%d, pts=%d { %f,%f }\n",t,ti,i,points,p[0],p[1]);

  // Bring t back to 0..1
  t=t-ti;
  
  // Get control values/points; make sure they exist
  // Note that Hermite splines take just p0 and p1 (from, to)
  // More info is explicitly stored in tangent vectors, which
  // add direction at the start/end (future).
  p0=p[i];
  if(i+1<points)
  {
    p1=p[i+1];
  } else
  {
    // Close circle?
    if(IsLoop())p1=p[0];
    else        p1=p0;
  }
  
  // Points for tangent calculation (precalculate in the future)
  //
  // Prev point (p(i-1))
  if(i>0)
  {
    pm1=p[i-1];
  } else
  {
    // Close circle?
    if(IsLoop())pm1=p[points-1];
    else        pm1=p[0];
  }
  // Next point
  if(i+2<points)
  {
    p2=p[i+2];
  } else
  {
    // Close circle, or take best end
    if(IsLoop())p2=p[i+2-points];
    else if(i+1<points)p2=p[i+1];
    else p2=p[i];
  }
  
  // Tangents
  dfloat m0,m1;
  m0=0.5f*((p0-pm1)+p1-p0);
  m1=0.5f*((p1-p0)+p2-p1);
  
//qdbg("  pm1=%f, p0=%f, p1=%f, p2=%f (m0=%f, m1=%f)\n",pm1,p0,p1,p2,m0,m1);
  
  // Calc matrix mult
  t2=t*t;               // t squared
  t3=t2*t;              // t^3
  // Hermite matrix
#ifdef OBS
qdbg("  p0 attrib %f, p1 attrib %f\n",(2.0f*t3-3.0f*t2+1.0f),
 (-2.0f*+3.0f*t2));
#endif
  pt=(2.0f*t3-3.0f*t2+1.0f)*p0+
     (t3-2.0f*t2+t)*m0+
     (t3-t2)*m1+
     (-2.0f*t3+3.0f*t2)*p1;
//qdbg("  pt=%f\n",pt);
  return pt;
}
