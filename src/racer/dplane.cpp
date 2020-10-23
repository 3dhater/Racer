/*
 * DPlane - planes
 * 04-01-2001: Created! (19:33:59)
 * NOTES:
 * - 3D planes only for now
 * - Most functions are inlined in d3/types.h
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

/**************
* Classifying *
**************/
int DPlane3::Classify(const DVector3 *v) const
// Classify point as being in front, in the back, or on the plane
{
  dfloat f=v->x*n.x+v->y*n.y+v->z*n.z+d;
  if(f>D3_EPSILON)return FRONT;
  else if(f<D3_EPSILON)return BACK;
  // Otherwise it's close enough to the plane
  else return COPLANAR;
}
int DPlane3::ClassifyFrontOrBack(const DVector3 *v) const
// Classify point as either being in front, or the back
// Never returns COPLANAR; a point on the plane will be returned as
// FRONT.
{
  dfloat f=v->x*n.x+v->y*n.y+v->z*n.z+d;
//qdbg("DPlane3: distance to plane=%f\n",f);
  if(f>=0.0f)return FRONT;
  else       return BACK;
}

/************
* Debugging *
************/
void DPlane3::DbgPrint(cstring s) const
{
  qdbg("DPlane3: normal(%8.2f,%8.2f,%8.2f), distance %8.2f [%s]\n",
    n.x,n.y,n.z,d,s);
}
