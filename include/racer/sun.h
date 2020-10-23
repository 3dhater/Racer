// racer/sun.h

#ifndef __RACER_SUN_H
#define __RACER_SUN_H

#include <d3/texture.h>

class RSun
// Defines a sun for special FX
{
 public:
  enum Flags
  {
    VISIBLE=1,                  // Sun in view? (dynamic flag)
    ACTIVE=2                    // Sun is shining?
  };
  enum Max
  {
    MAX_TEX=5,
    MAX_FLARE=8                 // Number of flare components
  };
  enum VisibilityMethod
  {
    VM_NONE,                    // Just paint sun if in view
    VM_ZBUFFER,                 // Get a ZBuffer sample
    VM_RAYTRACE                 // Trace a ray from the viewer to the sun (NYI)
  };

 protected:
  // Static
  DVector3  pos;                // Sun position
  int       flags;
  int       visibilityMethod;   // Method of determining if sun is blocked
  DTexture *tex[MAX_TEX];       // Images used
  rfloat    whiteoutFactor;     // When to whiteout (larger=more whiteout)
  // Flare
  rfloat    length[MAX_FLARE],   // Distance to main flare (sun itself)
            size[MAX_FLARE];     // Relative size
  int       flareTex[MAX_FLARE]; // Which texture?

  // Dynamic
  DVector3 vWin;                // Where it is in the window

 public:
  RSun();
  ~RSun();

  // Attribs
  DVector3 *GetPosition(){ return &pos; }
  DVector3 *GetWindowPosition(){ return &vWin; }

  void Enable(){ flags|=ACTIVE; }
  void Disable(){ flags&=~ACTIVE; }
  void   SetWhiteoutFactor(rfloat v){ whiteoutFactor=v; }
  rfloat GetWhiteoutFactor(){ return whiteoutFactor; }

  // During the game
  void CalcWindowPosition();
  void Paint();
};

#endif

