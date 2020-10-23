/*
 * RPort - viewport rendering
 * 02-05-01: Created!
 * NOTES:
 * - Contains the objects that should be painted in a rectangle in the view
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/port.h>

RPort::RPort()
{
  rect.x=rect.y=0;
  rect.wid=640;
  rect.hgt=480;
}
RPort::~RPort()
{
}

void RPort::SetCuller(DCullerSphereList *_culler)
{
  culler=_culler;
}

/********
* Paint *
********/
void RPort::Paint()
// Paint all objects, sorted etc.
{
}

