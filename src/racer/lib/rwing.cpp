/*
 * RWing
 * 09-07-2001: Created! (19:29:26)
 * NOTES:
 * - When changing 'angle' live in a race, make sure to recalc 'fixedFactor'!
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

/*********
* c/dtor *
*********/
RWing::RWing(RCar *_car)
{
  car=_car;
  span=cord=0;
  angle=angleOffset=0;
  fixedFactorY=fixedFactorZ=0;
  coeff.SetToZero();
  center.SetToZero();
  airVelocity.SetToZero();
  force.SetToZero();
  torque.SetToZero();
}
RWing::~RWing()
{
}

/******
* I/O *
******/
bool RWing::Load(QInfo *info,cstring path)
{
qdbg("RWing:Load(%s)\n",path);

  char buf[128];
  
  // Size
  sprintf(buf,"%s.name",path);
  name=info->GetStringDirect(buf);
  sprintf(buf,"%s.span",path);
  span=info->GetFloat(buf);
  sprintf(buf,"%s.cord",path);
  cord=info->GetFloat(buf);
  
  // Coefficients; we use XYZ as in OpenGL
  // so SAE-x=OGL-Z, SAE-y=OGL-x and SAE-z=-OGL-y
  sprintf(buf,"%s.coeff_drag",path);
  coeff.z=info->GetFloat(buf);
  sprintf(buf,"%s.coeff_down",path);
  coeff.y=info->GetFloat(buf);
  // No lateral coefficient supported yet
  coeff.x=0;
  
  // Application center
  sprintf(buf,"%s.center",path);
  info->GetFloats(buf,3,(float*)&center);
  
  // Application angle
  sprintf(buf,"%s.angle",path);
  angle=info->GetFloat(buf)/RR_RAD2DEG;    // Convert to radians
  // Angle offset makes sure that a 0-degree wing still delivers forces
  sprintf(buf,"%s.angle_offset",path);
  angleOffset=info->GetFloat(buf)/RR_RAD2DEG;
  // Add it to the angle for simplicity in calculations
  angle+=angleOffset;

  PreCalc();
  
  return TRUE;
}
void RWing::PreCalc()
// Precalculate some things to speed up calculations later
{
  // Constant factor for the aero drag/downforce. Notice
  // the negation, as downforce pushes down (Y-) and drag pushes
  // in the Z- direction. Using negative coefficients in the car.ini
  // files though seems worse.
  fixedFactorY=-span*cord*coeff.y*angle*RMGR->scene->env->GetAirDensity();
  fixedFactorZ=-span*cord*coeff.z*angle*RMGR->scene->env->GetAirDensity();
}

/***************
* State saving *
***************/
bool RWing::LoadState(QFile *f)
{
  // Nothing needs to be loaded for the wing
  return TRUE;
}
bool RWing::SaveState(QFile *f)
{
  // Nothing needs to be saved for the wing
  return TRUE;
}

/**********
* Physics *
**********/
void RWing::CalcForces()
{
  float airVelSquared;
  
  // Get the relative air velocity
  // Take the linear velocity in body coordinates, and ignore
  // rotation velocity effects for now, which are relavitely
  // small at high (linear) speed (and at low linear speed,
  // the aerodynamic effects because of the rotation are small).
  airVelocity=*car->GetBody()->GetLinVelBC();
  //car->ConvertWorldToCarCoords(car->GetVelocity(),&airVelocity);
//car->GetVelocity()->DbgPrint("carVel");
//airVelocity.DbgPrint("air Vel");
  
  // Calculate results on wing
  airVelSquared=airVelocity.z*airVelocity.z;
  // Drag
  force.z=fixedFactorZ*airVelSquared;
  // Downforce
  force.y=fixedFactorY*airVelSquared;
  
  // No torque used yet, this is implicit when the force is applied
  //torque.SetToZero();
}
void RWing::CalcAccelerations()
{
#ifdef OBS
qdbg("RWing:CalcAccelerations()\n");
//torque.DbgPrint("torque");
force.DbgPrint("force");
center.DbgPrint("center");
#endif
  car->GetBody()->AddBodyForceAtBodyPos(&force,&center);
}
void RWing::Integrate()
{
}
