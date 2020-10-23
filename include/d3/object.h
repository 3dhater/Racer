// d3/dobject.h - declaration

#ifndef __DOBJECT_H
#define __DOBJECT_H

#ifdef WIN32
#include <windows.h>
#endif
#include <qlib/types.h>
#include <d3/types.h>

class DTransformation
// Basic location/orientation/scaling information for an object
// Used in ZPath for example.
{
 protected:
 public:
  // Location (may in the future be relative to that of a parent)
  dfloat x,y,z;
  // Orientation (XYZ rotation)
  dfloat xa,ya,za;
  // Scaling
  dfloat xs,ys,zs;
 public:
  DTransformation();
  void Reset();

  void Copy(DTransformation *t);
#ifdef FUTURE
  DTransformation& operator+(DTransformation& d1);
#endif
};

class DObject
// A DObject is some 3D object, be it particles, be it a single mesh,
// be it a collection of meshes.
// A DObject has some location and orientation in the universe
// This is a virtual base class
{
 private:
  DTransformation tf;
  int flags;

 public:
  enum Flags
  { HIDE=1,			// Don't paint object
    IS2D=2,			// Object is really a 2D object
    IS_CSG=4			// Is part of hierarchical geometry
                                // This means no OpenGL context switching
                                // and no handy setting of 3D views
                                // Assumes OpenGL context is set and ready
  };

 public:
  DObject();
  virtual ~DObject();

  void Set2D();

  // CSG
  void EnableCSG(){ flags|=IS_CSG; }
  void DisableCSG(){ flags&=~IS_CSG; }
  bool IsCSG(){ if(flags&IS_CSG)return TRUE; return FALSE; }

  // Attribs
  DTransformation *GetTransformation(){ return &tf; }
  dfloat GetX();
  dfloat GetY();
  dfloat GetZ();
  bool   IsHidden();
  bool   Is2D();

  void SetTransformation(DTransformation *t);
  void Move(dfloat x,dfloat y,dfloat z);
  void SetRotation(dfloat xa,dfloat ya,dfloat za);
  void SetScale(dfloat xs,dfloat ys,dfloat zs);

  void Hide();
  void Show();

  // Painting
  //void PaintBegin();
  //void PaintEnd();
  virtual void _Paint()=0;	// Derived classes Paint() functionality
  void Paint();
};

class DObjectGroup
// A group of DObjects, each with its own location etc.
{
};

#endif
