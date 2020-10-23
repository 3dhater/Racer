/*
 * RTrackCam
 * 01-04-2001: Created! (18:19:07)
 * NOTES:
 * FUTURE:
 * - Precalc stuff in CalcState()
 * - Precalc normalize direction of pos->posDir
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Size of preview cubes
#define CUBE_SIZE  2.0f

RTrackCam::RTrackCam()
{
  int i;
  
  flags=0;
  pos.SetToZero();
  posDir.SetToZero();
  posDolly.SetToZero();
  rot.SetToZero();
  rot2.SetToZero();

  for(i=0;i<3;i++)
  {
    cubePreview[i].SetToZero();
  }
  radius=1;
  group=0;
  zoomEdge=zoomClose=zoomFar=0;
  // Default is a directional camera, which knows front and back
  flags=ZOOMDOT;

  // State init
  car=0;
  normalizedDistance=0;
  zoom=0;
  curPos.SetToZero();
}
RTrackCam::~RTrackCam()
{
}

bool RTrackCam::Load(QInfo *info,cstring path)
// Read camera from a file
{
  return TRUE;
}

/**********
* Attribs *
**********/
void RTrackCam::SetCar(RCar *_car)
{
  car=_car;
}
void RTrackCam::GetOrigin(DVector3 *v)
// Returns origin of camera in 'v'
// BUGS: Doesn't take into account any dollying
{
  *v=pos;
}

/********
* State *
********/
void RTrackCam::CalcState()
// Calculate the current situation
{
  // Calc normalizedDistance
  normalizedDistance=ProjectedDistance()/radius;
  if(normalizedDistance<-1.0f)normalizedDistance=-1.0f;
  else if(normalizedDistance>1.0f)normalizedDistance=1.0f;
  
  // Calc zoom value
  if(flags&FIXED)
  {
    // Use fixed view
    zoom=zoomClose;
  } else if(flags&ZOOMDOT)
  {
    // Interpolate from zoomEdge->zoomClose->zoomFar
    if(normalizedDistance<0)
    {
      // Before the camera
      zoom=zoomClose+(zoomEdge-zoomClose)*-normalizedDistance;
    } else
    {
      // After the camera
      zoom=zoomClose+(zoomFar-zoomClose)*normalizedDistance;
    }
  } else
  {
    // Base zoom on car distance to camera (no front/back)
    // Only uses zoomClose and zoomEdge
    rfloat d;
    d=car->GetPosition()->SquaredDistanceTo(&pos);
    zoom=zoomClose+(zoomEdge-zoomClose)*d/(radius*radius);
  }
//qdbg("RTrackCam:CalcState(); zoom=%f\n",zoom);
}
bool RTrackCam::IsInRange()
// Returns TRUE if the car is in the circle of the camera
{
  return pos.SquaredDistanceTo(car->GetPosition())<radius*radius;
}
bool RTrackCam::IsFarAway()
// Returns TRUE if the car is quite far away from the camera.
// Normally, this is ok and the camera keeps following the car until
// a next camera picks up the view. However, once the car is very
// far away, we might have missed a weird action and we need
// to pick up the closest camera. 
{
  // The relative meaning 'far' is twice the radius here
#ifdef OBS
qdbg("RTC:Far? d^2=%f, limit=%f\n",
pos.SquaredDistanceTo(car->GetPosition()),2*radius*radius);
#endif
  return pos.SquaredDistanceTo(car->GetPosition())>2*radius*radius;
}
rfloat RTrackCam::ProjectedDistance()
// Returns the distance that the car is away from the camera
// This takes into account the direction of the camera, and the car
// position is projected on the direction of the cam, giving a distance
// in terms of longitudinal position (as far as the car is driving
// towards the camera)
{
  DVector3 dir,relCar;
  dfloat d;

  if(!car)return 0;
//qdbg("RTCam:PrjDist this=%p, car=\n",this);
  dir=posDir;
  dir.Subtract(&pos);
  dir.Normalize();
//dir.DbgPrint("PrjDist: dir");
  relCar=*car->GetPosition();
  relCar.Subtract(&pos);
  d=relCar.Dot(dir);
//qdbg("  d=%f\n",d);
  return d;
}

/******************
* Camera pointing *
******************/
void RTrackCam::Go(DVector3 *target)
// Point to the target
{
#ifdef RR_GFX_OGL
  if(flags&DOLLY)
  {
    // Interpolate from start to dolly end position
    curPos=posDolly;
    curPos.Subtract(&pos);
    curPos.Scale(0.5f*(normalizedDistance+1.0f));
    curPos.Add(&pos);
  } else
  {
    curPos=pos;
  }

#ifdef OBS_COMPARE_GLU
qdbg("gluLookAt(eye %f,%f,%f target %f,%f,%f)\n",
 curPos.x,curPos.y,curPos.z,target->x,target->y,target->z);
//#endif
//glMatrixMode(GL_PROJECTION);
//glLoadIdentity();
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
DMatrix4 m4;
glGetFloatv(GL_MODELVIEW_MATRIX,m4.GetM());
m4.DbgPrint("PRE lookAt");
glPushMatrix();
  gluLookAt(curPos.x,curPos.y,curPos.z,          // Camera eye
            target->x,target->y,target->z,       // Car center
            0,1,0);                              // Up vector
glGetFloatv(GL_MODELVIEW_MATRIX,m4.GetM());
m4.DbgPrint("gluLookAt");
glPopMatrix();
#endif

  // Use our own lookat, which stores the matrix in the QGLContext,
  // so we don't need to query the graphics pipe later on to get
  // the ModelView matrix.
//glPushMatrix();
  GetCurrentQGLContext()->LookAt(
            curPos.x,curPos.y,curPos.z,          // Camera eye
            target->x,target->y,target->z,       // Car center
            0,1,0);                              // Up vector
//glGetFloatv(GL_MODELVIEW_MATRIX,m4.GetM());
//m4.DbgPrint("gl->LookAt");
//glPopMatrix();
#endif
}
void RTrackCam::Go()
// Setup the view
{
//qdbg("RTrackCam:Go()\n");
  if(!car)
  {
    // Fixed view; use first preview cube (hopefully it was set in TrackEd)
    Go(&cubePreview[0]);
  } else
  {
    // Target the current car
    Go(car->GetPosition());
  }
}
