/*
 * RBody - the sprung mass
 * 05-08-00: Created! (16:48:45)
 * 10-12-00: Model support
 * 16-12-00: Default values to get a usable default body.
 * 03-02-01: Completely revised state; RBody is now a RRigidBody.
 * NOTES:
 * - The body is the centerpoint of the physics; the wheels
 * - ND_XXX should be reinstalled before release.
 * more or less just hang on the body.
 * (C) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/intersect.h>
#ifdef RR_ODE
// Testing
//#include <drawstuff/drawstuff.h>
#endif
DEBUG_ENABLE

// Local module trace?
//#define LTRACE

// The penetration depth at which Racer ignores the contact point.
// This happens when the car hits 2 triangles at the same spot, for example.
// An example is a fence with 2 polygons, 1 at each side. The car will
// get stuck, colliding with each polygon, toggling at every step.
// To avoid this, big penetration depths are declared to be a result
// of hits with polygons that aren't true. ODE allows penetration to occur,
// so that's why this is an issue.
#define BAD_POLYGON_THRESHOLD   0.5

// Damp rotations?
//#define DO_DAMP

#define DEF_LOCK  10
#define DEF_RADIUS  .20
#define DEF_XA      20

#undef  DBG_CLASS
#define DBG_CLASS "RBody"

/************
* Debugging *
************/
static void CHK_BOX(DBoundingBox *bbox)
{
#ifdef OBS
  DVector3 size;
  void *p1,*p2;
  bbox->GetBox()->GetSize(&size);
size.DbgPrint("CHK_BOX: extents");
qdbg("  bbox=%p, GetBox=%p\n",bbox,bbox->GetBox());
p1=malloc(17+28); qdbg("  next malloc addr=%p\n",p1); free(p1);
p1=calloc(17+28,1); qdbg("  next calloc addr=%p\n",p1); free(p1);
p2=qstrdup("camera0.offset.x");
qdbg("  next strdup addr=%p\n",p2); qfree(p2);
p2=qcalloc(17);qdbg("  next qcalloc addr=%p\n",p2); qfree(p2);
#endif
}

/*********
* C/DTOR *
*********/
RBody::RBody(RCar *_car)
  : RRigidBody()
{
  DBG_C("ctor")
  
  // Init rigid body
  SetTimeStep(RR_TIMESTEP);
  
  car=_car;
  cockpitZ=cockpitLength=0;
  quad=0;
  // Show all the car's bounding boxes if desired
  if(RMGR->IsDevEnabled(RManager::SHOW_CAR_BBOX))
    flags=DRAW_BBOX;
  else
    flags=0;
  coeffRestitution=1;
  size.SetToZero();
  vCollision.SetToZero();
  force.SetToZero();
  forceGravityWC.SetToZero();
  forceGravityCC.SetToZero();
  forceDragCC.SetToZero();
  aeroCenter.SetToZero();
  aeroCoeff.SetToZero();
  aeroArea=0;
  aabb.Clear();
  model=new RModel(car);
  modelIncar=new RModel(car);
  bbox=new DBoundingBox();
  modelBraking[0]=new RModel(car);
  modelBraking[1]=new RModel(car);
  modelLight[0]=new RModel(car);
  modelLight[1]=new RModel(car);
  
  Reset();
//qdbg("car.susp=%d\n",RMGR->infoDebug->GetInt("car.suspension",1));
//qdbg("devFlags=%d\n",RMGR->devFlags);
#ifdef OBS_IN_RMANAGER
  if(!RMGR->infoDebug->GetInt("car.suspension",1))
    RMGR->devFlags|=RManager::NO_SUSPENSION;
  if(!RMGR->infoDebug->GetInt("car.gravity_body",1))
    RMGR->devFlags|=RManager::NO_GRAVITY_BODY;
#endif
//qdbg("devFlags=%d\n",RMGR->devFlags);
}
RBody::~RBody()
{
  DBG_C("dtor")
  Destroy();
  // Delete permanently allocated variables
  QDELETE(bbox);
}

void RBody::Destroy()
// Destroy associated objects
// Doesn't kill the RBody object itself, but just members
// like the model and quadric. Only 'delete body' will remove
// things.
{
  DBG_C("Destroy")
  if(quad){ gluDeleteQuadric(quad); quad=0; }
  QDELETE(model);
  QDELETE(modelIncar);
  QDELETE(modelBraking[0]);
  QDELETE(modelBraking[1]);
  QDELETE(modelLight[0]);
  QDELETE(modelLight[1]);
}

/*******
* Dump *
*******/
bool RBody::LoadState(QFile *f)
{
  if(!RRigidBody::LoadState(f))return FALSE;
  
  // OBB?
#ifdef OBS
  f->Read(&force,sizeof(force));
  f->Read(&forceGravityWC,sizeof(forceGravityWC));
  f->Read(&forceGravityCC,sizeof(forceGravityCC));
  f->Read(&forceDragCC,sizeof(forceDragCC));
#endif
  return TRUE;
}
bool RBody::SaveState(QFile *f)
{
  if(!RRigidBody::SaveState(f))return FALSE;
  
  // OBB?
#ifdef OBS
  f->Write(&force,sizeof(force));
  f->Write(&forceGravityWC,sizeof(forceGravityWC));
  f->Write(&forceGravityCC,sizeof(forceGravityCC));
  f->Write(&forceDragCC,sizeof(forceDragCC));
#endif
  return TRUE;
}

void RBody::Reset()
{
  // Signal start wishes to rigid body class
  rotVel.x=RMGR->infoDebug->GetFloat("car.av_pitch");
  rotVel.y=RMGR->infoDebug->GetFloat("car.av_yaw");
  rotVel.z=RMGR->infoDebug->GetFloat("car.av_roll");
  linVel.SetX(RMGR->infoDebug->GetFloat("car.vx"));
  linVel.SetY(RMGR->infoDebug->GetFloat("car.vy"));
  linVel.SetZ(RMGR->infoDebug->GetFloat("car.vz"));
#ifdef RR_ODE
  // Pass on to ODE body
  dBodySetLinearVel(odeBody,linVel.x,linVel.y,linVel.z);
  dBodySetAngularVel(odeBody,rotVel.x,rotVel.y,rotVel.z);
#endif

#ifdef FUTURE
  rotation.SetX(RMGR->infoDebug->GetFloat("car.rotate_x")/RR_RAD_DEG_FACTOR);
  rotation.SetY(RMGR->infoDebug->GetFloat("car.rotate_y")/RR_RAD_DEG_FACTOR);
  rotation.SetZ(RMGR->infoDebug->GetFloat("car.rotate_z")/RR_RAD_DEG_FACTOR);
#endif
}

/**********
* Loading *
**********/
bool RBody::Load(QInfo *info,cstring path)
// Default path is "body" (if 'path'==0)
{
  DBG_C("Load")
  char buf[128];
  DVector3 position,vec;
  rfloat r;
  
  if(!path)path="body";
  
  Destroy();

  // Location (=car really)
  position.SetToZero();
  linPos.SetToZero();
#ifdef OBS
  sprintf(buf,"%s.x",path);
  position.x=info->GetFloat(buf);
  sprintf(buf,"%s.y",path);
  position.y=info->GetFloat(buf,1.0f);
  sprintf(buf,"%s.z",path);
  position.z=info->GetFloat(buf);
  linPos=position;
#endif
  
  // Mass and inertia matrix
  sprintf(buf,"%s.mass",path);
  r=info->GetFloat(buf,750.f);
  // Inertia in 3 dimensions
  sprintf(buf,"%s.inertia.x",path);
  vec.x=info->GetFloat(buf);
  sprintf(buf,"%s.inertia.y",path);
  vec.y=info->GetFloat(buf);
  sprintf(buf,"%s.inertia.z",path);
  vec.z=info->GetFloat(buf);

  // Minimal mass (otherwise it will get infinite acceleration)
  if(r<0.1f)r=0.1f;
  // Set mass using RRigidBody function, since this also
  // calculates 1/mass for faster calculations. Add all fixed things
  // to the body.
#ifdef RR_ODE
  // Set all at once
  SetMassInertia(r+car->GetEngine()->GetMass(),vec.x,vec.y,vec.z);
#else
  SetMass(r+car->GetEngine()->GetMass());
  SetInertia(vec.x,vec.y,vec.z);
#endif
 
  // Aerodynamics
  //sprintf(buf,"aero.body.center",path);
  info->GetFloats("aero.body.center",3,(float*)&aeroCenter);
  aeroCoeff.x=0;
  aeroCoeff.y=0;
  //sprintf(buf,"%s.aero.body.cx",path);
  aeroCoeff.z=info->GetFloat("aero.body.cx");
  //sprintf(buf,"%s.aero.body.area",path);
  aeroArea=info->GetFloat("aero.body.area");

  // Collisions
  sprintf(buf,"%s.restitution_coeff",path);
  coeffRestitution=info->GetFloat(buf);
  
  // Size
  sprintf(buf,"%s.width",path);
  size.x=info->GetFloat(buf,1.0f);
  sprintf(buf,"%s.height",path);
  size.y=info->GetFloat(buf,0.4f);
  sprintf(buf,"%s.length",path);
  size.z=info->GetFloat(buf,3.0f);

  sprintf(buf,"%s.cockpit_start",path);
  cockpitZ=info->GetFloat(buf,1.0f);
  sprintf(buf,"%s.cockpit_length",path);
  cockpitLength=info->GetFloat(buf,1.0f);

  // Models
  model=new RModel(car);
  model->Load(info,path);
  if(!model->IsLoaded())
  { quad=gluNewQuadric();
  } else
  {
    // Automatically calculate ACTUAL size
    DBox box;
    DVector3 realSize;
size.DbgPrint("Body indicated size");
    model->GetGeode()->GetBoundingBox(&box);
    box.GetSize(&realSize);
realSize.DbgPrint("Body model-calc'ed size");
    // Use calculated size, or manual one?
    // Default is to use a calculated box, to maintain
    // compatibility with v0.4.5 and below.
    // The manual box is useful to avoid too many collisions
    // with the track, which may keep the car inside the track (!).
    bbox->GetBox()->min=box.min;
    bbox->GetBox()->max=box.max;
    sprintf(buf,"%s.manual_box",path);
    if(info->GetInt(buf))
    {
      // Generate a bounding box; the center remains the same
      // as with a calculated box, the size is changed though
      DVector3 center;
      bbox->GetBox()->GetCenter(&center);
      bbox->GetBox()->min.x=center.x-size.x/2;
      bbox->GetBox()->min.y=center.y-size.y/2;
      bbox->GetBox()->min.z=center.z-size.z/2;
      bbox->GetBox()->max.x=center.x+size.x/2;
      bbox->GetBox()->max.y=center.y+size.y/2;
      bbox->GetBox()->max.z=center.z+size.z/2;
    } else
    {
      size=realSize;
    }
//box.min.DbgPrint("box.min");
//box.max.DbgPrint("box.max");
#ifdef RR_ODE
    // Use the bounding box to create a geometry object (for ODE collisions)
    // Note that ODE uses full side lengths.
    //odeGeom=dCreateBox(RMGR->odeSpace,size.x/2.0,size.y/2.0,size.z/2.0);
    odeGeom=dCreateBox(RMGR->odeSpace,size.x,size.y,size.z);
    dGeomSetBody(odeGeom,odeBody);
#endif
  }
  bbox->EnableCSG();
#ifdef FUTURE
  bbox->GetBox()->size.x=size.x;
  bbox->GetBox()->size.y=size.y;
  bbox->GetBox()->size.z=size.z;
#endif
  
  // Incar model (for dashboard views)
  modelIncar=new RModel(car);
  modelIncar->Load(info,path,"model_incar");

  // Braking
  modelBraking[0]=new RModel(car);
  modelBraking[0]->Load(info,path,"model_braking_l");
  modelBraking[1]=new RModel(car);
  modelBraking[1]->Load(info,path,"model_braking_r");
  modelLight[0]=new RModel(car);
  modelLight[0]->Load(info,path,"model_light_l");
  modelLight[1]=new RModel(car);
  modelLight[1]->Load(info,path,"model_light_r");

  return TRUE;
}

/**********
* Attribs *
**********/
void RBody::DisableBoundingBox()
{
  flags&=~DRAW_BBOX;
}
void RBody::EnableBoundingBox()
// When called, this will draw a bounding box together with the model
{
  flags|=DRAW_BBOX;
}

/********
* Paint *
********/
#ifdef RR_GFX_OGL
void gluCube(float wid,float hgt,float dep,bool fWire=FALSE)
// Draws a cube at Z=0 to Z=dep
{
  if(fWire)
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glBegin(GL_QUADS);
    // Top
    glNormal3d(0,1,0);
    glVertex3d(-wid/2,hgt/2,dep);
    glVertex3d(wid/2,hgt/2,dep);
    glVertex3d(wid/2,hgt/2,0);
    glVertex3d(-wid/2,hgt/2,0);
    // Back
    glNormal3d(0,0,1);
    glVertex3d(-wid/2,hgt/2,dep);
    glVertex3d(-wid/2,-hgt/2,dep);
    glVertex3d(wid/2,-hgt/2,dep);
    glVertex3d(wid/2,hgt/2,dep);
    // Front
    glNormal3d(0,0,-1);
    glVertex3d(-wid/2,hgt/2,0);
    glVertex3d(wid/2,hgt/2,0);
    glVertex3d(wid/2,-hgt/2,0);
    glVertex3d(-wid/2,-hgt/2,0);
    // Left
    glNormal3d(1,0,0);
    glVertex3d(wid/2,hgt/2,0);
    glVertex3d(wid/2,hgt/2,dep);
    glVertex3d(wid/2,-hgt/2,dep);
    glVertex3d(wid/2,-hgt/2,0);
    // Right
    glNormal3d(-1,0,0);
    glVertex3d(-wid/2,hgt/2,dep);
    glVertex3d(-wid/2,hgt/2,0);
    glVertex3d(-wid/2,-hgt/2,0);
    glVertex3d(-wid/2,-hgt/2,dep);
    // Bottom
    glNormal3d(0,-1,0);
    glVertex3d(-wid/2,-hgt/2,dep);
    glVertex3d(-wid/2,-hgt/2,0);
    glVertex3d(wid/2,-hgt/2,0);
    glVertex3d(wid/2,-hgt/2,dep);
  glEnd();

  // Restore state to default
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}
#endif

void RBody::PaintSetup()
// Setup the body orientation for the body parts that will follow.
// The ordering of the body parts can be varied more easily now that
// this orientation setup is separated from actually painting the body
// immediately.
{
  DBG_C("PaintSetup")
  //int i;
  //rfloat len;
  //float colBody[]={ 1,.2,.1 };

#ifdef RR_GFX_OGL
//qdbg("RBody:PaintSetup()\n");
//linPos.DbgPrint("linPos");

#ifdef RR_ODE
  const float *pos=dBodyGetPosition(odeBody);
  //glTranslatef(pos[0],pos[1],pos[2]);
#else
  glTranslatef(linPos.GetX(),linPos.GetY(),linPos.GetZ());
#endif

  // Get rotation matrix from the quaternion's derived 3x3 orientation
  DMatrix4 m4;
#ifdef RR_ODE

#define ND_TRANSPOSED
#ifdef ND_TRANSPOSED
  // Get from ODE's rigid body (note that it uses a 4x3 rotation matrix)
  const float *r3;
  float *pm4;
  m4.SetIdentity();
  r3=dBodyGetRotation(odeBody);
  pm4=m4.GetM();
  // Transposed
  pm4[0*4+0]=r3[0*4+0];
  pm4[0*4+1]=r3[1*4+0];
  pm4[0*4+2]=r3[2*4+0];
  //pm4[0*4+3]=r3[3*4+0];
  pm4[1*4+0]=r3[0*4+1];
  pm4[1*4+1]=r3[1*4+1];
  pm4[1*4+2]=r3[2*4+1];
  //pm4[1*4+3]=r3[3*4+1];
  pm4[2*4+0]=r3[0*4+2];
  pm4[2*4+1]=r3[1*4+2];
  pm4[2*4+2]=r3[2*4+2];
  //pm4[2*4+3]=r3[3*4+2];
/*
  pm4[3*4+0]=r3[0*4+3];
  pm4[3*4+1]=r3[1*4+3];
  pm4[3*4+2]=r3[2*4+3];
  //pm4[3*4+3]=r3[3*4+3];
*/
  pm4[3*4+0]=pos[0];
  pm4[3*4+1]=pos[1];
  pm4[3*4+2]=pos[2];
//qdbg("RBody:Paint(); pos=%f/%f/%f\n",pos[0],pos[1],pos[2]);
#endif

#ifdef ND
  // Non-transposed
  pm4[0*4+0]=r3[0*4+0];
  pm4[0*4+1]=r3[0*4+1];
  pm4[0*4+2]=r3[0*4+2];
  //pm4[0*4+3]=r3[3*4+0];
  pm4[1*4+0]=r3[1*4+0];
  pm4[1*4+1]=r3[1*4+1];
  pm4[1*4+2]=r3[1*4+2];
  //pm4[1*4+3]=r3[3*4+1];
  pm4[2*4+0]=r3[2*4+0];
  pm4[2*4+1]=r3[2*4+1];
  pm4[2*4+2]=r3[2*4+2];
  //pm4[2*4+3]=r3[3*4+2];
  //pm4[3*4+0]=r3[3*4+3];
  //pm4[3*4+1]=r3[3*4+3];
  //pm4[3*4+2]=r3[3*4+3];
  //pm4[3*4+3]=r3[3*4+3];
#endif
#ifdef ND_VANILLA
  for(i=0;i<4*3;i++)
    pm4[i]=r3[i];
#endif
  // Direct copy; mRotPos is Racer specific (right hand coord system) and
  // already converted from ODE
  //m4.FromMatrix3(&mRotPos);
#else
  // Racer's original
//qdbg("RBody:Paint(); FromMatrix3()\n");
  m4.FromMatrix3(&mRotPos);
#endif

#ifdef OBS
//CreateMatrix4FromMatrix3(mRotPos.GetM(),m4.GetM());
mRotPos.DbgPrint("mRotPos");
m4.DbgPrint("m4 for OpenGL");
#endif

  glMultMatrixf(m4.GetM());
#endif
}

void RBody::Paint()
// Paint the body.
// Assumes PaintSetup() has already been called to set the right orientation.
{
  DBG_C("Paint")

//qdbg("RBody:Paint()\n");

  rfloat len;
  float colBody[]={ 1.0f,0.2f,0.1f };

#ifdef OBS
  // Visual forces
  DVector3 v;
  glTranslatef(0,2,.01);
  v=forceGravityCC;
//qdbg("RB:Pnt; grav=%f\n",v.y);
  RGfxVector(&v,.001,0,1,1);
  glTranslatef(0,-2,-.01);
#endif

  // Which model to show?
  RCamera *cam=RMGR->scene->GetCamCar();
  RModel  *theModel;
  // We have a car camera and car camera mode is on?
  if(cam!=0&&RMGR->scene->GetCamMode()==RScene::CAM_MODE_CAR)
  {
    // Which model to paint?
    if(cam->flags&RCamera::INCAR)
    {
      // We're inside the car
      theModel=modelIncar;
    } else if(cam->flags&RCamera::NOCAR)
    {
      // We're completely outside; don't draw it at all!
      goto skip_model;
    } else
    {
      // Outside the car; show entire model
      theModel=model;
    }
  } else
  {
    // Use outside model
    theModel=model;
  }
  if(theModel!=0&&theModel->IsLoaded())
  {
    // Try to match sphere envmap
    extern dfloat d3Yaw;
    rfloat yaw;
    yaw=atan2f(mRotPos.GetRC(2,0),mRotPos.GetRC(2,2));
    //a-=car->GetBody()->GetYaw()*RR_RAD_DEG_FACTOR;
    d3Yaw=yaw*RR_RAD_DEG_FACTOR;
    theModel->Paint();
  } else
  {
#ifdef RR_GFX_OGL
   //paint_stub:
    // Remember this as the geometric center of the body
    glPushMatrix();
  
    // Stub graphics
    glMaterialfv(GL_FRONT,GL_DIFFUSE,colBody);
    // Back part
    glTranslated(0,0,-size.z/2);
    gluCube(size.x,size.y,cockpitZ);
    // Cockpit part is half as high
    glTranslated(0,-size.y/4,cockpitZ);
    gluCube(size.x*0.8,size.y/2,cockpitLength);
    // Front part
    len=size.z-cockpitZ-cockpitLength;
    glTranslated(0,size.y/4,cockpitLength);
    gluCube(size.x*1.0,size.y,len);
    // Hierarchical objects coming up (using center of geometry)
    glPopMatrix();
#endif
  }

  // Paint brake lights
  if(car->GetBrakes()>0)
  {
    modelBraking[0]->Paint();
    modelBraking[1]->Paint();
  }
  // Paint front lights
  modelLight[0]->Paint();
  modelLight[1]->Paint();

 skip_model:

  // Paint the dials
  if(car->GetViews()->GetView(1))
    car->GetViews()->GetView(1)->Paint3D();

  // Bounding box (debugging)
  if(flags&DRAW_BBOX)
    bbox->Paint();
}

/**********
* Animate *
**********/
void RBody::Integrate()
// Calculate new rigid body state
{
//totalForce.x=totalForce.z=0;  // $DEV
#ifdef OBS
totalForce.SetToZero();    // No linear moves $DEV
totalTorque.x=totalTorque.z=0;
#endif
//totalForce.DbgPrint("RBody:Integrate; totalForce");
//totalTorque.DbgPrint("RBody:Integrate; totalTorque");
  //totalTorque.SetToZero();
  IntegrateEuler();
#ifdef OBS_QUAT
  // Accelerate vehicle
  velocity+=RR_TIMESTEP*acceleration;
  translation=RR_TIMESTEP*velocity;
  position+=translation;
#endif

#ifdef OBS
  // Pass on wheel position change to suspension
  if(!susp->ApplyWheelTranslation(&translation,&velocity))
  { // Can't go there physically; correct
    position+=translation;
  }
#endif
}

/**********
* Physics *
**********/
void RBody::PreAnimate()
{
  IntegrateInit();
#ifdef DO_DAMP
  // Damping
  rfloat damp;
  damp=0.8f;
  damp=-damp;
  totalForce.x=damp*GetMass()*linVel.x;
  totalForce.y=damp*GetMass()*linVel.y;
  totalForce.z=damp*GetMass()*linVel.z;
  totalTorque.x=damp*inertiaTensor.GetRC(0,0)*rotVel.x;
  totalTorque.y=damp*inertiaTensor.GetRC(1,1)*rotVel.y;
  totalTorque.z=damp*inertiaTensor.GetRC(2,2)*rotVel.z;
#ifdef OBS
  totalTorque.x=damp*GetMass()*rotVel.x;
  totalTorque.y=damp*GetMass()*rotVel.y;
  totalTorque.z=damp*GetMass()*rotVel.z;
#endif
#endif
}

/***************************
* Calculate forces/torques *
***************************/
void RBody::CalcForces()
{
  // Gravity
  forceGravityWC.x=0;
  forceGravityWC.y=-GetMass()*RMGR->scene->env->GetGravity();
  forceGravityWC.z=0;

#ifdef LTRACE
qdbg("GetMass()=%f, gravity=%f\n",GetMass(),RMGR->scene->env->GetGravity());
forceGravityWC.DbgPrint("forceGravityWC RBody:CalcForces()");
#endif

#ifdef OBS
  // Into car coordinates
  car->ConvertWorldToCarOrientation(&forceGravityWC,&forceGravityCC);
qdbg("Gravity: WC=(%.2f,%.2f,%.2f), CC=(%.2f,%.2f,%.2f)\n",
forceGravityWC.x,forceGravityWC.y,forceGravityWC.z,
forceGravityCC.x,forceGravityCC.y,forceGravityCC.z);
#endif

  // Drag
  // Note that linVelBC is used directly as the reverse air velocity
  // (airVelocity=-GetLinVelBC()) and is not stored explicitly.
  // Also note that the -z*fabs(z) makes sure the force points
  // in the opposite direction of the Z velocity.
  // Sideways and vertical drag isn't supported yet.
  forceDragCC.x=forceDragCC.y=0;
  forceDragCC.z=0.5f*RMGR->scene->env->GetAirDensity()*
    aeroCoeff.z*aeroArea*-GetLinVelBC()->z*fabs(GetLinVelBC()->z);
}

/**************************
* Applying forces/torques *
**************************/
void RBody::CalcAccelerations()
{
  int i;
  //double ax,ay,az;
  //double accMag;
  DVector3 force,forceWC;
  
  // Linear acc.
  
  force.SetToZero();
  //
  // VERTICAL forces (Y)
  //
  for(i=0;i<car->GetWheels();i++)
  {
    // Force at suspension i
    force+=*car->GetSuspension(i)->GetForceBody();
#ifdef OBS
car->GetSuspension(i)->GetForceBody()->DbgPrint("susp->body force");
qdbg("RBody: susp%d force.y=%f\n",
i,car->GetWheel(i)->GetSuspension()->GetForceBody()->y);
#endif
  }

#ifdef OBS
  // Gravity
  if(!(RMGR->devFlags&RManager::NO_GRAVITY_BODY))
    force+=forceGravityWC;
#endif

#ifdef OBS
qdbg("  gravity force.y=%f\n",forceGravityWC.y);
qdbg("RB:AF; sum(suspForces)=(%f,%f,%f)\n",
force.x,force.y,force.z);
#endif

  // LATERAL and LONGITUDINAL (XZ) forces (in the ground plane)
  // Sum of all wheel forces (translational), in car coords
  for(i=0;i<car->GetWheels();i++)
  {
    force.x+=car->GetWheel(i)->GetForceBodyCC()->x;
    force.z+=car->GetWheel(i)->GetForceBodyCC()->z;
//car->GetWheel(i)->GetForceBodyCC()->DbgPrint("wheel.forceBodyCC");
  }
#ifdef OBS
qdbg("RB:AF; sum(latLongForces)=(%f,%f,%f)\n",
force.x,force.y,force.z);
#endif

  // Acceleration is in the world coordinate system
  car->ConvertCarToWorldOrientation(&force,&forceWC);
 
#ifdef LTRACE
force.DbgPrint("force RBody:CalcAccelerations");
forceWC.DbgPrint("forceWC RBody:CalcAccelerations");
#endif

  // Add gravity (simpler in world coordinates)
  if(!(RMGR->devFlags&RManager::NO_GRAVITY_BODY))
    forceWC+=forceGravityWC;

  // Total acceleration at CG in world coordinates
  // (future: add force separately, let RRigidBody add the forces)
  DVector3 pos(0,0,0);
  AddWorldForceAtBodyPos(&forceWC,&pos);
#ifdef LTRACE
forceWC.DbgPrint("forceWC total");
qdbg("RBody:AF() net forceWC=(%f,%f,%f)\n",forceWC.x,forceWC.y,forceWC.z);
//forceGravityWC.DbgPrint("forceGravityWC");
#endif

  // Aero DRAG force (a LONGITUDINAL force applied at some point)
//qdbg("RBody:AF(); forceDragCC=%f\n",forceDragCC.z);
//aeroCenter.DbgPrint("aeroCenter");
//aeroCenter.x=1000;
  AddBodyForceAtBodyPos(&forceDragCC,&aeroCenter);
}

void RBody::ApplyRotations()
// See what the forces are doing to the torques
// The torques make the car rotate around its CM
// FUTURE: project force onto the perpendicular line moving around
// the circle from CM to wheel center. Now the lateral force
// is just taken as if perpendicular.
{
  DVector3 *force,forceCar;
  DVector3 torque;
  //rfloat r,v;
  int    i;
  RWheel *w;
  
  torque.SetToZero();

//qdbg("RCar:ApplyRotations\n");
  for(i=0;i<car->GetWheels();i++)
  {
    w=car->GetWheel(i);

    // Yaw moment/torque
    //ConvertWorldToCarCoords(w->GetForceBody(),&forceCar);
    forceCar=*w->GetForceBodyCC();
//w->GetForceRoadTC()->DbgPrint("forceRoadTC (RBody:AR)");
    // To get the torque around the car's CM, we could project the lateral
    // force to be perpendicular to the circle with centerpoint CM
    // and radius r (distance from CM to wheel)
    // However, a 2nd way to express torque is to project 'r' itself, and
    // leave F intact. Since the LATERAL force is always perpendicular
    // to the Z axis, we can use the alternative:
    //   torque=F*wheel_z
    // Remember: torque also is rot_intertia*ang.acc, so:
    //   ang.acc.=F*wheel_z/rot_inertia
    // That works out much more elegantly, although in the other case
    // we are using cosines that stay constant, so not much extra work
    // would be needed in fact. But this is simpler.
    // 04-02-01; keeping it at torque to let RRigidBody do
    // the inertia right.
    torque.y+=forceCar.x*w->GetSuspension()->GetZ();
#ifdef OBS
qdbg("RBody:AR; LAT forceCar.x=%f,suspZ=%f, torque.y+=%f\n",
forceCar.x,w->GetSuspension()->GetZ(),forceCar.x*w->GetSuspension()->GetZ());
#endif
    // Same goes for longitudinal forces, only these are
    // projected so that we need the wheel_x coordinate
    torque.y+=-forceCar.z*w->GetSuspension()->GetX();
#ifdef OBS
qdbg("RBody:AR; LONG forceCar.z=%f,suspX=%f, torque.y+=%f\n",
forceCar.z,w->GetSuspension()->GetX(),-forceCar.z*w->GetSuspension()->GetX());
#endif

    if(RMGR->devFlags&RManager::NO_SUSPENSION)goto skip_pitch;

    // Pitch torque
    // Upward force because of surface normal force
    //force=w->GetForceRoadTC();
    force=w->GetForceBodyCC();
    torque.x-=force->y*w->GetPosition()->z;
    // Acceleration force applied at CG
    // The application point is probably not right, but should
    // use some suspension point somewhere instead.
    // Also, the CG is not really constant as the car pitches
    // and rolls.
    torque.x-=force->z*car->GetCGHeight();
//qdbg("RBody:AA; torque.x+=%f, force_y=%f, whl_z=%f\n",
//force->y*w->GetPosition()->z,force->y,w->GetPosition()->z);
//qdbg("RBody:AA; torque.x+=%f, force_z=%f, cg hgt=%f\n",
//force->z*car->GetCGHeight(),force->z,car->GetCGHeight());

   skip_pitch:;

    // Roll torque
    //
    // Rolling moment because of suspension upward force
    force=w->GetForceBodyCC();
    torque.z+=force->y*w->GetPosition()->x;

    // Rolling moment because of lateral forces (jacking?)
    // Note we should use the roll center of the wheel, and NOT
    // the CG.
    force=w->GetForceBodyCC();
    torque.z+=force->x*
      (car->GetCGHeight()-car->GetSuspension(i)->GetRollCenter()->y);
  }

  // Add a torque in body coordinates
//torque.DbgPrint("  body torque");
  AddBodyTorque(&torque);
}

/**********************
* Collision detection *
**********************/
// Global parameter passing between cbCarColl() and RBody::DetectCollisions()
static RCar *cCar;                // Input car
static DVector3 normal,contact;   // Output collision normal and contact point
#ifndef RR_ODE
static bool fHit;                 // Indicates whether a hit was found
#endif
static DVector3 boxOffsetCC;      // Offsetting of ODE's view of the bbox

bool cbCarColl(DAABBPolyInfo *pi)
// Gets called through query of the car's AABB with the track AABBTree.
// When called, this is just a hint that a part of the track is close to the
// AABB of the car. The OBB-triangle intersection will determine
// whether a hit is really happening.
{
  //RTreePolyInfo *rpi=(RTreePolyInfo*)pi->userData;
  //rfloat t,u,v;
//qdbg("cbCarColl\n");
  DVector3 *linVel,v0,inter;
  dfloat t;
  DOBB *obb=cCar->GetBody()->GetOBB();

  v0.SetToZero();
  linVel=cCar->GetVelocity();
//RMGR->profile->AddCount(0);
  if(d3FindTriObb(RR_TIMESTEP,(DVector3**)pi->v,v0,
    *obb,*linVel,t,inter)==0)
  {
//qdbg("Collision!\n");
//obb->GetCenter()->DbgPrint("obb center");
//obb->GetExtents()->DbgPrint("obb extents");

    // Collision between car and track!
    DVector3 e[2];

    // Calculate edges of (track) triangle
    e[0].x=pi->v[1][0]-pi->v[0][0];
    e[0].y=pi->v[1][1]-pi->v[0][1];
    e[0].z=pi->v[1][2]-pi->v[0][2];
    e[1].x=pi->v[2][0]-pi->v[0][0];
    e[1].y=pi->v[2][1]-pi->v[0][1];
    e[1].z=pi->v[2][2]-pi->v[0][2];
    normal.Cross(&e[0],&e[1]);

#ifdef RR_ODE

#ifdef ND_SKIPS_TOO_MANY_COLLISIONS
    // Check if car is moving away from triangle roughly.
    // Without this test, reverse oriented polygons (compare culling)
    // can make the car believe it is WAY inside, instead of just starting
    // to penetrate. This explodes the car's motion.
    if(normal.Dot(linVel)>0)
    {
//qdbg("Moving away roughly\n");
      // Search on, but discard this polygon
      return TRUE;
    }
#endif

    // Create contact points
    dContact contact[3];
    int i,n;
    for(i=0;i<3;i++)
    {
      contact[i].surface.mode=dContactBounce;
      //contact[i].surface.mu=dInfinity;
      contact[i].surface.mu=100.0f;
      contact[i].surface.mu2=0;
      contact[i].surface.bounce=cCar->GetBody()->GetCoeffRestitution();
      contact[i].surface.bounce_vel=0.01f;
    }

    // Create temporary collision plane (not efficient, but done for testing)
    normal.Normalize();
//normal.DbgPrint("normal");
    DVector3 *v=(DVector3*)&pi->v[0][0];
    DPlane3   plane(normal,*v);

    // Check if the center of the car is at the same side of the plane.
    // If not, this most probably is the wrong plane to collide with
    // (the car would be pulled THROUGH the triangle)
//qdbg("Distance of car center to plane = %f\n",
//plane.DistanceTo(cCar->GetPosition()));
    if(plane.ClassifyFrontOrBack(cCar->GetPosition())==DPlane3::BACK)
    {
//qdbg("Car classified as BACK of plane; discard\n");
      // Search on, but discard this polygon; the car is way to deep
      // in this; the polygon should have been culled.
      return TRUE;
    }

    // Create temporary plane; not that ODE uses a negated plane.d coeff.
    dGeomID gPlane=dCreatePlane(RMGR->odeSpace,plane.n.x,plane.n.y,
      plane.n.z,-plane.d);
    // Move the ODE body to correct for the fact that the center of mass
    // (=center of ODE box) is NOT equal to the center of geometry (to which
    // the collisions are checked).
    dBodyID bID=cCar->GetBody()->GetODEBody();
    const dReal *p=dBodyGetPosition(bID);
    dBodySetPosition(bID,
      p[0]+boxOffsetCC.x,p[1]+boxOffsetCC.y,p[2]+boxOffsetCC.z);
      //p[0]-boxOffsetCC.x,p[1]-boxOffsetCC.y,p[2]-boxOffsetCC.z);
    n=dCollide(cCar->GetBody()->GetODEGeom(),gPlane,3,
        &contact[0].geom,sizeof(dContact));
    // Restore body center position
    dBodySetPosition(bID,
      p[0]-boxOffsetCC.x,p[1]-boxOffsetCC.y,p[2]-boxOffsetCC.z);
      //p[0]+boxOffsetCC.x,p[1]+boxOffsetCC.y,p[2]+boxOffsetCC.z);
//plane.DbgPrint("plane of triangle");
//v->DbgPrint("point on tri");
//qdbg("Plane distance to v=%f\n",plane.DistanceTo(v));

//qdbg("%d contact pts\n",n);
    for(i=0;i<n;i++)
    {
      dContactGeom *p=&contact[i].geom;
//qdbg("Contact %d: %.2f,%.2f,%.2f normal %.2f,%.2f,%.2f, dep %.2f\n",
//i,p->pos[0],p->pos[1],p->pos[2],
//p->normal[0],p->normal[1],p->normal[2],p->depth);

      // Big depths are hopefully just the result of inverted normals.
      // So in this case, reject those points.
      // Ofcourse, the threshold value is some guesswork.
      if(p->depth>=BAD_POLYGON_THRESHOLD)
        continue;

//qdbg("  contact %d: depth %.4f\n",i,p->depth);
      dJointID c=dJointCreateContact(RMGR->odeWorld,RMGR->odeJointGroup,
        &contact[i]);
      dJointAttach(c,cCar->GetBody()->GetODEBody(),0);
    }
    // Remove plane geometry
    dGeomDestroy(gPlane);

//qdbg("Early exit\n");
//app->Exit(0);

#ifdef ND_SINGLE_CONTACT_POINT
    dContact contact;
    DVector3 vCenter;

    memset(&contact,0,sizeof(contact));
    contact.surface.mode=dContactBounce;
    contact.surface.mu=1.0;
    contact.surface.mu2=0;
    contact.surface.bounce=1.00;
    contact.surface.bounce_vel=0.1;
    contact.geom.normal[0]=normal.x;
    contact.geom.normal[1]=normal.y;
    contact.geom.normal[2]=normal.z;
    contact.geom.normal[3]=1;          // 0?
    // The contact point is still an issue; should get the center point
    // of the 'contact polygon' from the OBB entering the triangle/plane.
    pi->GetCenter(&vCenter);
    contact.geom.pos[0]=vCenter.x;
    contact.geom.pos[1]=vCenter.y;
    contact.geom.pos[2]=vCenter.z;
    contact.geom.pos[3]=1;             // 0?
    normal.Normalize();
//normal.DbgPrint("Collision normal");
//inter.DbgPrint("Intersection point");
    // Calculate penetration depth by intersecting the OBB with the
    // triangle's plane.
    DVector3 *v=(DVector3*)&pi->v[0][0];
    DPlane3 plane(normal,*v);
//v->DbgPrint("v on tri");
    t=d3FindOBBPlaneIntersection(cCar->GetBody()->GetOBB(),&plane,&inter);
    if(t==0)
    {
      // No penetration after all; continue searching other triangles.
qdbg("d3FindOBBPlane... finds no intersection.\n");
      return TRUE;
    }
    contact.geom.depth=t;
//qdbg("  depth=%f\n",t);

    // Create a contact that is attached to the body and the world
    dJointID c=
      dJointCreateContact(RMGR->odeWorld,RMGR->odeJointGroup,&contact);
    dJointAttach(c,cCar->GetBody()->GetODEBody(),0);
#endif

#else
    // Return a contact point if the OBB is *entering* the triangle only.

    // Check if the OBB isn't LEAVING the triangle
    if(normal.Dot(*linVel)>0)
    {
      // We're moving away (linearly at least), so don't
      // record this as a collision.
//qdbg("avoiding collision\n");
      return TRUE;
    }
    // Found a valid hit
    contact=inter;
    fHit=TRUE;
#endif

    if(n>0)
    {
      // We've got hits; now when the car hits a fence which is a double-
      // sided polygon (2 polygons), the 2 sides may generate contact
      // points that conflict (it may just be that I'm setting bounce_vel
      // totally wrong). For now though, we'll avoid extra contact points
      // once some contact points are already found. Will also improve
      // performance a bit, but perhaps conflicts arise in corners, where
      // 2 polygons are definitely hit at the same time.
      return FALSE;
    }
    // No need to search on (is it?)
    //return FALSE;
    // Search on for more hits
    return TRUE;
  }
  return TRUE;
}
void RBody::DetectCollisions()
// Detect collisions with track objects.
// Will create contact points in the future (ODE?)
{
  DVector3   centerBC;
  DAABBTree *trackTree;
  int        i,oldPart;

  oldPart=RMGR->profile->GetPart();
  RMGR->profile->SwitchTo(RProfile::PROF_CD_CARTRACK);

  // Prepare OBB for testing
  // Note this OBB creation can use some precalculations!
#define ND_NOT_THE_RIGHT_BODYPOS
#ifdef ND_NOT_THE_RIGHT_BODYPOS
  bbox->GetBox()->GetCenter(&centerBC);
  // Store this as the offset of the geometry center vs. the center of mass.
  // This is needed to patch up ODE's view of the bounding box later on in
  // the callback function.
  car->ConvertCarToWorldOrientation(&centerBC,&boxOffsetCC);
//centerBC.DbgPrint("centerBC");
/*{DVector3 size;
bbox->GetBox()->GetSize(&size);
size.DbgPrint("bbox size");
bbox->GetBox()->min.DbgPrint("bbox min");
bbox->GetBox()->max.DbgPrint("bbox max");
}*/
#ifdef RR_ODE
  // Translate to the corresponding body position, which then
  // is the center of the geometry's bounding box (which is a DIFFERENT
  // thing from the center of gravity, the center point of the car,
  // where all the forces relate to).
  // The ODE function doesn't exist yet in v0.025
  //dBodyGetPointPos(odeBody,centerBC.x,centerBC.y,centerBC.z,
    //(dVector3)&obb.center);
  car->ConvertCarToWorldCoords(&centerBC,&obb.center);
//obb.center.DbgPrint("obb.center (world coords)");
#else
  ConvertBodyToWorldPos(&centerBC,&obb.center);
#endif
#else
  // Get body center
  obb.center=linPos;
#endif
  
  bbox->GetBox()->GetSize(&obb.extents);
  // Make box twice as small because it uses l/2 lengths
  obb.extents.x*=0.5;
  obb.extents.y*=0.5;
  obb.extents.z*=0.5;
//obb.extents.DbgPrint("obb.extents");
  // Pass velocity (note: rotational velocity is not used!)
  obb.linVel=linVel;
  
  // Copy axes of OBB over
  for(i=0;i<3;i++)
  {
    obb.axis[i].x=mRotPos.GetRC(0,i);
    obb.axis[i].y=mRotPos.GetRC(1,i);
    obb.axis[i].z=mRotPos.GetRC(2,i);
  }

  // Prepare AABB for testing against the track's AABBTree
  // The AABB is created as quite a bad fit of the car's OBB. The advantage
  // is though that an algorithm to query the track with an AABB exists,
  // and this algorithm far outdoes brute force triangle checking. Most
  // of times, not one triangle is even checked.
  aabb.MakeFromOBB(&obb.center,&obb.extents,&mRotPos);
  if(RMGR->IsDevEnabled(RManager::NO_AABB_TREE))
    goto skip_aabb;
  trackTree=RMGR->GetTrackVRML()->GetTrackTree();
  if(!trackTree)goto skip_aabb;
  // Pass car on to callback
  cCar=car;
#ifndef RR_ODE
  fHit=FALSE;
#endif
  trackTree->Query(&aabb,cbCarColl);

#ifndef RR_ODE
  if(fHit)
  {
    // Resolve collision here; should create joint contact in the future,
    // in case multiple hits occur (and then some reasons!).
    // Calculate collision impulse magnitude (after Chris Hecker)
    dfloat j;
    j=( (-(1+coeffRestitution)*linVel).Dot(normal) ) /
      normal.Dot(GetOneOverMass()*normal);
    // Remember intersection point (debugging only really)
    vCollision=contact;
/*
qdbg("j=%f\n",j);
normal.DbgPrint("normal");
contact.DbgPrint("contact");
//linVel.DbgPrint("linVel PRE");
*/
    linVel=linVel+j*GetOneOverMass()*normal;
//linVel.DbgPrint("linVel POSTCOLL");
  } else
  {
    vCollision.SetToZero();
  }
#endif

 skip_aabb:
  RMGR->profile->SwitchTo(oldPart);

#ifdef OBS_USE_AABBTREE_NOT_BRUTE_FORCE
//return;
  if(RMGR->track->CollisionOBB(&obb,&inter,&normal))
  {
#ifdef OBS
inter.DbgPrint("collision point");
qdbg("Collision!\n");
normal.DbgPrint("normal of collision tri");
qdbg("  N dot V=%f (<0 is contact)\n",normal.Dot(linVel));
#endif

#ifdef ND_ALTERNATE_METHOD
    // Alternate collision response method from GameDev Mag, march 1999
    // Gives exact same results as the 'j=...' method below, which
    // doesn't need the normalization of the normal vector.
    {
      DVector3 Vn,Vt,nrm,Vnew;
      nrm=normal;
      normal.Normalize();
      Vn=normal.Dot(linVel)*normal;
      Vt=linVel; Vt.Subtract(&Vn);
      Vnew=Vt-coeffRestitution*Vn;
qdbg("GDMag:\n");
Vn.DbgPrint("Vn");
Vt.DbgPrint("Vt");
Vnew.DbgPrint("Vnew");
      // Restore for other method
      normal=nrm;
    }
#endif

    vCollision=inter;
    // The track is considered to have infinite mass
    // Calculate collision impulse magnitude
    dfloat j;
    j=( (-(1+coeffRestitution)*linVel).Dot(normal) ) /
      normal.Dot(GetOneOverMass()*normal);
//qdbg("j=%f\n",j);
//linVel.DbgPrint("linVel PRE");
    linVel=linVel+j*GetOneOverMass()*normal;
//linVel.DbgPrint("linVel POSTCOLL");
  } else
  {
    vCollision.SetToZero();
  }
#endif
}

/************
* Debugging *
************/
bool RBody::DbgCheck(cstring s)
{
qdbg("RBody:DbgCheck(%s)\n",s);
  CHK_BOX(bbox);
  return TRUE;
}
