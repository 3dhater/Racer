/*
 * RCamera - a camera
 * 06-10-00: Created!
 * 10-02-01: Support for Yaw/Pitch/Roll and quaternions
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Default lens angle (legacy Racer v0.4.7 and before)
// 50 is nice for the eyes, but feels a bit slow. 90 is very high
// I think and gives too much distortion. Between 60 and 70 I don't
// get motionsick and a good feeling of speed.
#define DEFAULT_FOV    50.0

RCamera::RCamera(RCar *_car)
  : lensAngle(DEFAULT_FOV),flags(0),name("defaultCam"),index(0)
{
  car=_car;

  offset.SetToZero();
  angle.SetToZero();
  angleOffset.SetToZero();
  view=0;

  // Setup a nice default camera
  offset.x=0;
  offset.y=-1;
  offset.z=-5;

  angle.x=20;
  angle.y=0;
  angle.z=0;
}
RCamera::~RCamera()
{
}

/**********
* Attribs *
**********/
void RCamera::SetFlags(int f)
{
  flags=f;
}
void RCamera::SetIndex(int n)
{
  index=n;
}
void RCamera::GetOrigin(DVector3 *v) const
// Stores the origin (in world coordinates) of the camera in 'v'
{
  DVector3 *p=car->GetPosition();
  // Total origin is the car position and the offset of the camera
  v->x=offset.x+p->x;
  v->y=offset.y+p->y;
  v->z=offset.z+p->z;
}

/******
* I/O *
******/
bool RCamera::Load(QInfo *info)
// Read camera settings
{
  char path[256],buf[256];
  int  n;

  sprintf(path,"camera%d",index);
  // Reset flags
  flags=0;

  // Offset to car
  sprintf(buf,"%s.offset.x",path);
  offset.x=info->GetFloat(buf,0);
  sprintf(buf,"%s.offset.y",path);
  offset.y=info->GetFloat(buf,-1);
  sprintf(buf,"%s.offset.z",path);
  offset.z=info->GetFloat(buf,-5);

  // Angle
  sprintf(buf,"%s.angle.x",path);
  angle.x=info->GetFloat(buf);
  sprintf(buf,"%s.angle.y",path);
  angle.y=info->GetFloat(buf);
  sprintf(buf,"%s.angle.z",path);
  angle.z=info->GetFloat(buf);

  // Following
  sprintf(buf,"%s.follow.pitch",path);
  if(info->GetInt(buf))flags|=FOLLOW_PITCH;
  sprintf(buf,"%s.follow.yaw",path);
  if(info->GetInt(buf))flags|=FOLLOW_YAW;
  sprintf(buf,"%s.follow.roll",path);
  if(info->GetInt(buf))flags|=FOLLOW_ROLL;

  // Lens
  sprintf(buf,"%s.fov",path);
  lensAngle=info->GetFloat(buf,DEFAULT_FOV);

  // View and model (incar 3D, outside or no car at all?)
  sprintf(buf,"%s.view",path);
  view=info->GetInt(buf);
  // Model; 0=outside (default), 1=incar, 2=nocar (bumpers views)
  sprintf(buf,"%s.model",path);
  n=info->GetInt(buf);
  if(n==1)flags|=INCAR;
  else if(n==2)flags|=NOCAR;
  else if(n!=0)
  { qwarn("RCamera:Load(); model %d is unknown",n);
    // Just use outside view
  }
  sprintf(buf,"%s.wheels",path);
  if(!info->GetInt(buf,1))
    flags|=NO_WHEELS;

  // Name
  sprintf(buf,"%s.name",path);
  info->GetString(buf,name);

  return TRUE;
}

/***********
* GRAPHICS *
***********/
void RCamera::Go()
// Go to the position of the camera
{
  rfloat a;
  rfloat y,p,r;
  DMatrix3 *mBody=car->GetBody()->GetRotPosM();

#ifdef OBS
qdbg("RCamera:Go()\n");
mBody->DbgPrint("mBody");
#endif

  // Position (location of the camera)
  glTranslatef(offset.x,offset.y,offset.z);

  a=angle.z;
  if(flags&FOLLOW_ROLL)
  {
    //a=0;
    // Calculate roll
#ifdef ND_TRANSPOSED_MATRIX
    if(mBody->GetRC(0,1)==0.0f&&mBody->GetRC(1,1)==0.0f)
    {
      // Special case
      r=atan2f(mBody->GetRC(1,0),mBody->GetRC(0,0));
    } else
    {
      r=atan2f(-mBody->GetRC(0,1),mBody->GetRC(1,1));
    }
#endif
    // Transposed matrix version of Real-Time Rendering method
    // of extracting roll
    if(mBody->GetRC(1,0)==0.0f&&mBody->GetRC(1,1)==0.0f)
    {
      // Special case
      r=atan2f(mBody->GetRC(0,1),mBody->GetRC(0,0));
    } else
    {
      r=atan2f(-mBody->GetRC(1,0),mBody->GetRC(1,1));
    }
//qdbg("  roll=%f\n",r*RR_RAD_DEG_FACTOR);
    a-=r*RR_RAD_DEG_FACTOR;
  }
  if(a)glRotatef(a,0,0,1);
  
  a=angle.x;
  if(flags&FOLLOW_PITCH)
  {
    //a=0;
    // Calculate pitch
    // Real-Time Rendering
    //p=asinf(mBody->GetRC(2,1));
    // My version; transposed matrix (!)
    //p=atan2(mBody->GetRC(2,1),mBody->GetRC(1,1));
    //p=atan2(mBody->GetRC(1,0),mBody->GetRC(2,0));
    //p=atan2(mBody->GetRC(2,2),mBody->GetRC(2,1));
    p=asinf(mBody->GetRC(1,2));
    //if(p>.5*PI)p-=PI;
//mBody->DbgPrint("mBody");
#ifdef OBS
qdbg("Up=(%.2f,%.2f,%.2f)\n",mBody->GetRC(0,0),mBody->GetRC(0,1),
mBody->GetRC(0,2));
#endif
//qdbg("  pitch=%f\n",p*RR_RAD_DEG_FACTOR);
    a-=p*RR_RAD_DEG_FACTOR;
  }
  if(a)glRotatef(a,1,0,0);

  // Rotation (around the car)
  // Note that the rotation is done in such a way that when rotating,
  // the camera will always point to the car. This is done to ease
  // camera animation.
  a=angle.y;
  if(flags&FOLLOW_YAW)
  {
    //a=0;
//mBody->DbgPrint("Follow yaw; body 3x3 matrix");
    //y=atan2f(-mBody->GetRC(0,2),mBody->GetRC(2,2));
    // The following is the Real-Time Rendering method to extract yaw
    // only for a transposed matrix
    y=atan2f(mBody->GetRC(2,0),mBody->GetRC(2,2));
//qdbg("  heading=%f\n",y*RR_RAD_DEG_FACTOR);
    //a-=car->GetBody()->GetYaw()*RR_RAD_DEG_FACTOR;
    a+=y*RR_RAD_DEG_FACTOR;
  }
  if(a)glRotatef(a,0,1,0);

#ifdef OBS
  // Position (offset; can be seen as the lookAt target)
  glTranslatef(offset.x,offset.y,offset.z);
#endif

  // Offset to car position
  DVector3 *cp=car->GetPosition();
  glTranslatef(-cp->x,-cp->y,-cp->z);
}
