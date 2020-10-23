/*
 * RTimeLine
 * 14-02-01: Created!
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

RTimeLine::RTimeLine(const DVector3 *from,const DVector3 *to)
{
  plane=0;
  Define(from,to);
}
RTimeLine::~RTimeLine()
{
  QDELETE(plane);
}

/*********
* Define *
*********/
void RTimeLine::Define(const DVector3 *_from,const DVector3 *_to)
// Define the line from '_from' to '_to'
{
  // Cleanup
  QDELETE(plane);

  from=*_from;
  to=*_to;

  // Calculate plane from the 2 points
  DVector3 tween;
  tween.x=(from.x+to.x)/2;
  tween.y=(from.y+to.y)/2;
  tween.z=(from.z+to.z)/2;
  // Direction of plane is up (given racing is mostly on flat track)
  tween.y+=1;
  plane=new DPlane3(from,tween,to);

  // Optimizations; precalculate distance from 'from' to 'to'
  distanceSquared=from.SquaredDistanceTo(&to);
}

/***********
* Crossing *
***********/
bool RTimeLine::CrossesPlane(const DVector3 *v1,const DVector3 *v2) const
// Returns TRUE if the line v1->v2 crosses the timeline
// Note that any direction of the crossing will do, to avoid having
// a deciding direction in the timeline.
// Take care when calculating lap times; make sure at least
// 2 timelines are available.
// If not, numerical instability may have the car cross the one and only
// start/finish line more than once (if standing nearly still right at
// the line).
{
#ifdef OBS
qdbg("RTimeLine::CrossesPlane()\n");
qdbg("  v1: dist^2(from)=%f, dist^2(to)=%f\n",
from.SquaredDistanceTo(v1),to.SquaredDistanceTo(v1));
qdbg("  v2: dist^2(from)=%f, dist^2(to)=%f\n",
from.SquaredDistanceTo(v2),to.SquaredDistanceTo(v2));
qdbg("  distanceSquared=%f\n",distanceSquared);
#endif

//from.DbgPrint("from");
//to.DbgPrint("to");

  // Check general vicinity first of the start and end point
  if(from.SquaredDistanceTo(v1)>distanceSquared||
     from.SquaredDistanceTo(v2)>distanceSquared||
     to.SquaredDistanceTo(v1)>distanceSquared||
     to.SquaredDistanceTo(v2)>distanceSquared)
    return FALSE;

  // 'v1' and 'v2' are now both in the spheroid inbetween 'from' and 'to'
  // (the intersection of the spheres around 'from' and 'to' with radius
  // distance(from,to)).
  // Note this does NOT work when v1 and v2 are far apart; the line v1-v2 may
  // still cross the line from-to (when v1 or v2 is outside the spheroid).
  // However, generally the integration of Racer is so fast that
  // this won't happen most probably in practice, if you keep the
  // from-to lines big enough (a track's width is certainly far enough).

//qdbg("CrossesPlane: v1 dist=%f, v2 dist=%f\n",
//plane->DistanceTo(v1),plane->DistanceTo(v2));
  // Plane is crossed when 'v1' is not on the same side as 'v2'
  if(plane->ClassifyFrontOrBack(v1)!=plane->ClassifyFrontOrBack(v2))
    return TRUE;
  return FALSE;
}
