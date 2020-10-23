// gfx.h - Racer graphical functions

#ifndef __GFX_H
#define __GFX_H

//#include <qlib/types.h>

void RGfxText3D(cstring s);
void RGfxText3D(float x,float y,float z,cstring s);
void RGfxGo(float x,float y,float z);

void RGfxArrow(float x,float y,float z,float *v,float scale=1.0f);
void RGfxAxes(float x,float y,float z,float len);
void RGfxVector(DVector3 *v,float scale=1.0f,float r=0.0f,float g=0.0f,
  float b=0.0f);

#endif
