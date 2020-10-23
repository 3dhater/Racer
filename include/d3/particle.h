// d3/particle.h

#ifndef __D3_PARTICLE_H
#define __D3_PARTICLE_H

#include <d3/types.h>
//#include <d3/texture.h>

class DTexture;

class DParticle
// Don't use base class here or virtual functions
{
 public:
  enum Flags
  {
    ACTIVE=1             // Particle is in use
  };

  // Basic attributes
  DVector3 pos,
           vel;
  DVector3 color;        // Using RGB float[3] (instead of QColor class)
  dfloat   alpha;        // Color alpha value (transparency), 0..1
  dfloat   age,
           lifeTime;     // Expected life duration
  dfloat   energy;       // Amount of energy
  DVector3 size;
  int      flags;

  // Effects
  DVector3 oldPos;       // For stretched sparks etc.

 public:
  DParticle();
  ~DParticle();

  void Reset();

  // Attribs
  bool IsActive(){ if(flags&ACTIVE)return TRUE; return FALSE; }
  void Revive(){ flags|=ACTIVE; }
  void Die(){ flags&=~ACTIVE; }
};

class DParticleGroup
// Some call this a particle system, but as the word 'system' is used
// for several different things, I use the term 'group' to indicate
// a group of particles (like all the particles in a single fountain
// make up 1 group).
{
 public:
  enum Type
  {
    SPARKS,                   // Stretched polygons
    SMOKE                     // Expanding smoke
  };
  enum BlendModes
  {
    ALPHA_BLEND,
    ADDITIVE
  };

 protected:
  int particleCount;
  DParticle *particle;

  // Dynamics
  int       type;
  dfloat    gravity;

  // Graphics
  DTexture *texture;          // For flat polygons
  int       blendMode;        // Type of painting

 public:
  DParticleGroup(int type=SPARKS,int maxParticle=100);
  ~DParticleGroup();

  // Attribs
  DTexture *GetTexture(){ return texture; }
  void SetTexture(DTexture *tex);

  // Particle management
  DParticle *GetFreeParticle();
  void Spawn(int count);
  // Smoke spawning
  void Spawn(DVector3 *pos,DVector3 *vel,dfloat lifeTime);
  // Killing particles
  void KillAll();

  void Update(dfloat dt);
 protected:
  void SetupBlendMode();
 public:
  void Render();
};

class DParticleManager
// Holds all particle systems
{
 public:
  enum Max
  {
    MAX_GROUP=10
  };
 protected:
  DParticleGroup *group[MAX_GROUP];
  int             groups;

 public:
  DParticleManager();
  ~DParticleManager();

  // Attribs
  int GetGroups(){ return groups; }
  DParticleGroup *GetGroup(int n){ return group[n]; }

  // Startup
  void Init();

  // Group management
  bool AddGroup(DParticleGroup *grp);
  bool RemoveGroup(DParticleGroup *grp);
  void RemoveGroups();

  // Handling
  void Update(dfloat dt);
  void Render();

  // Killing particles
  void KillAll();
};

#endif
