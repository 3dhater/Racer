// dpoly.h - declaration

#ifndef __DPOLY_H
#define __DPOLY_H

#include <d3/types.h>
#include <d3/texture.h>

struct DPolyPoint
{
  dfloat x,y,z;			// Location
  dfloat tx,ty,tz;		// Texture coords
  dfloat nx,ny,nz;		// Normal vector
};

class DPoly
{
 public:
  enum
  { //BLEND=1,				// Obsolete per 8-9-1999
    CULL=2
  };
  enum BlendModes
  // Blending modes corresponding to QCanvas' blend modes
  { BLEND_OFF=0,                        // Don't blend
    BLEND_SRC_ALPHA=1,                  // Most common blend; use alpha
    BLEND_CONSTANT=2                    // Use constant color
  };

 protected:
  // Class member variables
  DPolyPoint *point;		// Any # points
  int points;
  DTexture *texture;		// Texturemap
  int flags;
  int blendMode;

  // Special fx
  dfloat opacity;		// Makes polygon transparent

 public:
  DPoly();
  ~DPoly();

  // Attribs
  void Disable(int flags);
  void Enable(int flags);

  void   SetBlendMode(int mode=BLEND_SRC_ALPHA);
  void   SetOpacity(dfloat opacity);
  dfloat GetOpacity(){ return opacity; }

  dfloat GetWidth();
  dfloat GetHeight();

  void MoveRelative(dfloat x,dfloat y,dfloat z);

  void Define(int points,DPolyPoint *pts=0);
  void DefinePoint(int point,DPolyPoint *pt);
  void DefineTexture(DTexture *texture,QRect *r,int rotateCount=0,
    int mirrorFlags=0);

  // Dynamic polygon modification
  void SetWidth(dfloat w);
  void SetHeight(dfloat h);

  void Paint();
};

#endif
