/*
 * RRigidBody class definition
 * 03-02-01: Created!
 * NOTES:
 * - To use any RRigidBody, do the following:
 *   1. Set a time step using RRigidBody::SetTimeStep()
 * - To use a RRigidBody, take the following steps:
 *   1. Create it
 *   2. Set its mass
 *   3. Set its inertia values
 * - After that, use it as follows:
 *   1. Call IntegrateInit() before adding forces and such
 *   2. Call Add...Force/Torque() to add forces and torques
 *   3. IntegrateEuler() to integrate using Euler
 * FUTURE:
 * - Automatically call IntegrateInit() after IntegrateEuler()?
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Gyroscopic calculations?
#define USE_GYROSCOPIC_PRECESSION

// Local tracing?
//#define LTRACE

// Externally, a maximum torque is defined
// Internally, in this module, we have a maximum rotation speed
// which keeps things in line a bit, otherwise things get out
// of hand completely with big numbers.
#define MAX_ROT_VEL    120       // Approx. 2*PI*20 = 20 rotations/sec

rfloat RRigidBody::h=0.0f;
rfloat RRigidBody::h2=0.0f;
rfloat RRigidBody::h4=0.0f;
rfloat RRigidBody::h6=0.0f;

/*******
* CTOR *
*******/
RRigidBody::RRigidBody()
  //: qRotPos(1,0,0,0)             // Normal start
  //: qRotPos(1,0,-1,0)          // 90 degrees clockwise around yaw
  : qRotPos(0,0,-1,0)          // 180 degrees yaw
{
  mass=0;
  totalForce.SetToZero();
  totalTorque.SetToZero();
  linPos.SetToZero();
  linVel.SetToZero();
  linVelBC.SetToZero();
  rotVel.SetToZero();
  // Make initial 3x3 matrix
  mRotPos.FromQuaternionL2(&qRotPos,qRotPos.LengthSquared());
  // q/mRotPos
  inertiaTensor.SetIdentity();
#ifdef RR_ODE
  // Leave rigid body things to ODE
  odeBody=dBodyCreate(RMGR->odeWorld);
  // Set initial state
  dBodySetPosition(odeBody,linPos.x,linPos.y,linPos.z);
  odeGeom=0;
#endif
  IntegrateInit();
}

RRigidBody::~RRigidBody()
{
#ifdef RR_ODE
  if(odeBody)dBodyDestroy(odeBody);
#endif
}

/*******
* Dump *
*******/
bool RRigidBody::LoadState(QFile *f)
{
  f->Read(&linPos,sizeof(linPos));
  f->Read(&linVel,sizeof(linVel));
  f->Read(&qRotPos,sizeof(qRotPos));
  f->Read(&mRotPos,sizeof(mRotPos));
  f->Read(&rotVel,sizeof(rotVel));
#ifdef OBS
  f->Read(&h,sizeof(h));
  f->Read(&h2,sizeof(h2));
  f->Read(&h4,sizeof(h4));
  f->Read(&h6,sizeof(h6));
#endif
  f->Read(&totalForce,sizeof(totalForce));
  f->Read(&totalTorque,sizeof(totalTorque));
#ifdef RR_ODE
  // Pass on some info
  dBodySetPosition(odeBody,linPos.x,linPos.y,linPos.z);
  dBodySetLinearVel(odeBody,linVel.x,linVel.y,linVel.z);
  dBodySetAngularVel(odeBody,rotVel.x,rotVel.y,rotVel.z);
  //dBodySetRotation(odeBody,mRotPos.GetM());
  dBodySetQuaternion(odeBody,(dReal*)&qRotPos);
#endif

  return TRUE;
}
bool RRigidBody::SaveState(QFile *f)
{
  f->Write(&linPos,sizeof(linPos));
  f->Write(&linVel,sizeof(linVel));
  f->Write(&qRotPos,sizeof(qRotPos));
  f->Write(&mRotPos,sizeof(mRotPos));
  f->Write(&rotVel,sizeof(rotVel));
#ifdef OBS
  f->Write(&h,sizeof(h));
  f->Write(&h2,sizeof(h2));
  f->Write(&h4,sizeof(h4));
  f->Write(&h6,sizeof(h6));
#endif
  f->Write(&totalForce,sizeof(totalForce));
  f->Write(&totalTorque,sizeof(totalTorque));
  return TRUE;
}

/*************
* ATTRIBUTES *
*************/
void RRigidBody::SetInertia(rfloat x,rfloat y,rfloat z)
// Set evenly massed inertia tensor
// The x/y/z are the diagonal values in a 3x3 inertia tensor
// For most cars, it is reasonably ok to have balanced masses across
// all 3 axes.
{
#ifdef RR_ODE
  // Need to set mass and inertia at the same time
  qerr("RRigidBody:SetMass(): not used in ODE; use SetMassInertia()");
  return;
#endif
  inertiaTensor.SetIdentity();
  inertiaTensor.SetRC(0,0,x);
  inertiaTensor.SetRC(1,1,y);
  inertiaTensor.SetRC(2,2,z);

  // Precalculate inertia variables
  IyyMinusIzzOverIxx=(inertiaTensor.GetRC(1,1)-inertiaTensor.GetRC(2,2))/
                     inertiaTensor.GetRC(0,0);
  IzzMinusIxxOverIyy=(inertiaTensor.GetRC(2,2)-inertiaTensor.GetRC(0,0))/
                     inertiaTensor.GetRC(1,1);
  IxxMinusIyyOverIzz=(inertiaTensor.GetRC(0,0)-inertiaTensor.GetRC(1,1))/
                     inertiaTensor.GetRC(2,2);
  oneOverIxx=1.0f/inertiaTensor.GetRC(0,0);
  oneOverIyy=1.0f/inertiaTensor.GetRC(1,1);
  oneOverIzz=1.0f/inertiaTensor.GetRC(2,2);
#ifndef USE_GYROSCOPIC_PRECESSION
  IyyMinusIzzOverIxx=0;
  IzzMinusIxxOverIyy=0;
  IxxMinusIyyOverIzz=0;
#endif

}

void RRigidBody::SetMass(rfloat m)
{
#ifdef RR_ODE
  // Need to set mass and inertia at the same time
  qerr("RRigidBody:SetMass(): not used in ODE; use SetMassInertia()");
  return;
#endif

qdbg("RRigid:SetMass(%f)\n",m);
  mass=m;
  // Precalculate 1/mass for optimization reasons
  if(mass!=0.0f)oneOverMass=1.0f/mass;
  else          oneOverMass=0.0f;
}
void RRigidBody::SetMassInertia(rfloat m,rfloat I11,rfloat I22,rfloat I33)
// Set mass and inertia both at the same time. This is used for ODE,
// where this operation is supported. May expand with the CG position
// in the future.
// For ODE compiles (RR_ODE), don't use SetMass()/SetInertia() but this one.
{
#ifdef RR_ODE
  // Note that here the CG is at the origin of the body, and no non-diagonal
  // inertia values are given.
  dMassSetParameters(&odeMass,m,0,0,0,I11,I22,I33,0,0,0);
  dBodySetMass(odeBody,&odeMass);
  // Store locally for fast gets as well
qdbg("RRigid:SetMassInertia(%f)\n",m);
  mass=m;
  // Precalculate 1/mass for optimization reasons (may not be used, 20-11-01)
  if(mass!=0.0f)oneOverMass=1.0f/mass;
  else          oneOverMass=0.0f;
#else
  // Revert to the separate functions
  SetMass(m);
  SetInertia(I11,I22,I33);
#endif
}

void RRigidBody::SetTimeStep(rfloat _h)
// Set time step for all rigid bodies
{
  h=_h;
  // Precalculate other time steps for fast RK integration
  h2=h/2.0f;
  h4=h/4.0f;
  h6=h/6.0f;
}

void RRigidBody::CalcMatrixFromQuat()
// Take qRotPos and create mRotPos
// This explicit matrix calculation is needed when the quaternion
// state arrives through the network, and the matrix must be
// calculated for drawing the body later on.
{
  // Make new 3x3 matrix from quaternion
  mRotPos.FromQuaternionL2(&qRotPos,qRotPos.LengthSquared());
} 

/************************
* ADDING FORCES/TORQUES *
************************/
void RRigidBody::AddWorldForceAtBodyPos(DVector3 *force,DVector3 *pos)
// The 'force' is applied at location 'pos'
// 'force' is in world coordinates, 'pos' is in body coordinates
{
#ifdef LTRACE
qdbg("RRigid::AddWorldForceAtBodyPos\n");
force->DbgPrint("  force");
pos->DbgPrint("  pos");
//if(force->y>10000){char *p=0; *p=0; }  // Force debugging break
#endif

  DVector3 forceBody;
  // Calculate force in body coordinates
  mRotPos.TransposeTransform(force,&forceBody);
//mRotPos.DbgPrint("mRotPos");
//forceBody.DbgPrint("  forceBody");
//force->DbgPrint("  force");
  // Add the linear force in world coordinates
#ifdef RR_ODE
  dBodyAddForce(odeBody,force->x,force->y,force->z);
  //dBodyAddRelForce(odeBody,-force->z,force->y,-force->x);
#else
  totalForce.Add(force);
#endif
  
  DVector3 torqueBody;
  // Calculate torque in body coordinates
  torqueBody.Cross(pos,&forceBody);
#ifdef RR_ODE
  // Not tested (pos is always (0,0,0) in Racer; so no torque); but probably
  // this is right anyway.
  //dBodyAddRelTorque(odeBody,torqueBody.x,torqueBody.y,torqueBody.z);
  //dBodyAddRelTorque(odeBody,-torqueBody.z,torqueBody.y,torqueBody.x);
#else
  totalTorque.Add(&torqueBody);
#endif
//totalForce.DbgPrint("  totalForce");
//totalTorque.DbgPrint("  totalTorque");
}
void RRigidBody::AddBodyForceAtBodyPos(DVector3 *force,DVector3 *pos)
// The 'force' is applied at location 'pos'
// 'force' is in body coordinates, 'pos' is in body coordinates
{
#ifdef LTRACE
qdbg("RRigid::AddBodyForceAtBodyPos\n");
force->DbgPrint("  force");
pos->DbgPrint("  pos");
#endif

  DVector3 forceWorld,torqueBody;
  // Calculate force in world coordinates
  mRotPos.Transform(force,&forceWorld);
//qdbg("RRigid::AddBodyForceAtBodyPos\n");
//mRotPos.DbgPrint("mRotPos"); 
//force->DbgPrint("force");
//forceWorld.DbgPrint("forceWorld");

  // Add the linear force in world coordinates
#ifdef RR_ODE
  dBodyAddForce(odeBody,forceWorld.x,forceWorld.y,forceWorld.z);
#else
  totalForce.Add(&forceWorld);
#endif
  
  // Calculate torque in body coordinates
  torqueBody.Cross(pos,force);
#ifdef RR_ODE
  dBodyAddRelTorque(odeBody,torqueBody.x,torqueBody.y,torqueBody.z);
  //dBodyAddRelTorque(odeBody,-torqueBody.z,torqueBody.y,torqueBody.x);
#else
  totalTorque.Add(&torqueBody);
#endif
//totalForce.DbgPrint("  totalForce");
//totalTorque.DbgPrint("  totalTorque");
}

void RRigidBody::AddBodyTorque(DVector3 *torque)
// Add 'torque', given in body coordinates
{
#ifdef LTRACE
qdbg("RRigid:AddBodyTorque()\n");
torque->DbgPrint("torque");
#endif

#ifdef RR_ODE
  // Calculate force in world coordinates
  dBodyAddRelTorque(odeBody,torque->x,torque->y,torque->z);
/*
  DVector3 torqueWC;
  mRotPos.Transform(torque,&torqueWC);
  dBodyAddTorque(odeBody,torqueWC.x,torqueWC.y,torqueWC.z);
*/
#else
  totalTorque.Add(torque);
#endif

#ifdef OBS
totalForce.DbgPrint("  totalForce");
totalTorque.DbgPrint("  totalTorque");
#endif
}

void RRigidBody::AddWorldTorque(DVector3 *torque)
// Add 'torque', given in world coordinates
{
  qerr("RRigidBody::AddWorldTorque(); may contain bug (add torqueBody?)");
  return;

#ifdef LTRACE
qdbg("RRigid:AddWorldTorque\n");
torque->DbgPrint("torque");
#endif

  DVector3 torqueBody;
  // Convert world to body coordinates
  mRotPos.TransposeTransform(torque,&torqueBody);
#ifdef RR_ODE
  //dBodyAddRelTorque(odeBody,torqueBody.x,torqueBody.y,torqueBody.z);
  //dBodyAddRelTorque(odeBody,-torqueBody.z,torqueBody.y,torqueBody.x);
#else
  totalTorque.Add(&torqueBody);
#endif
}

#ifdef OBS
// Direct torques
// Forces with a point of application
void RRigidBody::AddWorldWorldForce(DVector3 *force,DVector3 *pos);
void RRigidBody::AddWorldBodyForce(DVector3 *force,DVector3 *pos);
void RRigidBody::AddBodyBodyForce(DVector3 *force,DVector3 *pos);

// Coordinate system conversions
void RRigidBody::CalcWorldPos(DVector3 *bodyPos,DVector3 *worldPos);
void RRigidBody::CalcBodyPos(DVector3 *worldPos,DVector3 *bodyPos);
void RRigidBody::CalcBodyWorldVel(DVector3 *bodyPos,DVector3 *worldVel);
void RRigidBody::CalcWorldWorldVel(DVector3 *worldPos,DVector3 *worldVel);
void RRigidBody::CalcWorldPosVel(DVector3 *bodyPos,DVector3 *worldPos,
  DVector3 *worldVel);

// Debugging helpers
rfloat RRigidBody::GetEnergy();
void   RRigidBody::GetMomentum(DVector3 *linear,DVector3 *rotational);
#endif

/**************
* INTEGRATION *
**************/
void RRigidBody::IntegrateInit()
// Call this function before adding forces/torques
// This function resets the current force/torque
{
#ifdef RR_ODE
  // Force clearing is done by ODE
#else
  totalForce.SetToZero();
  totalTorque.SetToZero();
#endif
  // Remember start values
  qRotPos0=qRotPos;
}

#define CLIP_POS(v,n) if((v)>(n)){clip=TRUE;clipVal=(v);(v)=(n);}
#define CLIP_NEG(v,n) if((v)<-(n)){clip=TRUE;clipVal=(v);(v)=-(n);}

void RRigidBody::IntegrateEuler()
// Integrate current force/torque over time 'h' ('h' given earlier)
{
#ifdef OBS
qdbg("RRB:IntegrateEuler; BEFORE\n");
mRotPos.DbgPrint("mRotPos");
linPos.DbgPrint("linPos");
linVel.DbgPrint("linVel");
rotVel.DbgPrint("rotVel");
inertiaTensor.DbgPrint("inertiaTensor");
#endif

  if(RMGR->IsDevEnabled(RManager::NO_GRAVITY_BODY))
  {
#ifdef RR_ODE
    qwarn("Can't set NO_GRAVITY_BODY under ODE");
#else
    // Cancel all force and torque
    totalForce.SetToZero();
    totalTorque.SetToZero();
#endif
  }

#ifdef RR_ODE
  // Integration is done for all rigid bodies elsewhere
//dBodyAddRelTorque(odeBody,1000,0,0);
//DVector3 vv(0,0,100),v0(0,0,0);
//AddBodyTorque(&vv);
//AddBodyForceAtBodyPos(&vv,&v0);
#else
// Cancel all force and torque
totalForce.SetToZero();
totalTorque.SetToZero();
DVector3 vv(10000,0,0),v0(0,0,0);
totalTorque.x=100;
//AddBodyForceAtBodyPos(&vv,&v0);

  // Check for excessive forces/torques
  bool clip=FALSE;
  rfloat max,clipVal;
  max=RMGR->maxForce;
  CLIP_POS(totalForce.x,max)
  CLIP_NEG(totalForce.x,max)
  CLIP_POS(totalForce.y,max)
  CLIP_NEG(totalForce.y,max)
  CLIP_POS(totalForce.z,max)
  CLIP_NEG(totalForce.z,max)
  max=RMGR->maxTorque;
  CLIP_POS(totalTorque.x,max)
  CLIP_NEG(totalTorque.x,max)
  CLIP_POS(totalTorque.y,max)
  CLIP_NEG(totalTorque.y,max)
  CLIP_POS(totalTorque.z,max)
  CLIP_NEG(totalTorque.z,max)
  //if(clip)qdbg("RRigidBody:Clipping force/torque (%f) to max\n",clipVal);
  
  // Linear movement
  linPos.x+=h*linVel.x;
  linPos.y+=h*linVel.y;
  linPos.z+=h*linVel.z;

  // Linear acceleration
#ifdef OBS
totalForce.DbgPrint("totalForce");
qdbg("oneOverMass=%f, h=%f\n",oneOverMass,h);
#endif
  linVel.x+=totalForce.x*oneOverMass*h;
  linVel.y+=totalForce.y*oneOverMass*h;
  linVel.z+=totalForce.z*oneOverMass*h;

  // Rotation
#ifdef OBS
qdbg("qRotPos0=(%.2f,%.2f,%.2f,%.2f)\n",
qRotPos0.w,qRotPos0.x,qRotPos0.y,qRotPos0.z);
#endif
  qRotPos.w-=h2*(qRotPos0.x*rotVel.x+qRotPos0.y*rotVel.y+qRotPos0.z*rotVel.z);
  qRotPos.x+=h2*(qRotPos0.w*rotVel.x-qRotPos0.z*rotVel.y+qRotPos0.y*rotVel.z);
  qRotPos.y+=h2*(qRotPos0.z*rotVel.x+qRotPos0.w*rotVel.y-qRotPos0.x*rotVel.z);
  qRotPos.z+=h2*(qRotPos0.x*rotVel.y-qRotPos0.y*rotVel.x+qRotPos0.w*rotVel.z);

  // Check if quaternion needs normalizing
  rfloat l2=qRotPos.x*qRotPos.x+qRotPos.y*qRotPos.y+
            qRotPos.z*qRotPos.z+qRotPos.w*qRotPos.w;
  if(l2<0.9999f||l2>1.0001f)
  {
    // Push quat back to 1.0 (avoid a sqrt())
//qdbg("RRB:IntegrateEuler; renormalize quat\n");
    float n=(l2+1.0f)/(2.0f*l2);
    qRotPos.x*=n;
    qRotPos.y*=n;
    qRotPos.z*=n;
    qRotPos.w*=n;
    l2*=n*n;
  }
  // Make new 3x3 matrix from quaternion
  mRotPos.FromQuaternionL2(&qRotPos,l2);

  // Rotational acceleration (with gyroscopic precession)
#ifdef OBS_PRECALCED
  rfloat IyyMinusIzzOverIxx,IzzMinusIxxOverIyy,IxxMinusIyyOverIzz;
  rfloat oneOverIxx,oneOverIyy,oneOverIzz;
  IyyMinusIzzOverIxx=(inertiaTensor.GetRC(1,1)-inertiaTensor.GetRC(2,2))/
                     inertiaTensor.GetRC(0,0);
  IzzMinusIxxOverIyy=(inertiaTensor.GetRC(2,2)-inertiaTensor.GetRC(0,0))/
                     inertiaTensor.GetRC(1,1);
  IxxMinusIyyOverIzz=(inertiaTensor.GetRC(0,0)-inertiaTensor.GetRC(1,1))/
                     inertiaTensor.GetRC(2,2);
  oneOverIxx=1.0f/inertiaTensor.GetRC(0,0);
  oneOverIyy=1.0f/inertiaTensor.GetRC(1,1);
  oneOverIzz=1.0f/inertiaTensor.GetRC(2,2);
#ifndef USE_GYROSCOPIC_PRECESSION
  IyyMinusIzzOverIxx=0;
  IzzMinusIxxOverIyy=0;
  IxxMinusIyyOverIzz=0;
#endif
#endif
  rotVel1.x=rotVel.x+(rotVel.y*rotVel.z*IyyMinusIzzOverIxx+
            totalTorque.x*oneOverIxx)*h;
  rotVel1.y=rotVel.y+(rotVel.z*rotVel.x*IzzMinusIxxOverIyy+
            totalTorque.y*oneOverIyy)*h;
  rotVel1.z=rotVel.z+(rotVel.x*rotVel.y*IxxMinusIyyOverIzz+
            totalTorque.z*oneOverIzz)*h;
  rotVel=rotVel1;
  // Top off rotating velocities
  CLIP_POS(rotVel.x,MAX_ROT_VEL)
  CLIP_NEG(rotVel.x,MAX_ROT_VEL)
  CLIP_POS(rotVel.y,MAX_ROT_VEL)
  CLIP_NEG(rotVel.y,MAX_ROT_VEL)
  CLIP_POS(rotVel.z,MAX_ROT_VEL)
  CLIP_NEG(rotVel.z,MAX_ROT_VEL)

  // Calculate linVel in body coordinates, since this will be used
  // by CalcBodyVelForBodyPos() (used in Racer for each wheel)
  mRotPos.TransposeTransform(&linVel,&linVelBC);
#endif
// ^^ !RR_ODE

#ifdef OBS
qdbg("RRigid:IntegrateEuler() AFTER\n");
mRotPos.DbgPrint("mRotPos");
linPos.DbgPrint("linPos");
linVel.DbgPrint("linVel");
rotVel.DbgPrint("rotVel");
totalTorque.DbgPrint("totalTorque (RRigidBody)");
#endif
}

#ifdef RR_ODE
static void ConvertODEMatrix(const float R[12],DMatrix3 *m)
// Take an ODE rotation matrix (4x3, column-major) and convert to D3 matrix
// (3x3, column-major). Does not include a translation.
{
  dfloat *matrix=m->GetM();
  matrix[0]=R[0];
  matrix[1]=R[1];
  matrix[2]=R[2];
  matrix[3]=R[4];
  matrix[4]=R[5];
  matrix[5]=R[6];
  matrix[6]=R[8];
  matrix[7]=R[9];
  matrix[8]=R[10];
}
#endif
void RRigidBody::PostIntegrate()
// After integration, since the introduction of ODE a couple of variables
// are synchronised to the state of ODE's rigid body. This allow some
// results to appear before actually converting every variable usage to ODE.
// After a while, these should disappear and the real ODE internal variables
// should be used for better performance.
{
#ifdef RR_ODE
  const float *v;

  // Sync linear position
  v=dBodyGetPosition(odeBody);
  linPos.x=v[0];
  linPos.y=v[1];
  linPos.z=v[2];

  // Rotation
  v=dBodyGetQuaternion(odeBody);
  qRotPos.w=v[0];
  qRotPos.x=v[1];
  qRotPos.y=v[2];
  qRotPos.z=v[3];
#ifdef ND_CAN_BE_DONE_CHEAPER
  // Derive matrix from the quat
  mRotPos.FromQuaternion(&qRotPos);
#endif
  // Derive matrix from ODE matrix (which uses row-major I think)
  ConvertODEMatrix(dBodyGetRotation(odeBody),&mRotPos);

  // Linear velocity
  v=dBodyGetLinearVel(odeBody);
  linVel.x=v[0];
  linVel.y=v[1];
  linVel.z=v[2];
  // Calculate linVel in body coordinates, since this will be used
  // by CalcBodyVelForBodyPos() (used in Racer for each wheel)
  mRotPos.TransposeTransform(&linVel,&linVelBC);

  // Rotational velocity
  v=dBodyGetAngularVel(odeBody);
  rotVel.x=v[0];
  rotVel.y=v[1];
  rotVel.z=v[2];

//mRotPos.DbgPrint("PostIntegrate mRotPos");
//DMatrix3 mODE;
//ConvertODEMatrix(dBodyGetRotation(odeBody),&mODE);
//mODE.DbgPrint("PostIntegrate mODE");
#endif
}

/********************************
* Coordinate system conversions *
********************************/
void RRigidBody::ConvertBodyToWorldPos(DVector3 *bodyPos,DVector3 *worldPos)
// Takes 'bodyPos', converts it to world coordinates, and puts
// the result in 'worldPos'
{
#ifdef OBS
qdbg("RRB: B2WPos\n");
bodyPos->DbgPrint("  bodyPos");
//mRotPos.DbgPrint("mRotPos");
#endif
  mRotPos.Transform(bodyPos,worldPos);
//worldPos->DbgPrint("  worldPos");
  worldPos->Add(&linPos);
//worldPos->DbgPrint("  worldPos+linPos");
}
void RRigidBody::ConvertBodyToWorldOrientation(DVector3 *bodyOr,
  DVector3 *worldOr)
// Takes 'bodyOr', converts it to world coordinates, and puts
// the result in 'worldOr'. Only does orientation, not position.
{
  mRotPos.Transform(bodyOr,worldOr);
}

void RRigidBody::ConvertWorldToBodyPos(DVector3 *worldPos,DVector3 *bodyPos)
// Takes 'worldPos', converts it to body coordinates, and puts
// the result in 'bodyPos'
{
  bodyPos->Subtract(&linPos);
  mRotPos.TransposeTransform(worldPos,bodyPos);
}
void RRigidBody::ConvertWorldToBodyOrientation(DVector3 *worldOr,
  DVector3 *bodyOr)
// Takes 'worldOr', converts it to body coordinates, and puts
// the result in 'bodyOr'. Only does orientation, not position.
{
  mRotPos.TransposeTransform(worldOr,bodyOr);
}

/*************
* Velocities *
*************/
void RRigidBody::CalcWorldVelForBodyPos(const DVector3 *posBC,
  DVector3 *velWC)
// Calculate velocity in world coordinates for body position 'posBC'
// Stores result in 'velWC'
{
  DVector3 velBC;
  velBC.Cross(&rotVel,posBC);
  mRotPos.Transform(&velBC,velWC);
  // Add world linear velocity
  velWC->Add(&linVel);
}
void RRigidBody::CalcBodyVelForBodyPos(const DVector3 *posBC,
  DVector3 *velBC)
// Calculate velocity in body coordinates for body position 'posBC'
// Stores result in 'velBC'
{
  //DVector3 linVelBC;
  // Calculate velocity because of rotation in body coords
  velBC->Cross(&rotVel,posBC);
  // Add body linear velocity
  //mRotPos.TransposeTransform(&linVel,&linVelBC);
  velBC->Add(&linVelBC);
}
