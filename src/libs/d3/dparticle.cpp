/*
 * DParticle - particles & particle groups & particle manager
 * 07-10-01: Created!
 * (c) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/particle.h>
// Some compiler require stdlib.h explicitly for rand()
#include <stdlib.h>
#include <qlib/app.h>
DEBUG_ENABLE

/***********
* Particle *
***********/
DParticle::DParticle()
{
  Reset();
}
DParticle::~DParticle()
{
}
void DParticle::Reset()
{
  pos.SetToZero();
  vel.SetToZero();
  color.SetToZero();
  alpha=0;
  age=0;
  flags=0;
  lifeTime=0;
  energy=0;
  size.SetToZero();
  oldPos.SetToZero();
}

/*****************
* Particle group *
*****************/
DParticleGroup::DParticleGroup(int _type,int maxParticle)
{
  particleCount=maxParticle;
  particle=new DParticle[particleCount];

  // Defaults
  type=_type;
  gravity=9.81;
  texture=0;
  blendMode=ALPHA_BLEND;
}
DParticleGroup::~DParticleGroup()
{
  delete[] particle;
}

DParticle *DParticleGroup::GetFreeParticle()
// Returns a particle ready for use, or 0 if its all filled
{
  int i;

  for(i=0;i<particleCount;i++)
  {
    if(!particle[i].IsActive())
      return &particle[i];
  }
  // No free particles
  return 0;
}

void DParticleGroup::SetTexture(DTexture *tex)
{
  texture=tex;
}

static dfloat frand(int max)
{
  return (dfloat)((rand()%(max*100))/100);
}
void DParticleGroup::Spawn(int count)
// Spawn a number of particles
{
  int i;
  DParticle *p;

  for(i=0;i<count;i++)
  {
    p=GetFreeParticle();
    // If none found, break off here (out of space)
    if(!p)break;
    p->age=0;
    p->lifeTime=10;
    p->pos.SetToZero();
    if(type==SPARKS)
    {
      p->vel.x=frand(50)-25.0f;
      p->vel.y=frand(50);
      p->vel.z=frand(50)-25.0f;
    } else if(type==SMOKE)
    {
      p->vel.x=frand(50)-0.0f;
      p->vel.y=frand(35);
      p->vel.z=frand(60)-25.0f;
      p->lifeTime=4;
    }
    p->size.x=40;
    p->size.y=40;
    p->size.z=40;
    p->Revive();
  }
}
void DParticleGroup::Spawn(DVector3 *pos,DVector3 *vel,dfloat lifeTime)
// Smoke convenience
{
  DParticle *p=GetFreeParticle();
  // Any free particle?
  if(!p)return;
  p->age=0;
  p->lifeTime=lifeTime;
  p->pos=*pos;
  p->vel.x=vel->x;
  p->vel.y=frand(2);
  p->vel.z=vel->z;
  p->size.x=p->size.y=p->size.z=.2;       // Start size
  p->Revive();
} 

void DParticleGroup::KillAll()
// Kills all particles.
{
  int i;

  for(i=0;i<particleCount;i++)
    particle[i].Die();
}

void DParticleGroup::Update(dfloat dt)
// Update the particle state
{
  int i;
  DParticle *p;
  dfloat gravityTimesDT=gravity*dt;

  for(i=0;i<particleCount;i++)
  {
    if(!particle[i].IsActive())continue;
    p=&particle[i];

    // Move particle
    p->pos.x+=p->vel.x*dt;
    p->pos.y+=p->vel.y*dt;
    p->pos.z+=p->vel.z*dt;

    p->age+=dt;
    if(type==SMOKE)
    {
      if(p->age>p->lifeTime)
      {
        p->Die();
        continue;
      }
      // Smoke particle grows ... m/s
      p->size.x+=dt*0.5;
      p->size.y+=dt*0.5;
      p->size.z+=dt*0.5;
      // Reduce velocity of movement
      p->vel.x-=2.5*dt*p->vel.x;
      p->vel.z-=2.5*dt*p->vel.z;
    } else if(type==SPARKS)
    {
      // Out the way?
      if(p->pos.y<0)
      {
        // Kill particle
        p->flags&=~DParticle::ACTIVE;
        continue;
      }
      // Gravity
      p->vel.y-=gravityTimesDT;
    }
  }
}
void DParticleGroup::SetupBlendMode()
{
  switch(blendMode)
  {
    case ALPHA_BLEND:
      glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_DST_COLOR);
      //glBlendFunc(GL_DST_COLOR,GL_ONE);
      //glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
      // SRC_COLOR+ONE is ok but only exists with NV_blend_square extension (?)
      //glBlendFunc(GL_SRC_COLOR,GL_ONE);
      // 1-DST_COL,ONE was suggested on comp.graphics.api.opengl
      glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
      break;
    case ADDITIVE:
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE);
      break;
    default:
      qwarn("DParticleGroup:SetupBlendMode(); unknown mode %d\n",blendMode);
      break;
  }
}
void DParticleGroup::Render()
// Render all particles
{
  int i;
  DParticle *p;
  DVector3 vRight,vUp;
  GLfloat col[4];
  GLfloat matModelView[16];
  DVector3 A,B,C,D;

  SetupBlendMode();

  //glDisable(GL_DEPTH_TEST);
  // For billboarding, get the modelview matrix
  glGetFloatv(GL_MODELVIEW_MATRIX,matModelView);

  // Use textured quads?
  if(texture)
  {
    // Get a right and up vector to billboard
    vRight.x=matModelView[0];
    vRight.y=matModelView[4];
    vRight.z=matModelView[8];
    vUp.x=matModelView[1];
    vUp.y=matModelView[5];
    vUp.z=matModelView[9];

    // Create offsets from particle position; only to be scaled by the size
    // later on.
    A.x=-vRight.x-vUp.x;
    A.y=-vRight.y-vUp.y;
    A.z=-vRight.z-vUp.z;
    B.x=vRight.x-vUp.x;
    B.y=vRight.y-vUp.y;
    B.z=vRight.z-vUp.z;
    C.x=vRight.x+vUp.x;
    C.y=vRight.y+vUp.y;
    C.z=vRight.z+vUp.z;
    D.x=-vRight.x+vUp.x;
    D.y=-vRight.y+vUp.y;
    D.z=-vRight.z+vUp.z;

    //QCV->Select();
    texture->Select();
//QShowGLErrors("DPG:Render");
    glEnable(GL_TEXTURE_2D);
#ifdef OBS
    //glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    //glBlendFunc(GL_SRC_COLOR,GL_ONE);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
    //glBlendFunc(GL_ONE,GL_ONE);
    //glColor4f(1,1,1,.5);
#endif

//glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
//glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_ONE);
//glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_SRC_COLOR);
//glBlendFunc(GL_SRC_COLOR,GL_ONE);

    glBegin(GL_QUADS);
    for(i=0;i<particleCount;i++)
    {
      p=&particle[i];
      if(!p->IsActive())continue;
      col[3]=1.0f-(p->age/p->lifeTime);
      //col[0]=col[1]=col[2]=col[3];
      //col[0]*=2;
      //glColor4fv(col);
      glColor3f(col[3],col[3],col[3]);
      //glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,col);
      //glMaterialfv(GL_FRONT,GL_DIFFUSE,col);
      glTexCoord2f(0,0);
      glVertex3f(p->pos.x+p->size.x*A.x,p->pos.y+p->size.y*A.y,
        p->pos.z+p->size.z*A.z);
      glTexCoord2f(0,1);
      glVertex3f(p->pos.x+p->size.x*B.x,p->pos.y+p->size.y*B.y,
        p->pos.z+p->size.z*B.z);
      glTexCoord2f(1,1);
      glVertex3f(p->pos.x+p->size.x*C.x,p->pos.y+p->size.y*C.y,
        p->pos.z+p->size.z*C.z);
      glTexCoord2f(1,0);
      glVertex3f(p->pos.x+p->size.x*D.x,p->pos.y+p->size.y*D.y,
        p->pos.z+p->size.z*D.z);
    }
    glEnd();
  } else
  {
    // No texture; just dots
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_POINTS);
  
    for(i=0;i<particleCount;i++)
    {
      p=&particle[i];
      if(!p->IsActive())continue;
  
      glVertex3f(p->pos.x,p->pos.y,p->pos.z);
    }
    glEnd();
  }
}

/*******************
* Particle manager *
*******************/
DParticleManager::DParticleManager()
{
  groups=0;
}
DParticleManager::~DParticleManager()
{
}

bool DParticleManager::AddGroup(DParticleGroup *grp)
// Returns FALSE if system is full
{
  if(groups==MAX_GROUP)return FALSE;
  group[groups]=grp;
  groups++;
  return TRUE;
}
bool DParticleManager::RemoveGroup(DParticleGroup *grp)
// Removes a single group.
// Returns FALSE if group was not found
{
  int i;
  for(i=0;i<groups;i++)
  {
    if(group[i]==grp)
    {
      // Found the group; shift other groups into gap
      for(;i<groups-1;i++)
        group[i]=group[i+1];
      groups--;
      return TRUE;
    }
  }
  return FALSE;
}
void DParticleManager::RemoveGroups()
{
  groups=0;
}

void DParticleManager::KillAll()
// Kill all particles from all groups
{
  int i;
  for(i=0;i<groups;i++)
  {
    group[i]->KillAll();
  }
}

void DParticleManager::Update(dfloat dt)
// Update all groups for 'dt' seconds.
{
  int i;
  for(i=0;i<groups;i++)
    group[i]->Update(dt);
}
void DParticleManager::Render()
{
  int i;
  // Set state for ALL particle groups
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glDepthMask(GL_FALSE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  for(i=0;i<groups;i++)
    group[i]->Render();
  glDepthMask(GL_TRUE);
}

