/*
 * RCar - a car
 * 05-08-00: Created! (13:28:24)
 * 12-12-00: Wheel loading not by name anymore
 * 30-01-01: Wheel lock is now halved (20deg lock=-10..+10)
 * NOTES:
 * FUTURE:
 * - Optimize PreCalcDriveLine()'s CalcPreClutchInertia() to just once
 * after loading the car and creating the driveline.
 * BUGS:
 * - Warp should look at track info to really get the highest position
 * for the worst-case wheel (not based the height info on the grid line)
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/global.h>
#include <math.h>
#include <qlib/profile.h>
DEBUG_ENABLE

// Extra checking?
//#define SELF_CHECK

// Paint car's AABB? (used for collisions, visual check)
//#define OPT_PAINT_AABB

#define CAR_INI_NAME		"car.ini"
#define VIEW_INI_NAME           "views.ini"
#define AR_NAME                 "data.ar"

#undef  DBG_CLASS
#define DBG_CLASS "RCar"

// Import from rbody.cpp
void gluCube(float wid,float hgt,float dep,bool fWire);

/*********
* C/Dtor *
*********/
RCar::RCar(cstring carName)
  : QObject()
{
  DBG_C("ctor")
  QASSERT_V(carName)
  QPROF("RCar ctor")
  
  int i;
  
  Name("RCar");

  index=0;
  flags=0;
  
  // Init member variables
  cg.SetToZero();
  texShadow=0;
  shadowWid=shadowLen=0;
  shadowOffset.SetToZero();
  differentials=0;
  driveLine=0;
  archive=0;
  ffMz=new RFilter(RMGR->GetMainInfo()->GetFloat("ini.ff_damping"));
  timingPosBC.SetToZero();
  Reset();

  // Decide car directory
  char buf[256],*dir;
  dir=getenv("RACER");
  if(dir)sprintf(buf,"%s/data/cars/%s",dir,carName);
  else   sprintf(buf,"data/cars/%s",carName);
  carDir=buf;
  // Remember short name of car (for replays for example)
  carShortName=carName;

  // Defaults
  wheels=0;
  for(i=0;i<MAX_WHEEL;i++)
    wheel[i]=0;
  wings=0;
  for(i=0;i<MAX_CAMERA;i++)
  { camera[i]=new RCamera(this);
    camera[i]->SetIndex(i);
  }
  curCamera=0;
  robot=0;
  enclosingNode=0;

  // Audio
  rapEngine=rapSkid=0;
#ifdef linux
  //  fprintf(stderr, "RMGR->audio = %d\n", RMGR->audio); 
#endif 
  if(RMGR->audio)
  {
    rapEngine=new RAudioProducer();
    RMGR->audio->AddProducer(rapEngine);
    rapSkid=new RAudioProducer();
    RMGR->audio->AddProducer(rapSkid);

    rapGearShift=new RAudioProducer();
    RMGR->audio->AddProducer(rapGearShift);
    rapGearMiss=new RAudioProducer();
    RMGR->audio->AddProducer(rapGearMiss);
    rapStarter=new RAudioProducer();
    RMGR->audio->AddProducer(rapStarter);
  }
  
  // Parts
  // Note the dependencies:
  // - driveline before engine (or any other driveline component)
  // - engine before body
  driveLine=new RDriveLine(this);
  engine=new REngine(this);
  gearbox=new RGearBox(this);
  body=new RBody(this);
  steer=new RSteer(this);
  for(i=0;i<MAX_WHEEL;i++)
    susp[i]=new RSuspension(this);
  for(i=0;i<MAX_WING;i++)
    wing[i]=new RWing(this);

  // Create initial driveline

  // Open archive file for compressed file access
  sprintf(buf,"%s/%s",carDir.cstr(),AR_NAME);
  archive=new QArchive(buf);

  {QPROF("open info files")

  // Open info files
  info=new QInfo(RFindFile(CAR_INI_NAME,carDir));
  // Open default info file
//qdbg("carDir='%s'\n",carDir.cstr());
  sprintf(buf,"%s/../default/%s",carDir.cstr(),CAR_INI_NAME);
//qdbg("buf='%s'\n",buf);
  infoDefault=new QInfo(buf);
  info->SetFallback(infoDefault);
  }
  
  // Auto-load
  Load();

  // Read views for this car
  {
    QInfo info(RFindFile(VIEW_INI_NAME,carDir));
    views=new RViews(this);
    views->Load(&info);
  }

  // No need to hold on to the archive anymore
  QDELETE(archive);

}
RCar::~RCar()
{
  int i;

  QDELETE(body);
  QDELETE(steer);
  QDELETE(engine);
  QDELETE(gearbox);
  for(i=0;i<MAX_WHEEL;i++)
    QDELETE(susp[i]);
  for(i=0;i<MAX_WHEEL;i++)
    QDELETE(wheel[i]);
  for(i=0;i<differentials;i++)
    QDELETE(differential[i]);
  QDELETE(driveLine);
  for(i=0;i<MAX_CAMERA;i++)
    QDELETE(camera[i]);
  for(i=0;i<MAX_WING;i++)
    QDELETE(wing[i]);
  QDELETE(views);
  QDELETE(robot);
  QDELETE(archive);

qdbg("RCar dtor: rapEngine=%p, skid=%p\n",rapEngine,rapSkid);
  if(rapEngine)
  {
    RMGR->audio->RemoveProducer(rapEngine);
    QDELETE(rapEngine);
  }
  if(rapSkid)
  {
    RMGR->audio->RemoveProducer(rapSkid);
    QDELETE(rapSkid);
  }
  // Delete other samples
  //...
  QDELETE(texShadow);
qdbg("RCar dtor: infos\n");
  QDELETE(infoDefault);
  QDELETE(info);
qdbg("RCar dtor: RET\n");
}

void RCar::Reset()
// Reset state, but don't destroy object.
// Resets network variables and such, but does not
// delete differentials, models etc.
{
  int i;

  // Network
  nextNetworkUpdate=0;
  for(i=0;i<RNetwork::MAX_CLIENT;i++)
    lastClientUpdate[i]=0;
}

/*******
* Dump *
*******/
bool RCar::LoadState(QFile *f)
{
  int i;
  // Flags determine AI or not which may be switched dynamically, so load
  f->Read(&flags,sizeof(flags));
  // The camera should be in the RScene?
  f->Read(&curCamera,sizeof(curCamera));
  f->Read(&cg,sizeof(cg));
  for(i=0;i<wheels;i++)
    if(!wheel[i]->LoadState(f))return FALSE;
  for(i=0;i<wheels;i++)
    if(!susp[i]->LoadState(f))return FALSE;
  for(i=0;i<wings;i++)
    if(!wing[i]->LoadState(f))return FALSE;
  if(!steer->LoadState(f))return FALSE;
  if(!engine->LoadState(f))return FALSE;
  if(!gearbox->LoadState(f))return FALSE;
  if(!body->LoadState(f))return FALSE;
  for(i=0;i<differentials;i++)
    if(!differential[i]->LoadState(f))return FALSE;
  //if(!robot->LoadState(f))return FALSE;
  return TRUE;
}
bool RCar::SaveState(QFile *f)
{
  int i;

  f->Write(&flags,sizeof(flags));
  f->Write(&curCamera,sizeof(curCamera));
  f->Write(&cg,sizeof(cg));
  for(i=0;i<wheels;i++)
    if(!wheel[i]->SaveState(f))return FALSE;
  for(i=0;i<wheels;i++)
    if(!susp[i]->SaveState(f))return FALSE;
  for(i=0;i<wings;i++)
    if(!wing[i]->SaveState(f))return FALSE;
  if(!steer->SaveState(f))return FALSE;
  if(!engine->SaveState(f))return FALSE;
  if(!gearbox->SaveState(f))return FALSE;
  if(!body->SaveState(f))return FALSE;
  for(i=0;i<differentials;i++)
    if(!differential[i]->SaveState(f))return FALSE;
  //if(!robot->SaveState(f))return FALSE;
  return TRUE;
}

/**********
* Loading *
**********/
bool RCar::Load()
// Read all parts
{
  DBG_C("Load")
  QPROF("RCar:Load()")

  char  buf[100];
  int   i,n;
  
  // Preferences for car 3D models
  dglobal.prefs.Reset();
  dglobal.prefs.envMode=DTexture::MODULATE;
  
  // Read car properties
  info->GetString("car.name",carName);
  cg.x=info->GetFloat("car.cg.x");
  cg.y=info->GetFloat("car.cg.y",.5f);
  cg.z=info->GetFloat("car.cg.z");

  // Shadow
  info->GetString("car.shadow.texture",buf);
  if(buf[0])
  {
    DBitMapTexture *tbm;
    QImage img(RFindFile(buf,carDir));
    if(img.IsRead())
    {
      tbm=new DBitMapTexture(&img);
      texShadow=(DTexture*)tbm;
    }
  }
  shadowWid=info->GetFloat("car.shadow.width");
  shadowLen=info->GetFloat("car.shadow.length");
  shadowOffset.x=info->GetFloat("car.shadow.offset_x");
  shadowOffset.y=info->GetFloat("car.shadow.offset_y");
  shadowOffset.z=info->GetFloat("car.shadow.offset_z");

  // Read component props
  body->Load(info,"body");
  
  // Read cameras
  for(i=0;i<MAX_CAMERA;i++)
    camera[i]->Load(info);

  // Read suspensions from wheels
  wheels=info->GetInt("car.wheels",4);
  if(wheels>MAX_WHEEL)
  { qwarn("RCar: wheels=%d, but max=%d",wheels,MAX_WHEEL);
    wheels=MAX_WHEEL;
  }
  for(i=0;i<wheels;i++)
  {
    sprintf(buf,"susp%d",i);
    susp[i]->Load(info,buf);
  }

  // Read anti-rollbars (AFTER suspensions are done)
  n=info->GetInt("antirollbar.count");
  for(i=0;i<n;i++)
  {
    int sl,sr;
    rfloat k;
    sprintf(buf,"antirollbar.arb%d.susp_left",i);
    sl=info->GetInt(buf);
    sprintf(buf,"antirollbar.arb%d.susp_right",i);
    sr=info->GetInt(buf);
    sprintf(buf,"antirollbar.arb%d.k",i);
    k=info->GetFloat(buf);
    // Check arguments
    if(sl<0||sl>=wheels)
    { qwarn("Out of range left suspension (%d) in antirollbar %d; "
        "arb ignored",sl,i);
      continue;
    }
    if(sr<0||sr>=wheels)
    { qwarn("Out of range right suspension (%d) in antirollbar %d; "
        "arb ignored",sr,i);
      continue;
    }
    // Link the suspensions and set the rate
    susp[sl]->SetARBOtherSuspension(susp[sr]);
    susp[sl]->SetARBRate(k);
    susp[sr]->SetARBOtherSuspension(susp[sl]);
    susp[sr]->SetARBRate(k);
  }

  // Read wings
  wings=info->GetInt("aero.wings");
  if(wings>MAX_WING)
  { qwarn("RCar: too many wings defined (%d, max=%d)",wings,MAX_WING);
    wings=MAX_WING;
  }
  for(i=0;i<wings;i++)
  {
    sprintf(buf,"aero.wing%d",i);
    wing[i]->Load(info,buf);
  }
  
  // Read wheels
  for(i=0;i<wheels;i++)
  {
    sprintf(buf,"wheel%d",i);
//qdbg("RCar: Load wheel '%s'\n",buf);
    wheel[i]=new RWheel(this);
    // Attach its suspension (for convenience)
    wheel[i]->SetSuspension(susp[i]);
    wheel[i]->SetWheelIndex(i);
    wheel[i]->Load(info,buf);
  }

  // Read steering wheel
  steer->Load(info,"steer");
  
  // Read engine
  engine->Load(info,"engine");

  // Read gearbox
  gearbox->Load(info);

#ifdef OBS
  // Read differentials
  // For now, hardcode an diff for the rear wheels of a 4-wheel car
  differential[0]=new RDifferential(this);
  differential[0]->Load(info,"differential");
  differential[0]->wheel[0]=wheel[2];
  wheel[2]->SetDifferential(differential[0],0);
  differential[0]->wheel[1]=wheel[3];
  wheel[3]->SetDifferential(differential[0],1);
  differential[0]->engine=engine;
  differentials++;
#endif
 
  // Read driveline (assumes wheels and engine have been loaded)
  // (reads clutch/handbrakes/differentials)
  driveLine->Load(info);

  // Motor sounds
  if(RMGR->audio)
  {
    info->GetString("engine.sample",buf);
    rapEngine->LoadSample(RFindFile(buf,GetDir(),
      "data/audio"));
    rapEngine->GetSample()->Loop(TRUE);
    rapEngine->GetSample()->Play();
    rapEngine->SetSampleSpeed(info->GetInt("engine.sample_rpm"));
    rapEngine->SetCurrentSpeed(0);
    
    info->GetString("car.skid.sample",buf);
    rapSkid->LoadSample(RFindFile(buf,GetDir(),
      "data/audio"));
    rapSkid->GetSample()->Play();
    rapSkid->SetVolume(0);

    rapGearShift->LoadSample(RFindFile("shift.wav",GetDir(),
      "data/audio"));
    rapGearShift->GetSample()->Loop(FALSE);
    rapGearMiss->LoadSample(RFindFile("shift_miss.wav",GetDir(),
      "data/audio"));
    rapGearMiss->GetSample()->Loop(FALSE);
    rapStarter->LoadSample(RFindFile("starter.wav",GetDir(),
      "data/audio"));
    rapStarter->GetSample()->Loop(FALSE);
  }

  // Debugging
  ctlBrakes=RMGR->infoDebug->GetInt("controls.brakes");

  // Restore gfx settings
  //dglobal.prefs.Reset();

  // Precalculate collision info (the car's bounding box)
  DBox box;
  body->GetModel()->GetGeode()->GetBoundingBox(&box);
  // Get maximum Y extent, including the suspensions max extent
  for(i=0;i<wheels;i++)
  {
    if(susp[i]->GetPosition()->y-susp[i]->GetMaxLength()-wheel[i]->GetRadius()<
       box.min.y)
      box.min.y=susp[i]->GetPosition()->y-susp[i]->GetMaxLength()-
                wheel[i]->GetRadius();
  }
  // Go for worst-case rotation bounding box that the car can ever have
  aabbWid=box.GetRadius();
  aabbHgt=aabbDep=aabbWid;

  //
  // Build driveline tree; always start with engine and gearbox
  //
  driveLine->SetRoot(engine);
  engine->AddChild(gearbox);
  driveLine->SetGearBox(gearbox);

  // Add differentials and wheels to the driveline
  // Note this code does NOT work for 3 diffs, need more work for that.
  driveLine->SetDifferentials(differentials);
  if(differentials>0)
  {
    // Hook first diff to the gearbox
    gearbox->AddChild(differential[0]);
  }
  // Make the wheels children of the differentials
  // A diff with 2 diffs as children is not yet supported.
  for(i=0;i<differentials;i++)
  {
    differential[i]->AddChild(differential[i]->wheel[0]);
    differential[i]->AddChild(differential[i]->wheel[1]);
  }

  // Now that the driveline is complete, precalculate some things.
  // Note that the preClutchInertia needs to be calculated only once,
  // since it is the engine's inertia, which never changes.
  driveLine->CalcPreClutchInertia();
  // Note that setting the gear (called shortly) recalculates all
  // effectives inertiae and ratios.

  // Put it in the default gear (neutral)
  gearbox->SetGear(0);

  // Dbg checks
  body->DbgCheck("RCar:Load()");

  return TRUE;
}

/**********
* Attribs *
**********/
rfloat RCar::GetMass()
// Returns total mass of all components
{
  int i;
  rfloat w=0;
  w=body->GetMass();
  w+=engine->GetMass();
  for(i=0;i<wheels;i++)
  { w+=wheel[i]->GetMass();
  }
  return w;
}
rfloat RCar::GetVelocityTach()
// Get velocity as to be displayed on the tachiometer (sp?)
// Note that the value is taken from the gearbox, NOT the real world
// body speed.
{
  rfloat gearboxVel,radius;
#ifdef SELF_CHECK
  // Test the gearbox velocity vs. the real car world linear velocity
  rfloat velWorld;
  velWorld=body->GetLinVel()->Length();
#endif
  // Derive velocity from the gearbox
  gearboxVel=gearbox->GetRotationVel();
  // Get rear wheel radius (optimize this)
  if(wheels>=3)radius=wheel[2]->GetRadius();
  else         radius=wheel[0]->GetRadius();
  //gearboxVel=(gearboxVel/gearbox->GetCumulativeRatio())*2*PI*radius;
  gearboxVel=(gearboxVel/gearbox->GetCumulativeRatio())*radius;
#ifdef SELF_CHECK
qdbg("Tach: world vel=%.3f, gearboxVel=%.3f\n",velWorld,gearboxVel);
qdbg("  radius=%f, cum. ratio=%f, gearbox rotV=%f\n",
radius,gearbox->GetCumulativeRatio(),gearbox->GetRotationVel());
#endif
  return gearboxVel;
  //return body->GetLinVel()->Length();
}
void RCar::AddDifferential(RDifferential *diff)
{
  if(differentials==MAX_DIFFERENTIAL)
  {
    qwarn("RCar::AddDifferential(); maximum (%d) exceeded",MAX_DIFFERENTIAL);
    return;
  }
  differential[differentials]=diff;
  differentials++;
}

void RCar::PreCalcDriveLine()
// Precalculate some data on the driveline.
// Call this function again when the gearing changes, since this
// affects the effective inertia and ratios.
// Assumes the driveline is completed (all components in the tree)
{
  driveLine->CalcCumulativeRatios();
  driveLine->CalcEffectiveInertiae();
  driveLine->CalcPostClutchInertia();

  driveLine->DbgPrint("RCar ctor");
}

/**********
* Animate *
**********/
void RCar::PreAnimate()
// Determine state in which all car parts are situated for later
// calculations.
{
  int i;
  
  // Decide on where we are a bit on the track (purely for CD optimization)
  DAABB aabb;
  DVector3 *p=body->GetPosition();
  // Create a rough AABB for our car, and find the new best enclosing
  // node for it.
  aabb.min.x=p->x-aabbWid/2;
  aabb.max.x=p->x+aabbWid/2;
  aabb.min.y=p->y-aabbHgt/2;
  aabb.max.y=p->y+aabbHgt/2;
  aabb.min.z=p->z-aabbDep/2;
  aabb.max.z=p->z+aabbDep/2;
  if(!enclosingNode)
    enclosingNode=RMGR->GetTrackVRML()->GetTrackTree()->GetRoot();
  enclosingNode=enclosingNode->FindEnclosingNode(&aabb);

  // Pre-work for next physics pass
  //engine->PreAnimate();
  //gearbox->PreAnimate();
  body->PreAnimate();
  for(i=0;i<wheels;i++)
  {
    wheel[i]->PreAnimate();
    susp[i]->PreAnimate();
  }
}
void RCar::Animate()
// Animate all parts
// Connects the parts of the car
{
  DBG_C("Animate")

  int i,t;
  //float force,torque;
  //float accMag;

  // Don't do physics of a network car
  if(IsNetworkCar())return;

//qdbg("---- frame %d\n",rStats.frame);
//qdbg("----\n");

  // Pass on controller data
  
  // Send steering angle through to steering wheels
  ApplySteeringToWheels();
 
  // Animate car as a whole
  PreAnimate();
  
  //
  // Calculate state of vehicle before calculating forces
  //
  // Calculate all forces on the car
  //
  engine->CalcForces();
  for(i=0;i<wings;i++)
    wing[i]->CalcForces();

  for(i=0;i<wheels;i++)
    susp[i]->CalcForces();

  for(i=0;i<wheels;i++)
    wheel[i]->CalcForces();

  // Do the driveline, after engine torque and wheel reaction forces are known
  driveLine->CalcForces();

#ifdef ND_DRIVELINE
  // Now that engine and wheel forces are unknown, check the diffs
  for(i=0;i<differentials;i++)
    differential[i]->CalcForces();
#endif

  // Calculate the resulting body forces
  body->CalcForces();

  // Calculate the resulting accelerations (some may have been calculated
  // already in CalcForces() though).
  driveLine->CalcAccelerations();

  //
  // Now that all forces have been generated, apply
  // them to get accelerations (translational/rotational)
  //
  for(i=0;i<wheels;i++)
    wheel[i]->CalcAccelerations();
  for(i=0;i<wings;i++)
    wing[i]->CalcAccelerations();
  body->CalcAccelerations();
  
  // Create body torques out of the wheel forces
  body->ApplyRotations();

  // After all forces & accelerations are done, deduce
  // data for statistics
  
#ifdef OBS_RPM_IS_NEW
  // Engine RPM is related to the rotation of the powered wheels,
  // since they are physically connected, somewhat
  // Take the minimal rotation
  float minV=99999.0f,maxV=0;
  for(i=0;i<wheels;i++)
    if(wheel[i]->IsPowered())
    { 
      if(wheel[i]->GetRotationV()>maxV)
        maxV=wheel[i]->GetRotationV();
#ifdef OBS_HMM
      if(wheel[i]->GetRotationV()<minV)
        minV=wheel[i]->GetRotationV();
#endif
//qdbg("  rpm: wheel%d: rotV=%f\n",i,wheel[i]->GetRotationV());
    }
  engine->ApplyWheelRotation(maxV);
#endif
  
  // Remember old location of car to check for hitting things
  // (see PostIntegrate())
  //DVector3 timingPosBC,timingPosPreWC,timingPosPostWC;
  //timingPosBC.SetToZero();
  body->ConvertBodyToWorldPos(&timingPosBC,&timingPosPreWC);
  
  // Detect collision with other cars/track before integrating
  // Should be done for all cars BEFORE integrating ANY of the
  // cars (!)
  DetectCollisions();
  
  // Integrate step for all parts
  body->Integrate();
  //engine->Integrate();
  steer->Integrate();
  for(i=0;i<wheels;i++)
    wheel[i]->Integrate();
  for(i=0;i<wheels;i++)
    susp[i]->Integrate();
  driveLine->Integrate();
#ifdef ND_FUTURE
  for(i=0;i<differentials;i++)
  {
    differential[i]->Integrate();
  }
#endif
  
  // Check for network updates
  t=RMGR->time->GetSimTime();
  if(t>nextNetworkUpdate)
  {
//qdbg("Time for a network update\n");
    // Send car state
    RMGR->network->GetGlobalMessage()->OutCarState(this,0);
    // Find next time 
    nextNetworkUpdate=t+RMGR->timePerNetworkUpdate;
  }
}
void RCar::PostIntegrate()
// Called after an integration step
{
  // Postprocess the rigid body (as ODE had some effect on variables,
  // and they are synced until a full conversion is done)
  body->PostIntegrate();

  // Check if something was hit/crossed
  body->ConvertBodyToWorldPos(&timingPosBC,&timingPosPostWC);
#ifdef OBS
timingPosPreWC.DbgPrint("timingpos pre");
timingPosPostWC.DbgPrint("timingpos post");
#endif
  int ctl=RMGR->scene->curTimeLine[index];
  if(RMGR->trackVRML->timeLine[ctl]->CrossesPlane(
    &timingPosPreWC,&timingPosPostWC))
  {
    // Notify scene of our achievement
    RMGR->scene->TimeLineCrossed(this);
  }
}
void RCar::SetInput(int ctlSteer,int _ctlThrottle,int _ctlBrakes)
// Process the inputs that come from the controller
// and pass them on to the objects in the car that need it
{
  DBG_C("SetInput")

  // Controller input
  ctlThrottle=_ctlThrottle;
  ctlBrakes=_ctlBrakes;
  steer->SetInput(ctlSteer);
  engine->SetInput(ctlThrottle);
}
void RCar::OnGfxFrame()
// Update the car in areas where physics integration is not a concern
// This is a place where you put things that need to be done only once
// every graphics frame, like polling controllers, settings sample
// frequencies etc.
{
  int i;
  rfloat skidFactor=0;
 
  // Leave network cars for now; may do audio still
  if(IsNetworkCar())return;

  for(i=0;i<wheels;i++)
    wheel[i]->OnGfxFrame();

  // Audio of engine
  rapEngine->SetCurrentSpeed(engine->GetRPM());
  if(engine->IsStalled())
    rapEngine->SetVolume(0);
  else
  {
    // Hook throttle to volume a bit
    int v;
    v=ctlThrottle/10+155;
    rapEngine->SetVolume(v);
  }

  // Skid sounds (collect all wheels)
  if(wheels)
  {
    for(i=0;i<wheels;i++)
      skidFactor+=wheel[i]->GetSkidFactor();
//qdbg("skidFactor=%f\n",skidFactor);
    // One wheel is enough to get full skid volume
    // Note that the SoundBlaster16 I have on Win32 has a VERY
    // non-linear volume slider.
    //rapSkid->SetVolume((skidFactor/((rfloat)wheels))*256.0f);
    rapSkid->SetVolume(skidFactor*256.0f);
  }

  // 3D speed
  float *pos=&body->GetPosition()->x,*vel=&body->GetVelocity()->x;
  rapEngine->GetSample()->Set3DAttributes(pos,vel);
  rapSkid->GetSample()->Set3DAttributes(pos,vel);

  engine->OnGfxFrame();
  gearbox->OnGfxFrame();
}

/***********
* Painting *
***********/
void RCar::PaintShadow()
// Paint the car's shadow
// Only 4-wheel vehicle are supported at this point.
// The order of the wheels is also important in that case:
//   wheel0=right-front (X+/Z+)
//   wheel1=left-front  (X-/Z+)
//   wheel2=right-rear  (X+/Z-)
//   wheel3=left-rear   (X-/Z-)
{
  DVector3 vCC,vWC;
  DVector3 sizeStorage,*size=&sizeStorage;
  //RSurfaceInfo *si;
  //int i;
  
  // This algo needs 4 points for a single quad
  if(wheels!=4)return;

  //size=body->GetSize();
  size->x=shadowWid;
  size->z=shadowLen;
//size->DbgPrint("size");

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  //glPolygonOffset(-.01,-.1);
  if(texShadow)
  {
    // Use shadow texturemap
    glEnable(GL_TEXTURE_2D);
    // The color is modulated, so you can decide how thick the shadow is.
    // Idea is to thin out the shadow when the car is not on the ground.
    glColor4f(0,0,0,.7f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
//glDisable(GL_BLEND);
    texShadow->Select();
  } else
  {
    // Paint a gray color to get something (not even THAT bad)
    glDisable(GL_TEXTURE_2D);
    glColor4f(0,0,0,.3f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  }
  //glBegin(GL_TRIANGLES);
  // Make sure shadow is visible just above the surface polygons,
  // but doesn't write any Z-values
  glEnable(GL_DEPTH_TEST);
  glPolygonOffset(-50,1);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glDepthMask(GL_FALSE);
 
  glPushMatrix();
  //glTranslatef(shadowOffset.x,shadowOffset.y,shadowOffset.z);

  glBegin(GL_QUADS);
    vCC.x=size->x/2;
    vCC.y=0;
    vCC.z=size->z/2;
    vCC.Add(&shadowOffset);
    ConvertCarToWorldCoords(&vCC,&vWC);
    // Project to ground
    //vWC.x=wheel[0]->GetSurfaceInfo()->x;
    vWC.y=wheel[0]->GetSurfaceInfo()->y;
    //vWC.z=wheel[0]->GetSurfaceInfo()->z;
//DVector3 v0=vWC;
//vWC.DbgPrint("wheel[0]");
    glTexCoord2f(1,0);
    glVertex3f(vWC.x,vWC.y,vWC.z);
    
    vCC.x=-size->x/2;
    vCC.y=0;
    vCC.z=size->z/2;
    vCC.Add(&shadowOffset);
    ConvertCarToWorldCoords(&vCC,&vWC);
    // Project to ground
    vWC.y=wheel[1]->GetSurfaceInfo()->y;
    glTexCoord2f(0,0);
    glVertex3f(vWC.x,vWC.y,vWC.z);
//vWC.Subtract(&v0);vWC.DbgPrint("wheel[1]");
    
    vCC.x=-size->x/2;
    vCC.y=0;
    vCC.z=-size->z/2;
    vCC.Add(&shadowOffset);
    ConvertCarToWorldCoords(&vCC,&vWC);
    // Project to ground
    vWC.y=wheel[3]->GetSurfaceInfo()->y;
    glTexCoord2f(0,1);
    glVertex3f(vWC.x,vWC.y,vWC.z);
//vWC.Subtract(&v0);vWC.DbgPrint("wheel[3]");

    vCC.x=size->x/2;
    vCC.y=0;
    vCC.z=-size->z/2;
    vCC.Add(&shadowOffset);
    ConvertCarToWorldCoords(&vCC,&vWC);
    // Project to ground
    vWC.y=wheel[2]->GetSurfaceInfo()->y;
    glTexCoord2f(1,1);
    glVertex3f(vWC.x,vWC.y,vWC.z);
//vWC.Subtract(&v0);vWC.DbgPrint("wheel[2]");
    
  glEnd();
  glPopMatrix();

  // Restore state
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  
  glPolygonOffset(0,0);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDepthMask(GL_TRUE);
}
void RCar::Paint()
// Paint the entire car
{
  DBG_C("Paint")
  int i;

  // Body hierarchy
#ifdef RR_GFX_OGL
  glPushMatrix();
#endif
//qdbg("RCar:Paint(); body\n");
  //PaintShadow();
  // Setup orientation for car hierarchical drawing
  body->PaintSetup();

  // Draw mostly solid objects first
  steer->Paint();
  //engine->Paint();
  for(i=0;i<wheels;i++)
    susp[i]->Paint();
  for(i=0;i<wheels;i++)
    wheel[i]->Paint();

  // Paint body last to get least trouble with transparent body materials
  body->Paint();

#ifdef RR_GFX_OGL
  glPopMatrix();
#ifdef OPT_PAINT_AABB
  // Paint the car's AABB (used for collisions) to check OBB->AABB functions
  // Note this must be done in world coordinates!
  glPushMatrix();
  DVector3 center;
  DAABB *aabb=body->GetAABB();
  center.x=(body->GetAABB()->min.x+body->GetAABB()->max.x)/2;
  center.y=(body->GetAABB()->min.y+body->GetAABB()->max.y)/2;
  center.z=(body->GetAABB()->min.z+body->GetAABB()->max.z)/2;
  glTranslatef(center.x,center.y,center.z);
  // The gluCube actually draws from 0..dep, NOT -dep/2..dep/2 (!)
  glTranslatef(0,0,-aabb->GetDepth()/2);
  gluCube(aabb->GetWidth(),aabb->GetHeight(),aabb->GetDepth(),TRUE);
  glColor3f(0,1,0);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glPointSize(5);
  glBegin(GL_POINTS);
    //glVertex3f(center.x,center.y,center.z);
    glVertex3f(0,0,0);
    glVertex3f(aabb->GetWidth(),aabb->GetHeight(),aabb->GetDepth());
    glVertex3f(-aabb->GetWidth(),aabb->GetHeight(),aabb->GetDepth());
    glVertex3f(aabb->GetWidth(),-aabb->GetHeight(),aabb->GetDepth());
    glVertex3f(-aabb->GetWidth(),-aabb->GetHeight(),aabb->GetDepth());
    glVertex3f(aabb->GetWidth(),aabb->GetHeight(),-aabb->GetDepth());
    glVertex3f(-aabb->GetWidth(),aabb->GetHeight(),-aabb->GetDepth());
    glVertex3f(aabb->GetWidth(),-aabb->GetHeight(),-aabb->GetDepth());
    glVertex3f(-aabb->GetWidth(),-aabb->GetHeight(),-aabb->GetDepth());
/*
    glVertex3f(aabb->min.x,aabb->min.y,aabb->min.z);
    glVertex3f(aabb->max.x,aabb->min.y,aabb->min.z);
    glVertex3f(aabb->min.x,aabb->max.y,aabb->min.z);
    glVertex3f(aabb->min.x,aabb->min.y,aabb->max.z);
    glVertex3f(aabb->max.x,aabb->max.y,aabb->min.z);
    glVertex3f(aabb->max.x,aabb->max.y,aabb->max.z);
    glVertex3f(aabb->min.x,aabb->max.y,aabb->max.z);
*/
  glEnd();
//GetPosition()->DbgPrint("car pos");
  glPopMatrix();
#endif
#endif
//qdbg("RCar:Paint(); RET\n");
}  

/**********
* Physics *
**********/
void RCar::ApplyRotations()
{
  body->ApplyRotations();
}

void RCar::ApplySteeringToWheels()
{
  int i;

  // Send steering angle through to steering wheels
  for(i=0;i<wheels;i++)
    if(wheel[i]->IsSteering())
    {
      float factor;
      
      factor=steer->GetLock()/wheel[i]->GetLock()*2.0f;
#ifdef RR_ODE
      // Weird
      //wheel[i]->SetHeading(-steer->GetAngle()/factor /*+GetHeading()*/);
      wheel[i]->SetHeading(steer->GetAngle()/factor /*+GetHeading()*/);
#else
      wheel[i]->SetHeading(steer->GetAngle()/factor /*+GetHeading()*/);
#endif
//qdbg("wheel%d: heading %f deg\n",
//i,(steer->GetAngle()/factor)*RR_RAD_DEG_FACTOR);
    }
}

/******************
* Warping the car *
******************/
void RCar::Warp(RCarPos *pos)
// Move the car to the specified location/orientation
// Used with Shift-R for example
{
  DQuaternion *q;
  DVector3 axis,vx,vy,vz;
  dfloat   offsetY;
  DVector3 *vBodyPos;
  DMatrix3  m;
  int i;

  q=body->GetRotPosQ();
  // Calculate axis along which car is oriented
  axis=pos->from;
  axis.Subtract(&pos->to);
//#ifdef OBS
pos->from.DbgPrint("Warp from");
pos->to.DbgPrint("Warp from");
axis.DbgPrint("Warp axis");
//#endif

#ifdef OBS_DOES_NOT_WORK
  // Calculate angle of rotation
  angle=0;
  // Store as a quaternion
  q->Rotation(&axis,angle);
#endif

  // Calculate Z direction of car
  vz=axis;
  // Calc X direction of car as cross product of 'vz' and
  // a line going straight up. This DOES mean the warp orientation
  // of the car should never be straight up, but this is highly
  // unusual.
  DVector3 vUp(0,1.0f,0);
  vx.Cross(&vUp,&vz);
  // Calc Y as the last vector perpendicular to both vz and vx
  vy.Cross(&vz,&vx);
  // Normalize all directions
  vx.Normalize();
  vy.Normalize();
  vz.Normalize();
  // Create the rotation matrix from the vectors
  m.SetRC(0,0,vx.x);
  m.SetRC(1,0,vx.y);
  m.SetRC(2,0,vx.z);
  m.SetRC(0,1,vy.x);
  m.SetRC(1,1,vy.y);
  m.SetRC(2,1,vy.z);
  m.SetRC(0,2,vz.x);
  m.SetRC(1,2,vz.y);
  m.SetRC(2,2,vz.z);
  // Store as a quaternion
  q->FromMatrix(&m);
  // Make sure the rigid body's rotation matrix is equivalent to 'q'
m.DbgPrint("Source matrix for quat");
  body->GetRotPosM()->FromQuaternion(q);
body->GetRotPosM()->DbgPrint("Result matrix from quat");

#ifdef RR_ODE
  // Pump into ODE body; notice ODE's quat matches D3's quat (17-11-01).
  // For this, I switched the quat's 'w' location to the front.
  // and D3 has 'w' as the last member. So we could just cast and pass &q.
  dBodySetQuaternion(body->GetODEBody(),(dReal*)q);
#ifdef OBS
  dQuaternion oq;
  oq[0]=q->w;
  oq[1]=q->x;
  oq[2]=q->y;
  oq[3]=q->z;
  dBodySetQuaternion(body->GetODEBody(),oq);
#endif
#ifdef ND_TEST
{DMatrix3 m3;
 dfloat *p=dBodyGetRotation(body->GetODEBody());
 m3.GetM()[0]=p[0];
 m3.GetM()[1]=p[1];
 m3.GetM()[2]=p[2];
 m3.GetM()[3]=p[4];
 m3.GetM()[4]=p[5];
 m3.GetM()[5]=p[6];
 m3.GetM()[6]=p[8];
 m3.GetM()[7]=p[9];
 m3.GetM()[8]=p[10];
 m3.DbgPrint("ODE derived matrix (4x3 -> 3x3)");
}
#endif
#endif

  // Initially move car center to the 'from' location
  vBodyPos=body->GetLinPos();
  *vBodyPos=pos->from;
  // Get front nose position of car by shifting along the 'vz' axis
  // (which is the car's nose direction in world coordinates)
  vz.Scale(body->GetSize()->z/2);
  vBodyPos->Subtract(&vz);
  // Make sure the car isn't IN the tarmac! (give it some height)
  offsetY=-10000;
  for(i=0;i<wheels;i++)
  {
    dfloat tirePatchOffsetY;
    // Calculate distance so the contact patch will just be on the track
    tirePatchOffsetY=susp[i]->GetLength()-
      susp[i]->GetPosition()->y+wheel[i]->GetRadius();
qdbg("Wheel %d: susplen=%f, suspPosY=%f, radius=%f\n",
i,susp[i]->GetLength(),susp[i]->GetPosition()->y,wheel[i]->GetRadius());
    if(tirePatchOffsetY>offsetY)
      offsetY=tirePatchOffsetY;
  }
vBodyPos->DbgPrint("vBodyPos start");
  vBodyPos->y+=offsetY;
vBodyPos->DbgPrint("vBodyPos with offsetY");
  // Add an extra space to avoid directly hitting the surface
  // (which results in funny 1st time lateral jumps)
  vBodyPos->y+=RMGR->GetMainInfo()->GetFloat("car.warp_offset_y");
vBodyPos->DbgPrint("vBodyPos plus warp offsetY");
//qdbg("Warp offsetY=%f\n",offsetY);

#ifdef RR_ODE
  // Pump into ODE body
  dBodySetPosition(body->GetODEBody(),vBodyPos->x,vBodyPos->y,vBodyPos->z);
#endif

  // Stop moving!
  Reset();
  body->Reset();
  for(i=0;i<wheels;i++)
  {
    susp[i]->Reset();
    // Reset wheel AFTER suspension
    wheel[i]->Reset();
  }
  engine->Reset();
  driveLine->Reset();
}
void RCar::Warp(int where,int index)
// Warp to a grid or pit pos
// Checks index so it's safer than the Warp(RCarPos *pos)
{
  if(where==WARP_GRID)
  {
    if(index<RMGR->track->GetGridPosCount())
    {
      Warp(RMGR->track->GetGridPos(index));
    } else
    {
      qwarn("Missing grid pos #%d",index);
    }
  } else if(where==WARP_PIT)
  {
    if(index<RMGR->track->GetPitPosCount())
    {
      Warp(RMGR->track->GetPitPos(index));
    } else
    {
      qwarn("Missing pit pos #%d",index);
    }
  }
}


/*********************
* Coordinate systems *
*********************/
void RCar::ConvertCarToWorldOrientation(DVector3 *from0,DVector3 *toFinal)
// Convert orientation 'from' from car coordinates (local)
// to world coordinates (into 'to')
// Order is reverse to YPR; RPY here (ZXY)
{
  // Pass on to body
  body->ConvertBodyToWorldOrientation(from0,toFinal);
}
void RCar::ConvertCarToWorldCoords(DVector3 *from,DVector3 *to)
// Convert vector 'from' from car coordinates (local)
// to world coordinates (into 'to')
// Should be used for coordinates, NOT orientations (a translation
// is used here as well as a rotation)
{
  // Pass on to body
  body->ConvertBodyToWorldPos(from,to);
}

void RCar::ConvertWorldToCarOrientation(DVector3 *from0,DVector3 *toFinal)
// Convert vector 'from' from world coordinates (global)
// to car coordinates (into 'to')
{
  // Pass on to body
  body->ConvertWorldToBodyOrientation(from0,toFinal);
}
void RCar::ConvertWorldToCarCoords(DVector3 *from,DVector3 *to)
// Convert vector 'from' from car coordinates (local)
// to world coordinates (into 'to')
// Should be used for coordinates, NOT orientations (a translation
// is used here as well as a rotation)
{
  // Pass on to body
  body->ConvertWorldToBodyPos(from,to);
}

/*******************
* Helper functions *
*******************/
void RCar::SetDefaultMaterial()
// When using models, materials are applied from the model files
// To use the stub graphics, these material changes must be reset
// to their default values
{
  float ambient[]={ 0.2f,0.2f,0.2f,1.0f };
  float diffuse[]={ 0.8f,0.8f,0.8f,1.0f };
  float specular[]={ 0,0,0,1.0 };
  float emission[]={ 0,0,0,1.0 };
  float shininess=0;

  glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
  glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
  glMaterialfv(GL_FRONT,GL_EMISSION,emission);
  glMaterialf(GL_FRONT,GL_SHININESS,shininess);
}

/**********************
* Collision detection *
**********************/
void RCar::DetectCollisions()
{
  body->DetectCollisions();
}

/*****
* AI *
*****/
void RCar::EnableAI()
{
  flags|=AI;
  if(!robot)
  {
    // Create AI driver
    robot=new RRobot(this);
  }
}
void RCar::DisableAI()
{
  flags&=~AI;
  // Don't delete any robot driver, since we might turn the AI on
  // later (for example, when the robot is pitting)
}

/**********
* Network *
**********/
void RCar::SetNetworkCar(bool yn)
// If 'yn'==TRUE, this is a network car.
// Not many physics calculations will be done on networked car.
{
  if(yn)flags|=NETWORK;
  else  flags&=~NETWORK;
}

/*****
* FX *
*****/
void RCar::ProcessFX()
{
  int i;
  for(i=0;i<wheels;i++)
    wheel[i]->ProcessFX();
}

/**********
* Replays *
**********/
void RCar::ProcessReplay()
{
  // Store the car details
  RMGR->replay->RecSetCar(index);
  //RMGR->replay->RecCarPosition(this);
  //RMGR->replay->RecCarOrientation(this);
  RMGR->replay->RecCar(this);
}

