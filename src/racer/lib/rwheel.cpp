/*
 * RWheel - a wheel
 * 05-08-00: Created! (03:46:17)
 * 11-12-00: Using RModel instead of direct geode. Bounding box support.
 * 26-06-01: Pacejka constants are now defined per-tire
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
#include <d3/geode.h>
DEBUG_ENABLE

// Local trace?
//#define LTRACE

// Use full 3D patch for surface detection?
#define USE_3D_PATCH

#define ENV_INI      "env.ini"

// For method #3, slipVector (Gregor Veble), use tan(SA) or SA?
#define OPT_SLIPVECTOR_USES_TANSA

// Skid methods
//#define SKID_METHOD_SEPARATE_LON_LAT
//#define SKID_METHOD_SLIP_RA_VECTOR
#define SKID_METHOD_Z_GREGOR

// Point at which skidmarks appear
#define SKIDMARK_SLIPRATIO  0.2

// Apply damping to slipRatio/slipAngle differential eq's at low speed?
//#define DO_DAMPING_LAT
//#define DO_DAMPING_LONG

// Apply low-speed enhancements? (obsolete since 18-5-01)
#define DO_LOW_SPEED

// Distance (in m) to start with wheel ray-track intersection; this is
// the height at which the ray is started to avoid getting a ray
// that starts BENEATH the track surface and therefore not hitting it.
#define DIST_INTERSECT   1.0

// Define the next symbol to check for wheel velocity reversal (vertically),
// and if so, the wheel is stopped momentarily. Is used to rule out
// incredibly high damper forces from pushing the wheel to full up or down.
// Should perhaps not be needed anymore combined with the implicit integrator.
// Note that this acts at the outermost positions of the wheel.
#define DAMP_VERTICAL_VELOCITY_REVERSAL

// Damp the wheel when crossing the equilibrium position?
// As the wheel passes its center position, the spring force reverses. To
// avoid adding energy into the system, when passing this position, damping
// is used to avoid overaccelerating the tire to the other side.
// Note that this acts when the wheel is near its center (rest) position,
// contrast this with DAMP_VERTICAL_VELOCITY_REVERSAL.
#define DAMP_VERTICAL_EQUILIBRIUM_REVERSAL

// Use implicit integration? This should be more stable with
// high damper rates.
#define INTEGRATE_IMPLICIT_VERTICAL

// Gregor Veble combined slip algorithm? (instead of Pacejka)
//#define DO_GREGOR
#ifdef DO_GREGOR
#undef DO_DAMPING_LAT
#undef DO_DAMPING_LONG
#endif

// Delayed slip angle?
//#define USE_SAE950311_LAT

// If not using SAE950311, damp SA at low speed? (to avoid jittering)
//#define USE_SA_DAMPING_AT_LOW_SPEED

// Wheel locking results in force going the direction of -slipVector?
//#define USE_WHEEL_LOCK_ADJUST

struct SurfaceFriction
{
  float staticFriction,
        kineticFriction;
};
//static SurfaceFriction surface,*curSurface;

RWheel::RWheel(RCar *_car)
  : RDriveLineComp()
{
  SetName("wheel");

  // Reset all vars that may be loaded by Load()
  // Other variables should be reset in Reset()
  car=_car;
  mass=750;
  radius=1;
  width=.2;               // 20 cm
  slices=5;
  propFlags=0;
  flags=0;
  lock=40;
  toe=0;
  ackermanFactor=1;
  frictionDefault=1;
  crvSlipTraction=0;
  crvSlipBraking=0;
  crvLatForce=0;
  crvSlip2FC=0;
  slipRatio=0;
  slipAngle=0;
  //load=0;
  //rapSkid=0;
  radiusLoaded=0;
  velMagnitude=0;
  distCGM=distCM=0;
  tireRate=0;
  aligningTorque=0;
  differential=0;
  differentialSide=0;
  rollingCoeff=0;
  csSlipLen=0;
  skidStrip=-1;
  skidLongStart=skidLongMax=0;
  skidLongStartNeg=skidLongMaxNeg=0;
  skidLatStart=skidLatMax=0;
  optimalSR=optimalSA=0.1;         // Avoid div0
  offsetWC.SetToZero();
  torqueTC.SetToZero();
  torqueFeedbackTC.SetToZero();
  torqueBrakingTC.SetToZero();
  torqueRollingTC.SetToZero();
  forceRoadTC.SetToZero();
  forceDampingTC.SetToZero();
  forceBrakingTC.SetToZero();
  forceBodyCC.SetToZero();
  forceGravityCC.SetToZero();
  velWheelCC.SetToZero();
  posContactWC.SetToZero();
  slipVectorCC.SetToZero();
  position.SetToZero();
  velocity.SetToZero();
  rotation.SetToZero();
  rotationV.SetToZero();
  rotationA.SetToZero();
  slipVector.SetToZero();
  forceVerticalCC.SetToZero();
  devPoint.SetToZero();
  Init();
  
#ifdef RR_GFX_OGL
  quad=0;
#endif
  model=0;
  bbox=0;
  modelBrake=0;
  matBrake=0;

//qlog(QLOG_INFO,"RWheel ctor: si: lastNode=%p\n",surfaceInfo.lastNode);
}
RWheel::~RWheel()
{
  Destroy();
}

void RWheel::Destroy()
{
#ifdef RR_GFX_OGL
  if(quad){ gluDeleteQuadric(quad); quad=0; }
#endif
  QDELETE(model);
  QDELETE(bbox);
  QDELETE(modelBrake);
  // Curves
  QDELETE(crvSlipTraction);
  QDELETE(crvSlipBraking);
  QDELETE(crvLatForce);
  QDELETE(crvSlip2FC);
}

void RWheel::Init()
// Reset vars to re-use component
{
  stateFlags=ATTACHED;
}

void RWheel::SetWheelIndex(int n)
// Set the wheel index (as seen from the car)
{
  wheelIndex=n;
  // Change name to be more descriptive (debugging)
  char buf[40];
  sprintf(buf,"wheel%d",n);
  SetName(buf);
}

/*******
* Dump *
*******/
bool RWheel::LoadState(QFile *f)
{
  RDriveLineComp::LoadState(f);
  f->Read(&stateFlags,sizeof(stateFlags));
  f->Read(&lowSpeedFactor,sizeof(lowSpeedFactor));
  f->Read(&slipAngle,sizeof(slipAngle));
  f->Read(&slipRatio,sizeof(slipRatio));
  //f->Read(&tanSlipAngle,sizeof(tanSlipAngle));
  f->Read(&relaxationLengthLat,sizeof(relaxationLengthLat));
  f->Read(&lastU,sizeof(lastU));
  f->Read(&signU,sizeof(signU));
  f->Read(&relaxationLengthLong,sizeof(relaxationLengthLong));
  f->Read(&differentialSlipRatio,sizeof(differentialSlipRatio));
  f->Read(&dampingFactorLong,sizeof(dampingFactorLong));
  f->Read(&dampingFactorLat,sizeof(dampingFactorLat));
  //f->Read(&load,sizeof(load));
  f->Read(&radiusLoaded,sizeof(radiusLoaded));
  //f->Read(&aligningTorque,sizeof(aligningTorque));
  f->Read(&surfaceInfo,sizeof(surfaceInfo));
  f->Read(&torqueTC,sizeof(torqueTC));
  f->Read(&torqueFeedbackTC,sizeof(torqueFeedbackTC));
  f->Read(&forceRoadTC,sizeof(forceRoadTC));
  f->Read(&forceDampingTC,sizeof(forceDampingTC));
  f->Read(&forceBrakingTC,sizeof(forceBrakingTC));
  f->Read(&forceBodyCC,sizeof(forceBodyCC));
  f->Read(&forceGravityCC,sizeof(forceGravityCC));
  f->Read(&velMagnitude,sizeof(velMagnitude));
  f->Read(&velWheelCC,sizeof(velWheelCC));
  f->Read(&velWheelTC,sizeof(velWheelTC));
  f->Read(&posContactWC,sizeof(posContactWC));
  f->Read(&slipVectorCC,sizeof(slipVectorCC));
  f->Read(&slipVectorTC,sizeof(slipVectorTC));
  f->Read(&slipVectorLength,sizeof(slipVectorLength));
  f->Read(&position,sizeof(position));
  f->Read(&velocity,sizeof(velocity));
  f->Read(&acceleration,sizeof(acceleration));
  f->Read(&rotation,sizeof(rotation));
  f->Read(&rotationV,sizeof(rotationV));
  f->Read(&rotationA,sizeof(rotationA));
  f->Read(&slipVector,sizeof(slipVector));
  f->Read(&frictionCoeff,sizeof(frictionCoeff));
  f->Read(&forceVerticalCC,sizeof(forceVerticalCC));
  return TRUE;
}
bool RWheel::SaveState(QFile *f)
{
  RDriveLineComp::SaveState(f);
  f->Write(&stateFlags,sizeof(stateFlags));
  f->Write(&lowSpeedFactor,sizeof(lowSpeedFactor));
  f->Write(&slipAngle,sizeof(slipAngle));
  f->Write(&slipRatio,sizeof(slipRatio));
  //f->Write(&tanSlipAngle,sizeof(tanSlipAngle));
  f->Write(&relaxationLengthLat,sizeof(relaxationLengthLat));
  f->Write(&lastU,sizeof(lastU));
  f->Write(&signU,sizeof(signU));
  f->Write(&relaxationLengthLong,sizeof(relaxationLengthLong));
  f->Write(&differentialSlipRatio,sizeof(differentialSlipRatio));
  f->Write(&dampingFactorLong,sizeof(dampingFactorLong));
  f->Write(&dampingFactorLat,sizeof(dampingFactorLat));
  //f->Write(&load,sizeof(load));
  f->Write(&radiusLoaded,sizeof(radiusLoaded));
  //f->Write(&aligningTorque,sizeof(aligningTorque));
  f->Write(&surfaceInfo,sizeof(surfaceInfo));
  f->Write(&torqueTC,sizeof(torqueTC));
  f->Write(&torqueFeedbackTC,sizeof(torqueFeedbackTC));
  f->Write(&forceRoadTC,sizeof(forceRoadTC));
  f->Write(&forceDampingTC,sizeof(forceDampingTC));
  f->Write(&forceBrakingTC,sizeof(forceBrakingTC));
  f->Write(&forceBodyCC,sizeof(forceBodyCC));
  f->Write(&forceGravityCC,sizeof(forceGravityCC));
  f->Write(&velMagnitude,sizeof(velMagnitude));
  f->Write(&velWheelCC,sizeof(velWheelCC));
  f->Write(&velWheelTC,sizeof(velWheelTC));
  f->Write(&posContactWC,sizeof(posContactWC));
  f->Write(&slipVectorCC,sizeof(slipVectorCC));
  f->Write(&slipVectorTC,sizeof(slipVectorTC));
  f->Write(&slipVectorLength,sizeof(slipVectorLength));
  f->Write(&position,sizeof(position));
  f->Write(&velocity,sizeof(velocity));
  f->Write(&acceleration,sizeof(acceleration));
  f->Write(&rotation,sizeof(rotation));
  f->Write(&rotationV,sizeof(rotationV));
  f->Write(&rotationA,sizeof(rotationA));
  f->Write(&slipVector,sizeof(slipVector));
  f->Write(&frictionCoeff,sizeof(frictionCoeff));
  f->Write(&forceVerticalCC,sizeof(forceVerticalCC));
  return TRUE;
}

/**********
* Loading *
**********/
bool RWheel::Load(QInfo *info,cstring path)
{
  char buf[128],fname[256];
  QInfo *infoCurve;
  DVector3 p;
  
  Destroy();

  // Location
  sprintf(buf,"%s.x",path);
  offsetWC.x=info->GetFloat(buf);
  sprintf(buf,"%s.y",path);
  offsetWC.y=info->GetFloat(buf);
  sprintf(buf,"%s.z",path);
  offsetWC.z=info->GetFloat(buf);

  // Physical attribs
  sprintf(buf,"%s.mass",path);
  mass=info->GetFloat(buf,20.0f);
  sprintf(buf,"%s.inertia",path);
  SetInertia(info->GetFloat(buf,2));
  sprintf(buf,"%s.radius",path);
  radius=info->GetFloat(buf,.25f);
  sprintf(buf,"%s.width",path);
  width=info->GetFloat(buf,.3f);
  sprintf(buf,"%s.tire_rate",path);
  tireRate=info->GetFloat(buf,140000.0f);
  // Skidding
  sprintf(buf,"%s.skidding.long_start",path);
  skidLongStart=info->GetFloat(buf);
  sprintf(buf,"%s.skidding.long_max",path);
  skidLongMax=info->GetFloat(buf);
  sprintf(buf,"%s.skidding.long_start_n",path);
  skidLongStartNeg=info->GetFloat(buf);
  sprintf(buf,"%s.skidding.long_max_n",path);
  skidLongMaxNeg=info->GetFloat(buf);
  sprintf(buf,"%s.skidding.lat_start",path);
  skidLatStart=info->GetFloat(buf);
  sprintf(buf,"%s.skidding.lat_max",path);
  skidLatMax=info->GetFloat(buf);
//qdbg("skid_lat_max=%f\n",skidLatMax);

  // Braking force (specified as torque)
  sprintf(buf,"%s.max_braking",path);
  maxBrakingTorque=info->GetFloat(buf,2000.0f);
  // Braking factor (multiplied by maxBrakingTorque to get actual
  // torque). Used to determine brake balance.
  sprintf(buf,"%s.braking_factor",path);
  brakingFactor=info->GetFloat(buf,1);

  // Lock in radians
  sprintf(buf,"%s.lock",path);
  lock=info->GetFloat(buf,40.0f)/RR_RAD_DEG_FACTOR;
  sprintf(buf,"%s.toe",path);
  toe=info->GetFloat(buf)/RR_RAD_DEG_FACTOR;
  sprintf(buf,"%s.ackerman",path);
  ackermanFactor=info->GetFloat(buf,1);
  // Reverse Ackerman effect for right wheels
  if(offsetWC.x<0)
    ackermanFactor=-ackermanFactor;

  sprintf(buf,"%s.steering",path);
  if(info->GetInt(buf,1))
    propFlags|=STEERING;
  sprintf(buf,"%s.powered",path);
  if(info->GetInt(buf))
    propFlags|=POWERED;
  // Get some friction base values
  sprintf(buf,"%s.friction_road",path);
  frictionDefault=info->GetFloat(buf,1.0f);
  sprintf(buf,"%s.rolling_coeff",path);
  rollingCoeff=info->GetFloat(buf);
  
  // Tire model
  sprintf(buf,"tire_model.relaxation_length_lat");
  relaxationLengthLat=info->GetFloat(buf,1);
  sprintf(buf,"tire_model.relaxation_length_long");
  relaxationLengthLong=info->GetFloat(buf,1);
  sprintf(buf,"tire_model.damping_speed");
  dampingSpeed=info->GetFloat(buf);
  sprintf(buf,"tire_model.damping_coefficient_lat");
  dampingCoefficientLat=info->GetFloat(buf);
  sprintf(buf,"tire_model.damping_coefficient_long");
  dampingCoefficientLong=info->GetFloat(buf);
  sprintf(buf,"%s.pacejka",path);
  pacejka.Load(info,buf);
  sprintf(buf,"%s.pacejka.optimal_slipratio",path);
  optimalSR=info->GetFloat(buf,0.1f);
  sprintf(buf,"%s.pacejka.optimal_slipangle",path);
  optimalSA=info->GetFloat(buf,0.1f);
  tanOptimalSA=tan(optimalSA);
  
  // Curves
  crvSlipTraction=new QCurve();
  crvSlipBraking=new QCurve();
  crvLatForce=new QCurve();
  crvSlip2FC=new QCurve();
 
#ifdef OBS_ITS_ALL_PACEJKA
  sprintf(buf,"%s.curves.traction_force",path);
  info->GetString(buf,fname);
//qdbg("fname_traction='%s'\n",fname);
  if(fname[0])
  { infoCurve=new QInfo(RFindFile(fname,car->GetDir()));
    crvSlipTraction->Load(infoCurve,"curve");
    QDELETE(infoCurve);
  }
  sprintf(buf,"%s.curves.braking_force",path);
  info->GetString(buf,fname);
//qdbg("fname_braking='%s'\n",fname);
  if(fname[0])
  { infoCurve=new QInfo(RFindFile(fname,car->GetDir()));
    crvSlipBraking->Load(infoCurve,"curve");
    QDELETE(infoCurve);
  }

  sprintf(buf,"%s.curves.lat_force",path);
  info->GetString(buf,fname);
//qdbg("fname_latforce='%s'\n",fname);
  if(fname[0])
  { infoCurve=new QInfo(RFindFile(fname,car->GetDir()));
    crvLatForce->Load(infoCurve,"curve");
    QDELETE(infoCurve);
  }
#endif

  if(RMGR->IsDevEnabled(RManager::USE_SLIP2FC))
  {
    // Read slip -> friction coefficient curves
    sprintf(buf,"%s.curves.slip2fc",path);
    info->GetString(buf,fname);
    if(fname[0])
    { infoCurve=new QInfo(RFindFile(fname,car->GetDir()));
      if(infoCurve->FileExists())
      {
        crvSlip2FC->Load(infoCurve,"curve");
      } else
      {
        // Use default value of 1 for all values
        crvSlip2FC->AddPoint(0,1);
      }
      QDELETE(infoCurve);
    }
    sprintf(buf,"%s.curves.slip2fc.velocity_factor",path);
    slip2FCVelFactor=info->GetFloat(buf);
  }

  // Paint attribs (default wheel representation)
  sprintf(buf,"%s.slices",path);
  slices=info->GetInt(buf,20);
  // Model (or stub gfx)
  model=new RModel(car);
  model->Load(info,path);
  if(!model->IsLoaded())
    quad=gluNewQuadric();
  bbox=new DBoundingBox();
  bbox->EnableCSG();
  bbox->GetBox()->min.x=-width;
  bbox->GetBox()->min.y=-radius;
  bbox->GetBox()->min.z=-radius;
  bbox->GetBox()->max.x=width;
  bbox->GetBox()->max.y=radius;
  bbox->GetBox()->max.z=radius;

  // Brake model (doesn't rotate along with wheel, except for steering)
  modelBrake=new RModel(car);
  modelBrake->Load(info,path,"model_brake");

  Reset();

#ifdef OBS
  // Take over car initial orientation
  DVector3 p;
  // Note this call is reversed wrt coord systems
  car->ConvertCarToWorldOrientation(&position,&p);
  rotation=*car->GetBody()->GetRotation();
qdbg("RWh: pos=%f,%f,%f, p=%f,%f,%f\n",position.x,position.y,position.z,
p.x,p.y,p.z);
  position=p;
#endif

  // Deduce some facts
  
  // Calc distance of wheel to CGM
  p=*susp->GetPosition()+offsetWC;
  distCGM=sqrt(p.x*p.x+p.z*p.z);
  
  // Calc angle from CGM to wheel
  angleCGM=atan2(p.x,p.z);
  
  // Calculate distance to CM
  distCM=distCGM;
  
  // Debugging
  rotationV.x=RMGR->infoDebug->GetFloat("car.wheel_rvx");
  return TRUE;
}

/********
* Reset *
********/
void RWheel::Reset()
{
  // Place wheel near suspension
  position=*susp->GetPosition()+offsetWC;
  position.y-=susp->GetRestLength();
  velocity.SetToZero();
  rotation.SetToZero();
  rotationV.SetToZero();
  rotationA.SetToZero();
  // Slip variables
  tanSlipAngle=0;
  slipAngle=0;
  slipRatio=0;
  lastU=0;
  signU=1;         // u>=0
  differentialSlipRatio=0;
  skidStrip=-1;
//qdbg("RWheel ctor: position=%f/%f/%f\n",position.x,position.y,position.z);

  // Set initial heading to toe value (steering will do this quickly enough
  // with SetHeading(), but the unsteered wheels remain static; rotation.y remains the same)
  rotation.y=toe;
}

/**********
* Attribs *
**********/
void RWheel::SetHeading(float h)
// Set the heading; applies toe to 'h' (so 'h' is the heading
// without taking toe into account
{
  if(h>lock)h=lock;
  else if(h<-lock)h=-lock;
  rotation.y=h+toe;
  // Apply Ackerman effect
  if((ackermanFactor<0&&h<0)||(ackermanFactor>0&&h>0))
    rotation.y*=fabs(ackermanFactor);
}
void RWheel::DisableBoundingBox()
{
  flags&=~DRAW_BBOX;
}
void RWheel::EnableBoundingBox()
{
  flags|=DRAW_BBOX;
}
#ifdef OBS
bool RWheel::IsAttached()
{
  if(stateFlags&ATTACHED)return TRUE;
  return FALSE;
}
bool RWheel::IsLowSpeed()
// Returns TRUE if wheel is turning slowly so you may need
// to reduce some effects to avoid low-speed numerical instability
{
  if(stateFlags&LOW_SPEED)return TRUE;
  return FALSE;
}
#endif

/********
* Paint *
********/
void RWheel::Paint()
{
  float colTyre[]={ .2,.2,.2 };
  DVector3 pos;
  
#ifdef RR_GFX_OGL
  //
  // OpenGL wheel painting
  //
  glPushMatrix();
  
  // Location
  //pos=position+*susp->GetPosition();
  pos=position;
//qdbg("RWh:Paint; pos=%f/%f/%f\n",pos.x,pos.y,pos.z);
//qdbg("Car pos=%f,%f,%f\n",car->GetPosition()->x,
//car->GetPosition()->y,car->GetPosition()->z);
  //pos.y-=susp->GetLength();
  //glTranslatef(position.GetX(),position.GetY(),position.GetZ());
  glTranslatef(pos.GetX(),pos.GetY(),pos.GetZ());
//qdbg("RWh:Paint: y=%f\n",pos.GetY());
  
#ifdef OBS
  // Forces
  float v[3];
  v[0]=0; v[1]=0; v[2]=force.GetZ();
  glColor3f(0,0,0);
  GfxArrow(0,-radius+.01,0,v,.001);
  // Lateral
  glRotatef(-90,0,1,0);
  v[0]=0; v[1]=0; v[2]=force.GetX();
  GfxArrow(0,-radius+.01,0,v,.001);
  glRotatef(90,0,1,0);
#endif
  
  // Vectors in CC
//qdbg("RWh:Paint; rotation.y=%f deg\n",rotation.y*RR_RAD_DEG_FACTOR);
  //glRotatef(car->GetBody()->GetRotation()->y*RR_RAD_DEG_FACTOR,0,1,0);
  
  if(RMGR->IsDevEnabled(RManager::SHOW_TIRE_FORCES))
  {
    DVector3 v;
    glTranslatef(0,-radius+.01,0);
    v=velWheelCC; v.y+=.05;
    RGfxVector(&v,.1,0,1,0);
    v=slipVectorCC; v.y+=.05;
    RGfxVector(&v,.1,1,1,0);
    // Split body force into XZ and Y for clarity
    v=forceBodyCC; v.y=0.05;
    RGfxVector(&v,.001,1,0,0);
    v=forceBodyCC; v.x=v.z=0;
    RGfxVector(&v,.001,1,0,0);
    glTranslatef(0,radius-.01,0);
  }
  
  //glRotatef(-car->GetBody()->GetRotation()->y*RR_RAD_DEG_FACTOR,0,1,0);
  
#ifdef ND_PAINT_FORCES
  glTranslatef(0,-radius+.01,0);
  car->ConvertCarToWorldCoords(&force,&v);
  //RGfxVector(&v,.001,0,0,0);
  //car->ConvertCarToWorldCoords(&forceTorque,&v);
  v=forceTorque;
  RGfxVector(&v,.001,0,1,0);
  glTranslatef(0,.01,0);
  //car->ConvertCarToWorldCoords(&forceFriction,&v);
  v=forceFriction;
  RGfxVector(&v,.001,1,0,0);
  glTranslatef(0,.01,0);
  car->ConvertWorldToCarCoords(&forceSlip,&v);
  RGfxVector(&v,.001,1,.5,.5);
  car->ConvertWorldToCarCoords(&slipVector,&v);
  RGfxVector(&v,.1,1,1,0);
  glTranslatef(0,-.01,0);
  glTranslatef(0,-.01,0);
  glTranslatef(0,radius-.01,0);
#endif
  
#ifdef OBS
  // Velocity
  v[0]=vx; v[1]=vy; v[2]=vz;
  glColor3f(1,0,0);
  GfxArrow(0,radius,0,v,.1);
#endif
  
#ifdef OBS
  // Text prepare
  GfxGo(0,0,0);
#endif
  
  if(flags&DRAW_BBOX)
  {
    // Draw the bounding box in the model-type rotation
    // (the stub must rotate so the cylinder is at the local Z-axis)
    glPushMatrix();
    // Heading
    glRotatef(rotation.y*RR_RAD_DEG_FACTOR,0,1,0);
    // Rotation of wheel (from radians to degrees)
    glRotatef(rotation.x*RR_RAD_DEG_FACTOR,1,0,0);
    bbox->Paint();
    glPopMatrix();
  }

  // Discs are painted at Z=0
  if(quad)
  {
    glRotatef(90,0,1,0);
    // Heading
    glRotatef(rotation.y*RR_RAD_DEG_FACTOR,0,1,0);
    // Rotation of wheel (from radians to degrees)
    glRotatef(rotation.x*RR_RAD_DEG_FACTOR,0,0,1);
  } else
  {
    // Heading
#ifdef RR_ODE
    // Weird shit
    //glRotatef(-rotation.y*RR_RAD_DEG_FACTOR,0,1,0);
    glRotatef(rotation.y*RR_RAD_DEG_FACTOR,0,1,0);
#else
    glRotatef(rotation.y*RR_RAD_DEG_FACTOR,0,1,0);
#endif
    // Brake model listens to heading but not the wheel rotation itself
    // Future may influence the color of the brake disk
    modelBrake->Paint();
    // Rotation of wheel (from radians to degrees)
    glRotatef(rotation.x*RR_RAD_DEG_FACTOR,1,0,0);
  }

  // Primitives
  if(model!=0&&model->IsLoaded())
  {
    model->Paint();
    goto skip_quad;
  } // else paint stub gfx
  
  car->SetDefaultMaterial();
  glDisable(GL_TEXTURE_2D);

  // Center point for quad
  glTranslatef(0,0,-width/2);
  
#ifdef OBS
  if(rdbg->flags&RF_WHEEL_INFO)
  { char buf[80];
    sprintf(buf,"Force lon=%.f, lat=%.f",
      force.z,force.x);
    GfxText3D(buf);
  }
#endif
 
  glMaterialfv(GL_FRONT,GL_DIFFUSE,colTyre);
  gluCylinder(quad,radius,radius,width,slices,1);
  gluQuadricOrientation(quad,GLU_INSIDE);
  gluDisk(quad,0,radius,slices,1);
  gluQuadricOrientation(quad,GLU_OUTSIDE);
  glTranslatef(0,0,width);
  gluDisk(quad,0,radius,slices,1);
  
 skip_quad:
  
  glPopMatrix();
#endif

}

/*****************
* Calculate Load *
*****************/
void RWheel::CalcLoad()
// Calculate normal force on wheel (in Newtons)
// Also calculates radius of loaded wheel and the force
// that the suspensions puts on the wheel.
{
  rfloat y;

#ifdef LTRACE
qdbg("RWheel:CalcLoad()\n");
#endif

  // Are we touching a surface? (simple Y only version)
#ifdef USE_3D_PATCH
  // surfaceInfo.y contains distance to track, instead of world Y value (!)
  y=surfaceInfo.surfaceDistance;
#else
  // surfaceInfo.y (v0.4.7) contains world Y intersection coordinate
  y=posContactWC.y-surfaceInfo.y;
#endif

#ifdef LTRACE
qdbg("RWheel:Pre; carY=%f, suspLen=%f, y (inTrack)=%f\n",
 car->GetPosition()->y,susp->GetLength(),y);
#endif
  // On surface when very near ground
  if(y<0)
  { stateFlags|=ON_SURFACE;
    forceRoadTC.y=-tireRate*y;
  } else
  { stateFlags&=~ON_SURFACE;
    // No load when in the air
    forceRoadTC.y=0;
  }
//qdbg("  IsOnSurface: %d\n",IsOnSurface());

  // Calculate suspension force on wheel
  //load=susp->GetForceBody()->y;
  //load=forceRoadTC.y;
  
  // Calculate radius of loaded wheel
  // Should deal with tire pressure and such
  // For now, no deformation
  radiusLoaded=radius;
  
#ifdef OBS
qdbg("RWheel:CalcLoad; force=%.2f (K=%.2ff)\n",
forceRoadTC.y,tireRate);
#endif
}

/***********************
* Calculate Slip Angle *
***********************/
void RWheel::CalcSlipAngle()
// Based on the wheel world velocity and heading, calculate
// the angle from the wheel heading to the wheel's actual velocity
// Contrary to SAE, our Y-axis points up, and slipAngle>0 means
// in our case that the you're in a right turn (basically)
// 31-03-01: SAE950311 implementation (using relaxation lengths
// and a differential equation for the slip angle) for low-speed etc.
{
  DVector3 velWheelWC;
  
  if(!IsOnSurface())
  {
//qdbg("CalcSA; not on surface\n");
    // Not touching the surface, no need to do slip angle
    slipAngle=0;
    // Tire springs back to 0 instantly (should be slowly actually)
    tanSlipAngle=0;
    velWheelTC.SetToZero();
    velWheelCC.SetToZero();
    return;
  }

#ifdef OBS
  // Calculate current wheel velocity in body (car) coords
  velWheelWC=*car->GetVelocity();
  // Convert velocity from world to car coords
  car->ConvertWorldToCarOrientation(&velWheelWC,&velWheelCC);
#ifdef OBS
  velWheelCC=*car->GetVelocity();
  car->ConvertCarToWorldOrientation(&velWheelCC,&velWheelWC);
#endif
  
  // Calculate wheel velocity because of rotation around yaw (Y) axis
  velWheelCC.x+=car->GetBody()->GetRotVel()->y*cos(-angleCGM)*distCGM;
  velWheelCC.z+=car->GetBody()->GetRotVel()->y*sin(-angleCGM)*distCGM;
  ConvertCarToTireOrientation(&velWheelCC,&velWheelTC);
#endif

#ifdef OBS
 if(!IsPowered())
 {qdbg("RWh:CSA; velWheelCC=%f,%f,%f\n",velWheelCC.x,velWheelCC.y,
 velWheelCC.z);
  velWheelTC.DbgPrint("velWheelTC");
 }
#endif

#ifdef OBS
  // Get velocity from suspension on body
  car->GetBody()->CalcBodyVelForBodyPos(susp->GetPosition(),&velWheelCC);
  ConvertCarToTireOrientation(&velWheelCC,&velWheelTC);
#ifdef OBS
 if(!IsPowered())
 qdbg("RWh:CSA; velWheelCC_RRB=%f,%f,%f\n",
 velWheelCC.x,velWheelCC.y,velWheelCC.z);
#endif
#endif

  // Get velocity of contact patch wrt the track (=world)
  DVector3 cpPos,*p;
  p=susp->GetPosition();
  // Taking the tire movement from really the contact patch location
  // seems to overrate the lateral movement the tire goes through.
  // This is due because the body rolls, but normally the suspension
  // won't roll the tire in exactly the same way, but instead,
  // the tire rotates with respect to the body, so the lateral
  // movements are actually quite small.
  // To simulate this, I approximate the movement with respect
  // to the suspension attachment point, which moves far less
  // laterally. This became apparent when dealing with much stiffer
  // tire and suspension spring rates, which made the car wobbly.
  // Taking a less laterally sensitive position solved this.
  cpPos.x=p->x;
  //cpPos.y=p->y+susp->GetLength()+radius;
  cpPos.y=p->y;
  cpPos.z=p->z;               // Kingpin effects are ignored
  car->GetBody()->CalcBodyVelForBodyPos(&cpPos,&velWheelCC);
  car->ConvertCarToWorldOrientation(&velWheelCC,&velWheelWC);
  ConvertCarToTireOrientation(&velWheelCC,&velWheelTC);
#ifdef OBS
 velWheelWC.DbgPrint("velWheelWC");
 velWheelCC.DbgPrint("velWheelCC");
 velWheelTC.DbgPrint("velWheelTC");
#endif
 
//qdbg("velWheelTC.z=%f, rotZ=%f\n",velWheelTC.z,rotationV.x*radius);

#ifdef USE_SAE950311_LAT
  // Derive change in tan(slipAngle) using SAE950311
  // u=longitudinal velocity, v=lateral velocity
  rfloat u,b,v,delta;
  u=velWheelTC.z;
  v=velWheelTC.x;
  b=relaxationLengthLat;
  
#ifdef OBS_SETTLE_DOWN
  // Make sure SA settles down at low speeds
  rfloat min=5.0f;
  if(u<min&&u>=0.0f)u=min;
  else if(u>-min&&u<=0)u=-min;
#endif
  
  // Calculate change in tan(SA)
  delta=(v/b)-(fabs(u)/b)*tanSlipAngle;
 if(!IsPowered())
 qdbg("CSA; u=%f, v=%f, delta=%f, tanSA=%f, SA=%f\n",
 u,v,delta,tanSlipAngle,slipAngle);
//qdbg("delta=%f, tanSA=%f, SA=%f\n",delta,tanSlipAngle,atan(tanSlipAngle));

  // Integrate
  tanSlipAngle+=delta*RR_TIMESTEP;
  // Derive new slip angle from state variable 'tanSlipAngle'
  slipAngle=atanf(tanSlipAngle);
  
#ifdef OBS
qdbg("CSA; u=%f, v=%f, delta=%f, tanSA=%f, SA=%f\n",
 u,v,delta,tanSlipAngle,slipAngle);
#endif
#else
  // Non-SAE950311 method (no lag in slip angle buildup)
  // Calculate slip angle as the angle between the wheel velocity vector
  // and the wheel direction vector
  if(velWheelCC.x>-RR_EPSILON_VELOCITY&&velWheelCC.x<RR_EPSILON_VELOCITY &&
     velWheelCC.z>-RR_EPSILON_VELOCITY&&velWheelCC.z<RR_EPSILON_VELOCITY)
  { // Very low speed; no slip angle
    //slipAngle=-GetHeading();
    slipAngle=0;
//qdbg("  RWh;CSA; LS => slipAngle=0\n");
  } else
  { // Enough velocity, calculate real angle
    slipAngle=atan2(velWheelCC.x,velWheelCC.z)-GetHeading();

#ifdef USE_SA_DAMPING_AT_LOW_SPEED
//qdbg("SA; vx=%f, vz=%f\n",velWheelCC.x,velWheelCC.z);
    // Damp down at low velocity
    if(fabs(velWheelCC.x)<0.3f&&fabs(velWheelCC.z)<0.3f)
    {
      rfloat max;
      max=fabs(velWheelCC.x);
      if(fabs(velWheelCC.z)<max)
        max=fabs(velWheelCC.z);
//qdbg("damp SA %f factor %f => %f\n",slipAngle*RR_RAD2DEG,max,slipAngle*max);
      slipAngle*=max*0.5f;
    }
    
#endif
    
    // Keep slipAngle between -180..180 degrees
    if(slipAngle<-PI)slipAngle+=2*PI;
    else if(slipAngle>PI)slipAngle-=2*PI;
#ifdef OBS
qdbg("  atan2(%f,%f)=%f\n",velWheelCC.x,velWheelCC.z,
 atan2(velWheelCC.x,velWheelCC.z));
#endif
  }

//qdbg("  slipAngle=%.3f deg\n",slipAngle*RR_RAD_DEG_FACTOR);
#endif

#ifdef OPT_SLIPVECTOR_USES_TANSA
  if(RMGR->frictionCircleMethod==RManager::FC_SLIPVECTOR)
  {
    // We need tan(SA) when combining Pacejka Fx/Fy later
    tanSlipAngle=tan(slipAngle);
//qdbg("FCSV: tanSA=%f\n",tanSlipAngle);
  }
#endif
}

/***********************
* Calculate Slip Ratio *
***********************/
void RWheel::CalcSlipRatio(DVector3 *velWheelTC)
// Calculate current slip ratio.
// Assumes that CalcSlipAngle() has been called (for velWheel?C to be valid)
// 31-03-01: SAE950311 implementation (using relaxation lengths
// and a differential equation for the slip ratio) for low-speed etc.
{
  //rfloat vGround,vFreeRolling;

//velWheelTC->DbgPrint("velWheelTC in CalcSR");
  if(!IsOnSurface())
  {
//qdbg("CalcSR; not on surface\n");
    // Not touching the surface, no slip ratio
    slipRatio=0;
    // Tire vibrations stop
    differentialSlipRatio=0;
    return;
  }

  // SAE950311 algorithm
  rfloat u,delta,B;
  u=velWheelTC->z;
  B=relaxationLengthLong;
  
  // Switch to normal slip ratio calculations when there is
  // speed (in the case when the velocities catch up on the timestep)
  // Notice this test can be optimized, because the timestep and B
  // are constants, so a 'turningPointU' can be defined.
//qdbg("Threshold: %f\n",u*RR_TIMESTEP/B);
  if(u*RR_TIMESTEP/B>0.5)
  {
    // Use straightforward slip ratio
    slipRatio=rotationV.x*radiusLoaded/u-1;
//qdbg("'u' big enough; straight SR %f\n",slipRatio);
    return;
  }
  
  if((lastU<0&&u>=0)||(lastU>=0&&u<0))
  {
    // 'u' changed sign, reverse slip
//qdbg("'u' changed sign from %f to %f; SR=-SR\n",u,lastU);
    //differentialSlipRatio=-differentialSlipRatio;
    // Damp while reversing slip; we're close to a standstill.
    differentialSlipRatio=-RMGR->dampSRreversal*differentialSlipRatio;
  }
  lastU=u;
  // Detect sign of 'u' (real velocity of wheel)
  if(u<0)signU=-1.0f; else signU=1.0f;
#ifdef OBS
  // Differential equation (RvG)
  delta=(u+rotationV.x*radius)/B-(u/B)*differentialSlipRatio;
#endif
  // Eq. 26
  //delta=(fabs(u)-rotationV.x*radius*signU)/B-(fabs(u)/B)*differentialSlipRatio;
  if(u<0)
  {
    // Eq. 25
    delta=(-u+rotationV.x*radius)/B+(u/B)*differentialSlipRatio;
  } else
  {
    // Eq. 20
    delta=(u-rotationV.x*radius)/B-(u/B)*differentialSlipRatio;
  }
  // Integrate
  differentialSlipRatio+=delta*RR_TIMESTEP;
  if(differentialSlipRatio>10)differentialSlipRatio=10;
  else if(differentialSlipRatio<-10)differentialSlipRatio=-10;
  // Set slip ratio for the rest of the sim. Note that
  // we use a different slip ratio definition from that of SAE950311
//qdbg("  old SR=%f => new SR=%f\n",slipRatio,-differentialSlipRatio);
  slipRatio=-differentialSlipRatio;

#ifdef OBS
 qdbg("CSR; u=%f, R=%.2f, w=%f, B=%f, delta=%f, DSR=%f, SR=%f\n",
 u,radius,rotationV.x,B,delta,differentialSlipRatio,slipRatio);
#endif
}

void RWheel::CalcSlipVelocity()
// Calculate slip velocity vector in TC and CC
{
  slipVectorTC.x=0;
  slipVectorTC.y=0;
  slipVectorTC.z=rotationV.x*radius;
  ConvertTireToCarOrientation(&slipVectorTC,&slipVectorCC);
  slipVectorCC-=velWheelCC;
  
  // Calculate length of slip
  slipVectorLength=slipVectorCC.Length();
  
  // Calculate resulting friction coefficient
  // This will be used to determine the tire's maximum force
  if(RMGR->IsDevEnabled(RManager::USE_SLIP2FC))
    frictionCoeff=crvSlip2FC->GetValue(slipVectorLength/slip2FCVelFactor);
  else
    frictionCoeff=1.0f;

//qdbg("W%d: |slip|=%f, FC=%f\n",wheelIndex,slipVectorLength,frictionCoeff);
}

void RWheel::CalcWheelPosWC()
// Calculate wheel position in world coordinates, looking at body
// pitch, roll and yaw.
// 2 different positions are of importance; the contact patch location,
// and the wheel center location. Currently only the CP location is calculated.
// The wheel center may be handy for driving on the side (90 degree roll).
// The CP WC position is used to match against the track (altitude).
{
  DVector3 posContactCC;

#ifdef OBS
qdbg("RWh:CWPWC; position=(%f,%f,%f)\n",
 position.x,position.y,position.z);
qdbg("RWh:CWPWC; susp->position=(%f,%f,%f)\n",
 susp->GetPosition()->x,susp->GetPosition()->y,susp->GetPosition()->z);
 car->GetBody()->GetPosition()->DbgPrint("car Position");
#endif

  // Start with contact patch position in car coordinates
  // Note that 'position' already contains the suspension Y offset
  posContactCC=position;
  // Go from wheel center to edge of wheel
  posContactCC.y-=radius;
#ifdef OBS
qdbg("RWh:CWPWC; contactCC=(%f,%f,%f), pitch=%f\n",
 posContactCC.x,posContactCC.y,posContactCC.z,
 car->GetBody()->GetRotation()->x*RR_RAD_DEG_FACTOR);
#endif

  // Convert around 
  car->ConvertCarToWorldCoords(&posContactCC,&posContactWC);
#ifdef OBS
qdbg("RWh:CWPWC;   contactCC=(%f,%f,%f)\n",
 posContactCC.x,posContactCC.y,posContactCC.z);
qdbg("RWh:CWPWC;   contactWC=(%f,%f,%f)\n",
 posContactWC.x,posContactWC.y,posContactWC.z);
 car->GetBody()->GetRotPosM()->DbgPrint("Body rotMatrix");
#endif

  // Store to show visually (debug check)
  devPoint=posContactWC;
}

/**********
* Pacejka *
**********/
void RWheel::CalcPacejka()
// Setup Pacejka and calculate combined forces
{
  rfloat normalForce;

#ifdef LTRACE
qdbg("RWheel:CalcPacejka()\n");
#endif

  // Determine normal force (load/forceRoadTC.y)
  if(RMGR->IsDevEnabled(RManager::USE_SUSP_AS_LOAD))
  {
    // For the tire forces calculations, use the suspension force
    // instead of the tire rate force. As the suspension force is damped
    // and more stable (lower spring rate), this can avoid numerical
    // instability in the force calculations, which was assumed to
    // happen in at least v0.4.7. Paul Harrington suggest to use
    // the suspension force directly, in a slightly different way (only
    // when the wheel just left the surface). I use it here for the tire
    // forces all the time, but still use the forceRoadTC.y (the less
    // stable version determined by the amount of tire 'sinking' into
    // the road) to accelerate the wheel up/down. This gives the same
    // up/down movements as before, but should generally smooth out
    // tire lateral/longitudinal forces and hopefully give more realistic
    // grip while avoiding higher sim frequencies.
    if(IsOnSurface())
      normalForce=susp->GetForceBody()->y;
    else
      normalForce=0;
//qdbg("use susp F %.2f instead of fRoadTC.y=%f\n",normalForce,forceRoadTC.y);
  } else
  {
    normalForce=forceRoadTC.y;
  }

  pacejka.SetCamber(0);
  // Note our positive slipAngle is the reverse of SAE's definition
  if(RMGR->frictionCircleMethod==RManager::FC_SLIPVECTOR)
  {
    // Gregor Veble's and also Brian Beckman's idea of mixing Pacejka
    // Note that Gregor's Veble 'z' is 'csSlipLen' here.
    // The load isn't touched
    pacejka.SetNormalForce(normalForce);
    // Lateral
//qdbg("  csSlipLen=%f, oSA=%f,oSR=%f\n",csSlipLen,optimalSA,optimalSR);
    if(csSlipLen<D3_EPSILON)
    {
//qdbg("  csSlipLen near 0; load %f\n",normalForce);
      pacejka.SetSlipAngle(0);
      pacejka.SetSlipRatio(0);
      // Calculate separate Flat/Flong (Fy/Fx)
      pacejka.Calculate();
    } else
    {
      if(slipAngle<0)
        pacejka.SetSlipAngle(csSlipLen*optimalSA);
      else
        pacejka.SetSlipAngle(-csSlipLen*optimalSA);
      // Longitudinal
      if(slipRatio<0)
        pacejka.SetSlipRatio(-csSlipLen*optimalSR);
      else
        pacejka.SetSlipRatio(csSlipLen*optimalSR);
      // Calculate separate Flat/Flong (Fy/Fx)
      pacejka.Calculate();
      // Combine
#ifdef LTRACE
qdbg("  pac PRE Fx=%f, Fy=%f, Mz=%f\n",pacejka.GetFx(),pacejka.GetFy(),
pacejka.GetMz());
qdbg("  SA=%f, optSA=%f\n",slipAngle,optimalSA);
qdbg("  tanSA=%f, tanOptSA=%f\n",tanSlipAngle,tanOptimalSA);
#endif
#ifdef OPT_SLIPVECTOR_USES_TANSA
      pacejka.SetFy( (fabs(tanSlipAngle)/(tanOptimalSA*csSlipLen))*
        pacejka.GetFy());
      pacejka.SetMz( (fabs(tanSlipAngle)/(tanOptimalSA*csSlipLen))*
        pacejka.GetMz());
#else
      // Use normal slip angle
      pacejka.SetFy( (fabs(slipAngle)/(optimalSA*csSlipLen))*pacejka.GetFy());
      pacejka.SetMz( (fabs(slipAngle)/(optimalSA*csSlipLen))*pacejka.GetMz());
#endif
      pacejka.SetFx( (fabs(slipRatio)/(optimalSR*csSlipLen))*pacejka.GetFx());
#ifdef LTRACE
qdbg("  pac POST Fx=%f, Fy=%f, Mz=%f\n",pacejka.GetFx(),pacejka.GetFy(),
pacejka.GetMz());
#endif
    }
  } else
  {
    // Calculate Fx and Fy really separate, and possible combine later
    pacejka.SetSlipAngle(-slipAngle);
    pacejka.SetSlipRatio(slipRatio);
    pacejka.SetNormalForce(normalForce);
    // Calculate separate Flat/Flong (Fy/Fx), and maximum force
    // Combined Pacejka (slipratio & slipangle) will be done later
    pacejka.Calculate();
  }

  // Adjust forces according to the surface
  // May also add here some Pacejka scaling to get quick grip changes
  RSurfaceType *st=surfaceInfo.surfaceType;
  if(st)
  {
#ifdef LTRACE
qdbg("  st=%p\n");
qdbg("    friction=%f\n",st->frictionFactor);
#endif
    if(st->frictionFactor!=1.0f)
    {
      pacejka.SetFx(pacejka.GetFx()*st->frictionFactor);
      pacejka.SetFy(pacejka.GetFy()*st->frictionFactor);
      // Do the same to the aligning moment, although not scientifically
      // based (does aligning torque scale linearly with surface friction?)
      pacejka.SetMz(pacejka.GetMz()*st->frictionFactor);
      pacejka.SetMaxLongForce(pacejka.GetMaxLongForce()*st->frictionFactor);
      pacejka.SetMaxLatForce(pacejka.GetMaxLatForce()*st->frictionFactor);
    }
  }

  // Put some results in appropriate variables
#ifdef LTRACE
qdbg("  pacejka.Fx=%f\n",pacejka.GetFx());
#endif
  forceRoadTC.z=pacejka.GetFx();
}

/**********
* Damping *
**********/
void RWheel::CalcDamping()
// Calculate damping of the tire at low speed, to avoid
// oscillations.
{
#if defined(DO_DAMPING_LAT) || defined(DO_DAMPING_LONG)
  rfloat u,v,b,B;
  
  if(!IsOnSurface())return;

  u=velWheelTC.z;
  v=velWheelTC.x;
#ifdef DO_DAMPING_LAT
  b=relaxationLengthLat;
#endif
#ifdef DO_DAMPING_LONG
  B=relaxationLengthLong;
#endif

#endif

#ifdef DO_DAMPING_LAT
  //
  // Lateral damping
  //
  
  
  // Calculate damping force, to be added later
  // Assumes CalcLoad() has already been called (for 'load' to be valid)
//qdbg("u=%f, v=%f\n",u,v);
  //if(velWheelTC.LengthSquared()<dampingSpeed*dampingSpeed&&load>0)
  if(fabs(v)<dampingSpeed)
  //if(velWheelTC.Length()<dampingSpeed)
  //if(car->GetBody()->GetLinVel()->Length()<dampingSpeed)
  //if(car->GetBody()->GetRotVel()->y<dampingSpeed&&load>0)
  //if(load>0)
  //if(IsLowSpeed()==TRUE&&load>0)
  {
    //qdbg("W%d DAMP\n",wheelIndex);
    forceDampingTC.x=2.0f*dampingCoefficientLat*v*
      sqrt((load*(pacejka.GetCorneringStiffness() /* *57.2f*/)) /
           (RMGR->scene->env->GetGravity()*b));
    forceDampingTC.x*=frictionCoeff;
#ifdef OBS
    dampingFactorLat=
      (fabs(car->GetBody()->GetRotVel()->y))*1.0f;
#endif
    //if(dampingFactorLat<0.1f)dampingFactorLat=0.1f;
      //(dampingSpeed-fabs(car->GetBody()->GetRotVel()->y))*1.0f;
#ifdef OBS
    // Factor damping
    dampingCoefficient=1.0f;
    dampingFactorLat=dampingCoefficient*(dampingSpeed-fabs(v));
    if(dampingFactorLat>1)dampingFactorLat=1;
    
    //slipAngle*=dampingFactorLat;
 dampingFactorLat=0;
    //dampingFactorLat=fabs(v)*3;
#endif

  // Natural frequency (debug)
  rfloat natFreq;
  natFreq=sqrtf(RMGR->scene->env->GetGravity()*pacejka.GetCorneringStiffness()/
    load*b);
#ifdef OBS
qdbg("W%d; natFreq=%f ~ %f Hz\n",wheelIndex,natFreq,natFreq/6.28f);
  car->GetBody()->GetLinVel()->DbgPrint("body linVel");
#endif

 forceRoadTC.x=pacejka.GetFy();
qdbg("Flat=%.2f, dampFlat=%.2f, cornStiffness=%.f, load %.f, v=%f\n",
 forceRoadTC.x,forceDampingTC.x,pacejka.GetCorneringStiffness(),load,v);
}
#ifdef OBS
qdbg("yawVel=%f, dampFactorLat=%f, tanSA=%f\n",
 car->GetBody()->GetRotVel()->y,dampingFactorLat,tanSlipAngle);
#endif

#endif

#ifdef DO_DAMPING_LONG
  //
  // Longitudinal damping
  //
  
  // Calculate damping force, to be added later
  // Assumes CalcLoad() has already been called (for 'load' to be valid)
  if(fabs(u)<dampingSpeed)
  //if(velWheelTC.Length()<dampingSpeed)
  //if(velWheelTC->LengthSquared()<dampingSpeed*dampingSpeed&&load>0)
  {
    forceDampingTC.z=2.0f*dampingCoefficientLong*u*
      sqrt((load*pacejka.GetLongitudinalStiffness()) /
           (RMGR->scene->env->GetGravity()*B));
    forceDampingTC.z*=frictionCoeff;
#ifdef OBS
qdbg("Fz=%f, dampFlon=%f, longStiffness=%.f, load %.f, u=%f, v=%f,\n",
 pacejka.GetFx(),forceDampingTC.z,pacejka.GetLongitudinalStiffness(),load,u,v);
#endif
  }
//qdbg("dampFactorLong=%f\n",dampingFactorLong);

#endif
}

/*************
* PreAnimate *
*************/
void RWheel::PreAnimate()
// Calculate wheel state.
{
//RMGR->profile->AddCount(1);

  // Reset vars
  forceVerticalCC.SetToZero();
  forceRoadTC.SetToZero();
  forceDampingTC.SetToZero();
  dampingFactorLong=dampingFactorLat=0;
  torqueTC.SetToZero();
  torqueFeedbackTC.SetToZero();
  
  // Calculate things that depend only on the state variables
  
  // Calculate wheel position in world coordinates
  CalcWheelPosWC();
  
  // Get surface underneath the wheel
  // Should take actual downward direction instead of simple down vector
  // to include loopings
  DVector3 dir0(0.0f,-1.0f,0.0f),dir,org;
  // Rotate direction so it matches the car's orientation
  car->ConvertCarToWorldOrientation(&dir0,&dir);
  // Start origin a meter above the wheel, to avoid missing the track,
  // but also not too much to avoid sensing a possible bridge ABOVE
  // the car (see the Carrera track for example).
  org.x=posContactWC.x-DIST_INTERSECT*dir.x;
  org.y=posContactWC.y-DIST_INTERSECT*dir.y;
  org.z=posContactWC.z-DIST_INTERSECT*dir.z;
  // Give the ray some more length, so you don't lose the shadow-track
  // intersection so quickly.
  dir.Scale(2);
  // Optimization; lots of the track are known to be far away
  surfaceInfo.startNode=car->GetEnclosingNode();
  // Trace into the track
  RMGR->GetTrack()->GetSurfaceInfo(&org,&dir,&surfaceInfo);
  // Calculate actual distance from the intersection point
  // to the contact patch point. 'surfaceInfo' contains
  // the intersection point as calculated from the 3D spline patch.
  // This may not be entirely where the contact patch is, because
  // the spline patch cannot follow the (linear) spline quads exactly,
  // otherwise it would not be a patch (but it does go through all
  // the quad corner points). But well, we just try and see if this
  // isn't noticed on the track.
  // Note that to get the distance (which is signed in this case),
  // I project the vector from the contact patch location to the
  // intersection point onto the ray. As the surface patches coordinates
  // may shift versus the actual ray-quad (or ray-triangle) intersection,
  // this projection (dot product) may not be entirely correct, but my
  // intuition tells me that any minor track shifts will just look ok
  // when you're driving it.
  // Also note the convenient 'dir' vector, which IS the ray we can
  // project the difference vector on (surfaceInfoIntersection-posContactWC).
  // As 'dir' is (0,-1,0), it is normalized (though rotated), so no
  // normalization has to be done. We can just project
  // the vector 'surfaceInfoIntersection-posContactWC' onto dir and get
  // the actual distance from the contact patch to the road.
  DVector3 v(surfaceInfo.x-posContactWC.x,surfaceInfo.y-posContactWC.y,
    surfaceInfo.z-posContactWC.z);
  // Fake surfaceInfo to give distance in 'y' for now.
  // We should really use a surfaceInfo.distanceToTrack variable
  surfaceInfo.surfaceDistance=v.Dot(dir);
#ifdef OBS
posContactWC.DbgPrint("posContactWC");
qdbg("surfaceDistance=%.2f\n",surfaceInfo.surfaceDistance);
qdbg("surface: x=%.2f, y=%.2f, z=%.2f\n",surfaceInfo.x,
surfaceInfo.y,surfaceInfo.z);
#endif
  //surfaceInfo.y=posContactWC.y-d;
//qdbg("d=%.3f\n",d);
//surfaceInfo.x=surfaceInfo.y=surfaceInfo.z=0;
#ifdef LTRACE
qdbg("surface found Y=%.2f\n",surfaceInfo.y);
#endif

#ifdef OBS
qdbg("car position: %.2f,%.2f,%.2f\n",
 car->GetPosition()->x,car->GetPosition()->y,car->GetPosition()->z);
#endif
//surfaceInfo.y-=5.33;

  // Load transfer
  CalcLoad();
  
  // Slip angle and ratio
  CalcSlipAngle();
  CalcSlipRatio(&velWheelTC);
  if(RMGR->frictionCircleMethod==RManager::FC_SLIPVECTOR)
  {
    // Calculate SA/SR vector length (>1 = wanting more than tire supports)
    rfloat lat,lon;
#ifdef OPT_SLIPVECTOR_USES_TANSA
    lat=tanSlipAngle/tanOptimalSA;
#else
    lat=slipAngle/optimalSA;
#endif
    lon=slipRatio/optimalSR;
    csSlipLen=sqrtf(lat*lat+lon*lon);
//qdbg("  latNrm=%f, lonNrm=%f, csSlipLen=%f\n",lat,lon,csSlipLen);
  }
  
  CalcSlipVelocity();
  
  // Low speed detection
#ifdef DO_LOW_SPEED
  if(rotationV.x>-RMGR->lowSpeed&&rotationV.x<RMGR->lowSpeed)
  { stateFlags|=LOW_SPEED;
    // Calculate factor from 0..1. 0 means real low, 1 is on the edge
    // of becoming high-enough speed.
    lowSpeedFactor=rotationV.x/RMGR->lowSpeed;
    // Don't zero out all
    if(lowSpeedFactor<0.01f)lowSpeedFactor=0.01f;
  } else
  { stateFlags&=~LOW_SPEED;
  }
#endif

  CalcPacejka();
  CalcDamping();
  
#ifdef OBS
  // Calculate velocity magnitude to base softening of forces on
  // when dealing with low speed numerical instability (wheels
  // shaking back & forth)
  velMagnitude=slipVectorCC.Length();
#endif

#ifdef OBS
qdbg("RWh:Pre; slipAngle=%.2f, SR=%.5f (vzTC=%.2f, rotX=%.2f),"
  " load=%.f\n",
 slipAngle*RR_RAD_DEG_FACTOR,slipRatio,velWheelTC.z,rotationV.x,load);
qdbg("         v=%f, vCarCC=%f,%f,%f\n",velMagnitude,
 car->GetVelocity()->x,car->GetVelocity()->y,car->GetVelocity()->z);
#endif
#ifdef OBS
qdbg("  velWheelTC=%f,%f,%f\n",velWheelTC.x,velWheelTC.y,velWheelTC.z);
#endif
}

/*************
* OnGfxFrame *
*************/
void RWheel::OnGfxFrame()
// Update audio frequencies and volumes (only once per gfx frame)
{
  // Audio
  skidFactor=0;
  
  // Do a rough approximation of slip sound
  if(!IsOnSurface())
  {
    // Can't skid in the air
    skidFactor=0;
    //rapSkid->SetVolume(0);
  } else
  {
    rfloat len;
    
    //
    // Some skid method tried here
    //

#ifdef SKID_METHOD_SEPARATE_LON_LAT
    // Longitudinal slip sounds
qdbg("SR=%f, skidLongMax=%f, longStart=%f\n",slipRatio,
skidLongMax,skidLongStart);
    if(slipRatio<0)
    {
      if(slipRatio<-skidLongMaxNeg)skidFactor+=1.0f;
      else if(slipRatio<-skidLongStartNeg)
      {
        // Ratio
        skidFactor+=-(slipRatio+skidLongStartNeg)/
          (skidLongMaxNeg-skidLongStartNeg);
      }
    } else
    {
      // Driving
      if(slipRatio>skidLongMax)skidFactor+=1.0f;
      else if(slipRatio>skidLongStart)
      {
        // Ratio
        skidFactor+=(slipRatio-skidLongStart)/
          (skidLongMax-skidLongStart);
      }
    }
qdbg("RWheel: SR=%f, longSkidFactor=%f\n",slipRatio,skidFactor);

    // Lateral skidding
    
    // Take normalized sideways slip
    // Note that at 20-07-01, slipVectorTC doesn't contain the real
    // slip velocity for the tire (see CalcSlipVelocity()), so I use
    // slipVectorCC instead.
    if(fabs(slipVectorCC.x)>D3_EPSILON)
    {
      if(fabs(slipVectorCC.x)>1.0f)
      {
        len=fabs(slipVectorCC.x/velWheelCC.x);
//qdbg("slip=%.2f, vel=%.2f, ratio=%.2f\n",slipVectorCC.x,velWheelCC.x,len);
      } else
      {
        // Small slip; take velocity directly
        len=slipVectorCC.x;
      }
      if(len>skidLatMax)skidFactor+=1.0f;
      else if(len>skidLatStart)
      {
        skidFactor+=(len-skidLatStart)/(skidLatMax-skidLatStart);
      }
    }
#endif

#ifdef SKID_METHOD_SLIP_RA_VECTOR
    // Try #3, using slip vector from Ratio and Angle
    skidFactor=0;
    DVector3 v;
    v.x=slipAngle*RR_RAD2DEG/6.0f;
    v.z=slipRatio;
    v.y=0;
    len=v.LengthSquared();
    if(len>0.2)
    {
      if(len>1.0)
      {
        skidFactor=1;
      } else
      {
        skidFactor+=(len-0.2)/(1.0-0.2);
      }
    }
#endif

#ifdef SKID_METHOD_Z_GREGOR
    // Method #4, use the already calculated csSlipLen
    // This is 'z' in Gregor Veble's terms
    if(csSlipLen>1)
    {
      skidFactor=1;
    } else if(csSlipLen>0.5)
    {
      skidFactor=(csSlipLen-0.5)/(1.0-0.5);
    }
    len=velWheelCC.LengthSquared();
    if(len<5.0)
    {
      // Damp quickly
      skidFactor*=len/20;
    }
#endif

  }
}

/**************
* Slip vector *
**************/
void RWheel::CalcSlipVector()
// OBS
// Based on the current wheel speed/rotation, calculate
// a slip vector; in which direction the wheel is slipping with
// respect to its theoretical direction
// The slip vector is in world coordinates
{
}

void RWheel::CalcFrictionCoefficient()
// OBS
// Based on the calculated slip vector, calculate the friction coefficient
// The faster a wheel is slipping with respect to the ground,
// the less grip will be available.
{
}

/*********
* Forces *
*********/
void RWheel::CapFrictionCircle()
// Make sure longitudinal and lateral force, when combined, don't
// exceed the maximum force the tire can generate
{
  // This feature is optional
  if(!RMGR->IsDevEnabled(RManager::APPLY_FRICTION_CIRCLE))return;

  switch(RMGR->frictionCircleMethod)
  {
    case RManager::FC_GENTA:
      // From G. Genta's 'Motor Vehicle Dynamics', page 83, eq. 2.24
      // (Fy/Fy0)^2+(Fx/Fx0)^2=1
      // Where Fy0 is the lateral force calculated using Pacejka, but
      // without taking into account any longitudinal force,
      // and Fx0 is the MAXIMUM longitudinal force. Fx is calculated
      // using Pacejka, and Fy is the unknown variable
      // Rearranging results in: Fy=Fy0*sqrt(1-(Fx/Fx0)^2)
      // This method favors longitudinal forces more than lateral forces,
      // which is uncertain to be correct.
      rfloat maxForce;
      maxForce=pacejka.GetMaxLongForce();
      // Adjust maximum force for current friction coefficient (based on slipvel)
      maxForce*=frictionCoeff;
      if(maxForce>D3_EPSILON)
      {
        dfloat Fy,Fy0,Fx,Fx0,ratioSquared;
        Fy0=pacejka.GetFy();
        Fx0=pacejka.GetMaxLongForce();
        Fx=pacejka.GetFx();
        ratioSquared=Fx/Fx0;
        ratioSquared*=ratioSquared;
        Fy=Fy0*sqrtf(1.0f-ratioSquared);
//qdbg("Flat FC'ed from %f to %f\n",Fy0,Fy);
        forceRoadTC.x=Fy;
      }
      break;
    case RManager::FC_VECTOR:
      // Vector trimming (equally downsizes long/lat if needed)
      // Looks a lot like SAE980243's method of capping force,
      // although I don't think that one does an ellipse very well.
      rfloat len;
      rfloat Flat,Flon,nrmFlat,nrmFlon,maxFlat,maxFlon;

      // Get road reaction forces
      Flon=pacejka.GetFx();
      Flat=pacejka.GetFy();
    
      // Normalize the forces
      maxFlat=pacejka.GetMaxLatForce();
      maxFlon=pacejka.GetMaxLongForce();
      // First check whether forces are possible
      if(fabs(maxFlat)<D3_EPSILON||
        fabs(maxFlon)<D3_EPSILON)return;
      nrmFlon=Flon/maxFlon;
      nrmFlat=Flat/maxFlat;
      // Calculate total vector size, but avoid the sqrt() for now
      len=nrmFlon*nrmFlon+nrmFlat*nrmFlat;
      //len=sqrtf(nrmFlon*nrmFlon+nrmFlat*nrmFlat);
  
      // If the vector exceeds the friction circle (which, because
      // it is normalized, is a real circle with radius 1), the
      // forces are downsized accordingly to fit the circle.
      // Note that 1 squared is still 1 (len is still squared)
      if(len>1.0f)
      {
        // Get actual length (non-squared)
        len=sqrt(len);
#ifdef OBS
qdbg("RWheel:CapFC(); vector trims (%.3f,%.3f) to (%.3f,%.3f)\n",
Flon,Flat,Flon/len,Flat/len);
qdbg("  len>1; len=%f\n",len);
#endif
        Flon/=len;
        Flat/=len;
        // Put forces back into the system
        forceRoadTC.z=Flon;
        forceRoadTC.x=Flat;
      }
      break;
    case RManager::FC_VOID:
      // Don't touch the forces. This may lead to forces
      // getting bigger than the tire can actually sustain.
      break;
    case RManager::FC_SLIPVECTOR:
      // Based on Gregor Veble's combined Pacejka forces ideas
      // This has already been done in PreAnimate(), since it required
      // modifying the INPUT to the Pacejka formulae.
      break;
    default:
      // Unkown method
      static bool reported;
      if(!reported)
      { reported=TRUE;
        qwarn("RWheel(%d); unknown friction circle method %d",
          wheelIndex,RMGR->frictionCircleMethod);
      }
      break;
  }
}

void RWheel::CalcForces()
// Calculate several force, mostly total forces though,
// as some lower-level forces have been calculated already
{
  //rfloat coeff;
  //rfloat maxForceMag,curForceMag;
  rfloat len;
  
#ifdef DO_GREGOR
  // Gregor Veble's model for combined slipRatio/Angle -> force
  rfloat Sx,Sy;
  rfloat S;
  rfloat F,Fx,Fy;
  rfloat SxFactor,SyFactor;

  Sx=slipRatio;
  Sy=tanSlipAngle;
  // Calculate total slip
  S=sqrtf(Sx*Sx+Sy*Sy);
  
  // Get total force from a Pacejka-like curve
  pacejka.SetCamber(0);
  // Note our positive slipAngle is the reverse of SAE's definition
  pacejka.SetSlipAngle(0);
  pacejka.SetSlipRatio(Sx);
  pacejka.SetNormalForce(forceRoadTC.y);
  pacejka.Calculate();
  
  // Get total force for the tire
  F=pacejka.GetFx();
  // Calculate X/Y components
  if(fabs(S)>D3_EPSILON)
  {
    SxFactor=Sx/S;
    SyFactor=Sy/S;
    Fx=F*SxFactor;
    Fy=F*SyFactor;
  } else
  {
    // Nearly no slip; no force
    Fx=Fy=0;
    SxFactor=0;
    SyFactor=0;
  }
  forceDampingTC.z*=fabs(SxFactor);
  forceDampingTC.x*=fabs(SyFactor);
//forceDampingTC.SetToZero();
//qdbg("Sx=%f, Sy=%f, S=%f, F=%f => Fx=%f, Fy=%f\n",Sx,Sy,S,F,Fx,Fy);
#endif

  //
  // VERTICAL FORCES
  //
  
  // Calculate gravitional force
  DVector3 forceGravityWC(0.f,-mass*RMGR->scene->env->GetGravity(),0.f);
  car->ConvertWorldToCarOrientation(&forceGravityWC,&forceGravityCC);
//qdbg("RWH:CF; forceGravityCC.y=%.2f (mass=%.2f)\n",forceGravityCC.y,mass);

  // Total vertical forces
  // Note that for vertical forces, TC orientation==CC orientation
  // so forceRoadTC doesn't need to be converted to CC
  forceVerticalCC=forceGravityCC+forceRoadTC+*susp->GetForceWheel();
#ifdef OBS
qdbg("RWh:CalcForces(); calc vertical forces\n");
forceGravityCC.DbgPrint("forceGravityCC");
forceRoadTC.DbgPrint("forceRoadTC");
susp->GetForceWheel()->DbgPrint("susp->GetForceWheel()");
forceVerticalCC.DbgPrint("forceVerticalCC");
#endif
  
  //
  // LONGITUDINAL FORCES
  //

  // Differential will do resulting long. torques for the engine

  // Calculate longitudinal road reaction force using Pacejka's magic formula
  rfloat pf;
  
#ifdef DO_GREGOR
  pf=Fx;
#else
  // Pacejka
  pf=pacejka.GetFx()*frictionCoeff;
#endif
#ifdef OBS
//qdbg("Curve Fx=%f, Pacejka Fx=%f\n",forceRoadTC.z,pf);
// if(this==car->GetWheel(2))
{ qdbg("Pacejka Fx=%.2f, nrmF=%f, SR=%f, SA=%f\n",
  pf,forceRoadTC.y,slipRatio,slipAngle);
}
#endif
//qdbg("Pacejka Fx=%f, signU=%f\n",pf,signU);
  forceRoadTC.z=signU*pf;
#ifdef DO_DAMPING_LONG
  // Add damping force because of slipRatio vibrations
  // at low speed (SAE950311)
  forceRoadTC.z+=forceDampingTC.z;
//qdbg("FroadTC.z=%f, Fdamp=%f\n",forceRoadTC.z,forceDampingTC.z);
#endif

//qdbg("RWh:CF; coeff=%f, load=%f\n",coeff,load);

  //
  // Calculate braking forces & torques (brakes & rolling resistance)
  //
  rfloat curBrakingTorque,factor;
  if(IsHandbraked())
  {
    // Add combined effect of brake pedal and handbrakes
    factor=((rfloat)car->GetBraking())*0.001f+
      car->GetDriveLine()->GetHandBrakeApplication();
    // It may be faster to just calculate here (skip the 'if')
    if(factor>=1.0f)curBrakingTorque=maxBrakingTorque;
    else            curBrakingTorque=factor*maxBrakingTorque;
//qdbg("Wheel%d handbraked %f\n",wheelIndex,factor);
  } else
  {
    // Calculate braking torque more directly
    curBrakingTorque=((rfloat)car->GetBraking())*maxBrakingTorque/1000.0f;
  }

  // Apply brake balance factor (to be able to get braking balance front/rear)
#ifdef OBS
qdbg("braking %d, curBrakingTorque=%f, factor %f\n",
 car->GetBraking(),curBrakingTorque,brakingFactor);
#endif
  curBrakingTorque*=brakingFactor;
//qdbg("curBrakingTorque=%f, rotV.x=%f\n",curBrakingTorque,rotationV.x);
  // Apply braking torque in the reverse direction of the wheel's rotation
  if(rotationV.x>0)
  { torqueBrakingTC.x=curBrakingTorque;
  } else
  { torqueBrakingTC.x=-curBrakingTorque;
  }
  forceBrakingTC.z=torqueBrakingTC.x/radiusLoaded;
#ifdef OBS
qdbg("  Tb=%f, Rl=%f, Fb=%f\n",torqueBrakingTC.x,radiusLoaded,
 forceBrakingTC.z);
#endif
//qdbg("RWh:CF; curBrakingTorque=%f\n",curBrakingTorque);

#ifdef OBS
  // Calculate engine force for legacy usage
  forceEngineTC.x=car->GetEngine()->GetTorque()/radiusLoaded;
#endif
  // Calculate feedback torque (goes back to differential)
  // This doesn't include braking and rolling resistance, which are
  // potential braking forces (which may not be fully used if the
  // wheel is not rotating), but only the forces which are always
  // there, like road reaction.
  torqueFeedbackTC.x=-forceRoadTC.z*radiusLoaded;
#ifdef LTRACE
qdbg("torqueFeedbackTC.x=%f, Froad.z=%f, r=%f\n",
 torqueFeedbackTC.x,forceRoadTC.z,radiusLoaded);
#endif

  // Calculate rolling resistance (from John Dixon, Fr=u*Fv
  // where Fv is the normal force, and 'u' the rolling coefficient)
  // Rolling coefficient may need to go up with speed though
  if(rotationV.x>0)
    torqueRollingTC.x=-rollingCoeff*forceRoadTC.y*radiusLoaded;
  else
    torqueRollingTC.x=rollingCoeff*forceRoadTC.y*radiusLoaded;
  if(surfaceInfo.surfaceType)
  {
    // Adjust torque for the surface
    torqueRollingTC.x*=surfaceInfo.surfaceType->rollingResistanceFactor;
  }

  // Calculate total braking torque (this is POTENTIAL, not always acted upon!)
  torqueBrakingTC.x=-forceBrakingTC.z*radiusLoaded;
  torqueBrakingTC.x+=torqueRollingTC.x;

#ifdef OBS
qdbg("W%d; Fbraking=%f, Froad=%f, Tfb.x=%f\n",
wheelIndex,forceBrakingTC.z,forceRoadTC.z,torqueFeedbackTC.x);
#endif
  
  //
  // LATERAL FORCES
  //
  
#ifdef OBS
  // Calc lateral force as a function of slip angle
  // Curve is only defined for positive slip angles
  if(slipAngle>=0.0)
  {
    coeff=crvLatForce->GetValue(slipAngle*RR_RAD_DEG_FACTOR);
  } else
  {
    coeff=-crvLatForce->GetValue(-slipAngle*RR_RAD_DEG_FACTOR);
  }
  // Hmm, include frictionDefault factor??
  forceRoadTC.x=-coeff*frictionDefault*load;
//qdbg("RWh:CF; slipAngle=%.2f deg, load=%f, Flat=%.2f\n",
//slipAngle*RR_RAD_DEG_FACTOR,load,forceRoadTC.x);
#endif

#ifdef DO_GREGOR
  pf=Fy;
#else
  pf=pacejka.GetFy()*frictionCoeff;
#endif
  //pf=forceRoadTC.x;
#ifdef OBS
qdbg("Pacejka Fy(lat)=%f, SR=%f, SA=%f, load=%f\n",
 pf,slipRatio,slipAngle,load);
#endif
  forceRoadTC.x=pf;
#ifdef DO_DAMPING_LAT
  // Add damping force because of slipAngle vibrations
  // at low speed (SAE950311)
  // Note the mix of 2 coordinate systems here
  forceRoadTC.x+=forceDampingTC.x;
//qdbg("Damping fRoad.x=%f, Fdamp=%f\n",forceRoadTC.x,forceDampingTC.x);
#endif

  if(RMGR->IsDevEnabled(RManager::NO_LATERAL_FORCES))
  {
    // Cancel lateral forces
    pacejka.SetFy(0);
    forceRoadTC.x=0;
  }

  // Friction circle may not be exceeded
  CapFrictionCircle();

#ifdef FUTURE
  // Low speed numerical instability patching
  if(IsLowSpeed())
  {
//qdbg("LS: factor=%f\n",lowSpeedFactor);
    // Long. force is often too high at low speed
    //forceRoadTC.z*=lowSpeedFactor;
  }
#endif

  // WHEEL LOCKING
  
  if(!RMGR->IsDevEnabled(RManager::WHEEL_LOCK_ADJUST))
    goto skip_wla;

  if(slipRatio<0)
  {
    DVector3 forceLockedCC,forceLockedTC;
    rfloat slideFactor,oneMinusSlideFactor,lenSlip,lenNormal,y;
    
    // As the wheel is braked, more and more sliding kicks in.
    // At the moment of 100% slide, the braking force points
    // in the reverse direction of the slip velocity. Inbetween
    // SR=0 and SR=-1 (and beyond), there is more and more sliding,
    // so the force begins to point more and more like the slip vel.
    
    // Calculate sliding factor (0..1)
    if(slipRatio<-1.0f)slideFactor=1.0f;
    else               slideFactor=-slipRatio;
//slideFactor*=.75f;
    oneMinusSlideFactor=1.0f-slideFactor;
    // Calculate 100% wheel lock direction
    forceLockedCC.x=slipVectorCC.x;
    forceLockedCC.y=0;
    forceLockedCC.z=slipVectorCC.z;
    // Make it match forceRoadTC's coordinate system
    ConvertCarToTireOrientation(&forceLockedCC,&forceLockedTC);
    // Calc magnitude of normal and locked forces
    lenSlip=forceLockedTC.Length();
    y=forceRoadTC.y; forceRoadTC.y=0;
    lenNormal=forceRoadTC.Length();
    forceRoadTC.y=y;
    if(lenSlip<D3_EPSILON)
    {
      // No force
      forceLockedTC.SetToZero();
    } else
    {
      // Equalize force magnitude
      forceLockedTC.Scale(lenNormal/lenSlip);
    }
#ifdef OBS
//if(this==car->GetWheel(2))
{forceRoadTC.DbgPrint("forceRoadTC");
 forceLockedCC.DbgPrint("forceLockedCC");
qdbg("lenSlip=%f, lenNormal=%f, slideFactor=%f\n",
 lenSlip,lenNormal,slideFactor);
}
#endif
    // Interpolate between both extreme forces
    forceRoadTC.x=oneMinusSlideFactor*forceRoadTC.x+
                  slideFactor*forceLockedTC.x;
    forceRoadTC.z=oneMinusSlideFactor*forceRoadTC.z+
                  slideFactor*forceLockedTC.z;
  }
 skip_wla:
  
  // ALIGNING TORQUE
  
  aligningTorque=pacejka.GetMz();
  // Damp aligning torque at low speed to avoid jittering of wheel
  // Friction will take care of forces in that case
  len=velWheelCC.LengthSquared();
  if(len<1.0)
  {
    // Damp
    aligningTorque*=len;
  }
 
//forceRoadTC.DbgPrint("RWh:CF forceRoadTC");
}

void RWheel::CalcTorqueForces(rfloat torque)
// OBS
// Apply torque (motor) to get a force that wants to rotate the wheel.
// The force on the ground may results in a friction force
// that pushes the car forward, rotating the wheels etc.
// Calc's forceTorque in car coords, forceFriction in car coords
{
}

void RWheel::CalcSuspForces()
// OBS
// Calculate suspension forces
{
}

/******************
* Applying forces *
******************/
void RWheel::CalcWheelAngAcc()
// Calculate wheel angular acceleration
{
  if(differential)
  {
    // Use differential to determine acceleration
    if(differential->IsLocked(differentialSide))
    {
      // Wheel is held still by braking torques
      rotationA.x=0;
    } else
    {
      // Wheel is allowed to roll
      rotationA.x=differential->GetAccOut(differentialSide);
    }
//qdbg("diff=%p, diffSide=%d\n",differential,differentialSide);
#ifdef OBS
qdbg("RWh:AF; rotationA=%.2f,%.2f,%.2f\n",
rotationA.x,rotationA.y,rotationA.z);
#endif
  } else
  {
    // Uses torque directly (no engine/differential forces)
    rotationA.x=(torqueFeedbackTC.x+torqueBrakingTC.x)/GetInertia();
  }
}
void RWheel::CalcBodyForce()
// Calculate force exterted on body
{
  DVector3 forceBodyTC;
  
  // The force on the body is exactly the road force
  // Note that the forceBodyTC.y value is the normal force
  // (which is load), but isn't directly applied to the body.
  // This IS done by the suspension.
  forceBodyTC=forceRoadTC;
  
#ifdef OBS
qdbg("RWh:AF; forceBodyTC=%.2f,%.2f,%.2f\n",
forceBodyTC.x,forceBodyTC.y,forceBodyTC.z);
#endif
  // Convert to car coordinates
  ConvertTireToCarOrientation(&forceBodyTC,&forceBodyCC);
//qdbg("RWh:AF; forceBodyCC=%.2f,%.2f,%.2f\n",
//forceBodyCC.x,forceBodyCC.y,forceBodyCC.z);
}
void RWheel::CalcAccelerations()
// Apply the forces to get the net results
// Leaves 'forceBody' for the car's acceleration (CG)
{
  rotationA.SetToZero();
  
  CalcWheelAngAcc();
  CalcBodyForce();

  // Vertical forces; suspension, gravity, ground, tire rate
  //acceleration=forceVerticalCC/GetMass();
  acceleration.x=acceleration.z=0.0f;
  acceleration.y=forceVerticalCC.y/GetMass();
#ifdef OBS
qdbg("RWh:AF; forceVerticalCC.y=%f, mass=%f\n",
forceVerticalCC.y,GetMass());
#endif

//qdbg("RWh:AF; acc=%f,%f,%f\n",acceleration.x,acceleration.y,acceleration.z);
}

void RWheel::AdjustCoupling()
// OBS
// Hierarchy of car is not what it's supposed to be
{
}

/************
* Integrate *
************/
void RWheel::Integrate()
// Integrate the wheel's position and velocity.
{
  DVector3 translation;
  rfloat   oldVX;

#ifdef OBS
//qdbg("RWh:Int0; pos=(%.2f,%.2f,%.2f)\n",position.x,position.y,position.z);
//qdbg("RWh;Int; rotV.x=%f, rotA.x=%f\n",rotationV.x,rotationA.x);
qdbg("  rotV.x=%f, rotA.x=%f\n",rotationV.x,rotationA.x);
#endif

#ifdef OBS
qdbg("RWh%d:Int; rot=%f, rotV=%f, rotA=%f\n",
 wheelIndex,rotation.x,rotationV.x,rotationA.x);
#endif
  
  //
  // ROTATIONAL acceleration
  //
  oldVX=rotationV.x;
#ifdef OBS
  // Check for locked wheel braking
  netForce=0; //forceEngineTC.z+forceRoadTC.z;
qdbg("Fe=%f, Fr=%f, Fb=%f\n",forceEngineTC.z,forceRoadTC.z,forceBrakingTC.z);
  if(rotationV.x>-RR_EPSILON_VELOCITY&&rotationV.x<RR_EPSILON_VELOCITY)
  {
    if((netForce<0&&forceBrakingTC.z>-netForce)||
       (netForce>0&&forceBrakingTC.z<-netForce))
    //if(forceBrakingTC.z<-netForce||forceBrakingTC.z>netForce)
    {
//qdbg("RWh:Int; braking force keeps wheel still\n");
      rotationV.x=0;
      goto skip_rot;
    }
  }
#endif

  rotationV.x+=rotationA.x*RR_TIMESTEP;
//qdbg("  rotV=%f\n",rotationV.x);
 //skip_rot:
  
  // Check for wheel velocity reversal; in this case, damp the
  // velocity to a minimum to get rid of jittering wheels at low
  // speed and/or high braking.
  if(differential)
  {
    if(oldVX>0&&rotationV.x<0)
    {
      // Lock the side
//qdbg("RWh%d; vel reversal, lock\n",wheelIndex);
      differential->Lock(differentialSide);
      rotationV.x=0;
      differentialSlipRatio=0;
    } else if(oldVX<0&&rotationV.x>0)
    {
//qdbg("RWh%d; vel reversal, lock\n",wheelIndex);
      // Lock the side
      differential->Lock(differentialSide);
      rotationV.x=0;
      differentialSlipRatio=0;
    }
  } else
  {
    // No differential, use controlled jittering
    if(oldVX>=0&&rotationV.x<0)
    {
      // Just lift the rotation over the 0 barrier; this will induce
      // jittering near 0, but it's so small that it's unnoticable.
      // If we keep it at 0, you get larger jitters either way (+/-)
      // until the wheel rotates again and starts the reversal again.
      rotationV.x=-0.0001;
    } else if(oldVX<=0&&rotationV.x>0)
    {
      rotationV.x=0.0001;
    }
  }
//qdbg("  rotV after velocity reversal=%f\n",rotationV.x);
  
  // Wheel rotation (spinning)
  //
  //rotation.x+=(rotationV.x+0.5f*rotationA.x*RR_TIMESTEP)*RR_TIMESTEP;
  rotation.x+=rotationV.x*RR_TIMESTEP;
  // Keep rotation in limits
  while(rotation.x>=2*PI)
    rotation.x-=2*PI;
  while(rotation.x<0)
    rotation.x+=2*PI;

//qdbg("RWh;Int; braking force=%f\n",forceBrakingTC.z);

  // Friction reversal measures
  if( (oldVX<0&&rotationV.x>0) || (oldVX>0&&rotationV.x<0) )
  { rotationV.x=0;
//qdbg("RWh:Int; friction reversal halt; %f -> %f\n",oldVX,rotationV.x);
  }

  //
  // VERTICAL linear movements
  //
  //translation=(velocity+0.5f*acceleration*RR_TIMESTEP)*RR_TIMESTEP;
  //translation=velocity*RR_TIMESTEP;
  // Calculate new velocity and check for velocity reversal
  DVector3 oldVelocity=velocity;

#ifdef INTEGRATE_IMPLICIT_VERTICAL
  // Use implicit integration, as the suspension dampers can explode
  // quite easily. Separate all non-damper forces (which will be
  // integrated explicitly) and damper forces (which will be done implicitly).
  DVector3 forceNonDamper;
  forceNonDamper=forceVerticalCC;
  forceNonDamper.y-=susp->GetForceDamper()->y;
  //acceleration.y=forceVerticalCC.y/GetMass();
  acceleration.y=forceNonDamper.y/GetMass();
//qdbg("RWheel:Int: forceNonDamper=%f, acc=%f\n",forceNonDamper.y,acceleration.y);
//qdbg("  grav=%f, roadTC=%f, susp=%f\n",forceGravityCC.y,forceRoadTC.y,
//susp->GetForceWheel()->y);

  // Implicit integration (derived by using the future velocity in the
  // equations of motion instead of the current velocity).
  velocity.y=(velocity.y+RR_TIMESTEP*forceNonDamper.y/GetMass())/
    (1.0+susp->GetBumpRate()/GetMass()*RR_TIMESTEP);
//qdbg("RWheel:Int: velocity=%f\n",velocity.y);
#endif

#ifdef DAMP_VERTICAL_VELOCITY_REVERSAL
  //velocity+=acceleration*RR_TIMESTEP;
  if((velocity.y>0&&oldVelocity.y<0)||
     (velocity.y<0&&oldVelocity.y>0))
  {
    // The wheel wants to change velocity in the vertical direction.
    // To avoid damper forces exploding the system, hold the wheel still
    // for this timestep. Blowups were seen with wheels with big damper
    // values.
    // The acceleration is also questionable in this case.
//qdbg("Wheel velocity reversal; holding wheel still\n");
    oldVelocity.y=velocity.y=acceleration.y=0;
  }
#endif

#ifdef DAMP_VERTICAL_EQUILIBRIUM_REVERSAL
  rfloat equilibriumY;
  // Calculate equilibrium position of the wheel's suspension.
  // This should ofcourse be precalculated (in the future).
  equilibriumY=susp->GetPosition()->y+offsetWC.y-susp->GetRestLength();
  // Rest of this method is done after calculating the translation.
#endif

  // Note that only the Y value is used for performance reasons.
  // Include 2nd order derivative (acceleration) (better than plain Euler)
  //translation=oldVelocity*RR_TIMESTEP+acceleration*RR_TIMESTEP*RR_TIMESTEP;
  translation.y=(oldVelocity.y+0.5f*acceleration.y*RR_TIMESTEP)*RR_TIMESTEP;

#ifdef DAMP_VERTICAL_EQUILIBRIUM_REVERSAL
  // 2nd part of this damping method
  if(translation.y>0)
  {
    if(position.y<equilibriumY&&position.y+translation.y>equilibriumY)
    {
      // Equilibrium reversal detected
     do_equi_damping:
//qdbg("Wheel equilibrium crossed; damping wheel motion\n");
      acceleration.y=0;
      // Calculate new (hopefully improved) translation
      translation.y=oldVelocity.y*RR_TIMESTEP;
    }
  } else
  {
    if(position.y>equilibriumY&&position.y+translation.y<equilibriumY)
      goto do_equi_damping;
  }
#endif

  // Update the position
  position.y+=translation.y;

  // The new velocity was already calculated above
  //velocity+=acceleration*RR_TIMESTEP;
#ifdef OBS
qdbg("Linear movements:\n");
 translation.DbgPrint("RWheel:translation");
 acceleration.DbgPrint("RWheel:acceleration");
 velocity.DbgPrint("RWheel:velocity");
#endif

#ifdef ND_ENERGY
  // Calculate energy
  rfloat x=susp->GetRestLength()-susp->GetLength();
  rfloat E=0.5*susp->GetK()*x*x+0.5*mass*velocity.y*velocity.y;
//qdbg("E_total=%.3f\n",E);
#endif

#ifdef OBS
qdbg("RWh:An; velZ=%f, accZ=%f, carAccZ=%f\n",
 velocity.z,acceleration.z,car->GetAcceleration()->z);
#endif
#ifdef OBS
qdbg("RWh:Int; pos=(%.2f,%.2f,%.2f)\n",position.x,position.y,position.z);
qdbg("  velocity=%f,%f,%f\n",velocity.x,velocity.y,velocity.z);
qdbg("  acc=%f,%f,%f\n",acceleration.x,acceleration.y,acceleration.z);
qdbg("  dt=%f\n",RR_TIMESTEP);
#endif

  // As 'position' is relative to the car's body, we can pass
  // the translation on to the suspension and see if the suspension
  // is ok with this movement (bump stops etc)
  if(!susp->ApplyWheelTranslation(&translation,&velocity))
  {
    // Can't go there physically; correct
#ifdef OBS
qdbg("RWh:Integrate; correct susp move by (%.2f,%.2f,%.2f)\n",
 translation.x,translation.y,translation.z);
qdbg("RWh:Integrate; position (%.2f,%.2f,%.2f) PRE\n",
 position.x,position.y,position.z);
#endif
    position.y+=translation.y;
    // Reset wheel velocity (impulse-like)
    //velocity.SetToZero();
    velocity.y=0;
#ifdef OBS
qdbg("RWh:Integrate; position (%.2f,%.2f,%.2f) POST\n",
 position.x,position.y,position.z);
#endif
  }
}

/*********************
* Coordinate systems *
*********************/
void RWheel::ConvertCarToTireOrientation(DVector3 *from,DVector3 *to)
// Convert vector 'from' from car coordinates
// to tire coordinates (into 'to')
// Assumes: from!=to
{
  rfloat angle,sinAngle,cosAngle;
  
  // Heading
  angle=-GetHeading();
  if(fabs(angle)<D3_EPSILON)
  { *to=*from;
    return;
  }
  sinAngle=sin(angle);
  cosAngle=cos(angle);
#ifdef OBS
qdbg("RWh:Car2Tire; angle=%f, sin=%f, cos=%f\n",angle,sinAngle,cosAngle);
from->DbgPrint("C2T from");
#endif
  // Rotate around Y axis to get heading of car right
  to->x=from->z*sinAngle+from->x*cosAngle;
  to->y=from->y;
  to->z=from->z*cosAngle-from->x*sinAngle;
//to->DbgPrint("C2T to");
}
void RWheel::ConvertCarToTireCoords(DVector3 *from,DVector3 *to)
{
  ConvertCarToTireOrientation(from,to);
  // Translate
  //...
  qerr("RWheel:ConvertCarToTireCoords NYI");
}
void RWheel::ConvertTireToCarOrientation(DVector3 *from,DVector3 *to)
// Convert vector 'from' from tire coordinates
// to car coordinates (into 'to')
// Assumes: from!=to
{
  rfloat angle,sinAngle,cosAngle;
  
  // Note that the tire is constrained in 5 dimensions, so only
  // 1 rotation needs to be done

  // Heading
  angle=GetHeading();
  sinAngle=sin(angle);
  cosAngle=cos(angle);
  
  // Rotate around Y axis to get heading of car right
  to->x=from->z*sinAngle+from->x*cosAngle;
  to->y=from->y;
  to->z=from->z*cosAngle-from->x*sinAngle;
}
void RWheel::ConvertTireToCarCoords(DVector3 *from,DVector3 *to)
{
  ConvertTireToCarOrientation(from,to);
  // Translate
  //...
  qerr("RWheel:ConvertTireToCarCoords NYI");
}

/*****
* FX *
*****/
void RWheel::ProcessFX()
// Check the state of the wheel and apply any effects
{
  // Skidmarks; note that skidding turn-off is filtered (must go well
  // below the point at which skidmarking is turned on) because
  // near the point of skidmarks some jittering occurs which can use
  // up many strips much too fast.
  if(fabs(slipRatio)>SKIDMARK_SLIPRATIO)
  {
    if(skidStrip==-1)
    {
      // Start a skid strip
      skidStrip=RMGR->smm->StartStrip();
      RMGR->replay->RecSkidStart(skidStrip);
//qdbg("Start strip %d\n",skidStrip);
    }
    // Add skid point
    DVector3 *v=(DVector3*)&surfaceInfo.x;
    skidStrip=RMGR->smm->AddPoint(skidStrip,v);
    // Store for the replay
    RMGR->replay->RecSkidPoint(skidStrip,v);
  } else if(fabs(slipRatio)<SKIDMARK_SLIPRATIO/5.0f)
  {
    if(skidStrip!=-1)
    {
      // End strip
      RMGR->smm->EndStrip(skidStrip);
      // Store for the replay
      RMGR->replay->RecSkidStop(skidStrip);
      skidStrip=-1;
    }
  }

  // Smoke, if wheel is turning (instability at low speed would
  // give smoke when just driving off)
  if(fabs(slipRatio)>SKIDMARK_SLIPRATIO&&rotationV.x>1)
  {
    // Add some smoke
    DVector3 pos;
    dfloat   lifeTime=2.0;

    pos.x=surfaceInfo.x;
    pos.y=surfaceInfo.y+0.1;    // Particle is 0.5m high
    pos.z=surfaceInfo.z;
    // Should give more life to the particle if it's heavy slipping
    RMGR->pgSmoke->Spawn(&pos,car->GetVelocity(),lifeTime);
    // Store for the replay
    RMGR->replay->RecSmoke(&pos,car->GetVelocity(),lifeTime);
  }
}

