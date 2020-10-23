/*
 * D3 - intersection of 2 spheres
 * 25-04-2001: Created!
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/intersect.h>
DEBUG_ENABLE

bool d3TestSphereSphere(DVector3 *c1,dfloat r1,DVector3 *c2,dfloat r2)
// Returns TRUE if spheres (c1,r1) and (c2,r2) intersect
// Non-functional yet
{
  dfloat rSquared;
  DVector3 diff(c1->x-c2->x,c1->y-c2->y,c1->z-c2->z);

  // Calculate total radius of both spheres
  rSquared=r1+r2;
  rSquared*=rSquared;

  return FALSE;
}
