/*
 * Racer - suspension
 * 03-09-00: Created! (16:39:01)
 * 12-12-00: Bounding box support.
 * 23-02-01: Roll centers introduced; car was highly unstable at >80 km/h.
 * NOTES:
 * BUGS:
 * - You may enter a state where a bounce starts which never ends; perhaps
 * a problem with the integration (x=x+v+1/2a might be better), or with
 * the high (vertical) spring rate of a tire. More damping must be introduced
 * somewhere as the shocks don't seem to.
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
#include <d3/geode.h>
DEBUG_ENABLE

// Draw stub models of the suspension?
//#define DRAW_STUB

// Local trace
//#define LTRACE

#undef  DBG_CLASS
#define DBG_CLASS "RSuspension"

RSuspension::RSuspension(RCar *_car)
{
  car=_car;
  length=0;
  restLength=0;
  minLength=maxLength=0;
  k=0;
  slices=0;
  arbOtherSusp=0;
  arbRate=0;
  position.SetToZero();
  rollCenter.SetToZero();
  pistonVelocity.SetToZero();
  forceBody.SetToZero();
  forceSpring.SetToZero();
  forceDamper.SetToZero();
  forceARB.SetToZero();
  Init();
 
#ifdef RR_GFX_OGL
  quad=gluNewQuadric();
#endif
  model=0;
  bbox=0;
}
RSuspension::~RSuspension()
{
  Destroy();
}

void RSuspension::Destroy()
{
#ifdef RR_GFX_OGL
  if(quad){ gluDeleteQuadric(quad); quad=0; }
#endif
  QDELETE(model);
  QDELETE(bbox);
}

/**********
* Attribs *
**********/
void RSuspension::DisableBoundingBox()
{
  flags&=~DRAW_BBOX;
}
void RSuspension::EnableBoundingBox()
{
  flags|=DRAW_BBOX;
}

/*******
* Dump *
*******/
bool RSuspension::LoadState(QFile *f)
{
  //f->Read(&position,sizeof(position));
  f->Read(&length,sizeof(length));
  f->Read(&pistonVelocity,sizeof(pistonVelocity));
  f->Read(&forceBody,sizeof(forceBody));
  f->Read(&forceWheel,sizeof(forceWheel));
  f->Read(&forceSpring,sizeof(forceSpring));
  f->Read(&forceDamper,sizeof(forceDamper));
  return TRUE;
}
bool RSuspension::SaveState(QFile *f)
{
  //f->Write(&position,sizeof(position));
  f->Write(&length,sizeof(length));
  f->Write(&pistonVelocity,sizeof(pistonVelocity));
  f->Write(&forceBody,sizeof(forceBody));
  f->Write(&forceWheel,sizeof(forceWheel));
  f->Write(&forceSpring,sizeof(forceSpring));
  f->Write(&forceDamper,sizeof(forceDamper));
  return TRUE;
}

/**********
* Loading *
**********/
void RSuspension::Init()
{
  flags=0;
}
bool RSuspension::Load(QInfo *info,cstring path)
{
  char buf[128];
  int  index;
  
  Destroy();

  // Deduce index by path
  if(path[0])
    index=path[strlen(path)-1]-'0';
  else
    index=0;

  // Location; offset to car center of geometry
  sprintf(buf,"%s.x",path);
  position.x=info->GetFloat(buf,(index&1)?-.4f:.4f);
  sprintf(buf,"%s.y",path);
  position.y=info->GetFloat(buf);
  sprintf(buf,"%s.z",path);
  position.z=info->GetFloat(buf,(index<2)?-1.0f:1.0f);

  // Roll center (default: X=center of car, Y at ground, Z at susp location)
  sprintf(buf,"%s.roll_center.x",path);
  rollCenter.x=info->GetFloat(buf);
  sprintf(buf,"%s.roll_center.y",path);
  rollCenter.y=info->GetFloat(buf);
  sprintf(buf,"%s.roll_center.z",path);
  rollCenter.z=info->GetFloat(buf,position.z);
  // Roll center is relative to suspension point
  rollCenter.Add(&position);
  
  // Physical attribs
  sprintf(buf,"%s.restlen",path);
  restLength=info->GetFloat(buf,.5f);
  sprintf(buf,"%s.minlen",path);
  minLength=info->GetFloat(buf,0.1f);
  sprintf(buf,"%s.maxlen",path);
  maxLength=info->GetFloat(buf,1.0f);
  sprintf(buf,"%s.k",path);
  k=info->GetFloat(buf,7500.0f);
  
  // Damper
  sprintf(buf,"%s.bump_rate",path);
  bumpRate=info->GetFloat(buf,2000.0f);
  sprintf(buf,"%s.rebound_rate",path);
  reboundRate=info->GetFloat(buf,2200.0f);

  // Gfx
  sprintf(buf,"%s.radius",path);
  radius=info->GetFloat(buf,.04f);
  sprintf(buf,"%s.slices",path);
  slices=info->GetInt(buf,4);
  
  // Model (or stub gfx)
  model=new RModel(car);
  model->Load(info,path);
  if(!model->IsLoaded())
    quad=gluNewQuadric();
  bbox=new DBoundingBox();
  bbox->EnableCSG();
#ifdef FUTURE
  bbox->GetBox()->size.x=radius*2;
  bbox->GetBox()->size.y=restLength*2;
  bbox->GetBox()->size.z=radius*2;
#endif

  // Deduce others
  length=restLength;
  
  return TRUE;
}

void RSuspension::Reset()
// After a car warp for example
{
//qdbg("RSusp:Reset, length=%f, restLen=%f\n",length,restLength);
  length=restLength;
  pistonVelocity.SetToZero();
  forceBody.SetToZero();
  forceWheel.SetToZero();
  forceSpring.SetToZero();
  forceDamper.SetToZero();
}

/********
* Paint *
********/
void RSuspension::Paint()
{
#ifdef RR_GFX_OGL
  
#ifdef DRAW_STUB
  float col[]={ .4,.4,.4 };

  glPushMatrix();
  
  // Location
  glTranslatef(position.GetX(),position.GetY(),position.GetZ());

  if(flags&DRAW_BBOX)
  {
    // Draw the bounding box in the model-type rotation
    // (the stub must rotate so the cylinder is at the local Z-axis)
    bbox->Paint();
  }

  // Discs are painted at Z=0
  glRotatef(90,1,0,0);
  
  // Primitives
  if(model!=0&&model->IsLoaded())
  {
    model->Paint();
    goto skip_quad;
  }
  
  car->SetDefaultMaterial();
  
#ifdef OBS
  // Center point for quad
  glTranslatef(0,0,-restLength/2);
#endif
  
  glMaterialfv(GL_FRONT,GL_DIFFUSE,col);
  // Draw cylinder with caps
//qdbg("radius=%f, length=%f, slices=%d\n",radius,length,slices);
  gluCylinder(quad,radius,radius,length,slices,1);
  gluQuadricOrientation(quad,GLU_INSIDE);
  gluDisk(quad,0,radius,slices,1);
  gluQuadricOrientation(quad,GLU_OUTSIDE);
  glTranslatef(0,0,length);
  gluDisk(quad,0,radius,slices,1);
  
 skip_quad:
  
  glPopMatrix();
#endif

#endif
}

/**********
* Physics *
**********/
void RSuspension::PreAnimate()
{
  forceBody.SetToZero();
  forceWheel.SetToZero();
}

void RSuspension::CalcForces()
{
  DVector3 force;
  double damperRate;

//qdbg("RSusp:CF; force=%.2f,%.2f,%.2f\n",force.x,force.y,force.z);
 
  // Note all forces here are purely vertical, so only the .y member
  // of the forces need to be used

  // Calculate spring force (because of stretching/compressing)
  force.x=0;
  force.y=k*(length-restLength);
  force.z=0;
  // Store this for telemetry and debugging
  forceSpring.y=force.y;
#ifdef LTRACE
qdbg("RSusp: force.y=%f, k=%.f, restLength=%f, length=%f\n",
force.y,k,restLength,length);
#endif

  // Calculate force of the damper
//qdbg("  piston v=%f\n",pistonVelocity.y);
  if(pistonVelocity.y<0)
  { // Stretching
//qdbg("    stretching\n");
    damperRate=reboundRate;
  } else
  {
    // Compressing
//qdbg("    compressing\n");
    damperRate=bumpRate;
  }
  // Force is proportional to damper rate and velocity
  // Note that these 2 lines could be combined, but for
  // debugging and telemetry, we'll store the actual damper force.
  forceDamper.y=-damperRate*pistonVelocity.y;
  force.y+=forceDamper.y;

  // Antirollbar also presses against suspension
  if(arbOtherSusp)
  {
    forceARB.y=arbRate*(length-arbOtherSusp->GetLength());
    force.y+=forceARB.y;
  }

  // Check for damper force NOT to reverse the velocity
  // (which would contradict the frictional property of the damper)
  // If it is too big, cut it off so the velocity of the wheel becomes 0
  // (it is somewhat like a constraint force)
  // May have to be done in Integrate() (and actually, the real
  // integration is done only in class RWheel).
  //...

#ifdef LTRACE
qdbg("  pistonVelocity=%.2f,.%2f,.%2f\n",
pistonVelocity.x,pistonVelocity.y,pistonVelocity.z);
qdbg("  damper force=%.2f,%.2f,%.2f\n",
forceDamper.x,forceDamper.y,forceDamper.z);
#endif

  // Total acts on 2 points; body and wheel
  //forceBody=-force;		// Future unary operator
  forceBody.x=forceBody.z=0;
  forceBody.y=-force.y;
  forceWheel=force;
#ifdef LTRACE
qdbg("RSusp:CF; forceWheel=%.2f,%.2f,%.2f\n",
forceWheel.x,forceWheel.y,forceWheel.z);
#endif
}
void RSuspension::CalcAccelerations()
// Obsolete
{
}

void RSuspension::Integrate()
{
  // Look at the suspension movement, check energy constraints
  // Spring total energy=E, Ep (potential energy) is the integral
  // over the work done to get the spring moved from its rest position.
  // This amounts to 1/2*k*x^2 (k=spring rate, x=offset)
  // Ek (kinetic energy) is the velocity of the spring's attached mass.
  // It is 1/2*m*v^2, but unsure whether to add both tire and sprung mass
  // to that.
  //...
}

/*******************
* External effects *
*******************/
bool RSuspension::ApplyWheelTranslation(DVector3 *translation,
  DVector3 *velocity)
// The wheel is moving; stretch or compress the spring
// If the translation leads to a physically impossible state,
// FALSE is returned and 'translation' contains the translation
// needed to get the wheel inside physically possible limits.
// Otherwise the wheel
// could move farther than the suspension will allow.
// We should signal this to the driver perhaps; a hard bump stop!
// 'velocity' contains the wheel velocity with respect to the car
{
#ifdef LTRACE
qdbg("RSusp:AWT; translate Y=%f, length=%f\n",translation->y,length);
#endif
  // Y changes length of suspension
  length-=translation->y;
  // Maximum reached?
  if(length<minLength)
  {
#ifdef LTRACE
qdbg("RSusp:AWT; l<min: length=%f, minLen=%f, org translateY=%f, tY=%f\n",
length,minLength,translation->y,minLength-length);
#endif
    translation->y=length-minLength;
    length=minLength;
    pistonVelocity.SetToZero();
    return FALSE;
  } else if(length>maxLength)
  {
#ifdef LTRACE
qdbg("RSusp:AWT; l>max: length=%f, minLen=%f, org translateY=%f, tY=%f\n",
length,minLength,translation->y,minLength-length);
#endif
    translation->y=maxLength-length;
    length=maxLength;
    pistonVelocity.SetToZero();
    return FALSE;
  }
  
  // Velocity of wheel wrt car
  // As the wheel is connected to the suspension vertically,
  // the piston velocity is the same
  pistonVelocity=*velocity;
#ifdef LTRACE
qdbg("RSusp::ApplyWheelTrans; pistonVelocity=%.2f,%.2f,%.2f\n",
pistonVelocity.x,pistonVelocity.y,pistonVelocity.z);
#endif
  return TRUE;
}
