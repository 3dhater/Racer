/*
 * D3 - intersection of triangle and quad
 * 02-08-01: Created!
 * NOTES:
 * - With help from Gregor Veble and Mathematica
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/intersect.h>
DEBUG_ENABLE


/*
 *
 *   2     3
 *    +---+
 *    |   |
 *    +---+
 *   0     1
 *
 * Lateral lines go from 0->1 and 2->3 (which is 'u') and longitudinal
 * lines move from 0->2 and 1->3 (which is 'v')
 *
 */

bool d3RayQuadIntersection2D(DVector3 *org,DVector3 *dir,
  DVector3 *v0,DVector3 *v1,DVector3 *v2,DVector3 *v3,
  dfloat *t,dfloat *uPtr,dfloat *vPtr)
// Based on a calculation done by Gregor Veble, this
// attempts to get UV coordinates from a quad.
// Only X/Z values are used, so it's 2D basically.
{
  float a,b,c,d,p,q,r,s;
  float u,v;
  float div,tmp;
  float X,Y;

  // Get hit point (2D)
  X=org->x; Y=org->z;

  // Precalc
  float x00,x01,x10,x11;
  float y00,y01,y10,y11;
#ifdef ND_BAD_UV
  x00=v0->x;
  x01=v2->x;
  x10=v0->z;
  x11=v2->z;
  y00=v1->x;
  y01=v3->x;
  y10=v1->z;
  y11=v3->z;
#endif
  x00=v0->x;
  x01=v2->x;
  x10=v1->x;
  x11=v3->x;
  y00=v0->z;
  y01=v2->z;
  y10=v1->z;
  y11=v3->z;

  a=x00; b=-x00+x10; c=-x00+x01; d=x00-x01-x10+x11;
  p=y00; q=-y00+y10; r=-y00+y01; s=y00-y01-y10+y11;

  div=2*d*q-2*b*s;
  if(fabs(div)<D3_EPSILON)u=10;
  else
  {
    tmp=d*p-c*q+b*r-a*s+s*X-d*Y;
    u=(1.0f/div)*(-c*q+b*r+a*s-s*X+d*(-p+Y)+
      sqrt(4*(d*r-c*s)*(-b*p+a*q-q*X+b*Y)+tmp*tmp));
#ifdef ND_OTHER_MATHEMATICA_RESULT
    // 2nd method
    u=(1.0f/div)*(-c*q+b*r+a*s-s*X+d*(-p+Y)-
      sqrt(4.0f*(d*r-c*s)*(-b*p+a*q-q*X+b*Y)+tmp*tmp));
#endif
  }

  if(u<0||u>1)return FALSE;

#ifdef ND_FIRST_MATHEMATICA_RESULT
  // More direct Mathematic attempt
  tmp=(x10*Y-
       x11*Y+X*y00-2.0f*x10*y00+x11*y00-X*y01+
       x10*y01+x01*(Y-y10)-X*y10+X*y11-
       x00*(Y-2.0f*y10+y11));
  u=(-x10*Y+x11*Y-X*y00-x11*y00+X*y01+x10*y01+X*y10-
     x01*(Y-2.0f*y00+y10)-X*y11+
     x00*(Y-2.0f*y01+
     y11)+sqrt(4.0f*(x00*Y-x10*Y-X*y00+x10*y00+X*y10-
       x00*y10)*(x11*(y00-y01)+
       x10*(-y00+y01)+(x00-x01)*(y10-y11))+
       tmp*tmp))/(2.0f*(x01*(y00-y10)+x11*(-y00+y10)-(x00-x10)*(y01-y11)));
#endif

  // Calculate 'v'
  div=2.0f*(-d*r+c*s);
  if(fabs(div)<D3_EPSILON)v=10;
  else
  {
    tmp=d*p-c*q+b*r-a*s+s*X-d*Y;
    v=(1.0f/div)*(-c*q+b*r-a*s+s*X+d*(p-Y)+
      sqrt(4*(d*r-c*s)*(-b*p+a*q-q*X+b*Y)+tmp*tmp));
  }

  if(v<0||v>1)return FALSE;

  *uPtr=u;
  *vPtr=v;

  return TRUE;
}

