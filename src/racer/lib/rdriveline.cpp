/*
 * RDriveLine - driveline components
 * 30-12-01: Created!
 * NOTES:
 * - All drivelines look alot alike; a single diff looks like:
 *   E -> clutch -> Gearbox -> Diff -> Left/Right
 * - The root of every driveline is therefore the engine.
 * - The child of the root of every driveline is the gearbox.
 * - The clutch is stored explicity in the driveline, and is not a component.
 * - The handbrakes are also stored here.
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/driveline.h>
DEBUG_ENABLE

// Local module trace?
//#define LTRACE

// Clutch must be smoothed out a lot when applying starts
#define DEFAULT_CLUTCH_LINEARITY 0.3f

// Threshold at which force an unlock of engine and gearbox.
// This can happen if the driver just throws in a gear,
// without clutching (and if autoclutch is off).
// We should penalize this with a gear scratch sound and
// damage.
#define DELTA_VEL_THRESHOLD 1.0f

/**********************
* Driveline component *
**********************/
RDriveLineComp::RDriveLineComp()
{
  // Init member variables
  parent=0;
  child[0]=child[1]=0;
  driveLine=0;
  name="<component>";

  inertia=0.01f;
  ratio=invRatio=1;
  effectiveInertiaDownStream=0;
  cumulativeRatio=0;
  tReaction=tBraking=tEngine=0;
  rotV=rotA=0;
}
RDriveLineComp::~RDriveLineComp()
{
}

//
// Attribs
//
void RDriveLineComp::SetParent(RDriveLineComp *_parent)
// Set a parent for this component. Mostly, this is done implicitly
// using AddChild().
{
  parent=_parent;
}
void RDriveLineComp::SetRatio(rfloat r)
{
  if(fabs(r)<D3_EPSILON)
  { qwarn("RDriveLineComp:SetRatio(%.2f); must be >0; adjusted to 1",r);
    r=1;
  }
  ratio=r;
  // Precalculate inverse ratio for faster calculations later
  invRatio=1.0f/r;
}
void RDriveLineComp::Reset()
// Reset variables for a fresh start (like when Shift-R is used)
{
  rotV=rotA=0;
}

RDriveLineComp *RDriveLineComp::GetChild(int n)
{
  switch(n)
  { case 0: return child[0];
    case 1: return child[1];
    default: qwarn("RDriveLineComp:GetChild(%d) out of range",n); return 0;
  }
}

void RDriveLineComp::AddChild(RDriveLineComp *comp)
// Add 'comp' as a child
{
  if(!child[0])child[0]=comp;
  else if(!child[1])child[1]=comp;
  else qwarn("RDriveLineComp:AddChild() failed; already has 2 children");

  // Declare us as parent of the child
  comp->SetParent(this);
}

/******
* I/O *
******/
bool RDriveLineComp::LoadState(QFile *f)
{
  f->Read(&rotV,sizeof(rotV));
  f->Read(&rotA,sizeof(rotA));
  return TRUE;
}
bool RDriveLineComp::SaveState(QFile *f)
{
  f->Write(&rotV,sizeof(rotV));
  f->Write(&rotA,sizeof(rotA));
  return TRUE;
}

/*****************
* Precalculation *
*****************/
void RDriveLineComp::CalcEffectiveInertia()
// Calculate the total effective inertia for this node plus the children
{
qdbg("RDLC:CalcEffectiveInertia() '%s'\n",name.cstr());
  effectiveInertiaDownStream=inertia*cumulativeRatio*cumulativeRatio;
  if(child[0])
  {
    child[0]->CalcEffectiveInertia();
    effectiveInertiaDownStream+=child[0]->GetEffectiveInertia();
  }
  if(child[1])
  {
    child[1]->CalcEffectiveInertia();
    effectiveInertiaDownStream+=child[1]->GetEffectiveInertia();
  }
}
void RDriveLineComp::CalcCumulativeRatio()
// Calculate the cumulative ratio for this component and entire tree
{
  cumulativeRatio=ratio;
  if(child[0])
  {
    child[0]->CalcCumulativeRatio();
  }
  if(child[1])
  {
    child[1]->CalcCumulativeRatio();
  }
  // Take ratio if child[0] only, since only singular links are supported
  // (like engine->gear->driveshaft)
  if(child[0])
    cumulativeRatio*=child[0]->GetCumulativeRatio();
    //cumulativeRatio*=child[0]->GetRatio();
}
void RDriveLineComp::CalcReactionForces()
// Calculate reaction forces from the leaves up
{
  tReaction=tBraking=0;
  if(child[0])
  {
    child[0]->CalcReactionForces();
    tReaction+=child[0]->GetReactionTorque();
    tBraking+=child[0]->GetBrakingTorque();
  }
  if(child[1])
  {
    child[1]->CalcReactionForces();
    tReaction+=child[1]->GetReactionTorque();
    tBraking+=child[1]->GetBrakingTorque();
  }
  // Multiply by the inverse ratio for upstream
  tReaction*=invRatio;
  tBraking*=invRatio;
}

void RDriveLineComp::CalcForces()
// Stub base class function
{
}
void RDriveLineComp::CalcAccelerations()
// Stub base class function
{
}
void RDriveLineComp::Integrate()
// Stub base class function
{
//qdbg("RDLC:Integrate() base class\n");
  rotV+=rotA*RR_TIMESTEP;
//qdbg("  rotV=%.2f (rotA=%.2f)\n",rotV,rotA);
}

/************
* Debugging *
************/
void RDriveLineComp::DbgPrint(int depth,cstring s)
{
  // Tab
  if(depth>0)
    qdbg("%*s",depth*2," ");
  qdbg("comp. '%s', ratio %.2f (cum. %.2f, 1/%.2f), inertia %.2f"
    " (effective %.2f)\n",name.cstr(),ratio,cumulativeRatio,invRatio,
    inertia,effectiveInertiaDownStream);

  if(child[0])child[0]->DbgPrint(depth+1,s);
  if(child[1])child[1]->DbgPrint(depth+1,s);
}

/************
* Driveline *
************/
RDriveLine::RDriveLine(RCar *_car)
{
  // Init members
  car=_car;
  root=0;
  preClutchInertia=postClutchInertia=totalInertia=0;
  prepostLocked=TRUE;
  diffs=0;

  // Clutch defaults
  clutchMaxTorque=100;
  clutchLinearity=DEFAULT_CLUTCH_LINEARITY;
  SetClutchApplication(1);
  tClutch=0;
  // Handbrake defaults
  handbrakeApplication=0;
}
RDriveLine::~RDriveLine()
{
}

void RDriveLine::SetRoot(RDriveLineComp *comp)
// Set the root component
{
  root=comp;
}

/**********
* Loading *
**********/
bool RDriveLine::Load(QInfo *info)
// Read driveline specs
// Assumes wheels have been loaded, and engine has been loaded too.
{
  char buf[256],*s;
  RWheel *w;
  int i,n;
  RDifferential *d;

  // Differentials
  n=info->GetInt("differentials.count");
  if(!n)
  {
    // Pre v0.5 Racer versions used a single differential definition.
    // Still support that for backward compatibility for older (mostly
    // converted) cars.
    d=new RDifferential(car);
    d->Load(info,"differential");
    d->wheel[0]=car->GetWheel(2);
    car->GetWheel(2)->SetDifferential(d,0);
    d->wheel[1]=car->GetWheel(3);
    car->GetWheel(3)->SetDifferential(d,1);
    d->engine=car->GetEngine();
    car->AddDifferential(d);
  } else
  {
    // True set of differentials
    // Check for known situations.
    if(n!=1&&n!=3)
    { qwarn("RDriveLine:Load(); unsupported number of differentials (%d)",n);
      qlog(QLOG_INFO,"Forcing to 1 differential");
      n=1;
    }
    for(i=0;i<n;i++)
    {
      // Create the next diff
      d=new RDifferential(car);
      sprintf(buf,"diff%d",i);
      d->Load(info,buf);
      car->AddDifferential(d);
    }
  }

  // Clutch
  clutchMaxTorque=info->GetFloat("clutch.max_torque");

  // Handbrakes
  info->GetString("handbrakes.wheels",buf);
  // Enable handbrakes for all the given wheels
  for(s=buf;*s;s++)
  {
    if(*s>='0'&&*s<='0'+car->GetWheels())
    {
      w=car->GetWheel(*s-'0');
      if(w)w->EnableHandbrakes();
    }
  }
  return TRUE;
}
bool RDriveLine::LoadState(QFile *f)
{
  f->Read(&preClutchInertia,sizeof(preClutchInertia));
  f->Read(&postClutchInertia,sizeof(postClutchInertia));
  f->Read(&totalInertia,sizeof(totalInertia));
  f->Read(&prepostLocked,sizeof(prepostLocked));
  f->Read(&tClutch,sizeof(tClutch));
  return TRUE;
}
bool RDriveLine::SaveState(QFile *f)
{
  f->Write(&preClutchInertia,sizeof(preClutchInertia));
  f->Write(&postClutchInertia,sizeof(postClutchInertia));
  f->Write(&totalInertia,sizeof(totalInertia));
  f->Write(&prepostLocked,sizeof(prepostLocked));
  f->Write(&tClutch,sizeof(tClutch));
  return TRUE;
}

/********
* Reset *
********/
void RDriveLine::Reset()
{
  autoClutch=FALSE;
  prepostLocked=FALSE;
}

/*****************
* Precalculation *
*****************/
void RDriveLine::CalcCumulativeRatios()
// Calculate the resulting spin factors (ratios) throughout the driveline
{
  if(root)
    root->CalcCumulativeRatio();
}

void RDriveLine::CalcEffectiveInertiae()
// Calculate all effective inertiae in the driveline
{
  if(root)
    root->CalcEffectiveInertia();
}

void RDriveLine::CalcPreClutchInertia()
// Calculate all inertia before the clutch (engine)
{
  // Engine should be the root
  if(!root)
  {
    qwarn("RDriveLine:CalcPreClutchInertia(); no root component");
    return;
  }
  // Note that we take the inertia without adding the ratios, since
  // this will be used when the engine rotates at a different speed
  // from the rest of the drivetrain.
  preClutchInertia=root->GetInertia();
}

void RDriveLine::CalcPostClutchInertia()
// Calculate all inertia after the clutch (gearbox, diffs, wheels)
{
  // Check for a driveline to be present
  if(!root)
  {
    qwarn("RDriveLine:CalcPreClutchInertia(); no root component");
    return;
  }
  postClutchInertia=root->GetChild(0)->GetEffectiveInertia();

  // Also calculate total inertia
  totalInertia=preClutchInertia+postClutchInertia;
}

void RDriveLine::SetClutchApplication(rfloat app)
// Set clutch application.
{
  if(app<0)app=0; else if(app>1.0f)app=1.0f;
  // Make it undergo severe linearity; I couldn't get
  // a smooth take-off without stalling the engine.
  app=clutchLinearity*app+(1.0f-clutchLinearity)*(app*app*app);
  clutchApplication=app;
  clutchCurrentTorque=app*clutchMaxTorque;
}

/************
* Debugging *
************/
void RDriveLine::DbgPrint(cstring s)
{
  qdbg("Driveline (%s); clutch: app=%.2f, maxT=%.2f, T=%.2f\n",s,
    clutchApplication,clutchMaxTorque,clutchCurrentTorque);
  // Print tree
  if(root)
    root->DbgPrint(0,s);
}

/********
* Input *
********/
void RDriveLine::SetInput(int ctlClutch,int ctlHandbrake)
// Inputs controller state. May be overruled though is some assisting is on.
{
  // Clutch
  if(IsAutoClutchActive())
  {
    // Automatic assist is on; don't accept user input for a while
    //SetClutchApplication(car->GetEngine()->GetAutoClutch());
  } else
  {
    // Manual
    SetClutchApplication(((rfloat)ctlClutch)/1000.0f);
  }

  // Handbrakes
  handbrakeApplication=((rfloat)ctlHandbrake)/1000.0f;
  if(handbrakeApplication<0)handbrakeApplication=0;
  else if(handbrakeApplication>1)handbrakeApplication=1;
}

/**********
* Physics *
**********/
void RDriveLine::CalcForces()
// Determine forces throughout the driveline.
// Assumes the wheel reaction forces are already calculated.
{
  if(root==0||gearbox==0)
  { qwarn("RDriveLine:CalcForces(); driveline not built yet");
    return;
  }

  // In neutral gear the pre and post parts are always unlocked,
  // and no clutch torque is applied.
  if(car->GetGearBox()->IsNeutral())
  {
    UnlockPrePost();
    //tClutch=0;
    clutchCurrentTorque=0;
    //goto skip_clutch_calc;
  }

  // Calculate current clutch torque (including torque direction)
  if(!IsPrePostLocked())
  {
    // Engine is running separately from the rest of the driveline.
    // The clutch works to make the velocity of pre-clutch (engine) and
    // post-clutch (rest) equal.
    if(root->GetRotationVel()>gearbox->GetRotationVel()*gearbox->GetRatio())
      tClutch=GetClutchCurrentTorque();
    else
      tClutch=-GetClutchCurrentTorque();
#ifdef LTRACE
qdbg("RDL:CF; engine rotV=%.2f, gearbox rotV=%.2f => clutchT=%.2f\n",
root->GetRotationVel(),gearbox->GetRotationVel(),tClutch);
#endif
  } // else acceleration will be given by the driveline (engine rotates
    // along with the rest of the driveline as a single assembly)

  // Spread wheel (leaf) reaction forces all the way to the engine (root)
  root->CalcReactionForces();

  if(IsPrePostLocked())
  {
    // Check if pre and post are still locked
    rfloat Te,Tr,Tb,Tc;
    Te=root->GetEngineTorque();
    Tr=root->GetReactionTorque();
    Tb=root->GetBrakingTorque();
    Tc=GetClutchCurrentTorque();
#ifdef LTRACE
qdbg("pre-post is locked: Te(%.2f) > (%.2f+%.2f+%.2f=%.2f) ?\n",
Te,Tr,Tb,Tc,Tr+Tb+Tc);
#endif
    if(fabs(Te-(Tr+Tb))>Tc)
    {
qdbg("  pre-post gets UNLOCKED\n");
      UnlockPrePost();
    }
  } // else it will get locked again when velocity reversal happens
    // of the engine vs. rest of the drivetrain (=gearbox in angular velocity)

  if(IsSingleDiff())
  {
    // Special case where some optimizations can be done
    RDifferential *diff=car->GetDifferential(0);
//qdbg("Single diff case:\n");
    if(IsPrePostLocked())
    {
      rfloat Te,r;
      Te=root->GetEngineTorque();
      r=root->GetCumulativeRatio();
//qdbg("  Diff gets Te=%.2f * ratio %.2f = %.2f\n",Te,r,Te*r);
      diff->CalcSingleDiffForces(Te*r,root->GetEffectiveInertia());
    } else
    {
      // Engine spins at a different rate from the rest of the driveline
      // In this case the clutch fully works on getting the pre- and post-
      // clutch angular velocities equal.
      // Note that if the clutch is fully depressed (engine totally decoupled
      // from the rest of the driveline), this torque will be 0, and the
      // postclutch driveline will just rotate freely.
//qdbg("  SingleDiff and prepost unlocked.\n");
      rfloat Tc,r;
      Tc=GetClutchTorque();
      r=root->GetCumulativeRatio();
//qdbg("  Diff gets Tc=%.2f * ratio %.2f = %.2f\n",Tc,r,Tc*r);
      diff->CalcSingleDiffForces(Tc*r,root->GetEffectiveInertia());
    }
  }
}

void RDriveLine::CalcAccelerations()
// Calculate the accelerations of all the parts in the driveline
{
#ifdef LTRACE
qdbg("RDriveLine::CalcAccelerations()\n");
#endif

  if(IsSingleDiff())
  {
    // Special case with speedier calculations
    RDifferential *diff=car->GetDifferential(0);
    RDriveLineComp *comp;
    rfloat acc=diff->GetAccIn();
    if(IsPrePostLocked())
    {
//qdbg("Single diff, prepost LOCKED\n");
      // Everything is decided by the differential acceleration
      // Wheels got their acceleration in RWheel::CalcAccelerations()
      // Pass acceleration up the tree; mind the ratios
      for(comp=diff;comp;comp=comp->GetParent())
      {
        comp->SetRotationAcc(acc);
        acc*=comp->GetRatio();
//qdbg("  comp '%s' acc %.2f (ratio %.2f)\n",
//comp->GetName(),acc,comp->GetRatio());
      }
    } else
    {
      // Separate pre- and postclutch accelerations
      // First calculate the engine's acceleration.
      root->CalcAccelerations();
      // Rest of the driveline takes its acceleration from the diff
      // Wheels got their acceleration in RWheel::CalcAccelerations()
      // Pass acceleration up the tree, EXCEPT for the engine; mind the ratios
      // May combine this into the loop above by a sentinel component
      // which is either '0' or 'root'. (for the PrePostLocked case)
      for(comp=diff;comp!=root;comp=comp->GetParent())
      {
        comp->SetRotationAcc(acc);
        acc*=comp->GetRatio();
//qdbg("  comp '%s' acc %.2f (ratio %.2f)\n",
//comp->GetName(),acc,comp->GetRatio());
      }
    }
  }
}

void RDriveLine::Integrate()
// Take a time step
{
#ifdef LTRACE
qdbg("RDriveLine::Integrate()\n");
#endif

  rfloat deltaVel,newDeltaVel;

  // Remember difference between engine and gear rotation
  deltaVel=root->GetRotationVel()-gearbox->GetRotationVel()*gearbox->GetRatio();
  if(IsPrePostLocked())
  {
    // Check for the engine and gearbox (ratio'd)
    // to rotate just about equally.
    // If not, the driver may have speedshifted without
    // applying the clutch. Unfortunately, this is possible
    // but should result in damage in the future, since
    // you get a lot of gear noise.
    if(fabs(deltaVel)>DELTA_VEL_THRESHOLD)
    {
      // Gear scratch sample
      car->GetRAPGearMiss()->GetSample()->Play();
      // Unlock pre-post to let things catch up again
      UnlockPrePost();
    }
  }

/*
qdbg("rotV: engine=%.3f, gearbox=%.3f\n",root->GetRotationVel(),gearbox->GetRotationVel());
qdbg("  engine=%p, root=%p, gearbox=%p\n",car->GetEngine(),root,gearbox);
qdbg("  rpm=%f\n",car->GetEngine()->GetRPM());
*/

  // Engine
  if(root)root->Integrate();
  if(gearbox)gearbox->Integrate();

  if(!IsPrePostLocked())
  {
    // Check if gearbox is catching up with engine
    newDeltaVel=
      root->GetRotationVel()-gearbox->GetRotationVel()*gearbox->GetRatio();
//qdbg("Check lock; oldDV=%.3f, newDV=%.3f\n",deltaVel,newDeltaVel);
    if((deltaVel>0&&newDeltaVel<0)||
       (deltaVel<0&&newDeltaVel>0))
    {
qdbg("  RE-LOCKED!\n");
      LockPrePost();
      // Force engine and gearbox velocity to be the same
qdbg("  engine rotV=%.3f, gearbox rotV=%.3f\n",
root->GetRotationVel(),gearbox->GetRotationVel());
      root->SetRotationVel(gearbox->GetRotationVel()*gearbox->GetRatio());
    }
  }
}

