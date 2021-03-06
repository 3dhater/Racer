/*
 * RSteer
 * 05-08-00: Created! (16:48:45)
 * 02-09-01: Added model to represent the wheel.
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Paint the stub wheel?
//#define DO_PAINT_STUB

#define DEF_LOCK  2
#define DEF_RADIUS  .20
#define DEF_XA      20

// Input scaling
//#define MAX_INPUT   1000

RSteer::RSteer(RCar *_car)
{
  car=_car;
  angle=0;
  lock=5;
  xa=0;
  position.SetToZero();
  radius=DEF_RADIUS;
  quad=0;
  model=new RModel(car);
  axisInput=0;
}
RSteer::~RSteer()
{
  QDELETE(model);
  if(quad){ gluDeleteQuadric(quad); quad=0; }
}

bool RSteer::Load(QInfo *info,cstring path)
// 'path' may be 0, in which case the default "steer" is used
{
  char buf[128];
  
  if(!path)path="steer";

  // Location
  sprintf(buf,"%s.x",path);
  position.x=info->GetFloat(buf);
  sprintf(buf,"%s.y",path);
  position.y=info->GetFloat(buf);
  sprintf(buf,"%s.z",path);
  position.z=info->GetFloat(buf);
  sprintf(buf,"%s.radius",path);
  radius=info->GetFloat(buf,DEF_RADIUS);
  sprintf(buf,"%s.xa",path);
  xa=info->GetFloat(buf,DEF_XA);
  
  // Physical attribs
  sprintf(buf,"%s.lock",path);
  lock=info->GetFloat(buf,DEF_LOCK)/RR_RAD_DEG_FACTOR;

  // Model (was new'ed in the ctor)
  model->Load(info,path);
  if(!model->IsLoaded())
  {
    // Stub gfx
    quad=gluNewQuadric();
  }
//qdbg("RSteer: r=%f, xa=%f\n",radius,xa);
  return TRUE;
}

/*******
* Dump *
*******/
bool RSteer::LoadState(QFile *f)
{
  f->Read(&angle,sizeof(angle));
  // 'axisInput'?
  return TRUE;
}
bool RSteer::SaveState(QFile *f)
{
  f->Write(&angle,sizeof(angle));
  return TRUE;
}

/**********
* Attribs *
**********/

/********
* Paint *
********/
void RSteer::Paint()
{
  glPushMatrix();
  
  // Location
  glTranslatef(position.x,position.y,position.z);
  
  // Rotation towards driver
  glRotatef(xa,1,0,0);
  
  // Rotate wheel
  glRotatef(-angle*RR_RAD_DEG_FACTOR,0,0,1);
 
  if(model!=0&&model->IsLoaded())
    model->Paint();
#ifdef DO_PAINT_STUB
  float colSteer[]={ .8,.4,.2 };
  float depth=.02;        // Fatness of steer
  int   slices=7;
  glMaterialfv(GL_FRONT,GL_DIFFUSE,colSteer);
  //glMaterialf(GL_FRONT,GL_SHININESS,80);
  // Draw a simple steer
  gluCylinder(quad,radius,radius,depth,slices,1);
  
  // Caps
  gluQuadricOrientation(quad,GLU_INSIDE);
  gluDisk(quad,0,radius,slices,1);
  gluQuadricOrientation(quad,GLU_OUTSIDE);
  glTranslatef(0,0,depth);
  gluDisk(quad,0,radius,slices,1);
#endif

  glPopMatrix();
}

/**********
* Animate *
**********/
void RSteer::Integrate()
{
//qdbg("RSteer: steerPos=%d => angle=%f deg\n",axisInput,angle*RR_RAD_DEG_FACTOR);
}

/********
* Input *
********/
void RSteer::SetInput(int steerPos)
// 'steerPos' is the angle of the input steering wheel
// ranging from -1000..+1000
{
  float a;

  // Clip
  if(steerPos<-1000)steerPos=-1000;
  else if(steerPos>1000)steerPos=1000;
  axisInput=steerPos;
  
  // Take controller position
  a=((float)axisInput)*lock/1000.0;
//qdbg("RSteer: axisInput=%d, a=%f,lock=%f\n",axisInput,a,lock);
  // Set this directly as the steering wheel's angles
  // But notice that because of counterclockwise angles being >0,
  // the sign is reversed
  angle=a;
}
