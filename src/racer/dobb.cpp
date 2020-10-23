/*
 * DOBB - oriented bounding box
 * 22-04-2001: Created! (20:23:06)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/obb.h>
DEBUG_ENABLE

DOBB::DOBB()
{
  center.SetToZero();
  extents.SetToZero();
  // Identity axes
  axis[0].Set(1,0,0);
  axis[1].Set(0,1,0);
  axis[2].Set(0,0,1);
}

DOBB::DOBB(DVector3 *icenter,DVector3 *iextents)
{
  center=*icenter;
  extents=*iextents;
  // Identity axes
  axis[0].Set(1,0,0);
  axis[1].Set(0,1,0);
  axis[2].Set(0,0,1);
}
DOBB::~DOBB()
{
}

DVector3 *DOBB::GetWorldCenter()
// Returns the center in world coordinates of the box
{
  return &center;
}
DVector3 *DOBB::GetWorldAxes()
// Returns a set of 3 vectors that are the boxes XYZ axes in the world
{
  return &axis[0];
}
dfloat *DOBB::GetWorldExtent()
// Returns dfloat[3] array that indicate the size of the box in world coords.
{
  return &extents.x;
}

void DOBB::DbgPrint(cstring s) const
{
}
