/*
 * DObject - base 3D object
 * 07-09-1999: Now also supports 2D objects (in a non-3D environment)
 * NOTES:
 * - A bit like QGel, only for 3D
 * - The word 'DTransformation' is hell to type; perhaps another name?
 * (C) 19-02-1999 (17:35) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <qlib/app.h>
DEBUG_ENABLE

/******************
* DTRANSFORMATION *
******************/
DTransformation::DTransformation()
{ Reset();
}

void DTransformation::Reset()
{
  x=y=z=0.0;
  xa=ya=za=0.0;
  xs=ys=zs=1.0;
}

void DTransformation::Copy(DTransformation *t)
// Copy all members of other transformation
{
  x=t->x; y=t->y; z=t->z;
  xa=t->xa; ya=t->ya; za=t->za;
  xs=t->xs; ys=t->ys; zs=t->zs;
}

//
// Math operators
//
#ifdef OBS
DTransformation& DTransformation::operator+(DTransformation& t)
{
  x+=t.x;
  return &this;
}
#endif

/**********
* DOBJECT *
**********/
DObject::DObject()
{
  // Standard attributes
  flags=0;
}

DObject::~DObject()
{
}

void DObject::Set2D()
// Make this object a 2D object
// Important, because the Paint() method WILL NOT WORK AT ALL for 2D objects.
// This, because of matrix stacks that makes the canvas *think* its in 2D,
// but its using a 3D matrix.
{
  flags|=IS2D;
}

dfloat DObject::GetX()
{ return tf.x;
}
dfloat DObject::GetY()
{ return tf.y;
}
dfloat DObject::GetZ()
{ return tf.z;
}
bool DObject::IsHidden()
{
  if(flags&HIDE)return TRUE;
  return FALSE;
}
bool DObject::Is2D()
{
  if(flags&IS2D)return TRUE;
  return FALSE;
}

void DObject::SetTransformation(DTransformation *t)
// Set all transformation attribs at once
{
  tf=*t;
}

void DObject::Move(dfloat nx,dfloat ny,dfloat nz)
// Move object to 3D location
{
  tf.x=nx; tf.y=ny; tf.z=nz;
}
void DObject::SetRotation(dfloat nx,dfloat ny,dfloat nz)
// Set XYZ rotation
{
  tf.xa=nx; tf.ya=ny; tf.za=nz;
}

void DObject::Hide()
{ flags|=HIDE;
}
void DObject::Show()
{ flags&=~HIDE;
}

#ifdef OBS
void DObject::PaintBegin()
{
  int i;
  glPushMatrix();
  // Move to right location
  if(x!=0||y!=0||z!=0)
    glTranslatef(x,y,z);
  // Rotate around x/y/z
  if(xa!=0.0)glRotatef(xa,1,0,0);
  if(ya!=0.0)glRotatef(ya,0,1,0);
  if(za!=0.0)glRotatef(za,0,0,1);
  // Scale x/y/z
}
void DObject::PaintEnd()
// Call this after the derived class painted itself
{
  glPopMatrix();
}
#endif

void DObject::Paint()
// Paint the object. Moves to the object's position/orientation.
// BUG: Doesn't scale yet...
{
  QCanvas *cv;

  if(flags&HIDE)return;

//qdbg("DObject::Paint()\n");
  if(!(flags&IS_CSG))
  { // Explicitly set an expected canvas to draw into
    cv=QCV;
    cv->Select();
  } else // else current rendering context is used
  {
    cv=0;
  }

  if(flags&IS2D)
  { // 2D objects are NOT relocated in 3D space
    _Paint();
    return;
  }

  if(!(flags&IS_CSG))
  { // Explicitly set a 3D view
    // Useful for mixing 2D and 3D, but slower when you're doing full 3D
    cv->Set3D();
  }

  glPushMatrix();
  // Move to right location
  if(tf.x!=0||tf.y!=0||tf.z!=0)
    glTranslatef(tf.x,tf.y,tf.z);
  // Rotate around x/y/z
  if(tf.xa!=0.0)glRotatef(tf.xa,1,0,0);
  if(tf.ya!=0.0)glRotatef(tf.ya,0,1,0);
  if(tf.za!=0.0)glRotatef(tf.za,0,0,1);
  // Scale x/y/z
  if(tf.xs!=1.0||tf.ys!=1.0||tf.zs!=1.0)
    glScalef(tf.xs,tf.ys,tf.zs);

  // Do actual object painting (derived functions)
  _Paint();

  glPopMatrix();
}
#ifdef ND_OBJ_INVISIBLE
void DObject::_Paint()
{
  glPopMatrix();
}
#endif

